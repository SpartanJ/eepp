#include <eepp/system/regex.hpp>
#include <pcre2.h>

namespace EE { namespace System {

SINGLETON_DECLARE_IMPLEMENTATION( RegExCache )

RegExCache::~RegExCache() {
	clear();
}

void RegExCache::insert( std::string_view key, void* cache ) {
	mCache.insert( { String::hash( key ), cache } );
}

void* RegExCache::find( const std::string_view& key ) {
	auto it = mCache.find( String::hash( key ) );
	return ( it != mCache.end() ) ? it->second : nullptr;
}

void RegExCache::clear() {
	for ( auto& cache : mCache )
		pcre2_code_free( reinterpret_cast<pcre2_code*>( cache.second ) );
	mCache.clear();
}

RegEx::RegEx( const std::string_view& pattern, bool useCache, bool initRegEx ) :
	PatternMatcher( PatternType::PCRE ),
	mPattern( pattern ),
	mMatchNum( 0 ),
	mCompiledPattern( nullptr ),
	mCaptureCount( 0 ),
	mValid( true ) {
	if ( initRegEx )
		init( pattern, useCache );
}

RegEx::RegEx( const std::string_view& pattern, bool useCache ) : RegEx( pattern, useCache, true ) {}

RegEx::~RegEx() {
	if ( !mCached && mCompiledPattern != nullptr ) {
		pcre2_code_free( reinterpret_cast<pcre2_code*>( mCompiledPattern ) );
	}
}

void RegEx::init( const std::string_view& pattern, bool useCache ) {
	int errornumber;
	PCRE2_SIZE erroroffset;
	PCRE2_SPTR pattern_sptr = reinterpret_cast<PCRE2_SPTR>( pattern.data() );

	if ( useCache && RegExCache::instance()->isEnabled() &&
		 ( mCompiledPattern = RegExCache::instance()->find( pattern ) ) ) {
		mValid = true;
		mCached = true;
		return;
	}

	mCompiledPattern = pcre2_compile( pattern_sptr,	  // the pattern
									  pattern.size(), // the length of the pattern
									  PCRE2_UTF,	  // default options
									  &errornumber,	  // for error number
									  &erroroffset,	  // for error offset
									  NULL			  // use default compile context
	);

	if ( mCompiledPattern == NULL ) {
		PCRE2_UCHAR buffer[256];
		pcre2_get_error_message( errornumber, buffer, sizeof( buffer ) );
		mValid = false;
		// 		throw std::runtime_error( "PCRE2 compilation failed at offset " +
		// 								  std::to_string( erroroffset ) + ": " +
		// 								  reinterpret_cast<const char*>( buffer ) );
	}

	pcre2_jit_compile( reinterpret_cast<pcre2_code*>( mCompiledPattern ), PCRE2_JIT_COMPLETE );

	int rc = pcre2_pattern_info( reinterpret_cast<pcre2_code*>( mCompiledPattern ),
								 PCRE2_INFO_CAPTURECOUNT, &mCaptureCount );
	if ( rc != 0 ) {
		// 		throw std::runtime_error( "PCRE2 pattern info failed with error code " +
		// 								  std::to_string( rc ) );
		mValid = false;
	} else if ( useCache && RegExCache::instance()->isEnabled() ) {
		RegExCache::instance()->insert( pattern, mCompiledPattern );
		mCached = true;
	}
}

bool RegEx::matches( const char* stringSearch, int stringStartOffset,
					 PatternMatcher::Range* matchList, size_t stringLength ) const {
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

	if ( matchList != nullptr ) {
		PCRE2_SIZE* ovector = pcre2_get_ovector_pointer( match_data );
		for ( size_t i = 0; i < static_cast<size_t>( rc ); ++i ) {
			matchList[i].start = static_cast<int>( ovector[2 * i] );
			matchList[i].end = static_cast<int>( ovector[2 * i + 1] );
			if ( matchList[i].start >= matchList[i].end ) {
				matchList[i].start = matchList[i].end = -1;
				mMatchNum--;
				break;
			}
		}
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
