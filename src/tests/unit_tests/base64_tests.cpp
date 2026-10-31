#include "utest.hpp"
#include <eepp/system/base64.hpp>

#include <random>
#include <string>

using namespace EE;
using namespace EE::System;

namespace {

constexpr Base64::DecodeMode Modes[] = { Base64::DecodeMode::AllowWhitespace,
										 Base64::DecodeMode::NoWhitespace,
										 Base64::DecodeMode::NoWhitespaceStrict };

std::string randomBytes( std::mt19937& rng, size_t size ) {
	std::string bytes( size, '\0' );
	for ( auto& byte : bytes )
		byte = static_cast<char>( rng() & 0xFF );
	return bytes;
}

size_t decodeInto( const std::string& encoded, Base64::DecodeMode mode, std::string& output,
				   size_t outLen ) {
	output.assign( outLen, '\0' );
	return Base64::decode( encoded.size(), encoded.data(), output.size(),
						   reinterpret_cast<unsigned char*>( output.data() ), mode );
}

} // namespace

UTEST( Base64, round_trips_every_length_in_every_mode ) {
	std::mt19937 rng( 3 );
	for ( size_t size = 0; size <= 700; ++size ) {
		const std::string bytes = randomBytes( rng, size );
		std::string encoded;
		ASSERT_TRUE( Base64::encode( bytes, encoded ) );
		for ( auto mode : Modes ) {
			std::string decoded;
			const size_t length =
				decodeInto( encoded, mode, decoded, Base64::decodeSafeOutLen( encoded.size() ) );
			ASSERT_EQ( size, length );
			decoded.resize( length );
			ASSERT_TRUE( bytes == decoded );
		}
		// An exactly sized output buffer is enough for every mode.
		std::string exact;
		ASSERT_EQ( size,
				   decodeInto( encoded, Base64::DecodeMode::NoWhitespaceStrict, exact, size ) );
		ASSERT_TRUE( bytes == exact );
	}
}

UTEST( Base64, stops_or_fails_at_an_invalid_byte_in_any_position ) {
	std::mt19937 rng( 5 );
	const std::string bytes = randomBytes( rng, 300 );
	std::string encoded;
	ASSERT_TRUE( Base64::encode( bytes, encoded ) );
	ASSERT_EQ( static_cast<size_t>( 400 ), encoded.size() );
	const size_t partialBytes[4] = { 0, 0, 1, 2 };
	for ( size_t position = 0; position < encoded.size(); ++position ) {
		for ( char invalid : { '*', '\x80', '\xFF', '\0' } ) {
			std::string corrupted = encoded;
			corrupted[position] = invalid;
			// Lenient modes keep everything decoded before the invalid byte.
			const size_t expected = position / 4 * 3 + partialBytes[position % 4];
			for ( auto mode :
				  { Base64::DecodeMode::AllowWhitespace, Base64::DecodeMode::NoWhitespace } ) {
				std::string decoded;
				ASSERT_EQ( expected, decodeInto( corrupted, mode, decoded, 400 ) );
				ASSERT_EQ( 0, bytes.compare( 0, expected, decoded, 0, expected ) );
			}
			std::string decoded;
			ASSERT_EQ(
				static_cast<size_t>( -1 ),
				decodeInto( corrupted, Base64::DecodeMode::NoWhitespaceStrict, decoded, 400 ) );
		}

		// Whitespace is skipped only by AllowWhitespace.
		std::string spaced = encoded;
		spaced.insert( position, " \n" );
		std::string decoded;
		ASSERT_EQ( bytes.size(),
				   decodeInto( spaced, Base64::DecodeMode::AllowWhitespace, decoded, 400 ) );
		decoded.resize( bytes.size() );
		ASSERT_TRUE( bytes == decoded );
		ASSERT_EQ( position / 4 * 3 + partialBytes[position % 4],
				   decodeInto( spaced, Base64::DecodeMode::NoWhitespace, decoded, 400 ) );
		ASSERT_EQ( static_cast<size_t>( -1 ),
				   decodeInto( spaced, Base64::DecodeMode::NoWhitespaceStrict, decoded, 400 ) );
	}
}

UTEST( Base64, reports_truncation_when_the_output_is_too_small ) {
	std::mt19937 rng( 7 );
	const std::string bytes = randomBytes( rng, 300 );
	std::string encoded;
	ASSERT_TRUE( Base64::encode( bytes, encoded ) );
	for ( auto mode : Modes ) {
		std::string decoded;
		EXPECT_EQ( static_cast<size_t>( -1 ), decodeInto( encoded, mode, decoded, 299 ) );
	}
}
