#include "utest.hpp"
#include <cstdlib>
#include <eepp/core/string.hpp>
#include <eepp/core/utf.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/sys.hpp>
#include <filesystem>
#include <string_view>
#include <vector>

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

UTEST( String, trim ) {
	// Separators are removed from both ends and interior ones are kept.
	EXPECT_TRUE( String::trim( std::string( "abc" ) ) == std::string( "abc" ) );
	EXPECT_TRUE( String::trim( std::string( "  a  " ) ) == std::string( "a" ) );
	// The char overload trims only that character, so tab and newline survive it; the
	// string_view overload takes a whole separator set.
	EXPECT_TRUE( String::trim( std::string( "\t a b \n" ) ) == std::string( "\t a b \n" ) );
	EXPECT_TRUE( String::trim( std::string( "  a  " ), ' ' ) == std::string( "a" ) );
	EXPECT_TRUE( String::trim( std::string( "xxbxx" ), 'x' ) == std::string( "b" ) );
	EXPECT_TRUE( String::trim( std::string( "\t\r\n a \t\r\n" ), std::string_view( " \t\r\n" ) ) ==
				 std::string( "a" ) );

	// A string made only of separators has nothing left once trimmed. It used to come back as a
	// shorter string of separators instead of as an empty one.
	EXPECT_TRUE( String::trim( std::string() ).empty() );
	EXPECT_TRUE( String::trim( std::string( " " ) ).empty() );
	EXPECT_TRUE( String::trim( std::string( "  " ) ).empty() );
	EXPECT_TRUE( String::trim( std::string( "        " ) ).empty() );
	EXPECT_TRUE( String::trim( std::string( "xxxx" ), 'x' ).empty() );
	EXPECT_TRUE( String::trim( std::string( "\t\r\n " ), std::string_view( " \t\r\n" ) ).empty() );

	std::string inPlace = "   ";
	String::trimInPlace( inPlace, ' ' );
	EXPECT_TRUE( inPlace.empty() );
	inPlace = "  a  ";
	String::trimInPlace( inPlace, ' ' );
	EXPECT_TRUE( inPlace == std::string( "a" ) );

	// The view overloads must report the trimmed range, not a truncated one.
	EXPECT_TRUE( String::trim( std::string_view() ).empty() );
	EXPECT_TRUE( String::trim( std::string_view( "     " ) ).empty() );
	EXPECT_TRUE( String::trim( std::string_view( "  a  " ) ) == std::string_view( "a" ) );
	EXPECT_TRUE( String::trim( std::string_view( "xx" ), std::string_view( "x" ) ).empty() );
	EXPECT_TRUE( String::trim( std::string_view( "xxa xx" ), std::string_view( "x " ) ) ==
				 std::string_view( "a" ) );

	// The UTF-32 overloads are separate implementations and had the same defect.
	EXPECT_TRUE( String::trim( String() ).empty() );
	EXPECT_TRUE( String::trim( String( "   " ) ).empty() );
	EXPECT_TRUE( String::trim( String( "  a  " ) ) == String( "a" ) );
	EXPECT_TRUE( String::trim( String( "xxx" ), std::string_view( "x" ) ).empty() );
	EXPECT_TRUE( String::trim( String( "  a  " ), std::string_view( " " ) ) == String( "a" ) );
	EXPECT_TRUE( String::trim( String::View( U"   " ) ).empty() );
	EXPECT_TRUE( String::trim( String::View( U"  a  " ) ) == String::View( U"a" ) );
	EXPECT_TRUE( String::trim( String::View( U"  " ), String::View( U" " ) ).empty() );
	EXPECT_TRUE( String::trim( String::View( U"  a  " ), String::View( U" " ) ) ==
				 String::View( U"a" ) );
}

UTEST( String, lTrimAndRTrim ) {
	// Only the requested side is removed and interior separators are kept.
	EXPECT_TRUE( String::lTrim( std::string( "  a  " ) ) == std::string( "a  " ) );
	EXPECT_TRUE( String::rTrim( std::string( "  a  " ) ) == std::string( "  a" ) );
	EXPECT_TRUE( String::lTrim( std::string( "abc" ) ) == std::string( "abc" ) );
	EXPECT_TRUE( String::rTrim( std::string( "abc" ) ) == std::string( "abc" ) );
	EXPECT_TRUE( String::lTrim( std::string( "xxa" ), 'x' ) == std::string( "a" ) );
	EXPECT_TRUE( String::rTrim( std::string( "axx" ), 'x' ) == std::string( "a" ) );
	EXPECT_TRUE( String::lTrim( std::string( "\t a " ), std::string_view( " \t" ) ) ==
				 std::string( "a " ) );

	// A string made only of separators has nothing left, as in trim().
	EXPECT_TRUE( String::lTrim( std::string() ).empty() );
	EXPECT_TRUE( String::rTrim( std::string() ).empty() );
	EXPECT_TRUE( String::lTrim( std::string( "   " ) ).empty() );
	EXPECT_TRUE( String::rTrim( std::string( "   " ) ).empty() );
	EXPECT_TRUE( String::lTrim( std::string( "xxxx" ), 'x' ).empty() );
	EXPECT_TRUE( String::rTrim( std::string( "xxxx" ), 'x' ).empty() );
	EXPECT_TRUE( String::lTrim( std::string( " \t\n " ), std::string_view( " \t\n" ) ).empty() );
	EXPECT_TRUE( String::rTrim( std::string( " \t\n " ), std::string_view( " \t\n" ) ).empty() );
	EXPECT_TRUE( String::lTrim( std::string_view( "   " ) ).empty() );
	EXPECT_TRUE( String::rTrim( std::string_view( "   " ) ).empty() );

	// The UTF-32 overloads are separate implementations.
	EXPECT_TRUE( String::lTrim( String( "   " ) ).empty() );
	EXPECT_TRUE( String::rTrim( String( "   " ) ).empty() );
	EXPECT_TRUE( String::lTrim( String( "  a  " ) ) == String( "a  " ) );
	EXPECT_TRUE( String::rTrim( String( "  a  " ) ) == String( "  a" ) );
	EXPECT_TRUE( String::lTrim( String::View( U"   " ) ).empty() );
	EXPECT_TRUE( String::rTrim( String::View( U"   " ) ).empty() );
	EXPECT_TRUE( String::lTrim( String::View( U"  a  " ) ) == String::View( U"a  " ) );
	EXPECT_TRUE( String::rTrim( String::View( U"  a  " ) ) == String::View( U"  a" ) );

	// Trimming one side and then the other is what trim() does in one step.
	EXPECT_TRUE( String::rTrim( String::lTrim( std::string( "  a b  " ) ) ) ==
				 String::trim( std::string( "  a b  " ) ) );
}

UTEST( String, readBySeparator ) {
	auto collect = []( const std::string& input, char sep ) {
		std::vector<std::string> chunks;
		String::readBySeparator(
			input, [&]( std::string_view chunk ) { chunks.emplace_back( chunk ); }, sep );
		return chunks;
	};

	// An empty buffer holds no chunks, so the callback is not handed a spurious empty one.
	EXPECT_TRUE( collect( std::string(), '\n' ).empty() );

	// A buffer without a separator is a single chunk.
	{
		auto chunks = collect( "abc", '\n' );
		EXPECT_EQ( chunks.size(), 1ul );
		EXPECT_TRUE( chunks[0] == std::string( "abc" ) );
	}

	// A trailing separator does not add an empty chunk.
	{
		auto chunks = collect( "a\n", '\n' );
		EXPECT_EQ( chunks.size(), 1ul );
		EXPECT_TRUE( chunks[0] == std::string( "a" ) );
	}

	// Empty lines between separators are preserved, and a lone separator is one empty chunk.
	{
		auto chunks = collect( "a\n\nb", '\n' );
		EXPECT_EQ( chunks.size(), 3ul );
		EXPECT_TRUE( chunks[0] == std::string( "a" ) );
		EXPECT_TRUE( chunks[1].empty() );
		EXPECT_TRUE( chunks[2] == std::string( "b" ) );
	}
	EXPECT_EQ( collect( "\n", '\n' ).size(), 1ul );

	// The separator is configurable.
	{
		auto chunks = collect( "a;b;", ';' );
		EXPECT_EQ( chunks.size(), 2ul );
		EXPECT_TRUE( chunks[0] == std::string( "a" ) );
		EXPECT_TRUE( chunks[1] == std::string( "b" ) );
	}

	// The stoppable variant stops at the first chunk that asks it to, and skips empty buffers.
	{
		int seen = 0;
		String::readBySeparatorStoppable( std::string( "a\nb\nc" ), [&]( std::string_view ) {
			++seen;
			return true;
		} );
		EXPECT_EQ( seen, 1 );

		seen = 0;
		String::readBySeparatorStoppable( std::string(), [&]( std::string_view ) {
			++seen;
			return false;
		} );
		EXPECT_EQ( seen, 0 );
	}
}

UTEST( String, splitCb ) {
	auto split = []( const std::string& input, const std::string& delims,
					 const std::string& preserve = "", const std::string& quote = "\"",
					 bool removeQuotes = false ) {
		std::vector<std::string> tokens;
		String::splitCb(
			[&]( std::string_view token ) {
				tokens.emplace_back( token );
				return true;
			},
			input, delims, preserve, quote, removeQuotes );
		return tokens;
	};

	// Tokens are split on any of the delimiter characters, and empty ones are dropped.
	{
		auto tokens = split( "a,b,c", "," );
		EXPECT_EQ( tokens.size(), 3ul );
		EXPECT_TRUE( tokens[0] == std::string( "a" ) );
		EXPECT_TRUE( tokens[2] == std::string( "c" ) );
	}
	EXPECT_EQ( split( "a,,c", "," ).size(), 2ul );

	// A buffer with no delimiter is one token, an empty buffer yields none.
	EXPECT_EQ( split( "abc", "," ).size(), 1ul );
	EXPECT_TRUE( split( "", "," ).empty() );

	// A quoted token keeps its quotes unless removeQuotes is requested.
	{
		auto kept = split( "\"a\",\"b\"", "," );
		EXPECT_EQ( kept.size(), 2ul );
		EXPECT_TRUE( kept[0] == std::string( "\"a\"" ) );

		auto stripped = split( "\"a\",\"b\"", ",", "", "\"", true );
		EXPECT_EQ( stripped.size(), 2ul );
		EXPECT_TRUE( stripped[0] == std::string( "a" ) );
		EXPECT_TRUE( stripped[1] == std::string( "b" ) );
	}

	// delimsPreserve hands the preserved separator over as a token of its own.
	{
		auto tokens = split( "a;b", "", ";" );
		EXPECT_EQ( tokens.size(), 3ul );
		EXPECT_TRUE( tokens[0] == std::string( "a" ) );
		EXPECT_TRUE( tokens[1] == std::string( ";" ) );
		EXPECT_TRUE( tokens[2] == std::string( "b" ) );
	}

	// Brackets group only when they are part of the quote set, which is what code splitting needs.
	EXPECT_EQ( split( "f(a,b),c", "," ).size(), 3ul );
	{
		auto tokens = split( "f(a,b),c", ",", "", "(" );
		EXPECT_EQ( tokens.size(), 2ul );
		EXPECT_TRUE( tokens[0] == std::string( "f(a,b)" ) );
		EXPECT_TRUE( tokens[1] == std::string( "c" ) );
	}
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

	std::string reusableUtf8;
	reusableUtf8.reserve( 128 );
	const char* utf8Storage = reusableUtf8.data();
	text.toUtf8( reusableUtf8 );
	EXPECT_STREQ( "áβ中", reusableUtf8.c_str() );
	EXPECT_EQ( utf8Storage, reusableUtf8.data() );
}

UTEST( String, acceleratedUtf8Conversion ) {
	const std::string ascii( 256, 'a' );
	const String asciiText( ascii );
	EXPECT_STDSTREQ( ascii, asciiText.toUtf8() );
	EXPECT_EQ( ascii.size(),
			   String::utf8EncodedLength( asciiText.getString(), TextHints::AllAscii ) );
	std::string asciiOutput;
	String::appendUtf8( asciiText.getString(), asciiOutput, TextHints::AllAscii );
	EXPECT_STDSTREQ( ascii, asciiOutput );

	const std::string unicode = std::string( 64, 'a' ) + "áβ中🙂" + std::string( 64, 'z' );
	const String unicodeText = String::fromUtf8( unicode );
	EXPECT_STDSTREQ( unicode, unicodeText.toUtf8() );
	EXPECT_EQ( unicode.size(), String::utf8EncodedLength( unicodeText.getString() ) );

	std::string appended = "prefix:";
	String::appendUtf8( unicodeText.getString(), appended );
	EXPECT_STDSTREQ( "prefix:" + unicode, appended );

	String::StringType malformed( 64, U'a' );
	malformed.push_back( static_cast<char32_t>( 0x110000 ) );
	malformed.append( 64, U'z' );
	const String malformedText( malformed );
	EXPECT_STDSTREQ( std::string( 64, 'a' ) + std::string( 64, 'z' ), malformedText.toUtf8() );
}

UTEST( String, acceleratedUtf8Decoding ) {
	const std::string ascii( 256, 'a' );
	const String asciiText( ascii );
	EXPECT_EQ( ascii.size(), asciiText.size() );
	EXPECT_STDSTREQ( ascii, asciiText.toUtf8() );

	const std::string unicode = std::string( 64, 'a' ) + "áβ中🙂" + std::string( 64, 'z' );
	String reused;
	reused.assignUtf8( unicode );
	EXPECT_STDSTREQ( unicode, reused.toUtf8() );
	EXPECT_EQ( reused.size(), String::utf8Length( unicode ) );

	const std::string withBom = "\xEF\xBB\xBF" + ascii;
	EXPECT_STDSTREQ( ascii, String( withBom ).toUtf8() );
	reused.assignUtf8( withBom );
	ASSERT_TRUE( !reused.empty() );
	EXPECT_EQ( static_cast<Uint32>( 0xFEFF ), static_cast<Uint32>( reused.front() ) );

	std::string malformed( 64, 'a' );
	malformed.append( "\xF0\x28\x8C\x28", 4 );
	malformed.append( 64, 'z' );
	String::StringType expected;
	Utf8::toUtf32( malformed.begin(), malformed.end(), std::back_inserter( expected ) );
	EXPECT_TRUE( String( malformed ).getString() == expected );
}

UTEST( String, byteStringEscapeAndUnescape ) {
	const std::string raw = "line one\r\nline two\t\a\b\f\v";
	const std::string escaped = String::escape( raw );
	EXPECT_STDSTREQ( "line one\\r\\nline two\\t\\a\\b\\f\\v", escaped );
	const std::string unescaped = String::unescape( escaped );
	EXPECT_STDSTREQ( raw, unescaped );
	const std::string quoted = String::unescape( R"(quoted\" slash\\ unknown\q)" );
	EXPECT_STDSTREQ( "quoted\" slash\\ unknown\\q", quoted );

	const std::string utf8Bytes = String::unescape( std::string_view{ R"(\303\261)" } );
	EXPECT_STDSTREQ( "ñ", utf8Bytes );
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
