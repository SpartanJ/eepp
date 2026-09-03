#include <array>
#include <eepp/system/base64.hpp>

namespace EE { namespace System {

namespace {

constexpr Uint8 BASE64_INVALID = 0xFF;
constexpr Uint8 BASE64_PADDING = 0xFE;
constexpr Uint8 BASE64_WHITESPACE = 0xFD;

constexpr std::array<Uint8, 256> makeBase64DecodeTable() {
	std::array<Uint8, 256> table{};

	for ( size_t i = 0; i < table.size(); ++i )
		table[i] = BASE64_INVALID;

	for ( size_t i = 0; i < 26; ++i ) {
		table[static_cast<unsigned char>( 'A' + i )] = static_cast<Uint8>( i );
		table[static_cast<unsigned char>( 'a' + i )] = static_cast<Uint8>( i + 26 );
	}

	for ( size_t i = 0; i < 10; ++i )
		table[static_cast<unsigned char>( '0' + i )] = static_cast<Uint8>( i + 52 );

	table[static_cast<unsigned char>( '+' )] = 62;
	table[static_cast<unsigned char>( '/' )] = 63;
	table[static_cast<unsigned char>( '=' )] = BASE64_PADDING;

	// ASCII whitespace accepted by the historical decoder via isspace():
	table[static_cast<unsigned char>( ' ' )] = BASE64_WHITESPACE;
	table[static_cast<unsigned char>( '\t' )] = BASE64_WHITESPACE;
	table[static_cast<unsigned char>( '\n' )] = BASE64_WHITESPACE;
	table[static_cast<unsigned char>( '\v' )] = BASE64_WHITESPACE;
	table[static_cast<unsigned char>( '\f' )] = BASE64_WHITESPACE;
	table[static_cast<unsigned char>( '\r' )] = BASE64_WHITESPACE;

	return table;
}

constexpr auto base64dec_tab = makeBase64DecodeTable();

template <bool AllowWhitespace>
size_t decodeBase64( size_t in_len, const char* in, size_t out_len, unsigned char* out ) {
	size_t ii = 0;
	size_t io = 0;
	Uint32 v = 0;
	unsigned rem = 0;

	while ( ii < in_len ) {
		/*
		 * Fast path: four ordinary base64 bytes become three output bytes.
		 *
		 * The OR validates all four table entries at once: valid base64 values
		 * are 0..63, while padding/whitespace/invalid entries have high bits set.
		 *
		 * This path is especially useful for protocol payloads that guarantee
		 * contiguous, whitespace-free base64 data.
		 */
		if ( rem == 0 && ii + 4 <= in_len && io + 3 <= out_len ) {
			const Uint8 a = base64dec_tab[static_cast<unsigned char>( in[ii] )];
			const Uint8 b = base64dec_tab[static_cast<unsigned char>( in[ii + 1] )];
			const Uint8 c = base64dec_tab[static_cast<unsigned char>( in[ii + 2] )];
			const Uint8 d = base64dec_tab[static_cast<unsigned char>( in[ii + 3] )];

			if ( ( a | b | c | d ) <= 63 ) {
				out[io] = static_cast<unsigned char>( ( a << 2 ) | ( b >> 4 ) );
				out[io + 1] = static_cast<unsigned char>( ( b << 4 ) | ( c >> 2 ) );
				out[io + 2] = static_cast<unsigned char>( ( c << 6 ) | d );

				ii += 4;
				io += 3;
				continue;
			}
		}

		const Uint8 ch = base64dec_tab[static_cast<unsigned char>( in[ii++] )];

		if ( ch > 63 ) {
			if constexpr ( AllowWhitespace ) {
				if ( ch == BASE64_WHITESPACE )
					continue;
			}

			// Preserve the old behavior: stop at '=' or the first parse error.
			break;
		}

		v = ( v << 6 ) | ch;
		rem += 6;

		if ( rem >= 8 ) {
			rem -= 8;

			if ( io >= out_len )
				return static_cast<size_t>( -1 );

			out[io++] = static_cast<unsigned char>( ( v >> rem ) & 255 );

			// Every four valid base64 characters rem returns to zero.
			if ( rem == 0 )
				v = 0;
		}
	}

	return io;
}

size_t decodeBase64Strict( size_t inLen, const char* input, size_t outLen, unsigned char* output ) {
	size_t outputOffset = 0;
	Uint32 value = 0;
	unsigned bits = 0;
	size_t padding = 0;
	for ( size_t offset = 0; offset < inLen; ++offset ) {
		const Uint8 decoded = base64dec_tab[static_cast<unsigned char>( input[offset] )];
		if ( decoded == BASE64_PADDING ) {
			padding = inLen - offset;
			if ( padding > 2 || inLen % 4 != 0 )
				return static_cast<size_t>( -1 );
			for ( size_t remainder = offset; remainder < inLen; ++remainder )
				if ( input[remainder] != '=' )
					return static_cast<size_t>( -1 );
			break;
		}
		if ( decoded > 63 || padding != 0 )
			return static_cast<size_t>( -1 );
		value = ( value << 6 ) | decoded;
		bits += 6;
		if ( bits >= 8 ) {
			bits -= 8;
			if ( outputOffset >= outLen )
				return static_cast<size_t>( -1 );
			output[outputOffset++] = static_cast<unsigned char>( ( value >> bits ) & 0xFF );
			if ( bits == 0 )
				value = 0;
		}
	}
	const size_t dataLength = inLen - padding;
	if ( dataLength % 4 == 1 || ( padding == 1 && dataLength % 4 != 3 ) ||
		 ( padding == 2 && dataLength % 4 != 2 ) )
		return static_cast<size_t>( -1 );
	// Reject non-canonical encodings whose unused bits are non-zero. Besides being strict, this
	// avoids accepting multiple byte strings for the same payload at protocol boundaries.
	if ( bits != 0 && ( value & ( ( 1u << bits ) - 1u ) ) != 0 )
		return static_cast<size_t>( -1 );
	return outputOffset;
}

} // namespace

size_t Base64::decode( size_t in_len, const char* in, size_t out_len, unsigned char* out ) {
	return decodeBase64<true>( in_len, in, out_len, out );
}

size_t Base64::decode( size_t in_len, const char* in, size_t out_len, unsigned char* out,
					   DecodeMode mode ) {
	if ( mode == DecodeMode::NoWhitespaceStrict )
		return decodeBase64Strict( in_len, in, out_len, out );
	return mode == DecodeMode::NoWhitespace ? decodeBase64<false>( in_len, in, out_len, out )
											: decodeBase64<true>( in_len, in, out_len, out );
}

size_t Base64::encode( size_t in_len, const unsigned char* in, size_t out_len, char* out ) {
	static const Uint8 base64enc_tab[] =
		"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

	size_t ii, io;
	Uint32 v;
	unsigned rem;

	for ( io = 0, ii = 0, v = 0, rem = 0; ii < in_len; ii++ ) {
		unsigned char ch;
		ch = in[ii];
		v = ( v << 8 ) | ch;
		rem += 8;
		while ( rem >= 6 ) {
			rem -= 6;
			if ( io >= out_len )
				return static_cast<size_t>( -1 ); /* truncation is failure */
			out[io++] = base64enc_tab[( v >> rem ) & 63];
		}
	}

	if ( rem ) {
		v <<= ( 6 - rem );
		if ( io >= out_len )
			return static_cast<size_t>( -1 ); /* truncation is failure */
		out[io++] = base64enc_tab[v & 63];
	}

	while ( io & 3 ) {
		if ( io >= out_len )
			return static_cast<size_t>( -1 ); /* truncation is failure */
		out[io++] = '=';
	}

	if ( io >= out_len )
		return static_cast<size_t>( -1 ); /* no room for null terminator */

	out[io] = 0;
	return io;
}

bool Base64::encode( std::string_view in, std::string& out ) {
	size_t b64len = encodeSafeOutLen( in.size() );

	if ( out.size() < b64len )
		out.resize( b64len );

	const size_t len = encode( in.size(), reinterpret_cast<const unsigned char*>( in.data() ),
							   out.size(), out.data() );

	if ( len != static_cast<size_t>( -1 ) && len != out.size() )
		out.resize( len );

	return len != static_cast<size_t>( -1 );
}

size_t Base64::decode( std::string_view in, std::string& out ) {
	return decode( in, out, DecodeMode::AllowWhitespace );
}

size_t Base64::decode( std::string_view in, std::string& out, DecodeMode mode ) {
	size_t d64len = decodeSafeOutLen( in.size() );

	if ( out.size() < d64len )
		out.resize( d64len );

	const size_t len = decode( in.size(), in.data(), out.size(),
							   reinterpret_cast<unsigned char*>( out.data() ), mode );

	if ( len != static_cast<size_t>( -1 ) && len != out.size() )
		out.resize( len );

	return len;
}

}} // namespace EE::System
