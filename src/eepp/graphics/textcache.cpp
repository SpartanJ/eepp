#include <eepp/graphics/textcache.hpp>
#include <eepp/graphics/font.hpp>
#include <eepp/graphics/globalbatchrenderer.hpp>
#include <eepp/graphics/renderer/gl.hpp>

namespace EE { namespace Graphics {

TextCache::TextCache() :
	mFont(NULL),
	mCachedWidth(0.f),
	mNumLines(1),
	mLargestLineCharCount(0),
	mFontColor(255,255,255,255),
	mFontShadowColor(0,0,0,255),
	mFlags(0),
	mVertexNumCached(0),
	mCachedCoords(false)
{
}

TextCache::TextCache( Graphics::Font * font, const String& text, ColorA FontColor, ColorA FontShadowColor ) :
	mText( text ),
	mFont( font ),
	mCachedWidth(0.f),
	mNumLines(1),
	mLargestLineCharCount(0),
	mFlags(0),
	mVertexNumCached(0),
	mCachedCoords(false)
{
	cacheWidth();
	updateCoords();
	setColor( FontColor );
	setShadowColor( FontShadowColor );
}

TextCache::~TextCache() {
}

void TextCache::create( Graphics::Font * font, const String& text, ColorA FontColor, ColorA FontShadowColor ) {
	mFont = font;
	mText = text;
	updateCoords();
	setColor( FontColor );
	setShadowColor( FontShadowColor );
	cacheWidth();
}

Graphics::Font * TextCache::getFont() const {
	return mFont;
}

void TextCache::setFont( Graphics::Font * font ) {
	mFont = font;
	cacheWidth();
}

String& TextCache::getText() {
	return mText;
}

void TextCache::updateCoords() {
	Uint32 size = (Uint32)mText.size() * GLi->quadVertexs();
	
	mRenderCoords.resize( size );
	mColors.resize( size, mFontColor );
}

void TextCache::setText( const String& text ) {
	bool needUpdate = false;

	if ( mText.size() != text.size() )
		needUpdate = true;

	mText = text;

	if ( needUpdate )
		updateCoords();

	cacheWidth();
}

const ColorA& TextCache::getColor() const {
	return mFontColor;
}

void TextCache::setAlpha( const Uint8& alpha ) {
	std::size_t s = mColors.size();
	for ( Uint32 i = 0; i < s; i++ ) {
		mColors[ i ].Alpha = alpha;
	}
}

void TextCache::setColor( const ColorA& color ) {
	if ( mFontColor != color ) {
		mFontColor = color;

		mColors.assign( mText.size() * GLi->quadVertexs(), mFontColor );
	}
}

void TextCache::setColor( const ColorA& color, Uint32 from, Uint32 to ) {
	std::vector<ColorA> colors( GLi->quadVertexs(), color );
	std::size_t s = mText.size();

	if ( to >= s ) {
		to = s - 1;
	}

	if ( from <= to && from < s && to <= s ) {
		size_t rto	= to + 1;
		Int32 rpos	= from;
		Int32 lpos	= 0;
		Uint32 i;
		Uint32 qsize = sizeof(ColorA) * GLi->quadVertexs();
		String::StringBaseType curChar;

		// New lines and tabs are not rendered, and not counted as a color
		// We need to skip those characters as nonexistent chars
		for ( i = 0; i < from; i++ ) {
			curChar = mText.at(i);
			if ( '\n' == curChar || '\t' == curChar || '\v' == curChar ) {
				if ( rpos > 0 ) {
					rpos--;
				}
			}
		}

		for ( Uint32 i = from; i < rto; i++ ) {
			curChar = mText.at(i);

			lpos	= rpos;
			rpos++;

			// Same here
			if ( '\n' == curChar || '\t' == curChar || '\v' == curChar ) {
				if ( rpos > 0 ) {
					rpos--;
				}
			}

			memcpy( &(mColors[ lpos * GLi->quadVertexs() ]), &colors[0], qsize );
		}
	}
}

const ColorA& TextCache::getShadowColor() const {
	return mFontShadowColor;
}

void TextCache::setShadowColor(const ColorA& color) {
	mFontShadowColor = color;
}

std::vector<eeVertexCoords>& TextCache::getVertextCoords() {
	return mRenderCoords;
}

std::vector<ColorA>& TextCache::getColors() {
	return mColors;
}

void TextCache::cacheWidth() {
	if ( NULL != mFont && mText.size() ) {
		mFont->cacheWidth( mText, mLinesWidth, mCachedWidth, mNumLines, mLargestLineCharCount );
	}else {
		mCachedWidth = 0;
	}

	mCachedCoords = false;
}

Float TextCache::getTextWidth() {
	return ( mFlags & FONT_DRAW_VERTICAL ) ? (Float)mFont->getFontHeight() * (Float)mNumLines : mCachedWidth;
}

Float TextCache::getTextHeight() {
	return ( mFlags & FONT_DRAW_VERTICAL ) ? mLargestLineCharCount * (Float)mFont->getFontHeight() : (Float)mFont->getFontHeight() * (Float)mNumLines;
}

const int& TextCache::getNumLines() const {
	return mNumLines;
}

const std::vector<Float>& TextCache::getLinesWidth() {
	return mLinesWidth;
}

void TextCache::draw( const Float& X, const Float& Y, const Vector2f& Scale, const Float& Angle, EE_BLEND_MODE Effect ) {
	if ( NULL != mFont ) {
		GlobalBatchRenderer::instance()->draw();

		if ( Angle != 0.0f || Scale != 1.0f ) {
			mFont->draw( *this, X, Y, mFlags, Scale, Angle, Effect );
		} else {
			GLi->translatef( X, Y, 0.f );
	
			mFont->draw( *this, 0, 0, mFlags, Scale, Angle, Effect );
	
			GLi->translatef( -X, -Y, 0.f );
		}
	}
}

const bool& TextCache::cachedCoords() const {
	return mCachedCoords;
}

void TextCache::cachedCoords( const bool& cached ) {
	mCachedCoords = cached;
}

const unsigned int& TextCache::cachedVerts() const {
	return mVertexNumCached;
}

void TextCache::cachedVerts( const unsigned int& num ) {
	mVertexNumCached = num;
}

void TextCache::setFlags( const Uint32& flags ) {
	if ( mFlags != flags ) {
		mFlags = flags;
		mCachedCoords = false;

		if ( ( mFlags & FONT_DRAW_VERTICAL ) != ( flags & FONT_DRAW_VERTICAL ) ) {
			cacheWidth();
		}
	}
}

const Uint32& TextCache::getFlags() const {
	return mFlags;
}

}}
