#include <eepp/system/log.hpp>
#include <eepp/system/regex.hpp>
#include <pcre2.h>

namespace EE { namespace System {

SINGLETON_DECLARE_IMPLEMENTATION( RegExCache )

RegExCache::~RegExCache() {
	clear();
}

void RegExCache::insert( std::string_view key, Uint32 options, void* cache ) {
	mCache.insert( { hashCombine( String::hash( key ), options ), cache } );
}

void* RegExCache::find( const std::string_view& key, Uint32 options ) {
	auto it = mCache.find( hashCombine( String::hash( key ), options ) );
	return ( it != mCache.end() ) ? it->second : nullptr;
}

void RegExCache::clear() {
	for ( auto& cache : mCache )
		pcre2_code_free( reinterpret_cast<pcre2_code*>( cache.second ) );
	mCache.clear();
}

RegEx::RegEx( const std::string_view& pattern, Uint32 options, bool useCache ) :
	PatternMatcher( PatternType::PCRE ),
	mPattern( pattern ),
	mMatchNum( 0 ),
	mCompiledPattern( nullptr ),
	mCaptureCount( 0 ),
	mValid( true ),
	mFilterOutCaptures( ( options & Options::FilterOutCaptures ) != 0 ) {
	int errornumber;
	PCRE2_SIZE erroroffset;
	PCRE2_SPTR pattern_sptr = reinterpret_cast<PCRE2_SPTR>( pattern.data() );

	if ( mFilterOutCaptures )
		options &= ~Options::FilterOutCaptures;

	if ( useCache && RegExCache::instance()->isEnabled() &&
		 ( mCompiledPattern = RegExCache::instance()->find( pattern, options ) ) ) {
		mValid = true;
		mCached = true;
		return;
	}

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
		Log::debug( "PCRE2 compilation failed at offset " + std::to_string( erroroffset ) + ": " +
					reinterpret_cast<const char*>( buffer ) );
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
		RegExCache::instance()->insert( pattern, options, mCompiledPattern );
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
	auto* compiledPattern = reinterpret_cast<pcre2_code*>( mCompiledPattern );
	pcre2_match_data* match_data = pcre2_match_data_create_from_pattern( compiledPattern, NULL );

	PCRE2_SPTR subject = reinterpret_cast<PCRE2_SPTR>( stringSearch );

	int rc = pcre2_match( compiledPattern,					// the compiled pattern
						  subject + stringStartOffset,		// the subject string
						  stringLength - stringStartOffset, // the length of the subject
						  0,								// start at offset in the subject
						  0,								// default options
						  match_data,						// match data
						  NULL								// match context
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
			int start = stringStartOffset + static_cast<int>( ovector[2 * i] );
			int end = stringStartOffset + static_cast<int>( ovector[2 * i + 1] );
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

}} // namespace EE::System
