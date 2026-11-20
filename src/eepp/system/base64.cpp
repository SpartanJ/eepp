#include <eepp/system/base64.hpp>

#include "base64scalar.hpp"

#include <array>
#include <eepp/system/cpu.hpp>

#if defined( EE_ARCH_X86_64 )
#include <immintrin.h>
#elif defined( EE_ARCH_ARM64 )
#include <arm_neon.h>
#endif

namespace EE { namespace System {

namespace {

using namespace EE::System::Private;

#if defined( EE_ARCH_X86_64 )
constexpr size_t Base64EncodeBlockSize = 24;

#if defined( __GNUC__ ) || defined( __clang__ )
__attribute__( ( target( "avx2" ) ) )
#endif
void encodeBase64BlocksAVX2( size_t blockCount, const unsigned char* in, char* out ) {
	// Each 128-bit lane encodes four triplets. The second load starts at byte 8 so both
	// overlapping loads stay inside the 24-byte block, even for the final block.
	const __m256i shuffle = _mm256_setr_epi8( 1, 0, 2, 1, 4, 3, 5, 4, 7, 6, 8, 7, 10, 9, 11, 10, 5,
											  4, 6, 5, 8, 7, 9, 8, 11, 10, 12, 11, 14, 13, 15, 14 );
	const __m256i highMask = _mm256_set1_epi32( 0x0FC0FC00 );
	const __m256i highShift = _mm256_set1_epi32( 0x04000040 );
	const __m256i lowMask = _mm256_set1_epi32( 0x003F03F0 );
	const __m256i lowShift = _mm256_set1_epi32( 0x01000010 );
	const __m256i offsets = _mm256_broadcastsi128_si256(
		_mm_setr_epi8( 'A', 'a' - 26, '0' - 52, '0' - 52, '0' - 52, '0' - 52, '0' - 52, '0' - 52,
					   '0' - 52, '0' - 52, '0' - 52, '0' - 52, '+' - 62, '/' - 63, 0, 0 ) );
	const __m256i upperEnd = _mm256_set1_epi8( 25 );
	const __m256i lowerEnd = _mm256_set1_epi8( 51 );
	for ( size_t block = 0; block < blockCount; ++block ) {
		const __m128i lo = _mm_loadu_si128( reinterpret_cast<const __m128i*>( in ) );
		const __m128i hi = _mm_loadu_si128( reinterpret_cast<const __m128i*>( in + 8 ) );
		const __m256i lanes = _mm256_inserti128_si256( _mm256_castsi128_si256( lo ), hi, 1 );
		// Shuffle each triplet ABC to BACB. The masks isolate alternating sextets and the
		// 16-bit multiplications shift them into their respective output bytes.
		const __m256i shuffled = _mm256_shuffle_epi8( lanes, shuffle );
		const __m256i high =
			_mm256_mulhi_epu16( _mm256_and_si256( shuffled, highMask ), highShift );
		const __m256i low = _mm256_mullo_epi16( _mm256_and_si256( shuffled, lowMask ), lowShift );
		const __m256i values = _mm256_or_si256( high, low );
		// Select offset 0 for uppercase, 1 for lowercase, and 2..13 for digits, '+' and '/'.
		const __m256i indices = _mm256_sub_epi8( _mm256_subs_epu8( values, lowerEnd ),
												 _mm256_cmpgt_epi8( values, upperEnd ) );
		const __m256i encoded = _mm256_add_epi8( values, _mm256_shuffle_epi8( offsets, indices ) );
		_mm256_storeu_si256( reinterpret_cast<__m256i*>( out ), encoded );
		in += Base64EncodeBlockSize;
		out += 32;
	}
}

const auto encodeBase64Blocks = CPU::hasAVX2() ? encodeBase64BlocksAVX2 : nullptr;
#elif defined( EE_ARCH_ARM64 )
constexpr size_t Base64EncodeBlockSize = 48;

void encodeBase64Blocks( size_t blockCount, const unsigned char* in, char* out ) {
	const uint8x16x4_t alphabet = vld1q_u8_x4( reinterpret_cast<const uint8_t*>( Base64Alphabet ) );
	const uint8x16_t mask = vdupq_n_u8( 63 );
	for ( size_t block = 0; block < blockCount; ++block ) {
		// Deinterleave 16 triplets, then interleave the four ASCII digits of each group.
		const uint8x16x3_t triplets = vld3q_u8( in );
		const uint8x16_t a = triplets.val[0];
		const uint8x16_t b = triplets.val[1];
		const uint8x16_t c = triplets.val[2];
		const uint8x16_t first = vshrq_n_u8( a, 2 );
		const uint8x16_t second = vorrq_u8( vshlq_n_u8( a, 4 ), vshrq_n_u8( b, 4 ) );
		const uint8x16_t third = vorrq_u8( vshlq_n_u8( b, 2 ), vshrq_n_u8( c, 6 ) );
		uint8x16x4_t encoded;
		encoded.val[0] = vqtbl4q_u8( alphabet, first );
		encoded.val[1] = vqtbl4q_u8( alphabet, vandq_u8( second, mask ) );
		encoded.val[2] = vqtbl4q_u8( alphabet, vandq_u8( third, mask ) );
		encoded.val[3] = vqtbl4q_u8( alphabet, vandq_u8( c, mask ) );
		vst4q_u8( reinterpret_cast<uint8_t*>( out ), encoded );
		in += Base64EncodeBlockSize;
		out += 64;
	}
}
#endif

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

/* SIMD block decoders. They decode whole blocks of valid base64 digits and stop at the first block
 * holding anything else (padding, whitespace, invalid bytes), or when the input or output space
 * for a block runs out. Blocks end on complete 4-digit groups, where the scalar decoders below
 * carry no state, so those finish from there with exactly the result they would produce alone.
 * Returns the bytes written; consumed receives the digits read. */
typedef size_t ( *base64_block_decode_func )( const char* in, size_t inLen, unsigned char* out,
											  size_t outLen, size_t& consumed );

size_t decodeBase64BlocksNone( const char*, size_t, unsigned char*, size_t, size_t& consumed ) {
	consumed = 0;
	return 0;
}

#if defined( EE_ARCH_X86_64 )
#if defined( __GNUC__ ) || defined( __clang__ )
__attribute__( ( target( "avx2" ) ) )
#endif
size_t decodeBase64BlocksAVX2( const char* in, size_t inLen, unsigned char* out, size_t outLen,
							   size_t& consumed ) {
	/* Wojciech Muła's pshufb validation and translation, as in aklomp/base64: each digit's high
	 * and low nibbles select bit classes whose intersection is non-zero for anything that is not
	 * a base64 digit (bytes >= 0x80 included), and the high nibble selects the offset that turns
	 * a digit into its 6-bit value ('/' shares '+'s nibble and gets its own fix-up). */
	const __m256i lutLo =
		_mm256_setr_epi8( 0x15, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x13, 0x1A,
						  0x1B, 0x1B, 0x1B, 0x1A, 0x15, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
						  0x11, 0x11, 0x13, 0x1A, 0x1B, 0x1B, 0x1B, 0x1A );
	const __m256i lutHi =
		_mm256_setr_epi8( 0x10, 0x10, 0x01, 0x02, 0x04, 0x08, 0x04, 0x08, 0x10, 0x10, 0x10, 0x10,
						  0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x01, 0x02, 0x04, 0x08, 0x04, 0x08,
						  0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10 );
	const __m256i lutRoll =
		_mm256_setr_epi8( 0, 16, 19, 4, -65, -65, -71, -71, 0, 0, 0, 0, 0, 0, 0, 0, 0, 16, 19, 4,
						  -65, -65, -71, -71, 0, 0, 0, 0, 0, 0, 0, 0 );
	const __m256i mask2F = _mm256_set1_epi8( 0x2F );
	/* Packs four 6-bit values per 32-bit lane into three bytes, then the lanes' 12 bytes into the
	 * low 24 bytes of the register. */
	const __m256i mergePairs = _mm256_set1_epi32( 0x01400140 );
	const __m256i mergeQuads = _mm256_set1_epi32( 0x00011000 );
	const __m256i packBytes =
		_mm256_setr_epi8( 2, 1, 0, 6, 5, 4, 10, 9, 8, 14, 13, 12, -1, -1, -1, -1, 2, 1, 0, 6, 5, 4,
						  10, 9, 8, 14, 13, 12, -1, -1, -1, -1 );
	const __m256i packLanes = _mm256_setr_epi32( 0, 1, 2, 4, 5, 6, -1, -1 );
	size_t ii = 0;
	size_t io = 0;
	/* Each block stores 32 bytes, of which 24 are output. */
	while ( ii + 32 <= inLen && io + 32 <= outLen ) {
		const __m256i digits = _mm256_loadu_si256( reinterpret_cast<const __m256i*>( in + ii ) );
		const __m256i hiNibbles = _mm256_and_si256( _mm256_srli_epi32( digits, 4 ), mask2F );
		const __m256i loNibbles = _mm256_and_si256( digits, mask2F );
		if ( !_mm256_testz_si256( _mm256_shuffle_epi8( lutLo, loNibbles ),
								  _mm256_shuffle_epi8( lutHi, hiNibbles ) ) )
			break;
		const __m256i roll = _mm256_shuffle_epi8(
			lutRoll, _mm256_add_epi8( _mm256_cmpeq_epi8( digits, mask2F ), hiNibbles ) );
		const __m256i values = _mm256_add_epi8( digits, roll );
		const __m256i merged =
			_mm256_madd_epi16( _mm256_maddubs_epi16( values, mergePairs ), mergeQuads );
		const __m256i packed =
			_mm256_permutevar8x32_epi32( _mm256_shuffle_epi8( merged, packBytes ), packLanes );
		_mm256_storeu_si256( reinterpret_cast<__m256i*>( out + io ), packed );
		ii += 32;
		io += 24;
	}
	consumed = ii;
	return io;
}
#elif defined( EE_ARCH_ARM64 )
/* The 6-bit value of every ASCII byte, 0xFF for anything that is not a base64 digit. */
constexpr std::array<Uint8, 128> makeBase64NeonTable() {
	std::array<Uint8, 128> table{};
	for ( size_t i = 0; i < table.size(); ++i )
		table[i] = base64dec_tab[i] <= 63 ? base64dec_tab[i] : 0xFF;
	return table;
}

constexpr auto base64neon_tab = makeBase64NeonTable();

size_t decodeBase64BlocksNEON( const char* in, size_t inLen, unsigned char* out, size_t outLen,
							   size_t& consumed ) {
	const uint8x16x4_t lowTable = vld1q_u8_x4( base64neon_tab.data() );
	const uint8x16x4_t highTable = vld1q_u8_x4( base64neon_tab.data() + 64 );
	const uint8x16_t sixtyFour = vdupq_n_u8( 64 );
	/* TBL yields 0 for indices >= 64 and TBX keeps the destination for them, so bytes 0-63 come
	 * from the first lookup and 64-127 from the second; bytes >= 128 stay 0 there and are marked
	 * invalid through their sign bit. */
	const auto translate = [&]( uint8x16_t digits ) {
		const uint8x16_t values =
			vqtbx4q_u8( vqtbl4q_u8( lowTable, digits ), highTable, vsubq_u8( digits, sixtyFour ) );
		return vorrq_u8( values,
						 vreinterpretq_u8_s8( vshrq_n_s8( vreinterpretq_s8_u8( digits ), 7 ) ) );
	};
	const auto* bytes = reinterpret_cast<const uint8_t*>( in );
	size_t ii = 0;
	size_t io = 0;
	/* vld4q deinterleaves 64 digits into the four positions of 16 groups; vst3q writes their 48
	 * bytes back interleaved. */
	while ( ii + 64 <= inLen && io + 48 <= outLen ) {
		const uint8x16x4_t digits = vld4q_u8( bytes + ii );
		const uint8x16_t a = translate( digits.val[0] );
		const uint8x16_t b = translate( digits.val[1] );
		const uint8x16_t c = translate( digits.val[2] );
		const uint8x16_t d = translate( digits.val[3] );
		if ( vmaxvq_u8( vorrq_u8( vorrq_u8( a, b ), vorrq_u8( c, d ) ) ) > 63 )
			break;
		uint8x16x3_t decoded;
		decoded.val[0] = vorrq_u8( vshlq_n_u8( a, 2 ), vshrq_n_u8( b, 4 ) );
		decoded.val[1] = vorrq_u8( vshlq_n_u8( b, 4 ), vshrq_n_u8( c, 2 ) );
		decoded.val[2] = vorrq_u8( vshlq_n_u8( c, 6 ), d );
		vst3q_u8( out + io, decoded );
		ii += 64;
		io += 48;
	}
	consumed = ii;
	return io;
}
#endif

base64_block_decode_func resolveBase64BlockDecodeFn() {
#if defined( EE_ARCH_ARM64 )
	return decodeBase64BlocksNEON;
#elif defined( EE_ARCH_X86_64 )
	if ( CPU::hasAVX2() )
		return decodeBase64BlocksAVX2;
#endif
	return decodeBase64BlocksNone;
}

/* Resolved once at load time: decodes run for every kitty graphics chunk and OSC 52 write. */
const base64_block_decode_func decodeBase64Blocks = resolveBase64BlockDecodeFn();

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
	size_t inputOffset = 0;
	while ( inputOffset + 4 <= inLen ) {
		const Uint8 a = base64dec_tab[static_cast<unsigned char>( input[inputOffset] )];
		const Uint8 b = base64dec_tab[static_cast<unsigned char>( input[inputOffset + 1] )];
		const Uint8 c = base64dec_tab[static_cast<unsigned char>( input[inputOffset + 2] )];
		const Uint8 d = base64dec_tab[static_cast<unsigned char>( input[inputOffset + 3] )];
		if ( ( a | b | c | d ) > 63 )
			break;
		if ( outputOffset + 3 > outLen )
			return static_cast<size_t>( -1 );
		output[outputOffset] = static_cast<unsigned char>( ( a << 2 ) | ( b >> 4 ) );
		output[outputOffset + 1] = static_cast<unsigned char>( ( b << 4 ) | ( c >> 2 ) );
		output[outputOffset + 2] = static_cast<unsigned char>( ( c << 6 ) | d );
		inputOffset += 4;
		outputOffset += 3;
	}

	const size_t remaining = inLen - inputOffset;
	if ( remaining == 0 )
		return outputOffset;
	if ( remaining == 1 || remaining > 4 )
		return static_cast<size_t>( -1 );

	const Uint8 a = base64dec_tab[static_cast<unsigned char>( input[inputOffset] )];
	const Uint8 b = base64dec_tab[static_cast<unsigned char>( input[inputOffset + 1] )];
	if ( a > 63 || b > 63 || outputOffset >= outLen )
		return static_cast<size_t>( -1 );
	output[outputOffset++] = static_cast<unsigned char>( ( a << 2 ) | ( b >> 4 ) );

	if ( remaining == 2 )
		return ( b & 0x0F ) == 0 ? outputOffset : static_cast<size_t>( -1 );

	const Uint8 c = base64dec_tab[static_cast<unsigned char>( input[inputOffset + 2] )];
	if ( c == BASE64_PADDING ) {
		return remaining == 4 && input[inputOffset + 3] == '=' && ( b & 0x0F ) == 0
				   ? outputOffset
				   : static_cast<size_t>( -1 );
	}
	if ( c > 63 || outputOffset >= outLen )
		return static_cast<size_t>( -1 );
	output[outputOffset++] = static_cast<unsigned char>( ( b << 4 ) | ( c >> 2 ) );

	if ( remaining == 3 )
		return ( c & 0x03 ) == 0 ? outputOffset : static_cast<size_t>( -1 );
	if ( input[inputOffset + 3] != '=' || ( c & 0x03 ) != 0 )
		return static_cast<size_t>( -1 );
	return outputOffset;
}

} // namespace

size_t Base64::decode( size_t in_len, const char* in, size_t out_len, unsigned char* out ) {
	return decode( in_len, in, out_len, out, DecodeMode::AllowWhitespace );
}

size_t Base64::decode( size_t in_len, const char* in, size_t out_len, unsigned char* out,
					   DecodeMode mode ) {
	size_t consumed = 0;
	const size_t produced = decodeBase64Blocks( in, in_len, out, out_len, consumed );
	in += consumed;
	in_len -= consumed;
	out += produced;
	out_len -= produced;
	size_t rest;
	if ( mode == DecodeMode::NoWhitespaceStrict )
		rest = decodeBase64Strict( in_len, in, out_len, out );
	else if ( mode == DecodeMode::NoWhitespace )
		rest = decodeBase64<false>( in_len, in, out_len, out );
	else
		rest = decodeBase64<true>( in_len, in, out_len, out );
	return rest == static_cast<size_t>( -1 ) ? rest : produced + rest;
}

size_t Base64::encode( size_t in_len, const unsigned char* in, size_t out_len, char* out ) {
	if ( out_len < encodeSafeOutLen( in_len ) )
		return static_cast<size_t>( -1 );

	size_t produced = 0;
#if defined( EE_ARCH_X86_64 )
	const bool hasBlockEncoder = encodeBase64Blocks != nullptr;
#elif defined( EE_ARCH_ARM64 )
	constexpr bool hasBlockEncoder = true;
#endif
#if defined( EE_ARCH_X86_64 ) || defined( EE_ARCH_ARM64 )
	if ( in_len >= Base64EncodeBlockSize && hasBlockEncoder ) {
		const size_t blockCount = in_len / Base64EncodeBlockSize;
		encodeBase64Blocks( blockCount, in, out );
		const size_t consumed = blockCount * Base64EncodeBlockSize;
		produced = blockCount * ( Base64EncodeBlockSize / 3 * 4 );
		in += consumed;
		in_len -= consumed;
		out += produced;
	}
#endif

	return produced + encodeBase64Scalar( in_len, in, out );
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
