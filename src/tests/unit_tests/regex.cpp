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

UTEST( RegEx, TextDocument ) {
	TextDocument doc;
	doc.textInput( "This number is 42.\nThe number is 69.\n" );
	auto res = doc.findAll( "\\d+", true, false, TextDocument::FindReplaceType::RegEx );
	EXPECT_EQ( res.size(), 2ul );
	if ( res.size() == 2ul ) {
		EXPECT_EQ( res[0].isValid(), true );
		EXPECT_EQ( res[1].isValid(), true );
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
