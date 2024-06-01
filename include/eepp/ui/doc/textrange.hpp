#ifndef EE_UI_DOC_TEXTRANGE_HPP
#define EE_UI_DOC_TEXTRANGE_HPP

#include <algorithm>
#include <eepp/core/debug.hpp>
#include <eepp/ui/doc/textposition.hpp>

namespace EE { namespace UI { namespace Doc {

class EE_API TextRange {
  public:
	TextRange() {}
	TextRange( const TextPosition& start, const TextPosition& end ) :
		mStart( start ), mEnd( end ) {}

	bool isValid() const { return mStart.isValid() && mEnd.isValid(); }

	void clear() {
		mStart = {};
		mEnd = {};
	}

	TextPosition& start() { return mStart; }

	TextPosition& end() { return mEnd; }

	const TextPosition& start() const { return mStart; }

	const TextPosition& end() const { return mEnd; }

	TextRange normalized() const { return TextRange( normalizedStart(), normalizedEnd() ); }

	TextRange& normalize() {
		auto normalize( normalized() );
		mStart = normalize.start();
		mEnd = normalize.end();
		return *this;
	}

	void reverse() { std::swap( mEnd, mStart ); }

	TextRange reversed() const { return TextRange( mEnd, mStart ); }

	void setStart( const TextPosition& position ) { mStart = position; }

	void setEnd( const TextPosition& position ) { mEnd = position; }

	void set( const TextPosition& start, const TextPosition& end ) {
		mStart = start;
		mEnd = end;
	}

	bool operator==( const TextRange& other ) const {
		return mStart == other.mStart && mEnd == other.mEnd;
	}

	bool operator!=( const TextRange& other ) const {
		return mStart != other.mStart || mEnd != other.mEnd;
	}

	bool operator<( const TextRange& other ) const {
		return mStart < other.mStart && mEnd < other.mEnd;
	}

	bool operator>( const TextRange& other ) const {
		return mStart > other.mStart && mEnd > other.mEnd;
	}

	bool operator<=( const TextRange& other ) const {
		return mStart <= other.mStart && mEnd <= other.mEnd;
	}

	bool operator>=( const TextRange& other ) const {
		return mStart >= other.mStart && mEnd >= other.mEnd;
	}

	TextRange operator+( const TextRange& other ) const {
		return TextRange( mStart + other.mStart, mEnd + other.mEnd );
	}

	TextRange operator+=( const TextRange& other ) const {
		return TextRange( mStart + other.mStart, mEnd + other.mEnd );
	}

	TextRange operator-( const TextRange& other ) const {
		return TextRange( mStart - other.mStart, mEnd - other.mEnd );
	}

	TextRange operator-=( const TextRange& other ) const {
		return TextRange( mStart - other.mStart, mEnd - other.mEnd );
	}

	bool contains( const TextPosition& position ) const {
		if ( !( position.line() > mStart.line() ||
				( position.line() == mStart.line() && position.column() >= mStart.column() ) ) )
			return false;
		if ( !( position.line() < mEnd.line() ||
				( position.line() == mEnd.line() && position.column() <= mEnd.column() ) ) )
			return false;
		return true;
	}

	bool intersectsLineRange( const TextRange& range ) const {
		eeASSERT( range.isNormalized() );
		return mStart.line() <= static_cast<Int64>( range.end().line() ) &&
			   static_cast<Int64>( range.start().line() ) <= mEnd.line();
	}

	template <typename T> bool intersectsLineRange( T fromLine, T toLine ) const {
		return mStart.line() <= static_cast<Int64>( toLine ) &&
			   static_cast<Int64>( fromLine ) <= mEnd.line();
	}

	template <typename T> bool intersectsLineRange( const std::pair<T, T>& range ) const {
		return mStart.line() <= static_cast<Int64>( range.second ) &&
			   static_cast<Int64>( range.first ) <= mEnd.line();
	}

	bool containsLine( const Int64& line ) const {
		return line >= mStart.line() && line <= mEnd.line();
	}

	bool contains( const TextRange& range ) const {
		return range.start() >= start() && range.end() <= end();
	}

	bool hasSelection() const { return isValid() && mStart != mEnd; }

	bool inSameLine() const { return isValid() && mStart.line() == mEnd.line(); }

	Int64 height() const {
		if ( mEnd.line() > mStart.line() )
			return mEnd.line() - mStart.line() + 1;
		return mStart.line() - mStart.line() + 1;
	}

	Int64 length() const {
		if ( !inSameLine() )
			return 0;
		if ( mEnd.column() > mStart.column() )
			return mEnd.column() - mStart.column();
		return mStart.column() - mEnd.column();
	}

	std::string toString() const {
		return String::format( "%s - %s", mStart.toString().c_str(), mEnd.toString().c_str() );
	}

	static TextRange fromString( const std::string& range ) {
		auto split = String::split( range, "-" );
		if ( split.size() == 2 ) {
			return { TextPosition::fromString( String::trim( split[0] ) ),
					 TextPosition::fromString( String::trim( split[1] ) ) };
		}
		return {};
	}

	bool isNormalized() const { return mStart <= mEnd; }

  private:
	TextPosition mStart;
	TextPosition mEnd;

	TextPosition normalizedStart() const { return mStart < mEnd ? mStart : mEnd; }

	TextPosition normalizedEnd() const { return mStart < mEnd ? mEnd : mStart; }
};

class EE_API TextRanges : public std::vector<TextRange> {
  public:
	TextRanges() {}

	TextRanges( const std::vector<TextRange>& ranges ) : std::vector<TextRange>( ranges ) {}

	TextRanges( const TextRange& ranges ) : std::vector<TextRange>( { ranges } ) {}

	bool isSorted() const { return mIsSorted; }

	bool isValid() const {
		for ( const auto& selection : *this ) {
			if ( !selection.isValid() )
				return false;
		}
		return true;
	}

	bool exists( const TextRange& range ) const {
		for ( const auto& r : *this )
			if ( range == r )
				return true;
		return false;
	}

	size_t findIndex( const TextRange& range ) const {
		for ( size_t i = 0; i < size(); ++i ) {
			if ( ( *this )[i] == range )
				return i;
		}
		return 0;
	}

	bool hasSelection() const {
		for ( const auto& r : *this )
			if ( r.hasSelection() )
				return true;
		return false;
	}

	void sort() {
		std::sort( begin(), end() );
		setSorted();
	}

	void setSorted() { mIsSorted = true; }

	bool merge() {
		if ( size() <= 1 )
			return false;
		size_t oldSize = size();
		TextRanges newRanges;
		newRanges.emplace_back( ( *this )[0] );
		for ( size_t i = 1; i < size(); ++i )
			if ( !newRanges.exists( ( *this )[i] ) )
				newRanges.emplace_back( ( *this )[i] );
		*this = newRanges;
		sort();
		return oldSize != size();
	}

	std::string toString() const {
		std::string str;
		for ( size_t i = 0; i < size(); ++i ) {
			str += ( *this )[i].toString();
			if ( i != size() - 1 )
				str += ";";
		}
		return str;
	}

	static TextRanges fromString( const std::string& str ) {
		auto rangesStr = String::split( str, ';' );
		TextRanges ranges;
		for ( const auto& rangeStr : rangesStr )
			ranges.emplace_back( TextRange::fromString( rangeStr ) );
		return ranges;
	}

  protected:
	bool mIsSorted{ false };
};

}}} // namespace EE::UI::Doc

#endif // EE_UI_DOC_TEXTRANGE_HPP
