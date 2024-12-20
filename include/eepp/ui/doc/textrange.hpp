#ifndef EE_UI_DOC_TEXTRANGE_HPP
#define EE_UI_DOC_TEXTRANGE_HPP

#include <algorithm>
#include <eepp/ui/doc/textposition.hpp>

namespace EE { namespace UI { namespace Doc {

class EE_API TextRange {
  public:
	TextRange();

	TextRange( const TextPosition& start, const TextPosition& end );

	bool isValid() const;

	void clear();

	TextPosition& start() { return mStart; }

	TextPosition& end() { return mEnd; }

	const TextPosition& start() const { return mStart; }

	const TextPosition& end() const { return mEnd; }

	TextRange normalized() const;

	TextRange& normalize();

	void reverse() { std::swap( mEnd, mStart ); }

	TextRange reversed() const { return TextRange( mEnd, mStart ); }

	void setStart( const TextPosition& position ) { mStart = position; }

	void setEnd( const TextPosition& position ) { mEnd = position; }

	void set( const TextPosition& start, const TextPosition& end );

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

	bool contains( const TextPosition& position ) const;

	bool intersectsLineRange( const TextRange& range ) const;

	template <typename T> bool intersectsLineRange( T fromLine, T toLine ) const {
		return mStart.line() <= static_cast<Int64>( toLine ) &&
			   static_cast<Int64>( fromLine ) <= mEnd.line();
	}

	template <typename T> bool intersectsLineRange( const std::pair<T, T>& range ) const {
		return mStart.line() <= static_cast<Int64>( range.second ) &&
			   static_cast<Int64>( range.first ) <= mEnd.line();
	}

	bool containsLine( const Int64& line ) const;

	bool contains( const TextRange& range ) const;

	bool intersects( const TextRange& range ) const;

	TextRange merge( const TextRange& range ) const;

	bool hasSelection() const { return isValid() && mStart != mEnd; }

	bool inSameLine() const { return isValid() && mStart.line() == mEnd.line(); }

	Int64 height() const;

	Int64 length() const;

	std::string toString() const;

	static TextRange fromString( const std::string& range );

	bool isNormalized() const { return mStart <= mEnd; }

  private:
	TextPosition mStart;
	TextPosition mEnd;

	TextPosition normalizedStart() const { return mStart < mEnd ? mStart : mEnd; }

	TextPosition normalizedEnd() const { return mStart < mEnd ? mEnd : mStart; }
};

class EE_API TextRanges : public std::vector<TextRange> {
  public:
	TextRanges();

	TextRanges( const std::vector<TextRange>& ranges );

	TextRanges( const TextRange& ranges );

	bool isSorted() const;

	bool isValid() const;

	bool exists( const TextRange& range ) const;

	size_t findIndex( const TextRange& range ) const;

	bool hasSelection() const;

	void sort();

	void setSorted();

	bool merge();

	std::string toString() const;

	static TextRanges fromString( const std::string& str );

  protected:
	bool mIsSorted{ false };
};

}}} // namespace EE::UI::Doc

#endif // EE_UI_DOC_TEXTRANGE_HPP
