#include <cstring>
#include <eepp/system/iostreamfile.hpp>
#include <eepp/system/sha1.hpp>

namespace EE { namespace System {

static inline Uint32 rotl( Uint32 value, int bits ) {
	return ( value << bits ) | ( value >> ( 32 - bits ) );
}

void SHA1::transform( Context& ctx, const unsigned char* block ) {
	Uint32 w[80];
	for ( int i = 0; i < 16; ++i ) {
		w[i] = ( static_cast<Uint32>( block[i * 4] ) << 24 ) |
			   ( static_cast<Uint32>( block[i * 4 + 1] ) << 16 ) |
			   ( static_cast<Uint32>( block[i * 4 + 2] ) << 8 ) |
			   static_cast<Uint32>( block[i * 4 + 3] );
	}
	for ( int i = 16; i < 80; ++i )
		w[i] = rotl( w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1 );

	Uint32 a = ctx.state[0], b = ctx.state[1], c = ctx.state[2], d = ctx.state[3], e = ctx.state[4];
	for ( int i = 0; i < 80; ++i ) {
		Uint32 f, k;
		if ( i < 20 ) {
			f = ( b & c ) | ( ~b & d );
			k = 0x5A827999;
		} else if ( i < 40 ) {
			f = b ^ c ^ d;
			k = 0x6ED9EBA1;
		} else if ( i < 60 ) {
			f = ( b & c ) | ( b & d ) | ( c & d );
			k = 0x8F1BBCDC;
		} else {
			f = b ^ c ^ d;
			k = 0xCA62C1D6;
		}
		Uint32 temp = rotl( a, 5 ) + f + e + k + w[i];
		e = d;
		d = c;
		c = rotl( b, 30 );
		b = a;
		a = temp;
	}
	ctx.state[0] += a;
	ctx.state[1] += b;
	ctx.state[2] += c;
	ctx.state[3] += d;
	ctx.state[4] += e;
}

void SHA1::init( Context& ctx ) {
	ctx.state[0] = 0x67452301;
	ctx.state[1] = 0xEFCDAB89;
	ctx.state[2] = 0x98BADCFE;
	ctx.state[3] = 0x10325476;
	ctx.state[4] = 0xC3D2E1F0;
	ctx.length = 0;
}

void SHA1::update( Context& ctx, const void* data, unsigned long size ) {
	const unsigned char* bytes = static_cast<const unsigned char*>( data );
	size_t used = static_cast<size_t>( ctx.length & 63 );
	ctx.length += size;

	if ( used ) {
		size_t available = 64 - used;
		if ( size < available ) {
			memcpy( &ctx.buffer[used], bytes, size );
			return;
		}
		memcpy( &ctx.buffer[used], bytes, available );
		transform( ctx, ctx.buffer );
		bytes += available;
		size -= available;
	}

	while ( size >= 64 ) {
		transform( ctx, bytes );
		bytes += 64;
		size -= 64;
	}

	memcpy( ctx.buffer, bytes, size );
}

void SHA1::final( Digest& result, Context& ctx ) {
	Uint64 bitLength = ctx.length * 8;
	size_t used = static_cast<size_t>( ctx.length & 63 );

	ctx.buffer[used++] = 0x80;

	if ( used > 56 ) {
		memset( &ctx.buffer[used], 0, 64 - used );
		transform( ctx, ctx.buffer );
		used = 0;
	}

	memset( &ctx.buffer[used], 0, 56 - used );

	for ( int i = 0; i < 8; ++i )
		ctx.buffer[56 + i] = static_cast<unsigned char>( bitLength >> ( 56 - i * 8 ) );

	transform( ctx, ctx.buffer );

	for ( int i = 0; i < 5; ++i )
		for ( int j = 0; j < 4; ++j )
			result[i * 4 + j] = static_cast<Uint8>( ctx.state[i] >> ( 24 - j * 8 ) );

	memset( &ctx, 0, sizeof( ctx ) );
}

SHA1::Result SHA1::result( Context& ctx ) {
	SHA1::Digest digest;
	final( digest, ctx );
	return { digest };
}

std::string SHA1::hexDigest( const SHA1::Digest& digest ) {
	static constexpr char Hex[] = "0123456789abcdef";
	std::string out;
	out.reserve( digest.size() * 2 );
	for ( Uint8 byte : digest ) {
		out += Hex[byte >> 4];
		out += Hex[byte & 0xF];
	}
	return out;
}

SHA1::Result SHA1::fromStream( IOStream& stream ) {
	Result res;
	Context ctx;
	Int64 buff_size = 512;
	Int64 size = (Int64)stream.getSize();
	char data[512];

	stream.seek( 0 );

	init( ctx );

	while ( size > 0 ) {
		Int64 chunk = size > buff_size ? buff_size : size;

		stream.read( data, chunk );

		update( ctx, data, chunk );

		size -= chunk;
	}

	final( res.digest, ctx );

	return res;
}

SHA1::Result SHA1::fromFile( std::string path ) {
	IOStreamFile file( path );
	return fromStream( file );
}

SHA1::Result SHA1::fromMemory( const Uint8* data, Uint64 size ) {
	Result res;

	Context ctx;

	init( ctx );

	update( ctx, data, size );

	final( res.digest, ctx );

	return res;
}

SHA1::Result SHA1::fromString( const std::string& str ) {
	return fromMemory( (const Uint8*)str.c_str(), str.size() );
}

SHA1::Result SHA1::fromString( const String& str ) {
	return fromMemory( (const Uint8*)str.c_str(), str.size() );
}

}} // namespace EE::System
