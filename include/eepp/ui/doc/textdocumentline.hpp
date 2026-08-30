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

	TextDocumentLine( String&& text, std::shared_ptr<Mutex> docMutex ) :
		mText( std::move( text ) ), mDocMutex( std::move( docMutex ) ) {
		updateState();
	}

	TextDocumentLine( const TextDocumentLine& ) = default;

	TextDocumentLine( TextDocumentLine&& other ) noexcept : mDocMutex( other.mDocMutex ) {
		ConditionalLock lock( mDocMutex != nullptr, mDocMutex.get() );
		mText = std::move( other.mText );
		mHash = other.mHash;
		mFlags = other.mFlags;
		other.mDocMutex.reset();
	}

	TextDocumentLine& operator=( const TextDocumentLine& ) = default;

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

	String::View getTextViewWithoutNewLine() const {
		if ( mDocMutex ) {
			Lock lock( *mDocMutex );
			return mText.view().substr( 0, mText.size() - 1 );
		}
		return mText.view().substr( 0, mText.size() - 1 );
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

	void insert( std::size_t position, const String& text ) {
		if ( mDocMutex ) {
			Lock lock( *mDocMutex );
			mText.insert( position, text );
			updateState();
		} else {
			mText.insert( position, text );
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

	bool isLatin1() const { return ( mFlags & TextHints::AllLatin1 ) != 0; }

	Uint32 getTextHints() const {
		if ( mDocMutex ) {
			Lock lock( *mDocMutex );
			return mFlags;
		}
		return mFlags;
	}

  protected:
	String mText;
	String::HashType mHash{ 0 };
	Uint32 mFlags{ 0 };
	std::shared_ptr<Mutex> mDocMutex;

	void updateState() {
		mHash = mText.getHash();
		mFlags = mText.getTextHints();
	}
};

}}} // namespace EE::UI::Doc

#endif // EE_UI_DOC_TEXTDOCUMENTLINE_HPP
