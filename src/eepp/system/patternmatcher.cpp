#include <cstring>
#include <eepp/system/luapattern.hpp>
#include <eepp/system/patternmatcher.hpp>
#include <eepp/system/regex.hpp>

using namespace std::literals;

namespace EE { namespace System {

#define MAX_DEFAULT_MATCHES 12

bool PatternMatcher::find( const char* stringSearch, int& startMatch, int& endMatch,
						   int stringStartOffset, int stringLength, int returnMatchIndex,
						   PatternMatcher::Range* matchesBuffer ) const {
	if ( matches( stringSearch, stringStartOffset, matchesBuffer, stringLength ) ) {
		range( returnMatchIndex, startMatch, endMatch, matchesBuffer );
		return true;
	} else {
		startMatch = -1;
		endMatch = -1;
		return false;
	}
}

bool PatternMatcher::find( const char* stringSearch, int& startMatch, int& endMatch,
						   int stringStartOffset, int stringLength, int returnMatchIndex ) const {
	PatternMatcher::Range matchesBuffer[MAX_DEFAULT_MATCHES];
	if ( matches( stringSearch, stringStartOffset, matchesBuffer, stringLength ) ) {
		range( returnMatchIndex, startMatch, endMatch, matchesBuffer );
		return true;
	} else {
		startMatch = -1;
		endMatch = -1;
		return false;
	}
}

bool PatternMatcher::find( const std::string& s, int& startMatch, int& endMatch, int offset,
						   int returnedMatchIndex, PatternMatcher::Range* matchesBuffer ) const {
	return find( s.c_str(), startMatch, endMatch, offset, s.size(), returnedMatchIndex,
				 matchesBuffer );
}

bool PatternMatcher::find( const std::string& s, int& startMatch, int& endMatch, int offset,
						   int returnedMatchIndex ) const {
	return find( s.c_str(), startMatch, endMatch, offset, s.size(), returnedMatchIndex );
}

bool PatternMatcher::range( int indexGet, int& startMatch, int& endMatch,
							PatternMatcher::Range* returnedMatched ) const {
	if ( indexGet == -1 )
		indexGet = getNumMatches() > 1 ? 1 : 0;
	if ( indexGet >= 0 && indexGet < (int)getNumMatches() ) {
		startMatch = returnedMatched[indexGet].start;
		endMatch = returnedMatched[indexGet].end;
		return true;
	}
	return false;
}

bool PatternMatcher::State::range( int index, int& start, int& end ) {
	return mPattern->range( index, start, end, mRanges );
}

bool PatternMatcher::State::matches( const char* string, size_t length ) {
	return mPattern->matches( string, 0, mRanges, length );
}

PatternMatcher::State::State( PatternMatcher* pattern, bool ownPattern ) :
	mRefCount( 1 ), mOwnPattern( ownPattern ) {
	mRanges = new Range[10];
	if ( ownPattern ) {
		switch ( pattern->getType() ) {
			case PatternType::LuaPattern:
				mPattern = new LuaPattern( pattern->getPattern() );
				break;
			case PatternType::PCRE:
				mPattern = new RegEx( pattern->getPattern() );
				break;
		}
	} else {
		mPattern = pattern;
	}
}

PatternMatcher::State::~State() {
	delete[] mRanges;
	if ( mOwnPattern )
		delete mPattern;
}

PatternMatcher::Match::Match( PatternMatcher& r, const char* string, bool ownPattern ) :
	mString( string ) {
	mLength = strlen( string );
	mState = new PatternMatcher::State( &r, ownPattern );
}

PatternMatcher::Match::Match( PatternMatcher& r, const std::string& string, bool ownPattern ) {
	mState = new PatternMatcher::State( &r, ownPattern );
	mString = string.c_str();
	mLength = string.size();
}

PatternMatcher::Match::~Match() {
	--mState->mRefCount;
	if ( mState->mRefCount == 0 )
		delete mState;
}

PatternMatcher::Match::Match( const PatternMatcher::Match& other ) :
	mState( other.mState ), mString( other.mString ), mLength( other.mLength ) {
	++mState->mRefCount;
}

PatternMatcher::Match& PatternMatcher::Match::operator=( const Match& ) {
	++mState->mRefCount;
	return *this;
}

void PatternMatcher::Match::next() {
	int m1 = 0, m2 = 0;
	mState->range( 0, m1, m2 );
	mString += m2;
	mLength -= m2;
}

std::string PatternMatcher::Match::group( int idx ) const {
	int m1, m2;
	if ( mState->range( idx, m1, m2 ) )
		return std::string( mString + m1, m2 - m1 );
	return "";
}

std::string_view PatternMatcher::Match::groupView( int idx ) const {
	static constexpr auto EMPTY = ""sv;
	int m1, m2;
	if ( mState->range( idx, m1, m2 ) )
		return std::string_view( mString + m1, m2 - m1 );
	return EMPTY;
}

bool PatternMatcher::Match::range( int idx, int& start, int& end ) const {
	return mState->range( idx, start, end );
}

std::string PatternMatcher::Match::operator[]( int index ) const {
	return group( index );
}

bool PatternMatcher::Match::matches() {
	return mState->matches( mString, mLength );
}

bool PatternMatcher::Match::subst( std::string& res ) {
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

PatternMatcher::Match PatternMatcher::gmatch( const char* s ) & {
	return PatternMatcher::Match( *this, s, false );
}

PatternMatcher::Match PatternMatcher::gmatch( const std::string& s ) & {
	return PatternMatcher::Match( *this, s, false );
}

PatternMatcher::Match PatternMatcher::gmatch( const char* s ) && {
	return PatternMatcher::Match( *this, s, true );
}

PatternMatcher::Match PatternMatcher::gmatch( const std::string& string ) && {
	return PatternMatcher::Match( *this, string, true );
}

std::string PatternMatcher::gsub( const char* text, const char* replace ) {
	PatternMatcher::Match ms( *this, text );
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

std::string PatternMatcher::gsub( const std::string& text, const std::string& replace ) {
	return gsub( text.c_str(), replace.c_str() );
}

}} // namespace EE::System
