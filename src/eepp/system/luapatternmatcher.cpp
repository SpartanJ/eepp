#include <cstring>
#include <eepp/system/lua-str.hpp>
#include <eepp/system/luapatternmatcher.hpp>

namespace EE { namespace System {

// Implementation based on the rx-cpp (https://github.com/stevedonovan/rx-cpp/).

const int MAX_DEFAULT_MATCHES = 12;
static bool sFailHandlerInitialized = false;

static void failHandler( const char* msg ) {
	throw std::string( msg );
}

std::string LuaPatternMatcher::match( const std::string& string, const std::string& pattern ) {
	LuaPatternMatcher matcher( pattern );
	int start = 0, end = 0;
	if ( matcher.find( string, start, end ) )
		return string.substr( start, end - start );
	return "";
}

LuaPatternMatcher::LuaPatternMatcher( const std::string& pattern ) : mPattern( pattern ) {
	if ( !sFailHandlerInitialized ) {
		sFailHandlerInitialized = true;
		lua_str_fail_func( failHandler );
	}
}

bool LuaPatternMatcher::matches( const char* stringSearch, int stringStartOffset,
								 LuaPatternMatcher::Match* matchList, size_t stringLength ) {
	LuaPatternMatcher::Match matchesBuffer[MAX_DEFAULT_MATCHES];
	if ( matchList == NULL )
		matchList = matchesBuffer;
	if ( stringLength == 0 )
		stringLength = strlen( stringSearch );
	try {
		mMatchNum = lua_str_match( stringSearch, stringStartOffset, stringLength, mPattern.c_str(),
								   (LuaMatch*)matchList );
	} catch ( const std::string& patternError ) {
		mErr = std::move( patternError );
		mMatchNum = 0;
	}
	return mMatchNum == 0 ? false : true;
}

bool LuaPatternMatcher::find( const char* stringSearch, int& startMatch, int& endMatch,
							  int stringStartOffset, int stringLength, int returnMatchIndex ) {
	LuaPatternMatcher::Match matchesBuffer[MAX_DEFAULT_MATCHES];
	if ( matches( stringSearch, stringStartOffset, matchesBuffer, stringLength ) ) {
		range( returnMatchIndex, startMatch, endMatch, matchesBuffer );
		return true;
	} else {
		startMatch = -1;
		endMatch = -1;
		return false;
	}
}

bool LuaPatternMatcher::range( int indexGet, int& startMatch, int& endMatch,
							   LuaPatternMatcher::Match* returnedMatched ) {
	if ( indexGet == -1 ) {
		indexGet = getNumMatches() > 1 ? 1 : 0;
	}
	if ( indexGet >= 0 && indexGet < getNumMatches() ) {
		startMatch = returnedMatched[indexGet].start;
		endMatch = returnedMatched[indexGet].end;
		return true;
	}
	return false;
}

int LuaPatternMatcher::getNumMatches() {
	return mMatchNum;
}

}} // namespace EE::System
