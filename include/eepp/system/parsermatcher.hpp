#ifndef EE_SYSTEM_PARSERMATCHER_HPP
#define EE_SYSTEM_PARSERMATCHER_HPP

#include <eepp/core/containers.hpp>
#include <eepp/system/patternmatcher.hpp>
#include <eepp/system/singleton.hpp>

#include <functional>
#include <string_view>

namespace EE { namespace System {

using ParserMatcherFn =
	std::function<size_t( const char* stringSearch, int stringStartOffset,
						  PatternMatcher::Range* matchList, size_t stringLength )>;

class EE_API ParserMatcherManager {
	SINGLETON_DECLARE_HEADERS( ParserMatcherManager );

  public:
	void registerBaseParsers();

	bool registeredBaseParsers() const;

	void registerParser( std::string_view parserName, ParserMatcherFn fn );

	bool hasParser( std::string_view parserName ) const;

	size_t matches( std::string_view parserName, const char* stringSearch, int stringStartOffset,
					PatternMatcher::Range* matchList, size_t stringLength );

  protected:
	UnorderedMap<std::size_t, ParserMatcherFn> mFns;
	bool mRegisteredBaseParsers{ false };
};

class EE_API ParserMatcher : public PatternMatcher {
  public:
	ParserMatcher( const std::string_view& parserName );

	virtual ~ParserMatcher() override;

	virtual bool isValid() const override;

	virtual bool matches( const char* stringSearch, int stringStartOffset,
						  PatternMatcher::Range* matchList, size_t stringLength ) const override;

	virtual bool matches( const std::string& str, PatternMatcher::Range* matchList = nullptr,
						  int stringStartOffset = 0 ) const override;

	virtual const size_t& getNumMatches() const override;

	const std::string_view& getPattern() const override { return mParserName; }

  protected:
	std::string_view mParserName;
	mutable size_t mMatchNum;
};

}} // namespace EE::System

#endif // EE_SYSTEM_PARSERMATCHER_HPP
