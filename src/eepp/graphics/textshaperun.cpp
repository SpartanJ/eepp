#include <eepp/graphics/fonttruetype.hpp>
#include <eepp/graphics/text.hpp>
#include <eepp/graphics/textshaperun.hpp>

#ifdef EE_TEXT_SHAPER_ENABLED
#include <harfbuzz/hb-ft.h>
#include <harfbuzz/hb.h>
#endif

namespace EE::Graphics {

TextShapeRun::TextShapeRun( String::View str, FontTrueType* font, Uint32 characterSize,
							Uint32 style, Float outlineThickness ) :
	mString( str ),
	mFont( font ),
	mCharacterSize( characterSize ),
	mStyle( style ),
	mOutlineThickness( outlineThickness ),
	mCurFont( mFont ) {
	findNextEnd();
}

String::View TextShapeRun::curRun() const {
	return mString.substr( mIndex, mIsNewLine ? mLen - 1 : mLen );
}

bool TextShapeRun::hasNext() const {
	return mIndex < mString.size();
}

std::size_t TextShapeRun::pos() const {
	return mIndex;
}

void TextShapeRun::next() {
	mIndex += mLen;
	findNextEnd();
}

bool TextShapeRun::runIsNewLine() const {
	return mIsNewLine;
}

FontTrueType* TextShapeRun::font() {
	return static_cast<FontTrueType*>( mCurFont );
}

void TextShapeRun::findNextEnd() {
#ifdef EE_TEXT_SHAPER_ENABLED
	Font* lFont = mStartFont ? mStartFont : mFont;
	std::size_t len = mString.size();
	std::size_t pos = 0;
	hb_script_t curScript = HB_SCRIPT_UNKNOWN;

	for ( std::size_t idx = mIndex; idx < len; ++idx, ++pos ) {
		auto ch = mString[idx];
		hb_script_t script = hb_unicode_script( hb_unicode_funcs_get_default(), ch );
		auto font = mFont
						->getGlyph( ch, mCharacterSize, mStyle & Text::Bold, mStyle & Text::Italic,
									mOutlineThickness )
						.font;
		mIsNewLine = ( ch == '\n' );

		if ( idx == mIndex ) {
			curScript = script;
			mStartFont = font;
			lFont = font;
			mCurFont = font;
			if ( curScript == HB_SCRIPT_COMMON || curScript == HB_SCRIPT_INHERITED )
				curScript = HB_SCRIPT_LATIN;
			mIsRTL = hb_script_get_horizontal_direction( curScript ) == HB_DIRECTION_RTL;
		}

		// Break run if:
		// - Newline
		// - Font changed
		// - Script changed
		hb_script_t effectiveScript =
			( script == HB_SCRIPT_COMMON || script == HB_SCRIPT_INHERITED ) ? (hb_script_t)curScript
																			: script;

		if ( mIsNewLine || ( lFont != nullptr && font != lFont ) || effectiveScript != curScript ) {
			mLen = mIsNewLine ? pos + 1 : pos;
			mCurFont = lFont;
			mIsRTL = hb_script_get_horizontal_direction( effectiveScript ) == HB_DIRECTION_RTL;
			return;
		}

		lFont = font;
		mCurFont = font;
		curScript = effectiveScript;
	}

	mLen = len - mIndex;
#else
	Font* lFont = mStartFont;
	std::size_t len = mString.size();
	std::size_t idx;
	std::size_t pos = 0;
	for ( idx = mIndex; idx < len; idx++, pos++ ) {
		Font* font = mFont
						 ->getGlyph( mString[idx], mCharacterSize, mStyle & Text::Bold,
									 mStyle & Text::Italic, mOutlineThickness )
						 .font;
		mIsNewLine = mString[idx] == '\n';
		if ( mIsNewLine || ( lFont != nullptr && font != lFont ) ) {
			mCurFont = lFont;
			mStartFont = font;
			mLen = mIsNewLine ? pos + 1 : pos;
			return;
		}
		lFont = font;
		mCurFont = font;
	}
	mLen = idx;
#endif
}

bool TextShapeRun::isRTL() const {
	return mIsRTL;
}

} // namespace EE::Graphics
