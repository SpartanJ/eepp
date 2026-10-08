#include "../../tools/ecode/plugins/autocomplete/usersnippetstore.hpp"
#include "utest.hpp"

using namespace ecode;

UTEST( UserSnippetStore, parsesJSONCAndCoreFields ) {
	auto parsed = UserSnippetStore::parseFile(
		R"json({
			// Language snippet file
			"For Loop": {
				"prefix": ["for", "for-const"],
				"body": ["for (const ${1:item} of ${2:items}) {", "\t$0", "}"],
				"description": "Loop over values"
			}, // trailing commas are valid JSONC
		})json",
		"javascript.json", UserSnippetSource::User, "javascript" );
	ASSERT_TRUE( parsed.valid );
	ASSERT_EQ( 1u, parsed.snippets.size() );
	const auto& snippet = parsed.snippets[0];
	EXPECT_STDSTREQ( "For Loop", snippet.name );
	ASSERT_EQ( 2u, snippet.prefixes.size() );
	EXPECT_STDSTREQ( "for-const", snippet.prefixes[1] );
	EXPECT_STDSTREQ( "for (const ${1:item} of ${2:items}) {\n\t$0\n}", snippet.body );
	EXPECT_STDSTREQ( "Loop over values", snippet.description );
	ASSERT_EQ( 1u, snippet.scopes.size() );
	EXPECT_STDSTREQ( "javascript", snippet.scopes[0] );

	auto emptyFirstLine = UserSnippetStore::parseFile(
		R"json({ "Lines": { "prefix": "lines", "body": ["", "second"] } })json", "lines.json",
		UserSnippetSource::User, "text" );
	ASSERT_EQ( 1u, emptyFirstLine.snippets.size() );
	EXPECT_STDSTREQ( "\nsecond", emptyFirstLine.snippets[0].body );
}

UTEST( UserSnippetStore, parsesGlobalAndScopedSnippets ) {
	auto parsed = UserSnippetStore::parseFile(
		R"json({
			"Global": { "prefix": "global", "body": "global$0" },
			"Scoped": {
				"scope": " cpp, C ",
				"prefix": "loop",
				"body": "loop$0"
			}
		})json",
		"shared.code-snippets", UserSnippetSource::User );
	ASSERT_TRUE( parsed.valid );
	ASSERT_EQ( 2u, parsed.snippets.size() );
	EXPECT_TRUE( parsed.snippets[0].scopes.empty() );
	ASSERT_EQ( 2u, parsed.snippets[1].scopes.size() );
	EXPECT_STDSTREQ( "cpp", parsed.snippets[1].scopes[0] );
	EXPECT_STDSTREQ( "c", parsed.snippets[1].scopes[1] );
}

UTEST( UserSnippetStore, skipsInvalidDefinitionsOnly ) {
	auto parsed = UserSnippetStore::parseFile(
		R"json({
			"Missing Body": { "prefix": "missing" },
			"Bad Prefix": { "prefix": ["", 1], "body": "bad" },
			"Valid": { "prefix": "ok", "body": "value" }
		})json",
		"test.json", UserSnippetSource::User, "cpp" );
	ASSERT_TRUE( parsed.valid );
	ASSERT_EQ( 1u, parsed.snippets.size() );
	EXPECT_STDSTREQ( "Valid", parsed.snippets[0].name );
	EXPECT_EQ( 2u, parsed.diagnostics.size() );
}

UTEST( UserSnippetStore, rejectsMalformedFiles ) {
	auto parsed =
		UserSnippetStore::parseFile( "{ invalid", "bad.json", UserSnippetSource::User, "cpp" );
	EXPECT_FALSE( parsed.valid );
	EXPECT_TRUE( parsed.snippets.empty() );
	ASSERT_EQ( 1u, parsed.diagnostics.size() );
	EXPECT_TRUE( parsed.diagnostics[0].find( "{ invalid" ) == std::string::npos );
}

UTEST( UserSnippetStore, matchesScopesPrefixesAndDuplicateTriggers ) {
	UserSnippetStore store;
	ASSERT_TRUE( store.updateFile(
		R"json({
			"First": { "prefix": ["for", "for-const"], "body": "first" },
			"Second": { "prefix": "for", "body": "second" }
		})json",
		"cpp.json", UserSnippetSource::User, "cpp" ) );
	ASSERT_TRUE(
		store.updateFile( R"json({ "Global": { "prefix": "format", "body": "global" } })json",
						  "global.code-snippets", UserSnippetSource::User ) );

	auto cpp = store.find( "CPP", "for", 10 );
	ASSERT_EQ( 3u, cpp.size() );
	EXPECT_STDSTREQ( "First", cpp[0].snippet.name );
	EXPECT_TRUE( std::any_of( cpp.begin(), cpp.end(), []( const auto& match ) {
		return match.snippet.name == "Second";
	} ) );
	EXPECT_TRUE( std::any_of( cpp.begin(), cpp.end(), []( const auto& match ) {
		return match.snippet.name == "Global";
	} ) );

	auto rust = store.find( "rust", "for", 10 );
	ASSERT_EQ( 1u, rust.size() );
	EXPECT_STDSTREQ( "Global", rust[0].snippet.name );

	auto punctuation = store.find( "cpp", "call(for-c", 10 );
	ASSERT_FALSE( punctuation.empty() );
	EXPECT_STDSTREQ( "for-const", punctuation[0].matchedPrefix );
	EXPECT_STDSTREQ( "for-c", punctuation[0].matchedInput );
	EXPECT_TRUE( store.find( "cpp", "unrelatedf", 10 ).empty() );
}

UTEST( UserSnippetStore, keepsLastGoodFileAndRemovesSources ) {
	UserSnippetStore store;
	ASSERT_TRUE( store.updateFile( R"json({ "One": { "prefix": "one", "body": "one" } })json",
								   "user.json", UserSnippetSource::User, "cpp" ) );
	ASSERT_TRUE(
		store.updateFile( R"json({ "Project": { "prefix": "project", "body": "project" } })json",
						  "project.code-snippets", UserSnippetSource::VSCodeProject ) );
	EXPECT_EQ( 2u, store.size() );
	EXPECT_FALSE( store.updateFile( "{ invalid", "user.json", UserSnippetSource::User, "cpp" ) );
	EXPECT_EQ( 2u, store.size() );
	store.removeSource( UserSnippetSource::VSCodeProject );
	EXPECT_EQ( 1u, store.size() );
	EXPECT_TRUE( store.removeFile( "user.json" ) );
	EXPECT_EQ( 0u, store.size() );
}

UTEST( UserSnippetStore, filtersByIncludedAndExcludedFilePatterns ) {
	UserSnippetStore store;
	ASSERT_TRUE( store.updateFile(
		R"json({
			"Tests": {
				"prefix": "testcase",
				"body": "test",
				"include": ["*_test.cpp", "tests/**/*.cpp"]
			},
			"Sources": {
				"prefix": "source",
				"body": "source",
				"exclude": ["generated/**", "*_test.cpp"]
			}
		})json",
		"cpp.code-snippets", UserSnippetSource::User ) );

	auto testFile = store.find( "cpp", "", 10, "unit/example_test.cpp" );
	ASSERT_EQ( 1u, testFile.size() );
	EXPECT_STDSTREQ( "Tests", testFile[0].snippet.name );

	auto nestedTest =
		store.findForLocator( "cpp", "test", 10, "tests\\unit\\example.cpp" );
	ASSERT_EQ( 1u, nestedTest.size() );
	EXPECT_STDSTREQ( "Tests", nestedTest[0].snippet.name );

	auto sourceFile = store.find( "cpp", "", 10, "src/example.cpp" );
	ASSERT_EQ( 1u, sourceFile.size() );
	EXPECT_STDSTREQ( "Sources", sourceFile[0].snippet.name );
	EXPECT_TRUE( store.find( "cpp", "", 10, "generated/example.cpp" ).empty() );
	auto nestedVendorTest = store.find( "cpp", "", 10, "vendor/tests/unit/example.cpp" );
	ASSERT_EQ( 1u, nestedVendorTest.size() );
	EXPECT_STDSTREQ( "Sources", nestedVendorTest[0].snippet.name );
}

UTEST( UserSnippetStore, locatorSearchesNamesPrefixesAndDescriptions ) {
	UserSnippetStore store;
	ASSERT_TRUE( store.updateFile(
		R"json({
			"Indexed Loop": {
				"prefix": ["fori", "for-index"],
				"body": "loop",
				"description": "Iterate over values"
			}
		})json",
		"cpp.json", UserSnippetSource::User, "cpp" ) );

	EXPECT_EQ( 1u, store.findForLocator( "cpp", "Indexed", 10 ).size() );
	EXPECT_EQ( 1u, store.findForLocator( "cpp", "fori", 10 ).size() );
	EXPECT_EQ( 1u, store.findForLocator( "cpp", "values", 10 ).size() );
	EXPECT_TRUE( store.findForLocator( "rust", "Indexed", 10 ).empty() );
}
