#ifndef EE_SYSTEM_REGEX
#define EE_SYSTEM_REGEX

#include <eepp/system/patternmatcher.hpp>

namespace EE { namespace System {

class EE_API RegEx : public PatternMatcher {
  public:
	RegEx( const std::string_view& pattern );

	virtual ~RegEx();

	bool isValid() const { return mValid; }

	virtual bool matches( const char* stringSearch, int stringStartOffset,
						  PatternMatcher::Range* matchList, size_t stringLength ) const override;

	virtual bool matches( const std::string& str, PatternMatcher::Range* matchList = nullptr,
						  int stringStartOffset = 0 ) const override;

	virtual const size_t& getNumMatches() const override;

	const std::string_view& getPattern() const override { return mPattern; }

  protected:
	std::string_view mPattern;
	mutable size_t mMatchNum;
	void* mCompiledPattern;
	int mCaptureCount;
	bool mValid{ false };
};

class EE_API RegExStorage : public RegEx {
  public:
	RegExStorage( const std::string& pattern );

	explicit RegExStorage( std::string&& pattern );

  protected:
	std::string mPatternStorage;
};

}} // namespace EE::System

#endif
