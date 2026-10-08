#include <eepp/system/lock.hpp>
#include <eepp/system/log.hpp>
#include <eepp/system/regex.hpp>

#include <oniguruma/oniguruma.h>
#include <pcre2.h>

namespace EE { namespace System {

namespace {

struct OnigInitializer {
	OnigInitializer() { onig_init(); }

	~OnigInitializer() { onig_end(); }
};

/** Releases a compiled pattern with the engine that produced it. The cache stores one of these
 *  inside every shared_ptr it hands out, which is what lets a RegEx keep using a pattern after the
 *  cache has evicted the entry. */
struct CompiledPatternDeleter {
	Uint32 options;

	void operator()( void* pattern ) const {
		if ( options & RegEx::Options::UseOniguruma )
			onig_free( static_cast<OnigRegex>( pattern ) );
		else
			pcre2_code_free( static_cast<pcre2_code*>( pattern ) );
	}
};

static OnigInitializer globalOnigInitializer;

} // namespace

SINGLETON_DECLARE_IMPLEMENTATION( RegExCache )

inline size_t getCacheHash( std::string_view key, Uint32 options ) {
	return hashCombine( std::hash<std::string_view>()( key ), options );
}

void RegExCache::insert( std::string_view pattern, Uint32 options, CompiledPattern compiled ) {
	Lock l( mMutex );
	mCache.put( getCacheHash( pattern, options ), std::move( compiled ) );
}

RegExCache::CompiledPattern RegExCache::find( std::string_view pattern, Uint32 options ) {
	Lock l( mMutex );
	auto cached = mCache.get( getCacheHash( pattern, options ) );
	return cached ? std::move( *cached ) : CompiledPattern();
}

size_t RegExCache::size() {
	Lock l( mMutex );
	return mCache.size();
}

void RegExCache::clear() {
	Lock l( mMutex );
	mCache.clear();
}

RegEx::RegEx( std::string_view pattern, Uint32 options, bool useCache ) :
	PatternMatcher( PatternType::PCRE ),
	mPattern( pattern ),
	mMatchNum( 0 ),
	mCaptureCount( 0 ),
	mOptions( options ),
	mValid( true ),
	mFilterOutCaptures( ( mOptions & Options::FilterOutCaptures ) != 0 ) {
	int errornumber;
	PCRE2_SIZE erroroffset;
	PCRE2_SPTR pattern_sptr = reinterpret_cast<PCRE2_SPTR>( pattern.data() );

	if ( useCache && RegExCache::instance()->isEnabled() &&
		 ( mCompiledPattern = RegExCache::instance()->find( pattern, mOptions ) ) ) {
		mValid = true;
		return;
	}

	if ( useCache && RegExCache::instance()->isEnabled() && ( mOptions & Options::AllowFallback ) &&
		 !( mOptions & Options::UseOniguruma ) &&
		 ( mCompiledPattern =
			   RegExCache::instance()->find( pattern, mOptions | Options::UseOniguruma ) ) ) {
		mValid = true;
		mOptions |= Options::UseOniguruma;
		return;
	}

	if ( mOptions & Options::UseOniguruma ) {
		initWithOnigumura( pattern, useCache );
		return;
	}

	if ( mFilterOutCaptures )
		options &= ~Options::FilterOutCaptures;

	if ( options & Options::AllowFallback )
		options &= ~Options::AllowFallback;

	if ( options & Options::UseOniguruma )
		options &= ~Options::UseOniguruma;

	auto* compiled = pcre2_compile( pattern_sptr,	// the pattern
									pattern.size(), // the length of the pattern
									options,		// default options
									&errornumber,	// for error number
									&erroroffset,	// for error offset
									NULL			// use default compile context
	);

	if ( compiled == NULL ) {
		PCRE2_UCHAR buffer[256];
		pcre2_get_error_message( errornumber, buffer, sizeof( buffer ) );
		mValid = false;
		if ( mOptions & Options::AllowFallback ) {
			initWithOnigumura( pattern, useCache );
		} else {
			Log::debug( "PCRE2 compilation failed at offset " + std::to_string( erroroffset ) +
						": " + reinterpret_cast<const char*>( buffer ) );
		}
		return;
	}

	mCompiledPattern =
		RegExCache::CompiledPattern( compiled, CompiledPatternDeleter{ mOptions } );

#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN
	pcre2_jit_compile( static_cast<pcre2_code*>( mCompiledPattern.get() ), PCRE2_JIT_COMPLETE );
#endif

	int rc = pcre2_pattern_info( static_cast<pcre2_code*>( mCompiledPattern.get() ),
								 PCRE2_INFO_CAPTURECOUNT, &mCaptureCount );
	if ( rc != 0 ) {
		Log::debug( "PCRE2 pattern info failed with error code " + std::to_string( rc ) );
		mValid = false;
	} else if ( useCache && RegExCache::instance()->isEnabled() ) {
		RegExCache::instance()->insert( pattern, mOptions, mCompiledPattern );
	}
}

void RegEx::MatchDataDeleter::operator()( void* matchData ) const {
	if ( options & Options::UseOniguruma )
		onig_region_free( static_cast<OnigRegion*>( matchData ), 1 );
	else
		pcre2_match_data_free( static_cast<pcre2_match_data*>( matchData ) );
}

RegEx::~RegEx() = default;

bool RegEx::matches( const char* stringSearch, int stringStartOffset,
					 PatternMatcher::Range* matchList, size_t stringLength ) const {
	if ( !mValid || !mCompiledPattern ) {
		mMatchNum = 0;
		return false;
	}

	if ( mOptions & Options::UseOniguruma ) {
		if ( !mMatchData )
			mMatchData = std::unique_ptr<void, MatchDataDeleter>( onig_region_new(),
																  MatchDataDeleter{ mOptions } );
		OnigRegion* region = static_cast<OnigRegion*>( mMatchData.get() );
		if ( !region ) {
			Log::error( "Onigumura: onig_region_new() failed." );
			mMatchNum = 0;
			return false;
		}

		const UChar* subjectPtr = reinterpret_cast<const UChar*>( stringSearch );
		const UChar* subjectStart = subjectPtr + stringStartOffset;
		const UChar* subjectEnd = subjectPtr + stringLength;

		OnigOptionType searchOpt = ONIG_OPTION_NONE;

		if ( stringStartOffset > static_cast<int>( stringLength ) ) {
			mMatchNum = 0;
			return false;
		}

		int ret = ( mOptions & Options::Anchored )
					  ? onig_match( static_cast<OnigRegex>( mCompiledPattern.get() ), subjectPtr,
									subjectEnd, subjectStart, region, searchOpt )
					  : onig_search( static_cast<OnigRegex>( mCompiledPattern.get() ), subjectPtr,
									 subjectEnd, subjectStart, subjectEnd, region, searchOpt );

		if ( ret >= 0 ) {
			mMatchNum = region->num_regs;

			if ( matchList != nullptr && mMatchNum > 0 ) {
				int curCap = 0;
				for ( int i = 0; i < region->num_regs; ++i ) {
					int start = static_cast<int>( region->beg[i] );
					int end = static_cast<int>( region->end[i] );
					if ( start == -1 || end == -1 )
						continue;
					if ( !mFilterOutCaptures ||
						 ( !( start == 0 && end == 0 ) && start != end &&
						   ( curCap == 0 || !( matchList[curCap - 1].start == start &&
											   matchList[curCap - 1].end == end ) ) ) ) {
						matchList[curCap].start = start;
						matchList[curCap].end = end;
						curCap++;
					}
				}
				if ( mMatchNum > 1 )
					mMatchNum = curCap;
			}

			return mMatchNum > 0;

		} else if ( ret == ONIG_MISMATCH ) { // No match
			mMatchNum = 0;
			return false;
		} else { // Error
			UChar errBuf[ONIG_MAX_ERROR_MESSAGE_LEN];
			onig_error_code_to_str( errBuf, ret );
			Log::debug( "Onigumura search error: %s", reinterpret_cast<const char*>( errBuf ) );
			mMatchNum = 0;
			return false;
		}
	}

	auto* compiledPattern = static_cast<pcre2_code*>( mCompiledPattern.get() );

	// The ovector size is taken from the compiled pattern, so one match data block serves every
	// call this object ever makes.
	if ( !mMatchData )
		mMatchData = std::unique_ptr<void, MatchDataDeleter>(
			pcre2_match_data_create_from_pattern( compiledPattern, NULL ),
			MatchDataDeleter{ mOptions } );
	pcre2_match_data* match_data = static_cast<pcre2_match_data*>( mMatchData.get() );
	if ( match_data == nullptr ) {
		mMatchNum = 0;
		return false;
	}

	PCRE2_SPTR subject = reinterpret_cast<PCRE2_SPTR>( stringSearch );

	int rc = pcre2_match( compiledPattern,	 // the compiled pattern
						  subject,			 // the subject string
						  stringLength,		 // the length of the subject
						  stringStartOffset, // start at offset in the subject
						  0,				 // default options
						  match_data,		 // match data
						  NULL				 // match context
	);

	if ( rc < 0 ) {
		mMatchNum = 0;
		// if ( rc == PCRE2_ERROR_NOMATCH )
		return false;
		// else
		//	throw std::runtime_error( "PCRE2 matching error " + std::to_string( rc ) );
	}

	mMatchNum = rc;

	if ( matchList != nullptr && mMatchNum > 0 ) {
		PCRE2_SIZE* ovector = pcre2_get_ovector_pointer( match_data );
		int curCap = 0;
		for ( size_t i = 0; i < static_cast<size_t>( rc ); ++i ) {
			int start = static_cast<int>( ovector[2 * i] );
			int end = static_cast<int>( ovector[2 * i + 1] );
			if ( !mFilterOutCaptures ||
				 ( !( start == 0 && end == 0 ) && start != end &&
				   ( curCap == 0 || !( matchList[curCap - 1].start == start &&
									   matchList[curCap - 1].end == end ) ) ) ) {
				matchList[curCap].start = start;
				matchList[curCap].end = end;
				curCap++;
			}
		}

		mMatchNum = curCap;
	}

	return mMatchNum > 0;
}

bool RegEx::matches( const std::string& str, PatternMatcher::Range* matchList,
					 int stringStartOffset ) const {
	return matches( str.c_str(), stringStartOffset, matchList, str.size() );
}

const size_t& RegEx::getNumMatches() const {
	return mMatchNum;
}

int RegEx::getCaptureCount() const {
	if ( !mCompiledPattern )
		return 0;
	if ( mOptions & Options::UseOniguruma )
		return onig_number_of_captures( static_cast<OnigRegex>( mCompiledPattern.get() ) );
	int captureCount = 0;
	return pcre2_pattern_info( static_cast<pcre2_code*>( mCompiledPattern.get() ),
							   PCRE2_INFO_CAPTURECOUNT, &captureCount ) == 0
			   ? captureCount
			   : 0;
}

bool RegEx::initWithOnigumura( std::string_view pattern, bool useCache ) {
	OnigOptionType opt = ONIG_OPTION_NONE;

	if ( mOptions & Options::Caseless )
		opt |= ONIG_OPTION_IGNORECASE;

	if ( mOptions & Options::Multiline )
		opt |= ONIG_OPTION_MULTILINE;

	OnigEncoding enc = mOptions & Options::Utf ? ONIG_ENCODING_UTF8 : ONIG_ENCODING_ASCII;
	OnigErrorInfo err;
	const UChar* patternPtr = reinterpret_cast<const UChar*>( pattern.data() );
	const UChar* patternEnd = patternPtr + pattern.size();
	OnigRegex regex;

	int ret = onig_new( &regex, patternPtr, patternEnd, opt, enc, ONIG_SYNTAX_DEFAULT, &err );

	if ( ret != ONIG_NORMAL ) {
		UChar errBuf[ONIG_MAX_ERROR_MESSAGE_LEN];
		onig_error_code_to_str( errBuf, ret, &err );
		Log::info( "Onigumura compilation failed: %s", reinterpret_cast<const char*>( errBuf ) );
		// onig_new() failed, so there is no pattern to release: mCompiledPattern is left null.
		mValid = false;
		return false;
	}

	mOptions |= Options::UseOniguruma;
	mCompiledPattern = RegExCache::CompiledPattern( regex, CompiledPatternDeleter{ mOptions } );
	mValid = true;
	mCaptureCount = onig_number_of_captures( static_cast<OnigRegex>( mCompiledPattern.get() ) );

	if ( useCache && RegExCache::instance()->isEnabled() ) {
		RegExCache::instance()->insert( pattern, mOptions, mCompiledPattern );
	}

	return false;
}

}} // namespace EE::System
