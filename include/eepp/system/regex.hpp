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

	void insert( std::string_view, Uint32 options, void* cache );

	void* find( const std::string_view&, Uint32 options );

	void clear();

  protected:
	bool mEnabled{ true };
	UnorderedMap<String::HashType, void*> mCache;
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
		FilterOutCaptures =
			0x08000000u, // It will filter out repeated captures and same range captures
	};

	RegEx( std::string_view pattern, Uint32 options = Options::Utf, bool useCache = true );

	virtual ~RegEx();

	virtual bool isValid() const override { return mValid; }

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
	bool mFilterOutCaptures{ false };
};

}} // namespace EE::System

#endif
