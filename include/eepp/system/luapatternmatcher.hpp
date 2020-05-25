#ifndef EE_SYSTEM_LUAPATTERNMATCHER_HPP
#define EE_SYSTEM_LUAPATTERNMATCHER_HPP

#include <eepp/config.hpp>
#include <string>

namespace EE { namespace System {

class EE_API LuaPatternMatcher {
  public:
	struct Match {
		int start;
		int end;
	};

	LuaPatternMatcher( const std::string& pattern );

	bool matches( const char* stringSearch, int stringStartOffset,
				  LuaPatternMatcher::Match* matches, size_t stringLength );

	bool find( const char* stringSearch, int stringStartOffset, int& startMatch, int& endMatch,
			   int stringLength = 0, int returnMatchIndex = 0 );

	bool find( const std::string& s, int offset, int& startMatch, int& endMatch,
			   int returnedMatchIndex = 0 ) {
		return find( s.c_str(), offset, startMatch, endMatch, s.size(), returnedMatchIndex );
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
