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

	bool isValid() const { return mLine != 0xffffffffu && mColumn != 0xffffffffu; }

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

	TextPosition operator+( const TextPosition& other ) const {
		return {mLine + other.line(), mColumn + other.column()};
	}

	TextPosition operator+=( const TextPosition& other ) const {
		return {mLine + other.line(), mColumn + other.column()};
	}

	TextPosition operator-( const TextPosition& other ) const {
		return {mLine - other.line(), mColumn - other.column()};
	}

	TextPosition operator-=( const TextPosition& other ) const {
		return {mLine - other.line(), mColumn - other.column()};
	}

	std::string toString() { return String::format( "L%lld,C%lld", mLine, mColumn ); }

  private:
	Int64 mLine{0xffffffff};
	Int64 mColumn{0xffffffff};
};

}}} // namespace EE::UI::Doc

#endif // EE_UI_DOC_TEXTPOSITION_HPP
