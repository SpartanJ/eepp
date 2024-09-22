#include "utest.h"
#include <eepp/system/luapattern.hpp>
#include <eepp/system/regex.hpp>
#include <eepp/ui/doc/textdocument.hpp>

using namespace EE::System;
using namespace EE::UI::Doc;

UTEST( RegEx, basicTest ) {
	RegEx regex( "\\d+" );
	std::string testStr = "The number is 42.";
	PatternMatcher::Range matches[10];
	regex.matches( testStr, matches );
	EXPECT_EQ( regex.isValid(), true );
	EXPECT_EQ( regex.getNumMatches(), 1ul );
	for ( size_t i = 0; i < regex.getNumMatches(); ++i ) {
		int start = matches[i].start;
		int end = matches[i].end;
		EXPECT_EQ( start, 14 );
		EXPECT_EQ( end, 16 );
	}
	RegExCache::destroySingleton();
}

UTEST( RegEx, cacheHit ) {
	for ( auto i = 0; i < 100; i++ ) {
		RegEx regex( "\\d+" );
		std::string testStr = "The number is 42.";
		PatternMatcher::Range matches[10];
		regex.matches( testStr, matches );
		EXPECT_EQ( regex.isValid(), true );
		EXPECT_EQ( regex.getNumMatches(), 1ul );
		for ( size_t i = 0; i < regex.getNumMatches(); ++i ) {
			int start = matches[i].start;
			int end = matches[i].end;
			EXPECT_EQ( start, 14 );
			EXPECT_EQ( end, 16 );
		}
	}
	RegExCache::destroySingleton();
}

UTEST( RegEx, captures ) {
	RegEx regex( "(\\d+) and (\\d+)" );
	std::string testStr = "The number is 42 and 23.";
	PatternMatcher::Range matches[10];
	regex.matches( testStr, matches );
	ASSERT_EQ( regex.isValid(), true );
	ASSERT_EQ( regex.getNumMatches(), 3ul );
	EXPECT_EQ( matches[0].start, 14 );
	EXPECT_EQ( matches[0].end, 23 );
	EXPECT_EQ( matches[1].start, 14 );
	EXPECT_EQ( matches[1].end, 16 );
	EXPECT_EQ( matches[2].start, 21 );
	EXPECT_EQ( matches[2].end, 23 );
	RegExCache::destroySingleton();
}

UTEST( RegEx, TextDocument ) {
	TextDocument doc;
	doc.textInput( "This number is 42.\nThe number is 23.\n" );
	auto res = doc.findAll( "\\d+", true, false, TextDocument::FindReplaceType::RegEx );
	ASSERT_EQ( res.size(), 2ul );
	if ( res.size() == 2ul ) {
		EXPECT_EQ( res[0].isValid(), true );
		EXPECT_TRUE( res[0].result == TextRange( { 0, 15 }, { 0, 17 } ) );
		EXPECT_EQ( res[1].isValid(), true );
		EXPECT_TRUE( res[1].result == TextRange( { 1, 14 }, { 1, 16 } ) );
	}
	RegExCache::destroySingleton();
}

UTEST( LuaPattern, basicTest ) {
	LuaPattern regex( "%d+" );
	std::string testStr = "The number is 42.";
	PatternMatcher::Range matches[10];
	regex.matches( testStr, matches );
	EXPECT_EQ( regex.getNumMatches(), 1ul );
	for ( size_t i = 0; i < regex.getNumMatches(); ++i ) {
		int start = matches[i].start;
		int end = matches[i].end;
		EXPECT_EQ( start, 14 );
		EXPECT_EQ( end, 16 );
	}
}
