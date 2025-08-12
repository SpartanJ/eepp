#ifndef EE_UI_DOC_TEXTDOCUMENTLINE_HPP
#define EE_UI_DOC_TEXTDOCUMENTLINE_HPP

#include <eepp/core/string.hpp>
#include <eepp/system/lock.hpp>
#include <eepp/system/mutex.hpp>
#include <memory>

using namespace EE::System;

namespace EE { namespace UI { namespace Doc {

class EE_API TextDocumentLine {
  public:
	TextDocumentLine( const String& text, std::shared_ptr<Mutex> docMutex ) :
		mText( text ), mDocMutex( docMutex ) {
		updateState();
	}

	~TextDocumentLine() {
		if ( mDocMutex ) {
			// Wait for any readers to finish before destruction
			Lock lock( *mDocMutex );
		}
	}

	void setText( String&& text ) {
		if ( mDocMutex ) {
			Lock lock( *mDocMutex );
			mText = std::move( text );
			updateState();
		} else {
			mText = std::move( text );
			updateState();
		}
	}

	const String& getText() const {
		if ( mDocMutex ) {
			Lock lock( *mDocMutex );
			return mText;
		}
		return mText;
	}

	String getTextWithoutNewLine() const {
		if ( mDocMutex ) {
			Lock lock( *mDocMutex );
			return mText.substr( 0, mText.size() - 1 );
		}
		return mText.substr( 0, mText.size() - 1 );
	}

	String::StringBaseType operator[]( std::size_t index ) const {
		if ( mDocMutex ) {
			Lock lock( *mDocMutex );
			return mText[index];
		}
		return mText[index];
	}

	void append( const String& text ) {
		if ( mDocMutex ) {
			Lock lock( *mDocMutex );
			mText.append( text );
			updateState();
		} else {
			mText.append( text );
			updateState();
		}
	}

	String substr( std::size_t pos = 0, std::size_t n = String::StringType::npos ) const {
		if ( mDocMutex ) {
			Lock lock( *mDocMutex );
			return mText.substr( pos, n );
		}
		return mText.substr( pos, n );
	}

	bool empty() const {
		if ( mDocMutex ) {
			Lock lock( *mDocMutex );
			return mText.empty();
		}
		return mText.empty();
	}

	size_t size() const {
		if ( mDocMutex ) {
			Lock lock( *mDocMutex );
			return mText.size();
		}
		return mText.size();
	}

	String::HashType getHash() const { return mHash; }

	bool isAscii() const { return ( mFlags & TextHints::AllAscii ) != 0; }

	Uint32 getTextHints() const {
		if ( mDocMutex ) {
			Lock lock( *mDocMutex );
			return mFlags;
		}
		return mFlags;
	}

  protected:
	String mText;
	String::HashType mHash;
	Uint32 mFlags{ 0 };
	std::shared_ptr<Mutex> mDocMutex;

	void updateState() {
		mHash = mText.getHash();
		mFlags = mText.isAscii() ? TextHints::AllAscii : 0;
	}
};

}}} // namespace EE::UI::Doc

#endif // EE_UI_DOC_TEXTDOCUMENTLINE_HPP
