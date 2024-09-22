#include "utest.h"
#include <eepp/system/luapattern.hpp>
#include <eepp/system/regex.hpp>

using namespace EE::System;

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
