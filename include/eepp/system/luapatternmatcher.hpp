#ifndef EE_SYSTEM_LUAPATTERNMATCHER_HPP
#define EE_SYSTEM_LUAPATTERNMATCHER_HPP

#include <eepp/config.hpp>
#include <string>

namespace EE { namespace System {

class EE_API LuaPatternMatcher {
  public:
	struct Match {
		int start{-1};
		int end{-1};
		bool isValid() { return -1 != start && -1 != end; }
	};

	static std::string match( const std::string& string, const std::string& pattern );

	static Match find( const std::string& string, const std::string& pattern );

	LuaPatternMatcher( const std::string& pattern );

	bool matches( const char* stringSearch, int stringStartOffset,
				  LuaPatternMatcher::Match* matchList, size_t stringLength );

	bool matches( const std::string& str, LuaPatternMatcher::Match* matchList,
				  int stringStartOffset = 0 ) {
		return matches( str.c_str(), stringStartOffset, matchList, str.size() );
	}

	bool find( const char* stringSearch, int& startMatch, int& endMatch, int stringStartOffset = 0,
			   int stringLength = 0, int returnMatchIndex = 0 );

	bool find( const std::string& s, int& startMatch, int& endMatch, int offset = 0,
			   int returnedMatchIndex = 0 ) {
		return find( s.c_str(), startMatch, endMatch, offset, s.size(), returnedMatchIndex );
	}

	int getNumMatches();

	bool range( int indexGet, int& startMatch, int& endMatch,
				LuaPatternMatcher::Match* returnedMatched );

  protected:
	std::string mErr;
	std::string mPattern;
	int mMatchNum;
};

}} // namespace EE::System

#endif // EE_SYSTEM_LUAPATTERNMATCHER_HPP
