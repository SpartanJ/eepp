#include "utest.h"
#include <cstdlib>
#include <eepp/core/string.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/sys.hpp>
#include <filesystem>

using namespace std::literals;

using namespace EE;
using namespace EE::System;

namespace {

class ScopedEnvironmentVariable {
  public:
	ScopedEnvironmentVariable( const char* name, const std::string& value ) : mName( name ) {
		if ( const char* currentValue = std::getenv( name ) ) {
			mHadValue = true;
			mValue = currentValue;
		}
		set( value.c_str() );
	}

	~ScopedEnvironmentVariable() {
		if ( mHadValue )
			set( mValue.c_str() );
		else
			unset();
	}

  private:
	void set( const char* value ) {
#if EE_PLATFORM == EE_PLATFORM_WIN
		_putenv_s( mName, value );
#else
		setenv( mName, value, 1 );
#endif
	}

	void unset() {
#if EE_PLATFORM == EE_PLATFORM_WIN
		_putenv_s( mName, "" );
#else
		unsetenv( mName );
#endif
	}

	const char* mName;
	std::string mValue;
	bool mHadValue{ false };
};

class ScopedTestDirectory {
  public:
	explicit ScopedTestDirectory( std::filesystem::path path ) : mPath( std::move( path ) ) {}

	~ScopedTestDirectory() { FileSystem::dirRemoveAll( mPath.string() ); }

  private:
	std::filesystem::path mPath;
};

} // namespace

UTEST( String, countLines ) {
	EXPECT_EQ( static_cast<size_t>( 0 ), String::countLines( "" ) );
	EXPECT_EQ( static_cast<size_t>( 1 ), String::countLines( "A" ) );
	EXPECT_EQ( static_cast<size_t>( 2 ), String::countLines( "A\n" ) );
	EXPECT_EQ( static_cast<size_t>( 2 ), String::countLines( "A\nB" ) );
	EXPECT_EQ( static_cast<size_t>( 3 ), String::countLines( "A\nB\n" ) );
	EXPECT_EQ( static_cast<size_t>( 2 ), String::countLines( "\n" ) );
	EXPECT_EQ( static_cast<size_t>( 3 ), String::countLines( "\n\n" ) );
}

UTEST( String, fromStringView ) {
	const std::string values = "1234.5px";
	Float floatValue = 0;
	EXPECT_TRUE( String::fromString( floatValue, std::string_view( values ).substr( 0, 6 ) ) );
	EXPECT_NEAR( 1234.5f, floatValue, 0.0001f );
	EXPECT_FALSE( String::fromString( floatValue, std::string_view( values ) ) );

	Int32 intValue = 0;
	EXPECT_TRUE( String::fromString( intValue, std::string_view( values ).substr( 0, 4 ) ) );
	EXPECT_EQ( 1234, intValue );
}

UTEST( String, reusableFormattingAndUtf8Assignment ) {
	std::string formatted;
	formatted.reserve( 128 );
	const char* formattedStorage = formatted.data();
	String::formatTo( formatted, "%s: %d", std::string_view{ "line" }, 42 );
	EXPECT_STREQ( "line: 42", formatted.c_str() );
	EXPECT_EQ( formattedStorage, formatted.data() );

	String text;
	text.reserve( 128 );
	const auto* textStorage = text.getString().data();
	text.assignUtf8( "áβ中" );
	const std::string utf8Text = text.toUtf8();
	EXPECT_STREQ( "áβ中", utf8Text.c_str() );
	EXPECT_EQ( textStorage, text.getString().data() );
}

UTEST( FileSystem, fileCountLines ) {
	std::string path = Sys::getTempPath() + "eepp_test_count_lines.txt";
	FileSystem::fileWrite( path, "A\nB\nC" );
	bool isBinary = false;
	EXPECT_EQ( static_cast<size_t>( 3 ), FileSystem::fileCountLines( path, &isBinary ) );
	EXPECT_FALSE( isBinary );

	FileSystem::fileWrite( path, "A\nB\nC\n" );
	EXPECT_EQ( static_cast<size_t>( 4 ), FileSystem::fileCountLines( path, &isBinary ) );
	EXPECT_FALSE( isBinary );

	// Empty file
	FileSystem::fileWrite( path, "" );
	EXPECT_EQ( static_cast<size_t>( 0 ), FileSystem::fileCountLines( path, &isBinary ) );
	EXPECT_FALSE( isBinary );

	// Binary test
	std::string binaryData = "A\n";
	binaryData += '\0';
	binaryData += "B\n";
	FileSystem::fileWrite( path, (const Uint8*)binaryData.data(), (Uint32)binaryData.size() );
	EXPECT_EQ( static_cast<size_t>( 0 ), FileSystem::fileCountLines( path, &isBinary ) );
	EXPECT_TRUE( isBinary );

	FileSystem::fileRemove( path );
}

UTEST( Sys, whichUsesPathAndCustomSearchPaths ) {
	const std::filesystem::path root = std::filesystem::path( Sys::getTempPath() ) /
									   ( "eepp-sys-which-" + std::to_string( Sys::getProcessID() ) +
										 "-" + std::to_string( Sys::getTicks() ) );
	ScopedTestDirectory cleanup( root );
	const std::filesystem::path firstDir = root / "first";
	const std::filesystem::path secondDir = root / "second";
	ASSERT_TRUE( std::filesystem::create_directories( firstDir ) );
	ASSERT_TRUE( std::filesystem::create_directories( secondDir ) );

#if EE_PLATFORM == EE_PLATFORM_WIN
	static constexpr auto EXECUTABLE_NAME = "eepp-which-probe.EXE";
	static constexpr auto LOOKUP_NAME = "eepp-which-probe";
	static constexpr auto PATH_SEPARATOR = ';';
	ScopedEnvironmentVariable pathExt( "PATHEXT", ".COM;.EXE;.BAT;.CMD" );
#else
	static constexpr auto EXECUTABLE_NAME = "eepp-which-probe";
	static constexpr auto LOOKUP_NAME = EXECUTABLE_NAME;
	static constexpr auto PATH_SEPARATOR = ':';
#endif

	const std::filesystem::path firstExecutable = firstDir / EXECUTABLE_NAME;
	const std::filesystem::path secondExecutable = secondDir / EXECUTABLE_NAME;
	ASSERT_TRUE( FileSystem::fileWrite( firstExecutable.string(), "first" ) );
	ASSERT_TRUE( FileSystem::fileWrite( secondExecutable.string(), "second" ) );
#if EE_PLATFORM != EE_PLATFORM_WIN
	std::error_code permissionError;
	const auto executablePermissions = std::filesystem::perms::owner_exec |
									   std::filesystem::perms::group_exec |
									   std::filesystem::perms::others_exec;
	std::filesystem::permissions( firstExecutable, executablePermissions,
								  std::filesystem::perm_options::add, permissionError );
	ASSERT_FALSE( permissionError );
	std::filesystem::permissions( secondExecutable, executablePermissions,
								  std::filesystem::perm_options::add, permissionError );
	ASSERT_FALSE( permissionError );
#endif

	const std::string path = firstDir.string() + PATH_SEPARATOR + secondDir.string();
	ScopedEnvironmentVariable scopedPath( "PATH", path );

	EXPECT_TRUE( Sys::which( LOOKUP_NAME ) == firstExecutable.string() );
	EXPECT_TRUE( Sys::which( firstExecutable.string() ) == firstExecutable.string() );
	EXPECT_TRUE( Sys::which( "eepp-which-missing" ).empty() );

	FileSystem::fileRemove( firstExecutable.string() );
	EXPECT_TRUE( Sys::which( LOOKUP_NAME ) == secondExecutable.string() );

	ScopedEnvironmentVariable emptyPath( "PATH", "" );
	EXPECT_TRUE( Sys::which( LOOKUP_NAME ).empty() );
	EXPECT_TRUE( Sys::which( LOOKUP_NAME, { firstDir.string(), secondDir.string() } ) ==
				 secondExecutable.string() );
}

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
	String complexLatin1 =
		String::fromUtf8( "Héllø Wørld"sv ); // Assuming these are in Latin1 range
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
	for ( int i = 0; i < 100; i++ ) {
		alt += ( i % 2 == 0 ) ? 'a' : (char)128;
	}
	EXPECT_FALSE( alt.isAscii() );

	// Block of invalid in middle of valid
	String block( 100, 'a' );
	for ( int i = 40; i < 60; i++ )
		block[i] = 200;
	EXPECT_FALSE( block.isAscii() );
}

UTEST( String, isLatin1HighBit ) {
	String strHigh;
	strHigh += (String::StringBaseType)0x80000000;
	EXPECT_FALSE( strHigh.isLatin1() );
}

UTEST( String, stripAnsiCodes ) {
	// 1. Basic color codes
	std::string redBold = "\x1B[1;31mHello\x1B[0m";
	String::stripAnsiCodes( redBold );
	EXPECT_STREQ( "Hello", redBold.c_str() );

	// 2. Cursor movement (CSI)
	std::string clearScreen = "\x1B[2JMove";
	String::stripAnsiCodes( clearScreen );
	EXPECT_STREQ( "Move", clearScreen.c_str() );

	// 3. No codes
	std::string plain = "Just text";
	String::stripAnsiCodes( plain );
	EXPECT_STREQ( "Just text", plain.c_str() );

	// 4. Multiple mixed codes
	std::string complex = "A\x1B[32mB\x1B[33mC\x1B[0m";
	String::stripAnsiCodes( complex );
	EXPECT_STREQ( "ABC", complex.c_str() );

	// 5. Code at end
	std::string endCode = "End\x1B[K";
	String::stripAnsiCodes( endCode );
	EXPECT_STREQ( "End", endCode.c_str() );

	// 6. Code at start
	std::string startCode = "\x1B[HStart";
	String::stripAnsiCodes( startCode );
	EXPECT_STREQ( "Start", startCode.c_str() );

	// 7. Long string (trigger SIMD paths)
	std::string longStr;
	std::string expected;
	for ( int i = 0; i < 1000; i++ ) {
		longStr += "a\x1B[31mb";
		expected += "ab";
	}
	String::stripAnsiCodes( longStr );
	EXPECT_STREQ( expected.c_str(), longStr.c_str() );

	// 8. Adjacent codes
	std::string adjacent = "Double\x1B[1m\x1B[31mColor";
	String::stripAnsiCodes( adjacent );
	EXPECT_STREQ( "DoubleColor", adjacent.c_str() );
}
