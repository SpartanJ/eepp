#include "utest.h"
#include <eepp/core/string.hpp>

using namespace std::literals;

using namespace EE;

UTEST( String, isAscii ) {
	// Empty string
	EXPECT_TRUE( String::isAscii( String::View( U"" ) ) );

	// Simple short ASCII string
	String strAscii( "Hello World" );
	EXPECT_TRUE( strAscii.isAscii() );

	// String with non-ASCII at the end
	String strNonAsciiEnd( "Hello world\u0080" );
	EXPECT_FALSE( strNonAsciiEnd.isAscii() );

	// String with non-ASCII at the beginning
	String strNonAsciiBegin( "\u0080Hello world" );
	EXPECT_FALSE( strNonAsciiBegin.isAscii() );

	// String with non-ASCII in the middle
	String strNonAsciiMid( "Hello \u0080 world" );
	EXPECT_FALSE( strNonAsciiMid.isAscii() );

	// Test boundary around 127
	String str127;
	str127 += (String::StringBaseType)127;
	EXPECT_TRUE( str127.isAscii() );

	String str128;
	str128 += (String::StringBaseType)128;
	EXPECT_FALSE( str128.isAscii() );

	// Test SIMD chunk boundaries (assumed 8 elements for AVX2, 4 for NEON)
	// We'll test lengths around 4, 8, 16, 32 to cover various chunk alignments

	// 1. Exact chunks + 0 remainder
	{
		// 32 chars (4x8 AVX2, 8x4 NEON)
		String longAscii( "01234567890123456789012345678901" );
		EXPECT_TRUE( longAscii.isAscii() );

		// 32 chars with invalid at last position 31
		String longNonAscii = longAscii;
		longNonAscii[31] = 129;
		EXPECT_FALSE( longNonAscii.isAscii() );

		// 32 chars with invalid at first position 0
		longNonAscii = longAscii;
		longNonAscii[0] = 129;
		EXPECT_FALSE( longNonAscii.isAscii() );
	}

	// 2. Exact chunks + remainder
	{
		// 33 chars (one element remainder)
		String longAscii( "01234567890123456789012345678901A" );
		EXPECT_TRUE( longAscii.isAscii() );

		// invalid at remainder
		String longNonAscii = longAscii;
		longNonAscii[32] = 130;
		EXPECT_FALSE( longNonAscii.isAscii() );
	}

	// 3. Just below chunk size (7 chars)
	{
		String shortAscii( "0123456" );
		EXPECT_TRUE( shortAscii.isAscii() );

		String shortNonAscii = shortAscii;
		shortNonAscii[6] = 131;
		EXPECT_FALSE( shortNonAscii.isAscii() );
	}

	// Large string verification
	{
		String largeAscii;
		for ( int i = 0; i < 1024; ++i )
			largeAscii += "A";
		EXPECT_TRUE( largeAscii.isAscii() );

		String largeNonAscii = largeAscii;
		largeNonAscii[512] = 200; // fail in the middle
		EXPECT_FALSE( largeNonAscii.isAscii() );
	}
}

UTEST( String, isLatin1 ) {
	// Empty string
	EXPECT_TRUE( String::isLatin1( String::View( U"" ) ) );

	// ASCII is also Latin1
	String strAscii( "Hello World" );
	EXPECT_TRUE( strAscii.isLatin1() );

	// Latin1 characters (128-255)
	String strLatin1;
	strLatin1 += (String::StringBaseType)0xFF; // 255
	EXPECT_TRUE( strLatin1.isLatin1() );

	// Non-Latin1 (>255)
	String strNonLatin1;
	strNonLatin1 += (String::StringBaseType)0x100; // 256
	EXPECT_FALSE( strNonLatin1.isLatin1() );

	// Boundary Check
	String str255;
	str255 += (String::StringBaseType)255;
	EXPECT_TRUE( str255.isLatin1() );

	// Complex string with Latin1 chars
	String complexLatin1 = String::fromUtf8( "Héllø Wørld"sv ); // Assuming these are in Latin1 range
	// Note: 'ø' is 0xF8 (248), 'é' is 0xE9 (233). Both in Latin1.
	EXPECT_TRUE( complexLatin1.isLatin1() );

	// Verify SIMD paths for isLatin1 (uses same template logic but limit=255)
	{
		// 32 chars of 255
		String longLatin1( 32, (String::StringBaseType)255 );
		EXPECT_TRUE( longLatin1.isLatin1() );

	}
}

UTEST( String, isAsciiHighBit ) {
	// Test comparison safety (unsigned vs signed issue)
	// 0x80000000 is a very large number, definitely not ASCII.
	// If signed comparison was used, it might be interpreted as negative and thus < 127.
	String strHigh;
	strHigh += (String::StringBaseType)0x80000000;
	EXPECT_FALSE( strHigh.isAscii() );

	String strHigh2;
	strHigh2 += (String::StringBaseType)0xFFFFFFFF;
	EXPECT_FALSE( strHigh2.isAscii() );

	// Mixed with ASCII
	String strMixed = "Hello";
	strMixed += (String::StringBaseType)0x80000000;
	EXPECT_FALSE( strMixed.isAscii() );
}

UTEST( String, isAsciiPatterns ) {
	// Alternating
	String alt;
	for (int i = 0; i < 100; i++) {
		alt += (i % 2 == 0) ? 'a' : (char)128;
	}
	EXPECT_FALSE( alt.isAscii() );

	// Block of invalid in middle of valid
	String block(100, 'a');
	for(int i=40; i<60; i++) block[i] = 200;
	EXPECT_FALSE( block.isAscii() );
}

UTEST( String, isLatin1HighBit ) {
	String strHigh;
	strHigh += (String::StringBaseType)0x80000000;
	EXPECT_FALSE( strHigh.isLatin1() );
}
