#include <eepp/graphics/exif.hpp>

#include <array>
#include <cstring>
#include <eepp/system/scopedop.hpp>
#include <vector>

namespace EE::Graphics {

using namespace EE::System;
namespace {

constexpr Uint64 MaxMetadata = 1024 * 1024;

bool readAt( IOStream& stream, Uint64 offset, void* target, size_t length ) {
	if ( stream.getSize() < 0 || offset > Uint64( stream.getSize() ) ||
		 length > Uint64( stream.getSize() ) - offset ||
		 stream.seek( offset ) != ios_size( offset ) )
		return false;
	auto* out = static_cast<char*>( target );
	while ( length ) {
		const auto count = stream.read( out, length );
		if ( count <= 0 || Uint64( count ) > length )
			return false;
		out += count;
		length -= count;
	}
	return true;
}

Uint32 big32( const Uint8* data ) {
	return Uint32( data[0] ) << 24 | Uint32( data[1] ) << 16 | Uint32( data[2] ) << 8 | data[3];
}

Exif::Metadata parse( IOStream& stream, Uint64 start, Uint64 size ) {
	Exif::Metadata result;
	if ( size < 8 || size > MaxMetadata )
		return result;
	std::vector<Uint8> data( size );
	if ( !readAt( stream, start, data.data(), data.size() ) )
		return result;
	const bool little = data[0] == 'I' && data[1] == 'I';
	if ( !little && !( data[0] == 'M' && data[1] == 'M' ) )
		return result;
	const auto u16 = [&]( size_t p ) -> Uint16 {
		return little ? Uint16( data[p] | Uint16( data[p + 1] ) << 8 )
					  : Uint16( Uint16( data[p] ) << 8 | data[p + 1] );
	};
	const auto u32 = [&]( size_t p ) -> Uint32 {
		return little ? Uint32( u16( p ) ) | Uint32( u16( p + 2 ) ) << 16
					  : Uint32( u16( p ) ) << 16 | u16( p + 2 );
	};
	if ( u16( 2 ) != 42 )
		return result;
	Uint32 offset = u32( 4 );
	Uint32 primary = offset;
	Uint32 jpegOffset = 0, jpegSize = 0;
	bool jpeg = false;
	for ( int directory = 0; directory < 2 && offset; ++directory ) {
		if ( offset < 8 || offset > size - 2 || ( directory && offset == primary ) )
			break;
		const size_t count = u16( offset );
		if ( count > 4096 || count * 12 + 6 > size - offset )
			break;
		for ( size_t i = 0; i < count; ++i ) {
			const size_t p = offset + 2 + i * 12;
			const Uint16 tag = u16( p ), type = u16( p + 2 );
			if ( u32( p + 4 ) != 1 )
				continue;
			if ( tag == 0x112 && type == 3 ) {
				const Uint16 value = u16( p + 8 );
				if ( value >= 1 && value <= 8 ) {
					if ( directory == 0 ) {
						result.orientation = static_cast<ImageOrientation>( value );
						result.thumbnailOrientation = result.orientation;
					} else {
						result.thumbnailOrientation = static_cast<ImageOrientation>( value );
					}
				}
			} else if ( directory && tag == 0x103 && type == 3 ) {
				jpeg = u16( p + 8 ) == 6;
			} else if ( directory && type == 4 ) {
				if ( tag == 0x201 )
					jpegOffset = u32( p + 8 );
				else if ( tag == 0x202 )
					jpegSize = u32( p + 8 );
			}
		}
		offset = u32( offset + 2 + count * 12 );
	}
	if ( jpeg && jpegSize >= 2 && jpegOffset >= 8 && jpegOffset <= size &&
		 jpegSize <= size - jpegOffset && data[jpegOffset] == 0xff &&
		 data[jpegOffset + 1] == 0xd8 ) {
		result.thumbnailOffset = start + jpegOffset;
		result.thumbnailLength = jpegSize;
	}
	return result;
}

} // namespace

Exif::Metadata Exif::read( IOStream& stream ) {
	const auto position = stream.tell();
	if ( !stream.isOpen() || position < 0 )
		return {};
	ScopedOp restore( nullptr, [&] { stream.seek( position ); } );
	std::array<Uint8, 12> header{};
	if ( !readAt( stream, 0, header.data(), header.size() ) )
		return {};
	const Uint64 size = stream.getSize();
	if ( ( header[0] == 'I' && header[1] == 'I' ) || ( header[0] == 'M' && header[1] == 'M' ) )
		return parse( stream, 0, std::min( size, MaxMetadata ) );
	if ( header[0] == 0xff && header[1] == 0xd8 ) {
		Uint64 offset = 2;
		for ( int segment = 0; segment < 256 && offset < size && offset < MaxMetadata; ++segment ) {
			Uint8 marker[4];
			if ( !readAt( stream, offset, marker, 2 ) || marker[0] != 0xff )
				break;
			if ( marker[1] == 0xff ) {
				++offset;
				continue;
			}
			if ( marker[1] == 0xda || marker[1] == 0xd9 )
				break;
			if ( marker[1] == 0x01 || ( marker[1] >= 0xd0 && marker[1] <= 0xd7 ) ) {
				offset += 2;
				continue;
			}
			if ( !readAt( stream, offset + 2, marker + 2, 2 ) )
				break;
			const Uint32 length = Uint32( marker[2] ) << 8 | marker[3];
			if ( length < 2 || length > size - offset - 2 )
				break;
			if ( marker[1] == 0xe1 && length >= 8 ) {
				char tag[6];
				if ( readAt( stream, offset + 4, tag, 6 ) &&
					 std::memcmp( tag, "Exif\0\0", 6 ) == 0 )
					return parse( stream, offset + 10, length - 8 );
			}
			offset += 2 + length;
		}
	} else {
		const bool png = std::memcmp( header.data(), "\x89PNG\r\n\x1a\n", 8 ) == 0;
		const bool webp = std::memcmp( header.data(), "RIFF", 4 ) == 0 &&
						  std::memcmp( header.data() + 8, "WEBP", 4 ) == 0;
		if ( !png && !webp )
			return {};
		Uint64 offset = png ? 8 : 12;
		for ( int chunk = 0; chunk < 256 && offset <= size && size - offset >= 8; ++chunk ) {
			Uint8 entry[8];
			if ( !readAt( stream, offset, entry, 8 ) )
				break;
			const Uint64 length = png ? big32( entry )
									  : Uint32( entry[4] ) | Uint32( entry[5] ) << 8 |
											Uint32( entry[6] ) << 16 | Uint32( entry[7] ) << 24;
			const Uint64 overhead = png ? 12 : 8 + ( length & 1 );
			if ( length > size - offset - 8 || overhead > size - offset - length )
				break;
			if ( std::memcmp( entry + ( png ? 4 : 0 ), png ? "eXIf" : "EXIF", 4 ) == 0 ) {
				Uint64 start = offset + 8, payload = length;
				char tag[6];
				if ( payload >= 6 && readAt( stream, start, tag, 6 ) &&
					 std::memcmp( tag, "Exif\0\0", 6 ) == 0 ) {
					start += 6;
					payload -= 6;
				}
				return parse( stream, start, payload );
			}
			offset += overhead + length;
		}
	}
	return {};
}

} // namespace EE::Graphics
