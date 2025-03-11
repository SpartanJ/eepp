#include <cstring>
#include <eepp/system/lua-str.hpp>
#include <eepp/system/luapattern.hpp>

using namespace std::literals;

namespace EE { namespace System {

#define MAX_DEFAULT_MATCHES 12
static bool sFailHandlerInitialized = false;

static void failHandler( const char* msg ) {
	throw std::string( msg );
}

LuaPatternStorage::LuaPatternStorage( const std::string& pattern ) :
	LuaPattern( "" ), mPatternStorage( pattern ) {
	mPattern = std::string_view{ mPatternStorage };
}

LuaPatternStorage::LuaPatternStorage( std::string&& pattern ) :
	LuaPattern( "" ), mPatternStorage( std::move( pattern ) ) {
	mPattern = std::string_view{ mPatternStorage };
}

const size_t& LuaPattern::getNumMatches() const {
	return mMatchNum;
}

std::string_view LuaPattern::getURLPattern() {
	return "https?://[%w_.~!*:@&+$/?%%#-]-%w[-.%w]*%.%w%w%w?%w?:?%d*/?[%w_.~!*:@&+$/?%%#=-]*"sv;
}

std::string_view LuaPattern::getURIPattern() {
	return "%w+://[%w_.~!*:@&+$/?%%#-]-%w[-.%w]*%.%w%w%w?%w?:?%d*/?[%w_.~!*:@&+$/?%%#=-]*"sv;
}

std::string LuaPattern::match( const std::string& string, const std::string_view& pattern ) {
	LuaPattern matcher( pattern );
	int start = 0, end = 0;
	if ( matcher.find( string, start, end ) )
		return string.substr( start, end - start );
	return "";
}

std::string LuaPattern::matchesAny( const std::vector<std::string>& stringvec,
									const std::string_view& pattern ) {
	LuaPattern matcher( pattern );
	int start = 0, end = 0;
	for ( const auto& str : stringvec ) {
		if ( matcher.find( str, start, end ) ) {
			return str.substr( start, end - start );
		}
	}
	return "";
}

PatternMatcher::Range LuaPattern::firstMatch( const std::string& string,
											  const std::string_view& pattern ) {
	LuaPattern matcher( pattern );
	int start = 0, end = 0;
	if ( matcher.find( string, start, end ) )
		return { start, end };
	return { -1, -1 };
}

bool LuaPattern::hasMatches( const std::string& string, const std::string_view& pattern ) {
	return LuaPattern::firstMatch( string, pattern ).isValid();
}

LuaPattern::LuaPattern( const std::string_view& pattern ) :
	PatternMatcher( PatternType::LuaPattern ), mPattern( pattern ), mMatchNum( 0 ) {
	if ( !sFailHandlerInitialized ) {
		sFailHandlerInitialized = true;
		lua_str_fail_func( failHandler );
	}
}

bool LuaPattern::matches( const char* stringSearch, int stringStartOffset,
						  PatternMatcher::Range* matchList, size_t stringLength ) const {
	if ( stringLength == 0 )
		stringLength = strlen( stringSearch );

	if ( matchList == nullptr ) {
		PatternMatcher::Range matchesBuffer[MAX_DEFAULT_MATCHES];
		try {
			mMatchNum = lua_str_match( stringSearch, stringStartOffset, stringLength,
									   mPattern.data(), (LuaMatch*)matchesBuffer );
		} catch ( const std::string& ) {
			mMatchNum = 0;
		}
	} else {
		try {
			mMatchNum = lua_str_match( stringSearch, stringStartOffset, stringLength,
									   mPattern.data(), (LuaMatch*)matchList );
		} catch ( const std::string& ) {
			mMatchNum = 0;
		}
	}
	return mMatchNum == 0 ? false : true;
}

bool LuaPattern::matches( const std::string& str, PatternMatcher::Range* matchList,
						  int stringStartOffset ) const {
	return matches( str.c_str(), stringStartOffset, matchList, str.size() );
}

}} // namespace EE::System
