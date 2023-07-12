#ifndef EE_UI_DOC_TEXTPOSITION_HPP
#define EE_UI_DOC_TEXTPOSITION_HPP

#include <eepp/config.hpp>
#include <eepp/core/string.hpp>
#include <string>

namespace EE { namespace UI { namespace Doc {

class EE_API TextPosition {
  public:
	TextPosition() {}

	TextPosition( Int64 line, Int64 column ) : mLine( line ), mColumn( column ) {}

	bool isValid() const { return mLine != 0xffffffff && mColumn != 0xffffffff; }

	Int64 line() const { return mLine; }

	Int64 column() const { return mColumn; }

	void setLine( Int64 line ) { mLine = line; }

	void setColumn( Int64 column ) { mColumn = column; }

	bool operator==( const TextPosition& other ) const {
		return mLine == other.mLine && mColumn == other.mColumn;
	}

	bool operator!=( const TextPosition& other ) const {
		return mLine != other.mLine || mColumn != other.mColumn;
	}

	bool operator<( const TextPosition& other ) const {
		return mLine < other.mLine || ( mLine == other.mLine && mColumn < other.mColumn );
	}

	bool operator>( const TextPosition& other ) const {
		return mLine > other.mLine || ( mLine == other.mLine && mColumn > other.mColumn );
	}

	bool operator<=( const TextPosition& other ) const {
		return mLine < other.mLine || ( mLine == other.mLine && mColumn <= other.mColumn );
	}

	bool operator>=( const TextPosition& other ) const {
		return mLine > other.mLine || ( mLine == other.mLine && mColumn >= other.mColumn );
	}

	TextPosition operator+( const TextPosition& other ) const {
		return { mLine + other.line(), mColumn + other.column() };
	}

	TextPosition operator+=( const TextPosition& other ) const {
		return { mLine + other.line(), mColumn + other.column() };
	}

	TextPosition operator-( const TextPosition& other ) const {
		return { mLine - other.line(), mColumn - other.column() };
	}

	TextPosition operator-=( const TextPosition& other ) const {
		return { mLine - other.line(), mColumn - other.column() };
	}

	std::string toPositionString() const { return String::format( ":%lld:%lld", mLine, mColumn ); }

	std::string toString() const { return String::format( "L%lld,C%lld", mLine, mColumn ); }

	static TextPosition fromString( const std::string& pos ) {
		auto split = String::split( pos, ',' );
		if ( split.size() == 2 && !split[0].empty() && !split[1].empty() ) {
			if ( split[0][0] == 'L' || split[0][0] == 'l' )
				split[0] = split[0].substr( 1 );
			if ( split[1][0] == 'C' || split[0][0] == 'c' )
				split[1] = split[1].substr( 1 );
			Int64 l, c;
			if ( String::fromString( l, split[0] ) && String::fromString( c, split[1] ) )
				return TextPosition( l, c );
		}
		return {};
	}

  private:
	Int64 mLine{ 0xffffffff };
	Int64 mColumn{ 0xffffffff };
};

}}} // namespace EE::UI::Doc

#endif // EE_UI_DOC_TEXTPOSITION_HPP
