#include "utest.hpp"
#include <eepp/system/sha1.hpp>
#include <string>

using namespace EE;
using namespace EE::System;

UTEST( SHA1, knownVectors ) {
	// FIPS 180-4 / RFC 3174 test vectors.
	EXPECT_STDSTREQ( "da39a3ee5e6b4b0d3255bfef95601890afd80709",
					 SHA1::fromString( std::string() ).toHexString() );
	EXPECT_STDSTREQ( "a9993e364706816aba3e25717850c26c9cd0d89d",
					 SHA1::fromString( std::string( "abc" ) ).toHexString() );
	EXPECT_STDSTREQ( "84983e441c3bd26ebaae4aa1f95129e5e54670f1",
					 SHA1::fromString(
						 std::string( "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq" ) )
						 .toHexString() );
	EXPECT_STDSTREQ( "34aa973cd4c4daa4f61eeb2bdbad27316534016f",
					 SHA1::fromString( std::string( 1000000, 'a' ) ).toHexString() );
}

UTEST( SHA1, incrementalUpdatesMatchOneShot ) {
	// Every split point, including ones that straddle the 64-byte block and padding boundaries.
	std::string data;
	for ( int i = 0; i < 200; ++i )
		data += static_cast<char>( 'a' + i % 26 );
	SHA1::Result expected = SHA1::fromString( data );
	for ( size_t split = 0; split <= data.size(); ++split ) {
		SHA1::Context ctx;
		SHA1::init( ctx );
		SHA1::update( ctx, data.data(), split );
		SHA1::update( ctx, data.data() + split, data.size() - split );
		EXPECT_TRUE( SHA1::result( ctx ) == expected );
	}
}
