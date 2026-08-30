#ifndef EE_UI_DOC_TEXTDOCUMENTLINE_HPP
#define EE_UI_DOC_TEXTDOCUMENTLINE_HPP

#include <atomic>
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
		updateTextHints();
	}

	TextDocumentLine( String&& text, std::shared_ptr<Mutex> docMutex ) :
		mText( std::move( text ) ), mDocMutex( std::move( docMutex ) ) {
		updateTextHints();
	}

	TextDocumentLine( const TextDocumentLine& other ) : mDocMutex( other.mDocMutex ) {
		ConditionalLock lock( mDocMutex != nullptr, mDocMutex.get() );
		mText = other.mText;
		mHash.store( other.mHash.load( std::memory_order_relaxed ), std::memory_order_relaxed );
		mFlags.store( other.mFlags.load( std::memory_order_acquire ), std::memory_order_relaxed );
	}

	TextDocumentLine( TextDocumentLine&& other ) noexcept : mDocMutex( other.mDocMutex ) {
		ConditionalLock lock( mDocMutex != nullptr, mDocMutex.get() );
		mText = std::move( other.mText );
		mHash.store( other.mHash.load( std::memory_order_relaxed ), std::memory_order_relaxed );
		mFlags.store( other.mFlags.load( std::memory_order_acquire ), std::memory_order_relaxed );
		other.mDocMutex.reset();
	}

	TextDocumentLine& operator=( const TextDocumentLine& other ) {
		if ( this == &other )
			return *this;

		String text;
		String::HashType hash;
		Uint32 flags;
		auto docMutex = other.mDocMutex;
		{
			ConditionalLock lock( docMutex != nullptr, docMutex.get() );
			text = other.mText;
			hash = other.mHash.load( std::memory_order_relaxed );
			flags = other.mFlags.load( std::memory_order_acquire );
		}

		auto oldDocMutex = mDocMutex;
		{
			ConditionalLock lock( oldDocMutex != nullptr, oldDocMutex.get() );
			mText = std::move( text );
			mHash.store( hash, std::memory_order_relaxed );
			mFlags.store( flags, std::memory_order_release );
			mDocMutex = std::move( docMutex );
		}
		return *this;
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
			invalidateHash();
			mText = std::move( text );
			updateTextHints();
		} else {
			invalidateHash();
			mText = std::move( text );
			updateTextHints();
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
			invalidateHash();
			mText.append( text );
			updateTextHints();
		} else {
			invalidateHash();
			mText.append( text );
			updateTextHints();
		}
	}

	void insert( std::size_t position, const String& text ) {
		if ( mDocMutex ) {
			Lock lock( *mDocMutex );
			invalidateHash();
			mText.insert( position, text );
			updateTextHints();
		} else {
			invalidateHash();
			mText.insert( position, text );
			updateTextHints();
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

	String::HashType getHash() const {
		Uint32 flags = mFlags.load( std::memory_order_acquire );
		if ( flags & HashValid )
			return mHash.load( std::memory_order_relaxed );

		ConditionalLock lock( mDocMutex != nullptr, mDocMutex.get() );
		flags = mFlags.load( std::memory_order_acquire );
		if ( !( flags & HashValid ) ) {
			mHash.store( mText.getHash(), std::memory_order_relaxed );
			mFlags.store( flags | HashValid, std::memory_order_release );
		}
		return mHash.load( std::memory_order_relaxed );
	}

	bool isAscii() const {
		return ( mFlags.load( std::memory_order_acquire ) & TextHints::AllAscii ) != 0;
	}

	bool isLatin1() const {
		return ( mFlags.load( std::memory_order_acquire ) & TextHints::AllLatin1 ) != 0;
	}

	Uint32 getTextHints() const { return mFlags.load( std::memory_order_acquire ) & ~HashValid; }

  protected:
	static constexpr Uint32 HashValid = 1u << 31;
	String mText;
	mutable std::atomic<String::HashType> mHash{ 0 };
	mutable std::atomic<Uint32> mFlags{ 0 };
	std::shared_ptr<Mutex> mDocMutex;

	void invalidateHash() { mFlags.fetch_and( ~HashValid, std::memory_order_release ); }

	void updateTextHints() { mFlags.store( mText.getTextHints(), std::memory_order_release ); }
};

}}} // namespace EE::UI::Doc

#endif // EE_UI_DOC_TEXTDOCUMENTLINE_HPP
