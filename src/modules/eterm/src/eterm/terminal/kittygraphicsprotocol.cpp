#include <eterm/terminal/kittygraphicsprotocol.hpp>

#include <eepp/graphics/image.hpp>
#include <eepp/system/base64.hpp>
#include <eepp/system/compression.hpp>
#include <eepp/system/iostreammemory.hpp>

#include <algorithm>
#include <charconv>
#include <cstring>
#include <limits>
#include <thread>
#include <type_traits>

#if EE_PLATFORM != EE_PLATFORM_WIN && EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN && \
	EE_PLATFORM != EE_PLATFORM_ANDROID
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

using namespace EE::System;

namespace eterm { namespace Terminal {

KittyGraphicsProtocol::KittyGraphicsProtocol( size_t maxStorageBytes, size_t maxImages,
											  size_t maxPlacements ) :
	mMaxStorageBytes( maxStorageBytes ), mMaxImages( maxImages ), mMaxPlacements( maxPlacements ) {}

namespace {

bool parseUnsigned( std::string_view value, Uint32& result ) {
	if ( value.empty() )
		return false;
	const char* end = value.data() + value.size();
	auto parsed = std::from_chars( value.data(), end, result );
	return parsed.ec == std::errc{} && parsed.ptr == end;
}

bool parseSigned( std::string_view value, Int32& result ) {
	if ( value.empty() )
		return false;
	const char* end = value.data() + value.size();
	auto parsed = std::from_chars( value.data(), end, result );
	return parsed.ec == std::errc{} && parsed.ptr == end;
}

bool parseCharacter( std::string_view value, char& result ) {
	if ( value.size() != 1 )
		return false;
	result = value.front();
	return true;
}

bool checkedPixelBytes( Uint32 width, Uint32 height, size_t channels, size_t& result ) {
	if ( width == 0 || height == 0 || width > std::numeric_limits<size_t>::max() / height )
		return false;
	const size_t pixels = static_cast<size_t>( width ) * height;
	if ( pixels > std::numeric_limits<size_t>::max() / channels )
		return false;
	result = pixels * channels;
	return true;
}

void recycleBuffer( std::vector<Uint8>& buffer, std::vector<Uint8>& cache ) {
	buffer.clear();
	if ( buffer.capacity() > cache.capacity() )
		buffer.swap( cache );
}

bool decodeBase64( std::string_view input, bool finalChunk, std::vector<Uint8>& output ) {
	if ( !finalChunk && input.size() % 4 != 0 )
		return false;
	const size_t oldSize = output.size();
	const size_t capacity = Base64::decodeSafeOutLen( input.size() );
	constexpr size_t MaxTransferBytes = 64 * 1024 * 1024;
	if ( oldSize > MaxTransferBytes || capacity > MaxTransferBytes - oldSize )
		return false;
	output.resize( oldSize + capacity );
	const size_t decodedSize =
		Base64::decode( input.size(), input.data(), capacity, output.data() + oldSize,
						Base64::DecodeMode::NoWhitespaceStrict );
	if ( decodedSize == static_cast<size_t>( -1 ) ) {
		output.resize( oldSize );
		return false;
	}
	output.resize( oldSize + decodedSize );
	return true;
}

bool readSharedMemory( const KittyGraphicsCommandData& data, std::vector<Uint8>& output ) {
#if EE_PLATFORM == EE_PLATFORM_WIN || EE_PLATFORM == EE_PLATFORM_EMSCRIPTEN || \
	EE_PLATFORM == EE_PLATFORM_ANDROID
	(void)data;
	(void)output;
	return false;
#else
	std::vector<Uint8> decodedName;
	if ( !decodeBase64( data.payload, true, decodedName ) || decodedName.empty() ||
		 decodedName.size() > 255 ||
		 std::find( decodedName.begin() + 1, decodedName.end(), '/' ) != decodedName.end() ||
		 std::find( decodedName.begin(), decodedName.end(), 0 ) != decodedName.end() )
		return false;
	std::string name( decodedName.begin(), decodedName.end() );
	const int descriptor = shm_open( name.c_str(), O_RDONLY, 0 );
	if ( descriptor == -1 )
		return false;
	// POSIX Kitty transfers are single-use. Unlink immediately after opening so all error paths
	// still retire the client-owned object while the descriptor keeps its contents alive.
	shm_unlink( name.c_str() );
	struct stat status{};
	const size_t offset = data.dataOffset.value_or( 0 );
	bool valid = fstat( descriptor, &status ) == 0 && status.st_size >= 0 &&
				 static_cast<Uint64>( status.st_size ) >= offset;
	size_t bytes = 0;
	if ( valid ) {
		const size_t available = static_cast<size_t>( status.st_size ) - offset;
		bytes = data.dataSize.value_or( static_cast<Uint32>(
			std::min<size_t>( available, std::numeric_limits<Uint32>::max() ) ) );
		valid = bytes <= available && bytes <= 128 * 1024 * 1024;
	}
	if ( valid && bytes != 0 ) {
		const size_t mappingBytes = offset + bytes;
		valid = mappingBytes >= bytes && mappingBytes <= 128 * 1024 * 1024;
		void* mapping = valid ? mmap( nullptr, mappingBytes, PROT_READ, MAP_SHARED, descriptor, 0 )
							  : MAP_FAILED;
		if ( mapping == MAP_FAILED ) {
			valid = false;
		} else {
			const auto* source = static_cast<const Uint8*>( mapping ) + offset;
			// mpv reuses the same shm name for every frame. If it reopened the object before
			// we unlinked it above, its next memcpy can overlap this read. Require consecutive
			// identical observations after yielding to the writer; otherwise retain the previous
			// displayed frame instead of publishing visibly torn rows.
			std::vector<Uint8> snapshot( source, source + bytes );
			bool stable = false;
			unsigned stableObservations = 0;
			constexpr unsigned RequiredStableObservations = 2;
			constexpr unsigned MaxSnapshotAttempts = 6;
			for ( unsigned attempt = 0; attempt < MaxSnapshotAttempts; ++attempt ) {
				std::this_thread::yield();
				if ( std::memcmp( snapshot.data(), source, bytes ) == 0 ) {
					if ( ++stableObservations == RequiredStableObservations ) {
						stable = true;
						break;
					}
				} else {
					stableObservations = 0;
					snapshot.assign( source, source + bytes );
				}
			}
			if ( stable )
				output = std::move( snapshot );
			else
				valid = false;
			munmap( mapping, mappingBytes );
		}
	}
	close( descriptor );
	return valid && bytes != 0;
#endif
}

bool placementContains( const TerminalVisiblePlacement& placement, Vector2i cell ) {
	const Int64 right = static_cast<Int64>( placement.visibleAnchorCell.x ) + placement.columns;
	const Int64 bottom = static_cast<Int64>( placement.visibleAnchorCell.y ) + placement.rows;
	return cell.x >= placement.visibleAnchorCell.x && cell.y >= placement.visibleAnchorCell.y &&
		   cell.x < right && cell.y < bottom;
}

} // namespace

KittyGraphicsParseResult KittyGraphicsProtocol::parse( std::string_view command ) {
	KittyGraphicsParseResult result;
	KittyGraphicsCommandData data;
	char action = 't';

	const size_t separator = command.find( ';' );
	std::string_view control = command.substr( 0, separator );
	if ( separator != std::string_view::npos )
		data.payload = command.substr( separator + 1 );

	while ( !control.empty() ) {
		const size_t comma = control.find( ',' );
		const std::string_view field = control.substr( 0, comma );
		control =
			comma == std::string_view::npos ? std::string_view{} : control.substr( comma + 1 );
		const size_t equals = field.find( '=' );
		if ( equals == std::string_view::npos || equals == 0 || equals + 1 == field.size() ) {
			result.error = KittyGraphicsError::InvalidArgument;
			return result;
		}

		const std::string_view key = field.substr( 0, equals );
		const std::string_view value = field.substr( equals + 1 );
		if ( key.size() != 1 )
			continue; // Unknown future keys are ignored for forward compatibility.

		Uint32 unsignedValue = 0;
		bool valid = true;
		switch ( key.front() ) {
			case 'a':
				valid = parseCharacter( value, action );
				break;
			case 't':
				valid = parseCharacter( value, data.transmission );
				break;
			case 'o':
				valid = parseCharacter( value, data.compression );
				break;
#define PARSE_UINT_FIELD( name )                   \
	valid = parseUnsigned( value, unsignedValue ); \
	if ( valid )                                   \
	data.name = unsignedValue
			case 'f':
				PARSE_UINT_FIELD( format );
				break;
			case 'S':
				PARSE_UINT_FIELD( dataSize );
				break;
			case 'O':
				PARSE_UINT_FIELD( dataOffset );
				break;
			case 'm':
				PARSE_UINT_FIELD( more );
				valid = valid && unsignedValue <= 1;
				break;
			case 'i':
				PARSE_UINT_FIELD( imageId );
				break;
			case 'I':
				PARSE_UINT_FIELD( imageNumber );
				break;
			case 'N':
				PARSE_UINT_FIELD( usageHint );
				break;
			case 'p':
				PARSE_UINT_FIELD( placementId );
				break;
			case 'q':
				PARSE_UINT_FIELD( quiet );
				valid = valid && unsignedValue <= 2;
				break;
			case 's':
				PARSE_UINT_FIELD( width );
				break;
			case 'v':
				PARSE_UINT_FIELD( height );
				break;
			case 'x':
				PARSE_UINT_FIELD( x );
				break;
			case 'y':
				PARSE_UINT_FIELD( y );
				break;
			case 'w':
				PARSE_UINT_FIELD( sourceWidth );
				break;
			case 'h':
				PARSE_UINT_FIELD( sourceHeight );
				break;
			case 'c':
				PARSE_UINT_FIELD( columns );
				break;
			case 'r':
				PARSE_UINT_FIELD( rows );
				break;
			case 'X':
				PARSE_UINT_FIELD( xOffset );
				break;
			case 'Y':
				PARSE_UINT_FIELD( yOffset );
				break;
			case 'C':
				PARSE_UINT_FIELD( cursorMovement );
				valid = valid && unsignedValue <= 1;
				break;
			case 'U':
				PARSE_UINT_FIELD( virtualPlacement );
				valid = valid && unsignedValue <= 1;
				break;
			case 'P':
				PARSE_UINT_FIELD( parentImageId );
				break;
			case 'Q':
				PARSE_UINT_FIELD( parentPlacementId );
				break;
			case 'H': {
				Int32 signedValue = 0;
				valid = parseSigned( value, signedValue );
				if ( valid )
					data.parentOffsetX = signedValue;
				break;
			}
			case 'V': {
				Int32 signedValue = 0;
				valid = parseSigned( value, signedValue );
				if ( valid )
					data.parentOffsetY = signedValue;
				break;
			}
			case 'd':
				valid = parseCharacter( value, data.deletion );
				break;
			case 'z': {
				Int32 signedValue = 0;
				valid = parseSigned( value, signedValue );
				if ( valid )
					data.zIndex = signedValue;
				break;
			}
			default:
				break;
		}
#undef PARSE_UINT_FIELD
		if ( !valid ) {
			result.error = KittyGraphicsError::InvalidArgument;
			return result;
		}
	}

	if ( data.imageId && data.imageNumber ) {
		result.error = KittyGraphicsError::InvalidArgument;
		return result;
	}

	switch ( action ) {
		case 't':
			result.command = KittyTransmitCommand{ data, false };
			break;
		case 'T':
			result.command = KittyTransmitCommand{ data, true };
			break;
		case 'p':
			result.command = KittyPutCommand{ data };
			break;
		case 'd':
			result.command = KittyDeleteCommand{ data };
			break;
		case 'f':
			result.command = KittyFrameCommand{ data };
			break;
		case 'a':
			result.command = KittyAnimationCommand{ data };
			break;
		case 'c':
			result.command = KittyComposeCommand{ data };
			break;
		case 'q':
			result.command = KittyQueryCommand{ data };
			break;
		default:
			result.error = KittyGraphicsError::InvalidArgument;
			break;
	}
	return result;
}

KittyGraphicsHandleResult KittyGraphicsProtocol::handle( std::string_view command,
														 Vector2i cursor ) {
	auto parsed = parse( command );
	if ( !parsed.command )
		return { {}, parsed.error, false };

	return std::visit(
		[this, cursor]( const auto& value ) -> KittyGraphicsHandleResult {
			using T = std::decay_t<decltype( value )>;
			if constexpr ( !std::is_same_v<T, KittyTransmitCommand> &&
						   !std::is_same_v<T, KittyQueryCommand> &&
						   !std::is_same_v<T, KittyFrameCommand> &&
						   !std::is_same_v<T, KittyDeleteCommand> ) {
				if ( mPending.active ) {
					mPending = {};
					return { response( value.data, KittyGraphicsError::InvalidArgument ),
							 KittyGraphicsError::InvalidArgument, false };
				}
			}
			if constexpr ( std::is_same_v<T, KittyTransmitCommand> ) {
				return handleTransmit( value.data, value.display, false, false, cursor );
			} else if constexpr ( std::is_same_v<T, KittyQueryCommand> ) {
				return handleTransmit( value.data, false, true, false, cursor );
			} else if constexpr ( std::is_same_v<T, KittyFrameCommand> ) {
				return handleTransmit( value.data, false, false, true, cursor );
			} else if constexpr ( std::is_same_v<T, KittyPutCommand> ) {
				return put( value.data, cursor );
			} else if constexpr ( std::is_same_v<T, KittyDeleteCommand> ) {
				return remove( value.data, cursor );
			} else if constexpr ( std::is_same_v<T, KittyAnimationCommand> ) {
				return controlAnimation( value.data );
			} else if constexpr ( std::is_same_v<T, KittyComposeCommand> ) {
				return composeFrames( value.data );
			} else {
				return { response( value.data, KittyGraphicsError::Unsupported ),
						 KittyGraphicsError::Unsupported, false };
			}
		},
		*parsed.command );
}

KittyGraphicsHandleResult
KittyGraphicsProtocol::handleTransmit( const KittyGraphicsCommandData& data, bool display,
									   bool query, bool frame, Vector2i cursor ) {
	const bool more = data.more.value_or( 0 ) != 0;
	if ( data.transmission == 's' ) {
		if ( mPending.active )
			mPending = {};
		PendingTransfer transfer;
		transfer.data = data;
		transfer.data.payload = {};
		transfer.display = display;
		transfer.query = query;
		transfer.frame = frame;
		if ( !readSharedMemory( data, transfer.decodedData ) )
			return { response( data, KittyGraphicsError::DecodeFailed ),
					 KittyGraphicsError::DecodeFailed, false };
		return finishTransfer( std::move( transfer ), cursor );
	}
	if ( mPending.active ) {
		if ( frame != mPending.frame || data.format || data.dataSize || data.imageId ||
			 data.imageNumber || data.usageHint || data.placementId || data.width || data.height ||
			 data.x || data.y || data.sourceWidth || data.sourceHeight || data.columns ||
			 data.rows || data.xOffset || data.yOffset || data.zIndex || data.cursorMovement ||
			 data.virtualPlacement || data.parentImageId || data.parentPlacementId ||
			 data.parentOffsetX || data.parentOffsetY || data.compression != 0 ||
			 data.transmission != 'd' ) {
			mPending = {};
			return { response( data, KittyGraphicsError::InvalidArgument ),
					 KittyGraphicsError::InvalidArgument, false };
		}
		if ( !decodeBase64( data.payload, !more, mPending.decodedData ) ) {
			mPending = {};
			return { response( data, KittyGraphicsError::InvalidData ),
					 KittyGraphicsError::InvalidData, false };
		}
		if ( more )
			return {};
		auto transfer = std::move( mPending );
		mPending = {};
		return finishTransfer( std::move( transfer ), cursor );
	}

	if ( data.transmission != 'd' )
		return { response( data, KittyGraphicsError::Unsupported ), KittyGraphicsError::Unsupported,
				 false };
	PendingTransfer transfer;
	transfer.data = data;
	transfer.data.payload = {};
	transfer.display = display;
	transfer.query = query;
	transfer.frame = frame;
	transfer.active = true;
	transfer.decodedData = std::move( mDecodedScratch );
	transfer.decodedData.clear();
	const Uint32 format = data.format.value_or( 32 );
	if ( format == 24 || format == 32 ) {
		size_t expectedBytes = 0;
		if ( data.width && data.height &&
			 checkedPixelBytes( *data.width, *data.height, format == 24 ? 3 : 4, expectedBytes ) &&
			 expectedBytes <= 64 * 1024 * 1024 )
			transfer.decodedData.reserve( expectedBytes );
	}
	if ( !decodeBase64( data.payload, !more, transfer.decodedData ) )
		return { response( data, KittyGraphicsError::InvalidData ), KittyGraphicsError::InvalidData,
				 false };
	if ( more ) {
		mPending = std::move( transfer );
		return {};
	}
	return finishTransfer( std::move( transfer ), cursor );
}

KittyGraphicsHandleResult KittyGraphicsProtocol::finishTransfer( PendingTransfer transfer,
																 Vector2i cursor ) {
	const auto& data = transfer.data;
	mStats.decodedBytes += transfer.decodedData.size();
	const Uint32 format = data.format.value_or( 32 );
	if ( format != 24 && format != 32 && format != 100 )
		return { response( data, KittyGraphicsError::Unsupported ), KittyGraphicsError::Unsupported,
				 false };

	Uint32 width = data.width.value_or( 0 );
	Uint32 height = data.height.value_or( 0 );
	std::vector<Uint8> pixels;
	if ( format == 100 ) {
		std::vector<Uint8> encoded = std::move( mEncodedScratch );
		encoded.clear();
		if ( data.compression == 'z' ) {
			if ( !data.dataSize || *data.dataSize == 0 || *data.dataSize > 64 * 1024 * 1024 )
				return { response( data, KittyGraphicsError::TooLarge ),
						 KittyGraphicsError::TooLarge, false };
			encoded.resize( *data.dataSize );
			IOStreamMemory source( reinterpret_cast<const char*>( transfer.decodedData.data() ),
								   transfer.decodedData.size() );
			IOStreamMemory destination( reinterpret_cast<char*>( encoded.data() ), encoded.size() );
			if ( Compression::decompress( destination, source ) != Compression::OK ||
				 static_cast<size_t>( destination.tell() ) != encoded.size() ) {
				return { response( data, KittyGraphicsError::DecodeFailed ),
						 KittyGraphicsError::DecodeFailed, false };
			}
		} else if ( data.compression == 0 ) {
			mEncodedScratch = std::move( encoded );
			mEncodedScratch.clear();
			encoded = std::move( transfer.decodedData );
		} else {
			return { response( data, KittyGraphicsError::Unsupported ),
					 KittyGraphicsError::Unsupported, false };
		}
		if ( encoded.size() < 24 || std::memcmp( encoded.data(), "\x89PNG\r\n\x1a\n", 8 ) != 0 ||
			 std::memcmp( encoded.data() + 12, "IHDR", 4 ) != 0 ) {
			return { response( data, KittyGraphicsError::InvalidData ),
					 KittyGraphicsError::InvalidData, false };
		}
		width = ( static_cast<Uint32>( encoded[16] ) << 24 ) |
				( static_cast<Uint32>( encoded[17] ) << 16 ) |
				( static_cast<Uint32>( encoded[18] ) << 8 ) | encoded[19];
		height = ( static_cast<Uint32>( encoded[20] ) << 24 ) |
				 ( static_cast<Uint32>( encoded[21] ) << 16 ) |
				 ( static_cast<Uint32>( encoded[22] ) << 8 ) | encoded[23];
		size_t rgbaBytes = 0;
		if ( !checkedPixelBytes( width, height, 4, rgbaBytes ) || rgbaBytes > 128 * 1024 * 1024 ) {
			return { response( data, KittyGraphicsError::TooLarge ), KittyGraphicsError::TooLarge,
					 false };
		}
		EE::Graphics::Image decoded( encoded.data(), static_cast<unsigned int>( encoded.size() ),
									 4 );
		if ( !decoded.getPixelsPtr() || decoded.getWidth() != width ||
			 decoded.getHeight() != height )
			return { response( data, KittyGraphicsError::DecodeFailed ),
					 KittyGraphicsError::DecodeFailed, false };
		pixels.assign( decoded.getPixelsPtr(), decoded.getPixelsPtr() + rgbaBytes );
		recycleBuffer( encoded, mEncodedScratch );
	} else {
		if ( width == 0 || height == 0 )
			return { response( data, KittyGraphicsError::InvalidArgument ),
					 KittyGraphicsError::InvalidArgument, false };
		size_t sourceBytes = 0;
		if ( !checkedPixelBytes( width, height, format == 24 ? 3 : 4, sourceBytes ) ||
			 sourceBytes > 128 * 1024 * 1024 ) {
			return { response( data, KittyGraphicsError::TooLarge ), KittyGraphicsError::TooLarge,
					 false };
		}
		if ( data.compression == 'z' ) {
			pixels.resize( sourceBytes );
			IOStreamMemory source( reinterpret_cast<const char*>( transfer.decodedData.data() ),
								   transfer.decodedData.size() );
			IOStreamMemory destination( reinterpret_cast<char*>( pixels.data() ), pixels.size() );
			if ( Compression::decompress( destination, source ) != Compression::OK ||
				 static_cast<size_t>( destination.tell() ) != sourceBytes ) {
				return { response( data, KittyGraphicsError::DecodeFailed ),
						 KittyGraphicsError::DecodeFailed, false };
			}
		} else if ( data.compression != 0 ) {
			return { response( data, KittyGraphicsError::Unsupported ),
					 KittyGraphicsError::Unsupported, false };
		} else {
			if ( transfer.decodedData.size() != sourceBytes )
				return { response( data, KittyGraphicsError::InvalidData ),
						 KittyGraphicsError::InvalidData, false };
			pixels = std::move( transfer.decodedData );
		}
	}

	if ( format == 24 ) {
		std::vector<Uint8> rgba = std::move( mPixelScratch );
		rgba.resize( static_cast<size_t>( width ) * height * 4 );
		for ( size_t sourceOffset = 0, destinationOffset = 0; sourceOffset < pixels.size();
			  sourceOffset += 3, destinationOffset += 4 ) {
			rgba[destinationOffset] = pixels[sourceOffset];
			rgba[destinationOffset + 1] = pixels[sourceOffset + 1];
			rgba[destinationOffset + 2] = pixels[sourceOffset + 2];
			rgba[destinationOffset + 3] = 255;
		}
		recycleBuffer( pixels, mDecodedScratch );
		pixels = std::move( rgba );
	}
	if ( transfer.frame ) {
		const KittyImageId imageId = resolveImageId( data );
		auto image = mImages.find( imageId );
		if ( image == mImages.end() )
			return { response( data, KittyGraphicsError::NotFound ), KittyGraphicsError::NotFound,
					 false };
		Uint32 frameNumber = data.rows.value_or( 0 );
		if ( frameNumber == 0 ) {
			frameNumber = 2;
			for ( const auto& frame : image->second.frames )
				frameNumber = std::max( frameNumber, frame.first + 1 );
		}
		const Uint32 destinationX = data.x.value_or( 0 );
		const Uint32 destinationY = data.y.value_or( 0 );
		if ( destinationX >= static_cast<Uint32>( image->second.size.getWidth() ) ||
			 destinationY >= static_cast<Uint32>( image->second.size.getHeight() ) ||
			 width > image->second.size.getWidth() - destinationX ||
			 height > image->second.size.getHeight() - destinationY ) {
			return { response( data, KittyGraphicsError::InvalidArgument ),
					 KittyGraphicsError::InvalidArgument, false };
		}

		std::vector<Uint8>* destinationPixels = nullptr;
		bool createdFrame = false;
		if ( frameNumber == 1 ) {
			if ( image->second.rgba.use_count() != 1 )
				image->second.rgba = std::make_shared<std::vector<Uint8>>( *image->second.rgba );
			destinationPixels = image->second.rgba.get();
		} else {
			auto frame = image->second.frames.find( frameNumber );
			if ( frame == image->second.frames.end() ) {
				Image::Frame newFrame;
				newFrame.rgba.resize( static_cast<size_t>( image->second.size.getWidth() ) *
									  image->second.size.getHeight() * 4 );
				if ( data.columns ) {
					const Uint32 baseFrame = *data.columns;
					if ( baseFrame == 1 )
						newFrame.rgba = *image->second.rgba;
					else {
						auto base = image->second.frames.find( baseFrame );
						if ( base == image->second.frames.end() )
							return { response( data, KittyGraphicsError::NotFound ),
									 KittyGraphicsError::NotFound, false };
						newFrame.rgba = base->second.rgba;
					}
				} else if ( data.yOffset ) {
					const Uint32 color = *data.yOffset;
					for ( size_t offset = 0; offset < newFrame.rgba.size(); offset += 4 ) {
						newFrame.rgba[offset] = static_cast<Uint8>( color >> 24 );
						newFrame.rgba[offset + 1] = static_cast<Uint8>( color >> 16 );
						newFrame.rgba[offset + 2] = static_cast<Uint8>( color >> 8 );
						newFrame.rgba[offset + 3] = static_cast<Uint8>( color );
					}
				}
				const size_t maxFrameStorage =
					mMaxStorageBytes > std::numeric_limits<size_t>::max() / 5
						? std::numeric_limits<size_t>::max()
						: mMaxStorageBytes * 5;
				if ( mFrameStorageBytes > maxFrameStorage ||
					 newFrame.rgba.size() > maxFrameStorage - mFrameStorageBytes )
					return { response( data, KittyGraphicsError::NoSpace ),
							 KittyGraphicsError::NoSpace, false };
				newFrame.gapMs = data.zIndex.value_or( 40 );
				newFrame.usageHint = data.usageHint.value_or( 0 );
				mFrameStorageBytes += newFrame.rgba.size();
				frame = image->second.frames.emplace( frameNumber, std::move( newFrame ) ).first;
				createdFrame = true;
			} else {
				if ( data.zIndex && *data.zIndex != 0 )
					frame->second.gapMs = *data.zIndex;
			}
			destinationPixels = &frame->second.rgba;
		}

		const bool replace = data.xOffset.value_or( 0 ) == 1;
		std::vector<Uint8> finalPatch( pixels.size() );
		const size_t imageStride = static_cast<size_t>( image->second.size.getWidth() ) * 4;
		const size_t patchStride = static_cast<size_t>( width ) * 4;
		for ( Uint32 row = 0; row < height; ++row ) {
			Uint8* destination = destinationPixels->data() +
								 static_cast<size_t>( destinationY + row ) * imageStride +
								 static_cast<size_t>( destinationX ) * 4;
			const Uint8* source = pixels.data() + static_cast<size_t>( row ) * patchStride;
			Uint8* published = finalPatch.data() + static_cast<size_t>( row ) * patchStride;
			if ( replace ) {
				std::memcpy( destination, source, patchStride );
				std::memcpy( published, source, patchStride );
				continue;
			}
			for ( size_t column = 0; column < patchStride; column += 4 ) {
				const Uint32 alpha = source[column + 3];
				const Uint32 inverseAlpha = 255 - alpha;
				for ( size_t channel = 0; channel < 3; ++channel )
					destination[column + channel] =
						static_cast<Uint8>( ( source[column + channel] * alpha +
											  destination[column + channel] * inverseAlpha + 127 ) /
											255 );
				destination[column + 3] = static_cast<Uint8>(
					alpha + ( destination[column + 3] * inverseAlpha + 127 ) / 255 );
				std::memcpy( published + column, destination + column, 4 );
			}
		}
		TerminalGraphicsUpdate update;
		update.type = frameNumber == 1 ? TerminalGraphicsUpdateType::UpdateRegion
					  : createdFrame   ? TerminalGraphicsUpdateType::CreateFrame
									   : TerminalGraphicsUpdateType::UpdateFrameRegion;
		update.imageId = imageId;
		update.frameNumber = frameNumber;
		update.imageSize = image->second.size;
		update.region =
			createdFrame
				? Rect( 0, 0, image->second.size.getWidth(), image->second.size.getHeight() )
				: Rect( destinationX, destinationY, destinationX + width, destinationY + height );
		update.rgba = createdFrame
						  ? std::make_shared<const std::vector<Uint8>>( *destinationPixels )
						  : std::make_shared<const std::vector<Uint8>>( std::move( finalPatch ) );
		if ( !createdFrame && !mUpdates.empty() && mUpdates.back().type == update.type &&
			 mUpdates.back().imageId == update.imageId &&
			 mUpdates.back().frameNumber == update.frameNumber &&
			 mUpdates.back().region == update.region )
			mUpdates.back() = std::move( update );
		else
			mUpdates.emplace_back( std::move( update ) );
		++mStats.rectangleUpdates;
		mPresentationDirty = true;
		if ( format == 24 )
			recycleBuffer( pixels, mPixelScratch );
		else if ( format == 32 )
			recycleBuffer( pixels, mDecodedScratch );
		return { response( data, KittyGraphicsError::None, imageId ), KittyGraphicsError::None,
				 true };
	}

	const bool anonymous = !data.imageId && !data.imageNumber;
	KittyImageId imageId = data.imageId.value_or( 0 );
	if ( anonymous && transfer.display ) {
		auto placement = std::find_if(
			mPlacements.begin(), mPlacements.end(), [&]( const Placement& candidate ) {
				auto candidateImage = mImages.find( candidate.visible.imageId );
				return candidate.visible.visibleAnchorCell == cursor &&
					   candidate.visible.placementId == 0 && candidateImage != mImages.end() &&
					   candidateImage->second.anonymous;
			} );
		if ( placement != mPlacements.end() )
			imageId = placement->visible.imageId;
	}
	if ( imageId == 0 )
		imageId = allocateImageId();
	if ( imageId == 0 )
		return { response( data, KittyGraphicsError::NoSpace ), KittyGraphicsError::NoSpace,
				 false };
	if ( transfer.query )
		return { response( data, KittyGraphicsError::None, imageId ), KittyGraphicsError::None,
				 false };

	auto existing = mImages.find( imageId );
	const size_t oldBytes = existing == mImages.end() ? 0 : existing->second.rgba->size();
	if ( !ensureCapacity( pixels.size(), imageId, existing == mImages.end() ) )
		return { response( data, KittyGraphicsError::NoSpace ), KittyGraphicsError::NoSpace,
				 false };
	existing = mImages.find( imageId );
	if ( existing != mImages.end() ) {
		for ( const auto& frame : existing->second.frames )
			mFrameStorageBytes -= frame.second.rgba.size();
		mPlacements.erase( std::remove_if( mPlacements.begin(), mPlacements.end(),
										   [imageId]( const Placement& p ) {
											   return p.visible.imageId == imageId;
										   } ),
						   mPlacements.end() );
		mPrimaryPlacements.erase( std::remove_if( mPrimaryPlacements.begin(),
												  mPrimaryPlacements.end(),
												  [imageId]( const Placement& p ) {
													  return p.visible.imageId == imageId;
												  } ),
								  mPrimaryPlacements.end() );
	}
	std::shared_ptr<std::vector<Uint8>> pixelStorage;
	if ( existing != mImages.end() && existing->second.rgba.use_count() == 1 ) {
		pixelStorage = existing->second.rgba;
		pixelStorage->assign( pixels.begin(), pixels.end() );
		if ( format == 24 )
			recycleBuffer( pixels, mPixelScratch );
		else if ( format == 32 )
			recycleBuffer( pixels, mDecodedScratch );
	} else {
		pixelStorage = std::make_shared<std::vector<Uint8>>( std::move( pixels ) );
	}
	Image image;
	image.size = Sizei( width, height );
	image.imageNumber = data.imageNumber.value_or( 0 );
	image.usageHint = data.usageHint.value_or( 0 );
	image.anonymous = anonymous;
	image.creationSerial = ++mCreationSerial;
	image.rgba = std::move( pixelStorage );
	mStorageBytes = mStorageBytes - oldBytes + image.rgba->size();
	const bool replaced = existing != mImages.end();
	auto inserted = mImages.insert_or_assign( imageId, std::move( image ) ).first;

	TerminalGraphicsUpdate update;
	update.type = replaced ? TerminalGraphicsUpdateType::ReplaceImage
						   : TerminalGraphicsUpdateType::CreateImage;
	update.imageId = imageId;
	update.imageSize = inserted->second.size;
	update.region =
		Rect( 0, 0, inserted->second.size.getWidth(), inserted->second.size.getHeight() );
	update.rgba = inserted->second.rgba;
	mUpdates.emplace_back( std::move( update ) );
	++mStats.fullImageUpdates;
	++mPresentationGeneration;
	mPresentationDirty = true;
	auto result = KittyGraphicsHandleResult{
		response( data, KittyGraphicsError::None, anonymous ? 0 : imageId ),
		KittyGraphicsError::None, true };
	if ( transfer.display ) {
		auto placementData = data;
		placementData.imageId = imageId;
		placementData.imageNumber.reset();
		auto placementResult = put( placementData, cursor );
		if ( placementResult.error != KittyGraphicsError::None )
			return placementResult;
		result.cursorMovement = placementResult.cursorMovement;
	}
	return result;
}

KittyGraphicsHandleResult KittyGraphicsProtocol::put( const KittyGraphicsCommandData& data,
													  Vector2i cursor ) {
	const KittyImageId imageId = resolveImageId( data );
	auto image = mImages.find( imageId );
	if ( image == mImages.end() )
		return { response( data, KittyGraphicsError::NotFound ), KittyGraphicsError::NotFound,
				 false };
	const Uint32 sourceX = data.x.value_or( 0 );
	const Uint32 sourceY = data.y.value_or( 0 );
	if ( sourceX >= static_cast<Uint32>( image->second.size.getWidth() ) ||
		 sourceY >= static_cast<Uint32>( image->second.size.getHeight() ) ) {
		return { response( data, KittyGraphicsError::InvalidArgument ),
				 KittyGraphicsError::InvalidArgument, false };
	}
	const Uint32 sourceWidth =
		std::min<Uint32>( data.sourceWidth.value_or( image->second.size.getWidth() - sourceX ),
						  image->second.size.getWidth() - sourceX );
	const Uint32 sourceHeight =
		std::min<Uint32>( data.sourceHeight.value_or( image->second.size.getHeight() - sourceY ),
						  image->second.size.getHeight() - sourceY );
	if ( sourceWidth == 0 || sourceHeight == 0 ) {
		return { response( data, KittyGraphicsError::InvalidArgument ),
				 KittyGraphicsError::InvalidArgument, false };
	}

	TerminalVisiblePlacement visible;
	visible.imageId = imageId;
	visible.placementId = data.placementId.value_or( 0 );
	visible.visibleAnchorCell = cursor;
	const bool relative = data.parentImageId.has_value() || data.parentPlacementId.has_value();
	if ( relative ) {
		if ( data.virtualPlacement.value_or( 0 ) == 1 )
			return { response( data, KittyGraphicsError::InvalidArgument ),
					 KittyGraphicsError::InvalidArgument, false };
		if ( !data.parentImageId || !data.parentPlacementId )
			return { response( data, KittyGraphicsError::InvalidArgument ),
					 KittyGraphicsError::InvalidArgument, false };
		auto parent =
			std::find_if( mPlacements.begin(), mPlacements.end(), [&]( const Placement& p ) {
				return p.visible.imageId == *data.parentImageId &&
					   p.visible.placementId == *data.parentPlacementId;
			} );
		if ( parent == mPlacements.end() )
			return { response( data, KittyGraphicsError::NoParent ), KittyGraphicsError::NoParent,
					 false };
		KittyImageId ancestorImage = parent->visible.imageId;
		KittyPlacementId ancestorPlacement = parent->visible.placementId;
		for ( size_t depth = 0; ancestorImage != 0; ++depth ) {
			if ( ancestorImage == imageId && ancestorPlacement == visible.placementId )
				return { response( data, KittyGraphicsError::Cycle ), KittyGraphicsError::Cycle,
						 false };
			if ( depth >= 64 )
				return { response( data, KittyGraphicsError::TooDeep ), KittyGraphicsError::TooDeep,
						 false };
			auto ancestor = std::find_if(
				mPlacements.begin(), mPlacements.end(), [&]( const Placement& placement ) {
					return placement.visible.imageId == ancestorImage &&
						   placement.visible.placementId == ancestorPlacement;
				} );
			if ( ancestor == mPlacements.end() || ancestor->parentImageId == 0 )
				break;
			ancestorImage = ancestor->parentImageId;
			ancestorPlacement = ancestor->parentPlacementId;
		}
		visible.visibleAnchorCell =
			parent->visible.visibleAnchorCell +
			Vector2i( data.parentOffsetX.value_or( 0 ), data.parentOffsetY.value_or( 0 ) );
	}
	visible.sourcePixels = Rect( sourceX, sourceY, sourceX + sourceWidth, sourceY + sourceHeight );
	visible.columns = data.columns.value_or( 0 );
	visible.rows = data.rows.value_or( 0 );
	if ( visible.columns == 0 && visible.rows == 0 && mCellPixelWidth && mCellPixelHeight ) {
		visible.columns =
			static_cast<Uint32>( ( static_cast<Uint64>( sourceWidth ) + data.xOffset.value_or( 0 ) +
								   mCellPixelWidth - 1 ) /
								 mCellPixelWidth );
		visible.rows = static_cast<Uint32>( ( static_cast<Uint64>( sourceHeight ) +
											  data.yOffset.value_or( 0 ) + mCellPixelHeight - 1 ) /
											mCellPixelHeight );
	} else if ( visible.columns != 0 && visible.rows == 0 && mCellPixelHeight ) {
		const Uint64 scaledHeight =
			static_cast<Uint64>( sourceHeight ) * visible.columns * mCellPixelWidth;
		const Uint64 denominator = static_cast<Uint64>( sourceWidth ) * mCellPixelHeight;
		visible.rows = static_cast<Uint32>( ( scaledHeight + denominator - 1 ) / denominator );
	} else if ( visible.rows != 0 && visible.columns == 0 && mCellPixelWidth ) {
		const Uint64 scaledWidth =
			static_cast<Uint64>( sourceWidth ) * visible.rows * mCellPixelHeight;
		const Uint64 denominator = static_cast<Uint64>( sourceHeight ) * mCellPixelWidth;
		visible.columns = static_cast<Uint32>( ( scaledWidth + denominator - 1 ) / denominator );
	}
	if ( visible.columns == 0 )
		visible.columns = 1;
	if ( visible.rows == 0 )
		visible.rows = 1;
	visible.firstCellPixelOffset =
		Vector2i( data.xOffset.value_or( 0 ), data.yOffset.value_or( 0 ) );
	if ( ( mCellPixelWidth && data.xOffset.value_or( 0 ) >= mCellPixelWidth ) ||
		 ( mCellPixelHeight && data.yOffset.value_or( 0 ) >= mCellPixelHeight ) )
		return { response( data, KittyGraphicsError::InvalidArgument ),
				 KittyGraphicsError::InvalidArgument, false };
	visible.zIndex = data.zIndex.value_or( 0 );
	if ( visible.columns == 0 || visible.rows == 0 )
		return { response( data, KittyGraphicsError::InvalidArgument ),
				 KittyGraphicsError::InvalidArgument, false };
	bool replacingPlacement = false;
	if ( visible.placementId != 0 ) {
		for ( auto it = mPlacements.begin(); it != mPlacements.end(); ++it ) {
			if ( it->visible.imageId == imageId &&
				 it->visible.placementId == visible.placementId ) {
				const Vector2i movement = visible.visibleAnchorCell - it->visible.visibleAnchorCell;
				std::vector<std::pair<KittyImageId, KittyPlacementId>> movedParents{
					{ imageId, visible.placementId } };
				for ( size_t parentIndex = 0; parentIndex < movedParents.size(); ++parentIndex ) {
					for ( auto& child : mPlacements ) {
						if ( child.parentImageId == movedParents[parentIndex].first &&
							 child.parentPlacementId == movedParents[parentIndex].second ) {
							child.visible.visibleAnchorCell += movement;
							movedParents.emplace_back( child.visible.imageId,
													   child.visible.placementId );
						}
					}
				}
				mPlacements.erase( it );
				replacingPlacement = true;
				break;
			}
		}
	}
	if ( !replacingPlacement && mPlacements.size() >= mMaxPlacements )
		return { response( data, KittyGraphicsError::NoSpace ), KittyGraphicsError::NoSpace,
				 false };
	mPlacements.push_back( { visible, ++mPlacementSerial, data.virtualPlacement.value_or( 0 ) == 1,
							 data.parentImageId.value_or( 0 ),
							 data.parentPlacementId.value_or( 0 ) } );
	++mPresentationGeneration;
	mPresentationDirty = true;
	KittyGraphicsHandleResult result{ response( data, KittyGraphicsError::None, imageId ),
									  KittyGraphicsError::None, true };
	if ( data.cursorMovement.value_or( 0 ) == 0 && !relative &&
		 data.virtualPlacement.value_or( 0 ) == 0 )
		result.cursorMovement = Vector2i( visible.columns, visible.rows );
	return result;
}

KittyGraphicsHandleResult KittyGraphicsProtocol::remove( const KittyGraphicsCommandData& data,
														 Vector2i cursor ) {
	mPending = {};
	const char selector = data.deletion;
	const bool deleteImageData = selector >= 'A' && selector <= 'Z';
	const char normalized = deleteImageData ? static_cast<char>( selector - 'A' + 'a' ) : selector;
	constexpr std::string_view ValidSelectors = "acfinpqrxyz";
	if ( ValidSelectors.find( normalized ) == std::string_view::npos )
		return { response( data, KittyGraphicsError::InvalidArgument ),
				 KittyGraphicsError::InvalidArgument, false };

	const KittyImageId selectedImage = resolveImageId( data );
	if ( ( normalized == 'f' || normalized == 'i' || normalized == 'n' ) && selectedImage == 0 )
		return { response( data, KittyGraphicsError::NotFound ), KittyGraphicsError::NotFound,
				 false };
	if ( normalized == 'f' ) {
		auto image = mImages.find( selectedImage );
		if ( image == mImages.end() )
			return { response( data, KittyGraphicsError::NotFound ), KittyGraphicsError::NotFound,
					 false };
		const bool changed = !image->second.frames.empty();
		for ( const auto& frame : image->second.frames ) {
			mFrameStorageBytes -= frame.second.rgba.size();
			TerminalGraphicsUpdate update;
			update.type = TerminalGraphicsUpdateType::DeleteFrame;
			update.imageId = selectedImage;
			update.frameNumber = frame.first;
			mUpdates.emplace_back( std::move( update ) );
		}
		image->second.frames.clear();
		image->second.currentFrame = 1;
		image->second.animationState = 1;
		if ( changed ) {
			++mPresentationGeneration;
			mPresentationDirty = true;
		}
		return { response( data, KittyGraphicsError::None, selectedImage ),
				 KittyGraphicsError::None, changed };
	}

	Vector2i selectedCell = cursor;
	if ( normalized == 'p' || normalized == 'q' ) {
		if ( !data.x || !data.y || *data.x == 0 || *data.y == 0 )
			return { response( data, KittyGraphicsError::InvalidArgument ),
					 KittyGraphicsError::InvalidArgument, false };
		selectedCell =
			Vector2i( static_cast<Int32>( *data.x - 1 ), static_cast<Int32>( *data.y - 1 ) );
	}
	if ( normalized == 'x' && ( !data.x || *data.x == 0 ) )
		return { response( data, KittyGraphicsError::InvalidArgument ),
				 KittyGraphicsError::InvalidArgument, false };
	if ( normalized == 'y' && ( !data.y || *data.y == 0 ) )
		return { response( data, KittyGraphicsError::InvalidArgument ),
				 KittyGraphicsError::InvalidArgument, false };
	if ( normalized == 'r' && ( !data.x || !data.y || *data.x > *data.y ) )
		return { response( data, KittyGraphicsError::InvalidArgument ),
				 KittyGraphicsError::InvalidArgument, false };
	if ( normalized == 'z' && !data.zIndex )
		return { response( data, KittyGraphicsError::InvalidArgument ),
				 KittyGraphicsError::InvalidArgument, false };

	std::vector<KittyImageId> affectedImages;
	auto matches = [&]( const Placement& placement ) {
		auto visible = placement.visible;
		visible.visibleAnchorCell.y += mScrollPosition;
		if ( placement.virtualPlacement && normalized != 'i' && normalized != 'n' &&
			 normalized != 'r' )
			return false;
		bool match = false;
		switch ( normalized ) {
			case 'a':
				match = static_cast<Int64>( visible.visibleAnchorCell.y ) + visible.rows > 0 &&
						( mScreenRows == 0 || visible.visibleAnchorCell.y < mScreenRows );
				break;
			case 'c':
				match = placementContains( visible, cursor );
				break;
			case 'i':
			case 'n':
				match = visible.imageId == selectedImage;
				if ( match && data.placementId )
					match = visible.placementId == *data.placementId;
				break;
			case 'p':
				match = placementContains( visible, selectedCell );
				break;
			case 'q':
				match = placementContains( visible, selectedCell ) && data.zIndex &&
						visible.zIndex == *data.zIndex;
				break;
			case 'r':
				match = visible.imageId >= *data.x && visible.imageId <= *data.y;
				break;
			case 'x':
				match = placementContains( visible, Vector2i( static_cast<Int32>( *data.x - 1 ),
															  visible.visibleAnchorCell.y ) );
				break;
			case 'y':
				match = placementContains( visible, Vector2i( visible.visibleAnchorCell.x,
															  static_cast<Int32>( *data.y - 1 ) ) );
				break;
			case 'z':
				match = visible.zIndex == *data.zIndex;
				break;
		}
		if ( match )
			affectedImages.emplace_back( visible.imageId );
		return match;
	};
	const size_t oldPlacementCount = mPlacements.size();
	mPlacements.erase( std::remove_if( mPlacements.begin(), mPlacements.end(), matches ),
					   mPlacements.end() );
	std::vector<KittyImageId> orphanedRelativeImages;
	bool removedChild = true;
	while ( removedChild ) {
		removedChild = false;
		mPlacements.erase(
			std::remove_if(
				mPlacements.begin(), mPlacements.end(),
				[&]( const Placement& placement ) {
					if ( placement.parentImageId == 0 )
						return false;
					const bool parentExists = std::any_of(
						mPlacements.begin(), mPlacements.end(), [&]( const Placement& parent ) {
							return parent.visible.imageId == placement.parentImageId &&
								   parent.visible.placementId == placement.parentPlacementId;
						} );
					if ( !parentExists ) {
						affectedImages.emplace_back( placement.visible.imageId );
						orphanedRelativeImages.emplace_back( placement.visible.imageId );
						removedChild = true;
					}
					return !parentExists;
				} ),
			mPlacements.end() );
	}
	std::sort( orphanedRelativeImages.begin(), orphanedRelativeImages.end() );
	orphanedRelativeImages.erase(
		std::unique( orphanedRelativeImages.begin(), orphanedRelativeImages.end() ),
		orphanedRelativeImages.end() );
	for ( KittyImageId imageId : orphanedRelativeImages ) {
		if ( !isImagePlaced( imageId ) )
			eraseImage( imageId );
	}

	bool deletedAnyImage = false;
	if ( deleteImageData ) {
		if ( normalized == 'a' ) {
			for ( const auto& image : mImages )
				affectedImages.emplace_back( image.first );
		} else if ( normalized == 'i' || normalized == 'n' ) {
			affectedImages.emplace_back( selectedImage );
		} else if ( normalized == 'r' ) {
			for ( const auto& image : mImages ) {
				if ( image.first >= *data.x && image.first <= *data.y )
					affectedImages.emplace_back( image.first );
			}
		}
		std::sort( affectedImages.begin(), affectedImages.end() );
		affectedImages.erase( std::unique( affectedImages.begin(), affectedImages.end() ),
							  affectedImages.end() );
		for ( KittyImageId imageId : affectedImages ) {
			if ( isImagePlaced( imageId ) )
				continue;
			auto image = mImages.find( imageId );
			if ( image == mImages.end() )
				continue;
			eraseImage( imageId );
			deletedAnyImage = true;
		}
	}

	const bool changed = oldPlacementCount != mPlacements.size() || deletedAnyImage;
	if ( changed ) {
		++mPresentationGeneration;
		mPresentationDirty = true;
	}
	return { response( data, KittyGraphicsError::None, selectedImage ), KittyGraphicsError::None,
			 changed };
}

KittyGraphicsHandleResult
KittyGraphicsProtocol::controlAnimation( const KittyGraphicsCommandData& data ) {
	const KittyImageId imageId = resolveImageId( data );
	auto image = mImages.find( imageId );
	if ( image == mImages.end() )
		return { response( data, KittyGraphicsError::NotFound ), KittyGraphicsError::NotFound,
				 false };
	bool changed = false;
	if ( data.columns ) {
		const Uint32 frameNumber = *data.columns;
		if ( frameNumber == 0 || ( frameNumber != 1 && image->second.frames.find( frameNumber ) ==
														   image->second.frames.end() ) )
			return { response( data, KittyGraphicsError::NotFound ), KittyGraphicsError::NotFound,
					 false };
		changed = image->second.currentFrame != frameNumber;
		image->second.currentFrame = frameNumber;
	}
	if ( data.width ) {
		if ( *data.width < 1 || *data.width > 3 )
			return { response( data, KittyGraphicsError::InvalidArgument ),
					 KittyGraphicsError::InvalidArgument, false };
		image->second.animationState = static_cast<Uint8>( *data.width );
		image->second.frameClock.restart();
		if ( *data.width == 1 )
			image->second.loopsCompleted = 0;
		changed = true;
	}
	if ( data.height && *data.height != 0 ) {
		image->second.loopCount = *data.height;
		changed = true;
	}
	if ( data.zIndex && *data.zIndex != 0 ) {
		const Uint32 frameNumber = data.rows.value_or( image->second.currentFrame );
		if ( frameNumber == 1 )
			image->second.rootGapMs = *data.zIndex;
		else {
			auto frame = image->second.frames.find( frameNumber );
			if ( frame == image->second.frames.end() )
				return { response( data, KittyGraphicsError::NotFound ),
						 KittyGraphicsError::NotFound, false };
			frame->second.gapMs = *data.zIndex;
		}
		changed = true;
	}
	if ( changed ) {
		++mPresentationGeneration;
		mPresentationDirty = true;
	}
	return { response( data, KittyGraphicsError::None, imageId ), KittyGraphicsError::None,
			 changed };
}

bool KittyGraphicsProtocol::updateAnimations() {
	bool changed = false;
	for ( auto& entry : mImages ) {
		auto& image = entry.second;
		if ( image.animationState < 2 || image.frames.empty() )
			continue;
		Int32 gap = image.rootGapMs;
		if ( image.currentFrame != 1 ) {
			auto current = image.frames.find( image.currentFrame );
			if ( current == image.frames.end() )
				continue;
			gap = current->second.gapMs;
		}
		if ( gap > 0 && image.frameClock.getElapsedTime().asMilliseconds() < gap )
			continue;
		Uint32 nextFrame = std::numeric_limits<Uint32>::max();
		for ( const auto& frame : image.frames ) {
			if ( frame.first > image.currentFrame && frame.first < nextFrame )
				nextFrame = frame.first;
		}
		if ( nextFrame == std::numeric_limits<Uint32>::max() ) {
			if ( image.animationState == 2 )
				continue;
			++image.loopsCompleted;
			if ( image.loopCount > 1 && image.loopsCompleted >= image.loopCount - 1 ) {
				image.animationState = 1;
				continue;
			}
			nextFrame = 1;
		}
		image.currentFrame = nextFrame;
		image.frameClock.restart();
		changed = true;
	}
	if ( changed ) {
		++mPresentationGeneration;
		mPresentationDirty = true;
	}
	return changed;
}

void KittyGraphicsProtocol::setCellPixelSize( Uint32 width, Uint32 height ) {
	mCellPixelWidth = width;
	mCellPixelHeight = height;
}

void KittyGraphicsProtocol::setPlaceholderCells(
	std::vector<TerminalGraphicsPlaceholderCell> cells ) {
	if ( mPlaceholderCells == cells )
		return;
	mPlaceholderCells = std::move( cells );
	++mPresentationGeneration;
	mPresentationDirty = true;
}

KittyGraphicsHandleResult
KittyGraphicsProtocol::composeFrames( const KittyGraphicsCommandData& data ) {
	const KittyImageId imageId = resolveImageId( data );
	auto image = mImages.find( imageId );
	if ( image == mImages.end() || !data.columns || !data.rows )
		return { response( data, image == mImages.end() ? KittyGraphicsError::NotFound
														: KittyGraphicsError::InvalidArgument ),
				 image == mImages.end() ? KittyGraphicsError::NotFound
										: KittyGraphicsError::InvalidArgument,
				 false };
	auto pixelsFor = [&]( Uint32 frameNumber ) -> std::vector<Uint8>* {
		if ( frameNumber == 1 )
			return image->second.rgba.get();
		auto frame = image->second.frames.find( frameNumber );
		return frame == image->second.frames.end() ? nullptr : &frame->second.rgba;
	};
	const Uint32 sourceFrame = *data.rows;
	const Uint32 destinationFrame = *data.columns;
	auto* source = pixelsFor( sourceFrame );
	auto* destination = pixelsFor( destinationFrame );
	if ( !source || !destination )
		return { response( data, KittyGraphicsError::NotFound ), KittyGraphicsError::NotFound,
				 false };
	const Uint32 destinationX = data.x.value_or( 0 );
	const Uint32 destinationY = data.y.value_or( 0 );
	const Uint32 sourceX = data.xOffset.value_or( 0 );
	const Uint32 sourceY = data.yOffset.value_or( 0 );
	const Uint32 width = data.sourceWidth.value_or( image->second.size.getWidth() );
	const Uint32 height = data.sourceHeight.value_or( image->second.size.getHeight() );
	const Uint32 imageWidth = image->second.size.getWidth();
	const Uint32 imageHeight = image->second.size.getHeight();
	if ( width == 0 || height == 0 || sourceX > imageWidth || sourceY > imageHeight ||
		 destinationX > imageWidth || destinationY > imageHeight || width > imageWidth - sourceX ||
		 height > imageHeight - sourceY || width > imageWidth - destinationX ||
		 height > imageHeight - destinationY )
		return { response( data, KittyGraphicsError::InvalidArgument ),
				 KittyGraphicsError::InvalidArgument, false };
	if ( sourceFrame == destinationFrame && sourceX < destinationX + width &&
		 destinationX < sourceX + width && sourceY < destinationY + height &&
		 destinationY < sourceY + height )
		return { response( data, KittyGraphicsError::InvalidArgument ),
				 KittyGraphicsError::InvalidArgument, false };

	std::vector<Uint8> sourceCopy = std::move( mComposeSourceScratch );
	sourceCopy.resize( static_cast<size_t>( width ) * height * 4 );
	std::vector<Uint8> result( sourceCopy.size() );
	const size_t imageStride = static_cast<size_t>( imageWidth ) * 4;
	const size_t rowBytes = static_cast<size_t>( width ) * 4;
	for ( Uint32 row = 0; row < height; ++row )
		std::memcpy( sourceCopy.data() + static_cast<size_t>( row ) * rowBytes,
					 source->data() + static_cast<size_t>( sourceY + row ) * imageStride +
						 static_cast<size_t>( sourceX ) * 4,
					 rowBytes );
	if ( destinationFrame == 1 && image->second.rgba.use_count() != 1 ) {
		image->second.rgba = std::make_shared<std::vector<Uint8>>( *image->second.rgba );
		destination = image->second.rgba.get();
	}
	const bool replace = data.cursorMovement.value_or( 0 ) == 1;
	for ( Uint32 row = 0; row < height; ++row ) {
		Uint8* target = destination->data() +
						static_cast<size_t>( destinationY + row ) * imageStride +
						static_cast<size_t>( destinationX ) * 4;
		const Uint8* overlay = sourceCopy.data() + static_cast<size_t>( row ) * rowBytes;
		Uint8* published = result.data() + static_cast<size_t>( row ) * rowBytes;
		for ( size_t offset = 0; offset < rowBytes; offset += 4 ) {
			if ( replace ) {
				std::memcpy( target + offset, overlay + offset, 4 );
			} else {
				const Uint32 alpha = overlay[offset + 3];
				const Uint32 inverseAlpha = 255 - alpha;
				for ( size_t channel = 0; channel < 3; ++channel )
					target[offset + channel] =
						static_cast<Uint8>( ( overlay[offset + channel] * alpha +
											  target[offset + channel] * inverseAlpha + 127 ) /
											255 );
				target[offset + 3] =
					static_cast<Uint8>( alpha + ( target[offset + 3] * inverseAlpha + 127 ) / 255 );
			}
			std::memcpy( published + offset, target + offset, 4 );
		}
	}
	recycleBuffer( sourceCopy, mComposeSourceScratch );
	TerminalGraphicsUpdate update;
	update.type = destinationFrame == 1 ? TerminalGraphicsUpdateType::UpdateRegion
										: TerminalGraphicsUpdateType::UpdateFrameRegion;
	update.imageId = imageId;
	update.frameNumber = destinationFrame;
	update.imageSize = image->second.size;
	update.region = Rect( destinationX, destinationY, destinationX + width, destinationY + height );
	update.rgba = std::make_shared<const std::vector<Uint8>>( std::move( result ) );
	mUpdates.emplace_back( std::move( update ) );
	mPresentationDirty = true;
	return { response( data, KittyGraphicsError::None, imageId ), KittyGraphicsError::None, true };
}

KittyImageId KittyGraphicsProtocol::resolveImageId( const KittyGraphicsCommandData& data ) const {
	if ( data.imageId )
		return *data.imageId;
	KittyImageId newestId = 0;
	Uint64 newestSerial = 0;
	if ( data.imageNumber ) {
		for ( const auto& image : mImages ) {
			if ( image.second.imageNumber == *data.imageNumber &&
				 image.second.creationSerial > newestSerial ) {
				newestId = image.first;
				newestSerial = image.second.creationSerial;
			}
		}
	}
	return newestId;
}

KittyImageId KittyGraphicsProtocol::allocateImageId() {
	for ( Uint64 attempts = 0; attempts < std::numeric_limits<Uint32>::max(); ++attempts ) {
		const KittyImageId candidate = mNextImageId++;
		if ( mNextImageId == 0 )
			mNextImageId = 1;
		if ( candidate != 0 && mImages.find( candidate ) == mImages.end() )
			return candidate;
	}
	return 0;
}

bool KittyGraphicsProtocol::isImagePlaced( KittyImageId imageId ) const {
	auto belongsToImage = [imageId]( const Placement& placement ) {
		return placement.visible.imageId == imageId;
	};
	return std::any_of( mPlacements.begin(), mPlacements.end(), belongsToImage ) ||
		   std::any_of( mPrimaryPlacements.begin(), mPrimaryPlacements.end(), belongsToImage );
}

void KittyGraphicsProtocol::eraseImage( KittyImageId imageId ) {
	auto image = mImages.find( imageId );
	if ( image == mImages.end() )
		return;
	mStorageBytes -= image->second.rgba->size();
	for ( const auto& frame : image->second.frames )
		mFrameStorageBytes -= frame.second.rgba.size();
	mImages.erase( image );
	TerminalGraphicsUpdate update;
	update.type = TerminalGraphicsUpdateType::DeleteImage;
	update.imageId = imageId;
	mUpdates.emplace_back( std::move( update ) );
}

bool KittyGraphicsProtocol::ensureCapacity( size_t bytes, KittyImageId replacingId,
											bool addingImage ) {
	auto replaced = mImages.find( replacingId );
	const size_t replacedBytes = replaced == mImages.end() ? 0 : replaced->second.rgba->size();
	auto hasCapacity = [&] {
		return bytes <= mMaxStorageBytes &&
			   mStorageBytes - replacedBytes <= mMaxStorageBytes - bytes &&
			   ( !addingImage || mImages.size() < mMaxImages );
	};
	while ( !hasCapacity() ) {
		auto candidate = mImages.end();
		for ( auto image = mImages.begin(); image != mImages.end(); ++image ) {
			if ( image->first == replacingId || isImagePlaced( image->first ) )
				continue;
			if ( candidate == mImages.end() ||
				 ( image->second.usageHint != 0 && candidate->second.usageHint == 0 ) ||
				 ( ( image->second.usageHint != 0 ) == ( candidate->second.usageHint != 0 ) &&
				   image->second.creationSerial < candidate->second.creationSerial ) )
				candidate = image;
		}
		if ( candidate == mImages.end() )
			return false;
		eraseImage( candidate->first );
		++mStats.evictions;
	}
	return true;
}

std::string KittyGraphicsProtocol::response( const KittyGraphicsCommandData& data,
											 KittyGraphicsError error,
											 KittyImageId imageId ) const {
	const Uint32 quiet = data.quiet.value_or( 0 );
	if ( quiet == 2 || ( quiet == 1 && error == KittyGraphicsError::None ) )
		return {};
	const KittyImageId responseId = imageId != 0 ? imageId : data.imageId.value_or( 0 );
	if ( responseId == 0 && error == KittyGraphicsError::None )
		return {};
	std::string value = "\033_G";
	if ( responseId != 0 ) {
		value += "i=" + std::to_string( responseId );
		if ( data.imageNumber )
			value += ",I=" + std::to_string( *data.imageNumber );
		if ( data.placementId )
			value += ",p=" + std::to_string( *data.placementId );
	}
	value += ";";
	if ( error == KittyGraphicsError::None ) {
		value += "OK";
	} else {
		value += error == KittyGraphicsError::Unsupported ? "ENOTSUP"
				 : error == KittyGraphicsError::NoSpace	  ? "ENOSPC"
				 : error == KittyGraphicsError::NotFound  ? "ENOENT"
				 : error == KittyGraphicsError::NoParent  ? "ENOPARENT"
				 : error == KittyGraphicsError::Cycle	  ? "ECYCLE"
				 : error == KittyGraphicsError::TooDeep	  ? "ETOODEEP"
														  : "EINVAL";
	}
	value += "\033\\";
	return value;
}

std::vector<TerminalGraphicsUpdate> KittyGraphicsProtocol::takeUpdates() {
	std::vector<TerminalGraphicsUpdate> updates;
	updates.swap( mUpdates );
	return updates;
}

std::shared_ptr<TerminalGraphicsPresentation> KittyGraphicsProtocol::takePresentation() {
	auto presentation = std::make_shared<TerminalGraphicsPresentation>();
	presentation->generation = mPresentationGeneration;
	presentation->placements.reserve( mPlacements.size() );
	for ( const auto& placement : mPlacements ) {
		if ( placement.virtualPlacement )
			continue;
		auto visible = placement.visible;
		if ( placement.parentImageId != 0 ) {
			auto parent =
				std::find_if( mPlacements.begin(), mPlacements.end(), [&]( const Placement& p ) {
					return p.visible.imageId == placement.parentImageId &&
						   p.visible.placementId == placement.parentPlacementId;
				} );
			if ( parent != mPlacements.end() && parent->virtualPlacement ) {
				bool found = false;
				Vector2i minimum( std::numeric_limits<Int32>::max(),
								  std::numeric_limits<Int32>::max() );
				for ( const auto& cell : mPlaceholderCells ) {
					if ( cell.imageId == parent->visible.imageId &&
						 ( parent->visible.placementId == 0 ||
						   cell.placementId == parent->visible.placementId ) ) {
						minimum.x = std::min( minimum.x, cell.cell.x );
						minimum.y = std::min( minimum.y, cell.cell.y );
						found = true;
					}
				}
				if ( !found )
					continue;
				visible.visibleAnchorCell += minimum - parent->visible.visibleAnchorCell;
			}
		}
		auto image = mImages.find( visible.imageId );
		if ( image != mImages.end() )
			visible.frameNumber = image->second.currentFrame;
		visible.visibleAnchorCell.y += mScrollPosition;
		const Int64 bottom = static_cast<Int64>( visible.visibleAnchorCell.y ) + visible.rows;
		if ( bottom > 0 && ( mScreenRows == 0 || visible.visibleAnchorCell.y < mScreenRows ) )
			presentation->placements.emplace_back( std::move( visible ) );
	}
	for ( const auto& cell : mPlaceholderCells ) {
		auto prototype =
			std::find_if( mPlacements.begin(), mPlacements.end(), [&]( const Placement& p ) {
				return p.virtualPlacement && p.visible.imageId == cell.imageId &&
					   ( cell.placementId == 0 || p.visible.placementId == cell.placementId );
			} );
		if ( prototype == mPlacements.end() || cell.imageRow >= prototype->visible.rows ||
			 cell.imageColumn >= prototype->visible.columns )
			continue;
		auto visible = prototype->visible;
		visible.visibleAnchorCell = cell.cell;
		visible.columns = 1;
		visible.rows = 1;
		const int sourceWidth = visible.sourcePixels.Right - visible.sourcePixels.Left;
		const int sourceHeight = visible.sourcePixels.Bottom - visible.sourcePixels.Top;
		const int left = visible.sourcePixels.Left + static_cast<Int64>( sourceWidth ) *
														 cell.imageColumn /
														 prototype->visible.columns;
		const int right = visible.sourcePixels.Left + static_cast<Int64>( sourceWidth ) *
														  ( cell.imageColumn + 1 ) /
														  prototype->visible.columns;
		const int top = visible.sourcePixels.Top + static_cast<Int64>( sourceHeight ) *
													   cell.imageRow / prototype->visible.rows;
		const int bottom = visible.sourcePixels.Top + static_cast<Int64>( sourceHeight ) *
														  ( cell.imageRow + 1 ) /
														  prototype->visible.rows;
		visible.sourcePixels = Rect( left, top, right, bottom );
		auto image = mImages.find( visible.imageId );
		if ( image != mImages.end() )
			visible.frameNumber = image->second.currentFrame;
		presentation->placements.emplace_back( std::move( visible ) );
	}
	std::stable_sort(
		presentation->placements.begin(), presentation->placements.end(),
		[]( const TerminalVisiblePlacement& left, const TerminalVisiblePlacement& right ) {
			if ( left.zIndex != right.zIndex )
				return left.zIndex < right.zIndex;
			return left.imageId < right.imageId;
		} );
	mPresentationDirty = false;
	return presentation;
}

const std::vector<Uint8>* KittyGraphicsProtocol::imagePixels( KittyImageId imageId ) const {
	auto image = mImages.find( imageId );
	return image == mImages.end() ? nullptr : image->second.rgba.get();
}

bool KittyGraphicsProtocol::hasVirtualPlacements() const {
	return std::any_of( mPlacements.begin(), mPlacements.end(),
						[]( const Placement& placement ) { return placement.virtualPlacement; } );
}

void KittyGraphicsProtocol::reset() {
	mImages.clear();
	mPlacements.clear();
	mPrimaryPlacements.clear();
	mPlaceholderCells.clear();
	mUpdates.clear();
	mPending = {};
	mStorageBytes = 0;
	mFrameStorageBytes = 0;
	TerminalGraphicsUpdate reset;
	reset.type = TerminalGraphicsUpdateType::ResetAll;
	mUpdates.emplace_back( std::move( reset ) );
	++mPresentationGeneration;
	mPresentationDirty = true;
}

void KittyGraphicsProtocol::clearScreen() {
	const size_t oldSize = mPlacements.size();
	mPlacements.erase(
		std::remove_if( mPlacements.begin(), mPlacements.end(),
						[&]( const Placement& placement ) {
							if ( placement.virtualPlacement )
								return false;
							const int visibleTop =
								placement.visible.visibleAnchorCell.y + mScrollPosition;
							return static_cast<Int64>( visibleTop ) + placement.visible.rows > 0 &&
								   ( mScreenRows == 0 || visibleTop < mScreenRows );
						} ),
		mPlacements.end() );
	if ( oldSize == mPlacements.size() )
		return;
	++mPresentationGeneration;
	mPresentationDirty = true;
}

void KittyGraphicsProtocol::setAlternateScreen( bool alternate ) {
	if ( alternate ) {
		mPrimaryPlacements = std::move( mPlacements );
		mPlacements.clear();
	} else {
		mPlacements = std::move( mPrimaryPlacements );
		mPrimaryPlacements.clear();
	}
	++mPresentationGeneration;
	mPresentationDirty = true;
}

void KittyGraphicsProtocol::scrollScreen( int top, int bottom, int rows, bool preserveHistory ) {
	if ( rows == 0 || top > bottom )
		return;
	const size_t oldSize = mPlacements.size();
	std::vector<Uint64> scrolledOut;
	for ( auto& placement : mPlacements ) {
		auto& visible = placement.visible;
		const Int64 placementBottom =
			static_cast<Int64>( visible.visibleAnchorCell.y ) + visible.rows;
		if ( visible.visibleAnchorCell.y < top ||
			 placementBottom > static_cast<Int64>( bottom ) + 1 )
			continue;
		visible.visibleAnchorCell.y += rows;
		if ( preserveHistory )
			continue;
		const Uint32 oldRows = visible.rows;
		const int sourceTop = visible.sourcePixels.Top;
		const int sourceHeight = visible.sourcePixels.Bottom - sourceTop;
		const int clipTop = std::max( 0, top - visible.visibleAnchorCell.y );
		const int clipBottom = std::max( 0, visible.visibleAnchorCell.y +
												static_cast<int>( visible.rows ) - bottom - 1 );
		if ( clipTop + clipBottom >= static_cast<int>( visible.rows ) ) {
			scrolledOut.emplace_back( placement.internalId );
			continue;
		}
		visible.sourcePixels.Top =
			sourceTop + static_cast<Int64>( sourceHeight ) * clipTop / oldRows;
		visible.sourcePixels.Bottom =
			sourceTop + static_cast<Int64>( sourceHeight ) * ( oldRows - clipBottom ) / oldRows;
		visible.visibleAnchorCell.y += clipTop;
		visible.rows -= clipTop + clipBottom;
		if ( clipTop > 0 )
			visible.firstCellPixelOffset.y = 0;
	}
	if ( !scrolledOut.empty() ) {
		mPlacements.erase( std::remove_if( mPlacements.begin(), mPlacements.end(),
										   [&]( const Placement& p ) {
											   return std::find(
														  scrolledOut.begin(), scrolledOut.end(),
														  p.internalId ) != scrolledOut.end();
										   } ),
						   mPlacements.end() );
	}
	if ( oldSize != mPlacements.size() || !mPlacements.empty() ) {
		++mPresentationGeneration;
		mPresentationDirty = true;
	}
}

void KittyGraphicsProtocol::setViewport( int scrollPosition, int historyLength, int screenRows ) {
	if ( mScrollPosition == scrollPosition && mHistoryLength == historyLength &&
		 mScreenRows == screenRows )
		return;
	mScrollPosition = scrollPosition;
	mHistoryLength = historyLength;
	mScreenRows = screenRows;
	const size_t oldSize = mPlacements.size();
	mPlacements.erase( std::remove_if( mPlacements.begin(), mPlacements.end(),
									   [historyLength]( const Placement& p ) {
										   return p.visible.visibleAnchorCell.y < 0 &&
												  static_cast<Int64>(
													  p.visible.visibleAnchorCell.y ) +
														  p.visible.rows <=
													  -historyLength;
									   } ),
					   mPlacements.end() );
	if ( oldSize != 0 || oldSize != mPlacements.size() ) {
		++mPresentationGeneration;
		mPresentationDirty = true;
	}
}

void KittyGraphicsProtocol::resync() {
	mUpdates.clear();
	TerminalGraphicsUpdate reset;
	reset.type = TerminalGraphicsUpdateType::ResetAll;
	mUpdates.emplace_back( std::move( reset ) );
	for ( const auto& image : mImages ) {
		TerminalGraphicsUpdate create;
		create.type = TerminalGraphicsUpdateType::CreateImage;
		create.imageId = image.first;
		create.imageSize = image.second.size;
		create.region = Rect( 0, 0, image.second.size.getWidth(), image.second.size.getHeight() );
		create.rgba = image.second.rgba;
		mUpdates.emplace_back( std::move( create ) );
		for ( const auto& frame : image.second.frames ) {
			TerminalGraphicsUpdate createFrame;
			createFrame.type = TerminalGraphicsUpdateType::CreateFrame;
			createFrame.imageId = image.first;
			createFrame.frameNumber = frame.first;
			createFrame.imageSize = image.second.size;
			createFrame.region =
				Rect( 0, 0, image.second.size.getWidth(), image.second.size.getHeight() );
			createFrame.rgba = std::make_shared<const std::vector<Uint8>>( frame.second.rgba );
			mUpdates.emplace_back( std::move( createFrame ) );
		}
	}
	mPresentationDirty = true;
}

}} // namespace eterm::Terminal
