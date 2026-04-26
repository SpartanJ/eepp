#include <eepp/graphics/fonttruetype.hpp>
#include <eepp/graphics/text.hpp>
#include <eepp/graphics/textshaperun.hpp>

namespace EE::Graphics {

TextShapeRun::TextShapeRun( String::View str, FontTrueType* font, Uint32 characterSize,
							Uint32 style, Float outlineThickness, bool isRTL ) :
	mString( str ),
	mFont( font ),
	mCharacterSize( characterSize ),
	mStyle( style ),
	mOutlineThickness( outlineThickness ),
	mCurFont( mFont ),
	mIsRTL( isRTL ) {
	if ( mIsRTL )
		mIndex = mString.size();
	findNextEnd();
}

String::View TextShapeRun::curRun() const {
	if ( mIsRTL )
		return mString.substr( mIndex - mLen, mIsNewLine ? mLen - 1 : mLen );
	return mString.substr( mIndex, mIsNewLine ? mLen - 1 : mLen );
}

bool TextShapeRun::hasNext() const {
	if ( mIsRTL )
		return mIndex > 0;
	return mIndex < mString.size();
}

std::size_t TextShapeRun::pos() const {
	if ( mIsRTL )
		return mIndex - mLen;
	return mIndex;
}

std::size_t TextShapeRun::length() const {
	return mLen;
}

void TextShapeRun::next() {
	if ( mIsRTL ) {
		mIndex -= mLen;
	} else {
		mIndex += mLen;
	}
	findNextEnd();
}

bool TextShapeRun::runIsNewLine() const {
	return mIsNewLine;
}

FontTrueType* TextShapeRun::font() {
	return static_cast<FontTrueType*>( mCurFont );
}

void TextShapeRun::findNextEnd() {
	if ( !hasNext() ) {
		mLen = 0;
		return;
	}

#ifdef EE_TEXT_SHAPER_ENABLED
	if ( !mIsRTL ) {
		Font* lFont = mStartFont ? mStartFont : mFont;
		std::size_t len = mString.size();
		std::size_t pos = 0;

		for ( std::size_t idx = mIndex; idx < len; ++idx, ++pos ) {
			auto ch = mString[idx];
			auto font = mFont
							->getGlyph( ch, mCharacterSize, mStyle & Text::Bold,
										mStyle & Text::Italic, mOutlineThickness )
							.font;
			mIsNewLine = ( ch == '\n' );

			if ( idx == mIndex ) {
				mStartFont = font;
				lFont = font;
				mCurFont = font;
			}

			// Break run if:
			// - Newline
			// - Font changed
			if ( mIsNewLine || ( lFont != nullptr && font != lFont ) ) {
				mLen = mIsNewLine ? pos + 1 : pos;
				mCurFont = lFont;
				return;
			}

			lFont = font;
			mCurFont = font;
		}

		mLen = len - mIndex;
	} else {
		Font* lFont = mStartFont ? mStartFont : mFont;
		std::size_t pos = 0;

		for ( ; pos < mIndex; ++pos ) {
			std::size_t idx = mIndex - 1 - pos;
			auto ch = mString[idx];
			auto font = mFont
							->getGlyph( ch, mCharacterSize, mStyle & Text::Bold,
										mStyle & Text::Italic, mOutlineThickness )
							.font;
			mIsNewLine = ( ch == '\n' );

			if ( pos == 0 ) {
				mStartFont = font;
				lFont = font;
				mCurFont = font;
			}

			if ( mIsNewLine || ( lFont != nullptr && font != lFont ) ) {
				mLen = mIsNewLine ? pos + 1 : pos;
				mCurFont = lFont;
				return;
			}

			lFont = font;
			mCurFont = font;
		}

		mLen = mIndex;
	}
#else
	if ( !mIsRTL ) {
		Font* lFont = mStartFont;
		std::size_t len = mString.size();
		std::size_t pos = 0;
		for ( std::size_t idx = mIndex; idx < len; idx++, pos++ ) {
			Font* font = mFont
							 ->getGlyph( mString[idx], mCharacterSize, mStyle & Text::Bold,
										 mStyle & Text::Italic, mOutlineThickness )
							 .font;
			mIsNewLine = mString[idx] == '\n';
			if ( pos == 0 )
				lFont = font;
			if ( mIsNewLine || ( lFont != nullptr && font != lFont ) ) {
				mCurFont = lFont;
				mStartFont = font;
				mLen = mIsNewLine ? pos + 1 : pos;
				return;
			}
			lFont = font;
			mCurFont = font;
		}
		mLen = pos;
	} else {
		Font* lFont = mStartFont;
		std::size_t pos = 0;
		for ( ; pos < mIndex; ++pos ) {
			std::size_t idx = mIndex - 1 - pos;
			Font* font = mFont
							 ->getGlyph( mString[idx], mCharacterSize, mStyle & Text::Bold,
										 mStyle & Text::Italic, mOutlineThickness )
							 .font;
			mIsNewLine = mString[idx] == '\n';
			if ( pos == 0 )
				lFont = font;
			if ( mIsNewLine || ( lFont != nullptr && font != lFont ) ) {
				mCurFont = lFont;
				mStartFont = font;
				mLen = mIsNewLine ? pos + 1 : pos;
				return;
			}
			lFont = font;
			mCurFont = font;
		}
		mLen = mIndex;
	}
#endif
}

} // namespace EE::Graphics
