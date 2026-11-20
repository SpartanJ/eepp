#ifndef EE_SYSTEM_BASE64SCALAR_HPP
#define EE_SYSTEM_BASE64SCALAR_HPP

#include <eepp/system/base64.hpp>

namespace EE::System::Private {

constexpr char Base64Alphabet[] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

// The caller provides room for the encoded digits and the trailing NUL.
static inline size_t encodeBase64Scalar( size_t inLen, const unsigned char* in, char* out ) {
	size_t ii = 0;
	size_t io = 0;
	const size_t bulkEnd = inLen - inLen % 3;
	for ( ; ii < bulkEnd; ii += 3 ) {
		const Uint32 value = ( static_cast<Uint32>( in[ii] ) << 16 ) |
							 ( static_cast<Uint32>( in[ii + 1] ) << 8 ) | in[ii + 2];
		out[io++] = Base64Alphabet[value >> 18];
		out[io++] = Base64Alphabet[( value >> 12 ) & 63];
		out[io++] = Base64Alphabet[( value >> 6 ) & 63];
		out[io++] = Base64Alphabet[value & 63];
	}

	if ( ii < inLen ) {
		const Uint32 value = static_cast<Uint32>( in[ii] ) << 16 |
							 ( ii + 1 < inLen ? static_cast<Uint32>( in[ii + 1] ) << 8 : 0 );
		out[io++] = Base64Alphabet[value >> 18];
		out[io++] = Base64Alphabet[( value >> 12 ) & 63];
		out[io++] = ii + 1 < inLen ? Base64Alphabet[( value >> 6 ) & 63] : '=';
		out[io++] = '=';
	}

	out[io] = 0;
	return io;
}

} // namespace EE::System::Private

#endif
