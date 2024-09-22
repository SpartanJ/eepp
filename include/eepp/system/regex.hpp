#ifndef EE_SYSTEM_REGEX
#define EE_SYSTEM_REGEX

#include <eepp/core/containers.hpp>
#include <eepp/system/patternmatcher.hpp>
#include <eepp/system/singleton.hpp>

namespace EE { namespace System {

class EE_API RegExCache {
	SINGLETON_DECLARE_HEADERS( RegExCache )
  public:
	~RegExCache();

	bool isEnabled() const { return mEnabled; }

	void setEnabled( bool enabled );

	void insert( std::string_view, void* cache );

	void* find( const std::string_view& );

	void clear();

  protected:
	bool mEnabled{ true };
	UnorderedMap<String::HashType, void*> mCache;
};

class EE_API RegEx : public PatternMatcher {
  public:
	RegEx( const std::string_view& pattern, bool useCache = true );

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
	bool mCached{ false };

	RegEx( const std::string_view& pattern, bool useCache, bool init );

	void init( const std::string_view& pattern, bool useCache );
};

}} // namespace EE::System

#endif
