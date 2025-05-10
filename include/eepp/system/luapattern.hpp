#ifndef EE_SYSTEM_LUAPATTERNMATCHER_HPP
#define EE_SYSTEM_LUAPATTERNMATCHER_HPP

#include <eepp/system/patternmatcher.hpp>
#include <vector>

namespace EE { namespace System {

class EE_API LuaPattern : public PatternMatcher {
  public:
	static std::string_view getURLPattern();

	static std::string_view getURIPattern();

	static std::string matchesAny( const std::vector<std::string>& stringvec,
								   const std::string_view& pattern );

	static std::string match( const std::string& string, const std::string_view& pattern );

	static Range firstMatch( const std::string& string, const std::string_view& pattern );

	static bool hasMatches( const std::string& string, const std::string_view& pattern );

	LuaPattern( std::string_view pattern );

	virtual bool matches( const char* stringSearch, int stringStartOffset,
						  PatternMatcher::Range* matchList, size_t stringLength ) const;

	virtual bool matches( const std::string& str, PatternMatcher::Range* matchList = nullptr,
						  int stringStartOffset = 0 ) const;

	virtual const size_t& getNumMatches() const;

	const std::string_view& getPattern() const { return mPattern; }

	virtual bool isValid() const { return true; }

  protected:
	std::string_view mPattern;
	mutable size_t mMatchNum;
};

class EE_API LuaPatternStorage : public LuaPattern {
  public:
	LuaPatternStorage( const std::string& pattern );

	explicit LuaPatternStorage( std::string&& pattern );

  protected:
	std::string mPatternStorage;
};

}} // namespace EE::System

#endif // EE_SYSTEM_LUAPATTERNMATCHER_HPP
