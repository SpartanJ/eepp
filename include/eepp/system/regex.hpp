#ifndef EE_SYSTEM_REGEX
#define EE_SYSTEM_REGEX

#include <eepp/core/containers.hpp>
#include <eepp/core/lrucache.hpp>
#include <eepp/system/mutex.hpp>
#include <eepp/system/patternmatcher.hpp>
#include <eepp/system/singleton.hpp>
#include <memory>

namespace EE { namespace System {

class EE_API RegExCache {
	SINGLETON_DECLARE_HEADERS( RegExCache )
  public:
	/** A compiled pattern, shared by every RegEx that asked for the same pattern and options. The
	 *  deleter stored inside it releases the pattern with the engine that compiled it, so a pattern
	 *  stays valid for as long as a RegEx holds it, even after the cache evicts its entry. */
	using CompiledPattern = std::shared_ptr<void>;

	/** Cached patterns are evicted least-recently-used beyond this count. Syntax definitions are the
	 *  bulk of the working set (every language contributes a few dozen patterns), so this keeps a
	 *  whole session's worth resident while bounding the memory the cache can pin. */
	static constexpr size_t MaxCachedPatterns = 8192;

	bool isEnabled() const { return mEnabled; }

	void setEnabled( bool enabled );

	void insert( std::string_view pattern, Uint32 options, CompiledPattern compiled );

	CompiledPattern find( std::string_view pattern, Uint32 options );

	size_t size();

	void clear();

  protected:
	bool mEnabled{ true };
	DynamicLRU<MaxCachedPatterns, size_t, CompiledPattern> mCache;
	Mutex mMutex;
};

class EE_API RegEx : public PatternMatcher {
  public:
	enum Options : Uint32 {
		None = 0x00000000u,
		AllowEmptyClass = 0x00000001u,	 // C
		AltBsux = 0x00000002u,			 // C
		AutoCallout = 0x00000004u,		 // C
		Caseless = 0x00000008u,			 // C
		DollarEndonly = 0x00000010u,	 // J M D
		Dotall = 0x00000020u,			 // C
		Dupnames = 0x00000040u,			 // C
		Extended = 0x00000080u,			 // C
		Firstline = 0x00000100u,		 // J M D
		MatchUnsetBackref = 0x00000200u, // C J M
		Multiline = 0x00000400u,		 // C
		NeverUcp = 0x00000800u,			 // C
		NeverUtf = 0x00001000u,			 // C
		NoAutoCapture = 0x00002000u,	 // C
		NoAutoPossess = 0x00004000u,	 // C
		NoDotstarAnchor = 0x00008000u,	 // C
		NoStartOptimize = 0x00010000u,	 // J M D
		Ucp = 0x00020000u,				 // C J M D
		Ungreedy = 0x00040000u,			 // C
		Utf = 0x00080000u,				 // C J M D
		NeverBackslashC = 0x00100000u,	 // C
		AltCircumflex = 0x00200000u,	 // J M D
		AltVerbnames = 0x00400000u,		 // C
		UseOffsetLimit = 0x00800000u,	 // J M D
		ExtendedMore = 0x01000000u,		 // C
		Literal = 0x02000000u,			 // C
		MatchInvalidUtf = 0x04000000u,	 // J M D
		Anchored = 0x80000000u,
		NoUtfCheck = 0x40000000u,
		Endanchored = 0x20000000u,
		FilterOutCaptures =
			0x08000000u, // It will filter out repeated captures and same range captures
		AllowFallback = 0x10000000u,
		UseOniguruma = 0x20000000u,
	};

	RegEx( std::string_view pattern, Uint32 options = Options::Utf | Options::AllowFallback,
		   bool useCache = true );

	/** Movable, not copyable: the compiled pattern and the match data are owned. Moving hands them
	 *  over, copying would release them twice. */
	RegEx( RegEx&& ) noexcept = default;
	RegEx& operator=( RegEx&& ) noexcept = default;

	virtual ~RegEx();

	virtual bool isValid() const override { return mValid; }

	virtual bool matches( const char* stringSearch, int stringStartOffset,
						  PatternMatcher::Range* matchList, size_t stringLength ) const override;

	/** @note Not thread-safe: the object keeps reusable match state (mMatchNum, mMatchData), so a
	 *  single instance must not be used from several threads at once. Construct one per thread. */
	virtual bool matches( const std::string& str, PatternMatcher::Range* matchList = nullptr,
						  int stringStartOffset = 0 ) const override;

	virtual const size_t& getNumMatches() const override;

	int getCaptureCount() const;

	const std::string_view& getPattern() const override { return mPattern; }

  protected:
	/** Releases match data with the engine that created it. Owning it through this type is what
	 *  keeps a RegEx move-only: a copy would free the same block twice. */
	struct MatchDataDeleter {
		Uint32 options;

		void operator()( void* matchData ) const;
	};

	std::string_view mPattern;
	mutable size_t mMatchNum;
	/** Compiled pattern: pcre2_code* or OnigRegex, according to the Options::UseOniguruma bit of
	 *  mOptions. It owns the pattern and releases it with the engine that compiled it, so the cache
	 *  may drop its entry while this object is still using the pattern. */
	RegExCache::CompiledPattern mCompiledPattern;
	/** Match data reused by every matches() call, opaque so the engine headers stay out of this
	 *  one: pcre2_match_data* or OnigRegion*, according to the same option bit. It depends only on
	 *  the compiled pattern, which never changes, so it is created once and owned by this object. */
	mutable std::unique_ptr<void, MatchDataDeleter> mMatchData;
	int mCaptureCount{ 0 };
	Uint32 mOptions{ Options::Utf | Options::AllowFallback };
	bool mValid : 1 { false };
	bool mFilterOutCaptures : 1 { false };

	bool initWithOnigumura( std::string_view pattern, bool useCache );
};

}} // namespace EE::System

#endif
