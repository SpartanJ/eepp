#include <algorithm>
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
	if ( mIsRTL ) {
		mIndex = mString.size();
#ifdef EE_TEXT_SHAPER_ENABLED
		/* Walking backwards cannot tell where a cluster starts (regional indicators pair from
		 * the front), so segment once forwards. */
		for ( std::size_t start = 0; start < mString.size(); start = clusterEnd( start ) )
			mClusterStarts.push_back( start );
#endif
	}
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

#ifdef EE_TEXT_SHAPER_ENABLED
/* @p start must be a cluster start. Boundaries are tested within the cluster only, so a long run
 * of regional indicators is not rescanned for each pair. No rule joins two codepoints below
 * U+0300 except CR LF, which stays split at the newline. */
std::size_t TextShapeRun::clusterEnd( std::size_t start ) const {
	const String::View rest = mString.substr( start );
	std::size_t end = 1;
	while ( end < rest.size() && ( rest[end - 1] >= 0x300 || rest[end] >= 0x300 ) &&
			!String::isGraphemeBoundary( rest, end ) )
		++end;
	return start + end;
}

std::size_t TextShapeRun::clusterStart( std::size_t last ) const {
	auto next = std::upper_bound( mClusterStarts.begin(), mClusterStarts.end(), last );
	return next == mClusterStarts.begin() ? 0 : *( next - 1 );
}

/* One font shapes a whole cluster, so its joiners and selectors stay in its run. */
Font* TextShapeRun::clusterFont( std::size_t start, std::size_t end ) const {
	const bool bold = mStyle & Text::Bold;
	const bool italic = mStyle & Text::Italic;
	if ( end - start == 1 )
		return mFont->getGlyph( mString[start], mCharacterSize, bold, italic, mOutlineThickness )
			.font;
	return static_cast<FontTrueType*>( mFont )->getClusterFont(
		mString.substr( start, end - start ), mCharacterSize, bold, italic, mOutlineThickness );
}
#endif

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

		for ( std::size_t idx = mIndex; idx < len; ) {
			const std::size_t end = clusterEnd( idx );
			auto ch = mString[idx];
			auto font = clusterFont( idx, end );
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
			pos += end - idx;
			idx = end;
		}

		mLen = len - mIndex;
	} else {
		Font* lFont = mStartFont ? mStartFont : mFont;
		std::size_t pos = 0;

		while ( pos < mIndex ) {
			std::size_t idx = mIndex - 1 - pos;
			const std::size_t start = clusterStart( idx );
			auto ch = mString[idx];
			auto font = clusterFont( start, idx + 1 );
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
			pos += idx + 1 - start;
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
