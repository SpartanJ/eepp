#include "../../tools/ecode/plugins/autocomplete/snippetparser.hpp"
#include "utest.hpp"

using namespace ecode;

UTEST( SnippetParser, tabStopsAndLinkedPlaceholders ) {
	auto parsed = SnippetParser::parse( "func(${1:arg}, $1)$0" );
	EXPECT_STDSTREQ( "func(arg, arg)", parsed.text );
	ASSERT_EQ( 3u, parsed.tabStops.size() );
	EXPECT_EQ( 1u, parsed.tabStops[0].index );
	EXPECT_EQ( 5u, parsed.tabStops[0].start );
	EXPECT_EQ( 8u, parsed.tabStops[0].end );
	EXPECT_EQ( 1u, parsed.tabStops[1].index );
	EXPECT_EQ( 10u, parsed.tabStops[1].start );
	EXPECT_EQ( 13u, parsed.tabStops[1].end );
	EXPECT_EQ( 0u, parsed.tabStops[2].index );
	EXPECT_EQ( 14u, parsed.tabStops[2].start );
	EXPECT_EQ( 14u, parsed.tabStops[2].end );
}

UTEST( SnippetParser, bracedAndImplicitFinalStops ) {
	auto parsed = SnippetParser::parse( "$2 then ${1}" );
	EXPECT_STDSTREQ( " then ", parsed.text );
	ASSERT_EQ( 3u, parsed.tabStops.size() );
	EXPECT_EQ( 2u, parsed.tabStops[0].index );
	EXPECT_EQ( 0u, parsed.tabStops[0].start );
	EXPECT_EQ( 1u, parsed.tabStops[1].index );
	EXPECT_EQ( 6u, parsed.tabStops[1].start );
	EXPECT_EQ( 0u, parsed.tabStops[2].index );
	EXPECT_EQ( 6u, parsed.tabStops[2].start );
}

UTEST( SnippetParser, nestedPlaceholdersAndUnicodeOffsets ) {
	auto parsed = SnippetParser::parse( "é ${1:${2:value}}" );
	EXPECT_STDSTREQ( "é value", parsed.text );
	ASSERT_EQ( 3u, parsed.tabStops.size() );
	EXPECT_EQ( 2u, parsed.tabStops[0].index );
	EXPECT_EQ( 2u, parsed.tabStops[0].start );
	EXPECT_EQ( 7u, parsed.tabStops[0].end );
	EXPECT_EQ( 1u, parsed.tabStops[1].index );
	EXPECT_EQ( 2u, parsed.tabStops[1].start );
	EXPECT_EQ( 7u, parsed.tabStops[1].end );
	EXPECT_EQ( 0u, parsed.tabStops[2].index );
	EXPECT_EQ( 7u, parsed.tabStops[2].start );
}

UTEST( SnippetParser, nestedPlaceholderMetadata ) {
	auto choice = SnippetParser::parse( "${1:${2|debug,release|}}" );
	ASSERT_EQ( 3u, choice.tabStops.size() );
	ASSERT_EQ( 2u, choice.tabStops[0].choices.size() );
	EXPECT_STDSTREQ( "debug", choice.tabStops[0].choices[0] );
	EXPECT_STDSTREQ( "release", choice.tabStops[0].choices[1] );

	auto unknown = SnippetParser::parse( "${MISSING:$UNKNOWN}" );
	ASSERT_EQ( 2u, unknown.tabStops.size() );
	EXPECT_EQ( 1u, unknown.tabStops[0].index );
	EXPECT_TRUE( unknown.tabStops[0].synthetic );
	EXPECT_EQ( 0u, unknown.tabStops[1].index );
}

UTEST( SnippetParser, multilineOffsets ) {
	auto parsed = SnippetParser::parse( "α\n${1:β}$0" );
	EXPECT_STDSTREQ( "α\nβ", parsed.text );
	ASSERT_EQ( 2u, parsed.tabStops.size() );
	EXPECT_EQ( 2u, parsed.tabStops[0].start );
	EXPECT_EQ( 3u, parsed.tabStops[0].end );
	EXPECT_EQ( 3u, parsed.tabStops[1].start );
}

UTEST( SnippetParser, escapingAndChoices ) {
	auto escaped = SnippetParser::parse( R"(\$name \} \\ ${1:value})" );
	EXPECT_STDSTREQ( "$name } \\ value", escaped.text );
	EXPECT_TRUE( escaped.hasTabStops() );

	auto choice = SnippetParser::parse( R"(${1|one,two\,too,three\|four|} $1)" );
	EXPECT_STDSTREQ( "one one", choice.text );
	ASSERT_EQ( 3u, choice.tabStops.size() );
	ASSERT_EQ( 3u, choice.tabStops[0].choices.size() );
	EXPECT_STDSTREQ( "one", choice.tabStops[0].choices[0] );
	EXPECT_STDSTREQ( "two,too", choice.tabStops[0].choices[1] );
	EXPECT_STDSTREQ( "three|four", choice.tabStops[0].choices[2] );
}

UTEST( SnippetParser, variablesAndUnknownVariablePlaceholders ) {
	SnippetParser::VariableMap variables{ { "TM_FILENAME", "sample.cpp" },
										  { "TM_SELECTED_TEXT", "" } };
	auto parsed = SnippetParser::parse(
		"$TM_FILENAME ${TM_SELECTED_TEXT:fallback} ${MISSING:fallback} $UNKNOWN ${UNKNOWN}",
		variables );
	EXPECT_STDSTREQ( "sample.cpp  fallback UNKNOWN UNKNOWN", parsed.text );
	ASSERT_EQ( 3u, parsed.tabStops.size() );
	EXPECT_EQ( 1u, parsed.tabStops[0].index );
	EXPECT_TRUE( parsed.tabStops[0].synthetic );
	EXPECT_EQ( 2u, parsed.tabStops[1].index );
	EXPECT_TRUE( parsed.tabStops[1].synthetic );
	EXPECT_EQ( 0u, parsed.tabStops[2].index );
}

UTEST( SnippetParser, variableTransforms ) {
	SnippetParser::VariableMap variables{ { "TM_FILENAME", "foo.test.cpp" },
										  { "TM_CURRENT_WORD", "one-two" } };
	auto filename = SnippetParser::parse( R"(${TM_FILENAME/(.*)\..+$/$1/})", variables );
	EXPECT_STDSTREQ( "foo.test", filename.text );

	auto global =
		SnippetParser::parse( R"(${TM_CURRENT_WORD/([a-z]+)/${1:/upcase}/g})", variables );
	EXPECT_STDSTREQ( "ONE-TWO", global.text );

	auto conditional = SnippetParser::parse(
		R"(${TM_FILENAME/(foo)?(missing)?/${1:+yes}${2:?bad:good}/})", variables );
	EXPECT_STDSTREQ( "yesgood.test.cpp", conditional.text );

	auto zeroWidth = SnippetParser::parse( R"(${TM_CURRENT_WORD/(?=.)/_/g})", variables );
	EXPECT_STDSTREQ( "_o_n_e_-_t_w_o", zeroWidth.text );
	variables["TM_CURRENT_WORD"] = "éx";
	auto unicodeZeroWidth = SnippetParser::parse( R"(${TM_CURRENT_WORD/(?=.)/_/g})", variables );
	EXPECT_STDSTREQ( "_é_x", unicodeZeroWidth.text );
}

UTEST( SnippetParser, variableTransformCaseModifiers ) {
	SnippetParser::VariableMap variables{ { "TM_CURRENT_WORD", "hello-world value" } };
	EXPECT_STDSTREQ(
		"helloWorldValue",
		SnippetParser::parse( R"(${TM_CURRENT_WORD/(.*)/${1:/camelcase}/})", variables ).text );
	EXPECT_STDSTREQ(
		"HelloWorldValue",
		SnippetParser::parse( R"(${TM_CURRENT_WORD/(.*)/${1:/pascalcase}/})", variables ).text );
	EXPECT_STDSTREQ(
		"hello_world_value",
		SnippetParser::parse( R"(${TM_CURRENT_WORD/(.*)/${1:/snakecase}/})", variables ).text );
	EXPECT_STDSTREQ(
		"hello-world-value",
		SnippetParser::parse( R"(${TM_CURRENT_WORD/(.*)/${1:/kebabcase}/})", variables ).text );
}

UTEST( SnippetParser, malformedPlaceholderRemainsLiteral ) {
	auto parsed = SnippetParser::parse( "before ${1:unfinished" );
	EXPECT_STDSTREQ( "before ${1:unfinished", parsed.text );
	EXPECT_FALSE( parsed.hasTabStops() );
}
