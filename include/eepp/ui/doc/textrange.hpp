#ifndef EE_UI_DOC_TEXTRANGE_HPP
#define EE_UI_DOC_TEXTRANGE_HPP

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

	void setStart( const TextPosition& position ) { mStart = position; }

	void setEnd( const TextPosition& position ) { mEnd = position; }

	void set( const TextPosition& start, const TextPosition& end ) {
		mStart = start;
		mEnd = end;
	}

	bool operator==( const TextRange& other ) const {
		return mStart == other.mStart && mEnd == other.mEnd;
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

  private:
	TextPosition mStart;
	TextPosition mEnd;

	TextPosition normalizedStart() const { return mStart < mEnd ? mStart : mEnd; }

	TextPosition normalizedEnd() const { return mStart < mEnd ? mEnd : mStart; }
};

}}} // namespace EE::UI::Doc

#endif // EE_UI_DOC_TEXTRANGE_HPP
