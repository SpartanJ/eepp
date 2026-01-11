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

static OnigInitializer globalOnigInitializer;

} // namespace

SINGLETON_DECLARE_IMPLEMENTATION( RegExCache )

RegExCache::~RegExCache() {
	clear();
}

inline size_t getCacheHash( std::string_view key, Uint32 options ) {
	return hashCombine( std::hash<std::string_view>()( key ), options );
}

void RegExCache::insert( std::string_view key, Uint32 options, void* cache ) {
	auto hash = getCacheHash( key, options );
	Lock l( mMutex );
	mCache.insert( { hash, cache } );
	mCacheOpt.insert( { hash, options } );
}

void* RegExCache::find( std::string_view key, Uint32 options ) {
	Lock l( mMutex );
	auto it = mCache.find( getCacheHash( key, options ) );
	return ( it != mCache.end() ) ? it->second : nullptr;
}

void RegExCache::clear() {
	Lock l( mMutex );
	for ( auto& cache : mCache ) {
		auto opt = mCacheOpt.find( cache.first );
		if ( opt->second & RegEx::Options::UseOniguruma )
			onig_free( static_cast<OnigRegex>( cache.second ) );
		else
			pcre2_code_free( reinterpret_cast<pcre2_code*>( cache.second ) );
	}
	mCache.clear();
}

RegEx::RegEx( std::string_view pattern, Uint32 options, bool useCache ) :
	PatternMatcher( PatternType::PCRE ),
	mPattern( pattern ),
	mMatchNum( 0 ),
	mCompiledPattern( nullptr ),
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
		mCached = true;
		return;
	}

	if ( useCache && RegExCache::instance()->isEnabled() && ( mOptions & Options::AllowFallback ) &&
		 !( mOptions & Options::UseOniguruma ) &&
		 ( mCompiledPattern =
			   RegExCache::instance()->find( pattern, mOptions | Options::UseOniguruma ) ) ) {
		mValid = true;
		mCached = true;
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

	mCompiledPattern = pcre2_compile( pattern_sptr,	  // the pattern
									  pattern.size(), // the length of the pattern
									  options,		  // default options
									  &errornumber,	  // for error number
									  &erroroffset,	  // for error offset
									  NULL			  // use default compile context
	);

	if ( mCompiledPattern == NULL ) {
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

#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN
	pcre2_jit_compile( reinterpret_cast<pcre2_code*>( mCompiledPattern ), PCRE2_JIT_COMPLETE );
#endif

	int rc = pcre2_pattern_info( reinterpret_cast<pcre2_code*>( mCompiledPattern ),
								 PCRE2_INFO_CAPTURECOUNT, &mCaptureCount );
	if ( rc != 0 ) {
		Log::debug( "PCRE2 pattern info failed with error code " + std::to_string( rc ) );
		mValid = false;
	} else if ( useCache && RegExCache::instance()->isEnabled() ) {
		RegExCache::instance()->insert( pattern, mOptions, mCompiledPattern );
		mCached = true;
	}
}

RegEx::~RegEx() {
	if ( !mCached && mCompiledPattern != nullptr ) {
		pcre2_code_free( reinterpret_cast<pcre2_code*>( mCompiledPattern ) );
	}
}

bool RegEx::matches( const char* stringSearch, int stringStartOffset,
					 PatternMatcher::Range* matchList, size_t stringLength ) const {
	if ( !mValid || !mCompiledPattern ) {
		mMatchNum = 0;
		return false;
	}

	if ( mOptions & Options::UseOniguruma ) {
		OnigRegion* region = onig_region_new();
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
			onig_region_free( region, 1 );
			mMatchNum = 0;
			return false;
		}

		int ret = ( mOptions & Options::Anchored )
					  ? onig_match( static_cast<OnigRegex>( mCompiledPattern ), subjectPtr,
									subjectEnd, subjectStart, region, searchOpt )
					  : onig_search( static_cast<OnigRegex>( mCompiledPattern ), subjectPtr,
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

			onig_region_free( region, 1 );
			return mMatchNum > 0;

		} else if ( ret == ONIG_MISMATCH ) { // No match
			onig_region_free( region, 1 );
			mMatchNum = 0;
			return false;
		} else { // Error
			UChar errBuf[ONIG_MAX_ERROR_MESSAGE_LEN];
			onig_error_code_to_str( errBuf, ret );
			Log::debug( "Onigumura search error: %s", reinterpret_cast<const char*>( errBuf ) );
			onig_region_free( region, 1 );
			mMatchNum = 0;
			return false;
		}
	}

	auto* compiledPattern = reinterpret_cast<pcre2_code*>( mCompiledPattern );
	pcre2_match_data* match_data = pcre2_match_data_create_from_pattern( compiledPattern, NULL );

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
		pcre2_match_data_free( match_data );
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

	pcre2_match_data_free( match_data );
	return mMatchNum > 0;
}

bool RegEx::matches( const std::string& str, PatternMatcher::Range* matchList,
					 int stringStartOffset ) const {
	return matches( str.c_str(), stringStartOffset, matchList, str.size() );
}

const size_t& RegEx::getNumMatches() const {
	return mMatchNum;
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
		mValid = false;
		if ( mCompiledPattern ) {
			onig_free( regex );
			mCompiledPattern = nullptr;
		}
		return false;
	}

	mCompiledPattern = regex;
	mValid = true;
	mOptions |= Options::UseOniguruma;
	mCaptureCount = onig_number_of_captures( static_cast<OnigRegex>( mCompiledPattern ) );

	if ( useCache && RegExCache::instance()->isEnabled() ) {
		RegExCache::instance()->insert( pattern, mOptions, mCompiledPattern );
		mCached = true;
	}

	return false;
}

}} // namespace EE::System
