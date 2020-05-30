#ifndef EE_UI_DOC_TEXTDOCUMENTLINE_HPP
#define EE_UI_DOC_TEXTDOCUMENTLINE_HPP

#include <eepp/core/string.hpp>

namespace EE { namespace UI { namespace Doc {

class EE_API TextDocumentLine {
  public:
	TextDocumentLine( const String& text ) : mText( text ) { updateHash(); }

	void setText( const String& text ) {
		mText = text;
		updateHash();
	}

	const String& getText() const { return mText; }

	void operator=( const std::string& right ) { setText( right ); }

	String::StringBaseType operator[]( std::size_t index ) const { return mText[index]; }

	void insertChar( const unsigned int& pos, const String::StringBaseType& tchar ) {
		mText.insert( mText.begin() + pos, tchar );
		updateHash();
	}

	void append( const String& text ) {
		mText.append( text );
		updateHash();
	}

	void append( const String::StringBaseType& code ) {
		mText.append( code );
		updateHash();
	}

	String substr( std::size_t pos = 0, std::size_t n = String::StringType::npos ) const {
		return mText.substr( pos, n );
	}

	String::Iterator insert( String::Iterator p, const String::StringBaseType& c ) {
		auto it = mText.insert( p, c );
		updateHash();
		return it;
	}

	bool empty() const { return mText.empty(); }

	size_t size() const { return mText.size(); }

	size_t length() const { return mText.length(); }

	const String::HashType& getHash() const { return mHash; }

	std::string toUtf8() const { return mText.toUtf8(); }

  protected:
	String mText;
	String::HashType mHash;

	void updateHash() { mHash = mText.getHash(); }
};

}}}

#endif // EE_UI_DOC_TEXTDOCUMENTLINE_HPP
