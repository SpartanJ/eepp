#include <cstring>
#include <eepp/core/core.hpp>
#include <eepp/system/lua-str.hpp>
#include <eepp/system/luapattern.hpp>

using namespace std::literals;

namespace EE { namespace System {

const int MAX_DEFAULT_MATCHES = 12;
static bool sFailHandlerInitialized = false;

std::string_view LuaPattern::getURLPattern() {
	return "https?://[%w_.~!*:@&+$/?%%#-]-%w[-.%w]*%.%w%w%w?%w?:?%d*/?[%w_.~!*:@&+$/?%%#=-]*"sv;
}

std::string_view LuaPattern::getURIPattern() {
	return "%w+://[%w_.~!*:@&+$/?%%#-]-%w[-.%w]*%.%w%w%w?%w?:?%d*/?[%w_.~!*:@&+$/?%%#=-]*"sv;
}

static void failHandler( const char* msg ) {
	throw std::string( msg );
}

std::string LuaPattern::match( const std::string& string, const std::string_view& pattern ) {
	LuaPattern matcher( pattern );
	int start = 0, end = 0;
	if ( matcher.find( string, start, end ) )
		return string.substr( start, end - start );
	return "";
}

std::string LuaPattern::matchesAny( const std::vector<std::string>& stringvec,
									const std::string_view& pattern ) {
	LuaPattern matcher( pattern );
	int start = 0, end = 0;
	for ( const auto& str : stringvec ) {
		if ( matcher.find( str, start, end ) ) {
			return str.substr( start, end - start );
		}
	}
	return "";
}

LuaPattern::Range LuaPattern::find( const std::string& string, const std::string_view& pattern ) {
	LuaPattern matcher( pattern );
	int start = 0, end = 0;
	if ( matcher.find( string, start, end ) )
		return { start, end };
	return { -1, -1 };
}

bool LuaPattern::matches( const std::string& string, const std::string_view& pattern ) {
	return find( string, pattern ).isValid();
}

LuaPattern::LuaPattern( const std::string_view& pattern ) : mPattern( pattern ), mMatchNum( 0 ) {
	if ( !sFailHandlerInitialized ) {
		sFailHandlerInitialized = true;
		lua_str_fail_func( failHandler );
	}
}

bool LuaPattern::matches( const char* stringSearch, int stringStartOffset,
						  LuaPattern::Range* matchList, size_t stringLength ) const {
	LuaPattern::Range matchesBuffer[MAX_DEFAULT_MATCHES];
	if ( matchList == nullptr )
		matchList = matchesBuffer;
	if ( stringLength == 0 )
		stringLength = strlen( stringSearch );
	try {
		mMatchNum = lua_str_match( stringSearch, stringStartOffset, stringLength, mPattern.data(),
								   (LuaMatch*)matchList );
	} catch ( const std::string& patternError ) {
		mMatchNum = 0;
	}
	return mMatchNum == 0 ? false : true;
}

bool LuaPattern::matches( const std::string& str, LuaPattern::Range* matchList,
						  int stringStartOffset ) const {
	return matches( str.c_str(), stringStartOffset, matchList, str.size() );
}

bool LuaPattern::find( const char* stringSearch, int& startMatch, int& endMatch,
					   int stringStartOffset, int stringLength, int returnMatchIndex ) const {
	LuaPattern::Range matchesBuffer[MAX_DEFAULT_MATCHES];
	if ( matches( stringSearch, stringStartOffset, matchesBuffer, stringLength ) ) {
		range( returnMatchIndex, startMatch, endMatch, matchesBuffer );
		return true;
	} else {
		startMatch = -1;
		endMatch = -1;
		return false;
	}
}

bool LuaPattern::find( const std::string& s, int& startMatch, int& endMatch, int offset,
					   int returnedMatchIndex ) const {
	return find( s.c_str(), startMatch, endMatch, offset, s.size(), returnedMatchIndex );
}

bool LuaPattern::range( int indexGet, int& startMatch, int& endMatch,
						LuaPattern::Range* returnedMatched ) const {
	if ( indexGet == -1 )
		indexGet = getNumMatches() > 1 ? 1 : 0;
	if ( indexGet >= 0 && indexGet < (int)getNumMatches() ) {
		startMatch = returnedMatched[indexGet].start;
		endMatch = returnedMatched[indexGet].end;
		return true;
	}
	return false;
}

const size_t& LuaPattern::getNumMatches() const {
	return mMatchNum;
}

bool LuaPattern::LuaPattern::State::range( int index, int& start, int& end ) {
	return mPattern->range( index, start, end, mRanges );
}

bool LuaPattern::LuaPattern::State::matches( const char* string, size_t length ) {
	return mPattern->matches( string, 0, mRanges, length );
}

LuaPattern::State::State( LuaPattern* pattern, bool ownPattern ) :
	mRefCount( 1 ), mOwnPattern( ownPattern ) {
	mRanges = new Range[10];
	mPattern = ownPattern ? new LuaPattern( pattern->getPatern() ) : pattern;
}

LuaPattern::State::~State() {
	delete[] mRanges;
	if ( mOwnPattern )
		delete mPattern;
}

LuaPattern::Match::Match( LuaPattern& r, const char* string, bool ownPattern ) : mString( string ) {
	mLength = strlen( string );
	mState = new LuaPattern::LuaPattern::State( &r, ownPattern );
}

LuaPattern::Match::Match( LuaPattern& r, const std::string& string, bool ownPattern ) {
	mState = new LuaPattern::LuaPattern::State( &r, ownPattern );
	mString = string.c_str();
	mLength = string.size();
}

LuaPattern::Match::~Match() {
	--mState->mRefCount;
	if ( mState->mRefCount == 0 )
		delete mState;
}

LuaPattern::Match::Match( const LuaPattern::Match& other ) :
	mState( other.mState ), mString( other.mString ), mLength( other.mLength ) {
	++mState->mRefCount;
}

LuaPattern::Match& LuaPattern::Match::operator=( const Match& ) {
	++mState->mRefCount;
	return *this;
}

void LuaPattern::Match::next() {
	int m1 = 0, m2 = 0;
	mState->range( 0, m1, m2 );
	mString += m2;
	mLength -= m2;
}

std::string LuaPattern::Match::group( int idx ) const {
	int m1, m2;
	if ( mState->range( idx, m1, m2 ) )
		return std::string( mString + m1, m2 - m1 );
	return "";
}

std::string_view LuaPattern::Match::groupView( int idx ) const {
	static constexpr auto EMPTY = ""sv;
	int m1, m2;
	if ( mState->range( idx, m1, m2 ) )
		return std::string_view( mString + m1, m2 - m1 );
	return EMPTY;
}

bool LuaPattern::Match::range( int idx, int& start, int& end ) const {
	return mState->range( idx, start, end );
}

std::string LuaPattern::Match::operator[]( int index ) const {
	return group( index );
}

bool LuaPattern::Match::matches() {
	return mState->matches( mString, mLength );
}

bool LuaPattern::Match::subst( std::string& res ) {
	if ( !matches() ) {
		res.append( mString );
		return false;
	}
	int start = 0, end = 0;
	mState->range( 0, start, end );
	if ( start == 0 )
		return true;
	res.append( mString, start );
	return true;
}

LuaPattern::Match LuaPattern::gmatch( const char* s ) & {
	return LuaPattern::Match( *this, s, false );
}

LuaPattern::Match LuaPattern::gmatch( const std::string& s ) & {
	return LuaPattern::Match( *this, s, false );
}

LuaPattern::Match LuaPattern::gmatch( const char* s ) && {
	return LuaPattern::Match( *this, s, true );
}

LuaPattern::Match LuaPattern::gmatch( const std::string& string ) && {
	return LuaPattern::Match( *this, string, true );
}

std::string LuaPattern::gsub( const char* text, const char* replace ) {
	LuaPattern::Match ms( *this, text );
	std::string res;
	while ( ms.subst( res ) ) {
		for ( const char* ptr = replace; *ptr; ++ptr ) {
			if ( *ptr == '%' ) {
				++ptr;
				int ngroup = (int)*ptr - (int)'0';
				res += ms.group( ngroup );
			} else {
				res += *ptr;
			}
		}
		ms.next();
	}
	return res;
}

std::string LuaPattern::gsub( const std::string& text, const std::string& replace ) {
	return gsub( text.c_str(), replace.c_str() );
}

LuaPatternStorage::LuaPatternStorage( const std::string& pattern ) :
	LuaPattern( "" ), mPatternStorage( pattern ) {
	mPattern = std::string_view{ mPatternStorage };
}

LuaPatternStorage::LuaPatternStorage( std::string&& pattern ) :
	LuaPattern( "" ), mPatternStorage( std::move( pattern ) ) {
	mPattern = std::string_view{ mPatternStorage };
}

}} // namespace EE::System
