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
	Cache();
	UpdateCoords();
	Color( FontColor );
	ShadowColor( FontShadowColor );
}

TextCache::~TextCache() {
}

void TextCache::Create( Graphics::Font * font, const String& text, ColorA FontColor, ColorA FontShadowColor ) {
	mFont = font;
	mText = text;
	UpdateCoords();
	Color( FontColor );
	ShadowColor( FontShadowColor );
	Cache();
}

Graphics::Font * TextCache::Font() const {
	return mFont;
}

void TextCache::Font( Graphics::Font * font ) {
	mFont = font;
	Cache();
}

String& TextCache::Text() {
	return mText;
}

void TextCache::UpdateCoords() {
	Uint32 size = (Uint32)mText.size() * GLi->QuadVertexs();
	
	mRenderCoords.resize( size );
	mColors.resize( size, mFontColor );
}

void TextCache::Text( const String& text ) {
	bool needUpdate = false;

	if ( mText.size() != text.size() )
		needUpdate = true;

	mText = text;

	if ( needUpdate )
		UpdateCoords();

	Cache();
}

const ColorA& TextCache::Color() const {
	return mFontColor;
}

void TextCache::Alpha( const Uint8& alpha ) {
	std::size_t s = mColors.size();
	for ( Uint32 i = 0; i < s; i++ ) {
		mColors[ i ].Alpha = alpha;
	}
}

void TextCache::Color( const ColorA& color ) {
	if ( mFontColor != color ) {
		mFontColor = color;

		mColors.assign( mText.size() * GLi->QuadVertexs(), mFontColor );
	}
}

void TextCache::Color( const ColorA& color, Uint32 from, Uint32 to ) {
	std::vector<ColorA> colors( GLi->QuadVertexs(), color );
	std::size_t s = mText.size();

	if ( to >= s ) {
		to = s - 1;
	}

	if ( from <= to && from < s && to <= s ) {
		size_t rto	= to + 1;
		Int32 rpos	= from;
		Int32 lpos	= 0;
		Uint32 i;
		Uint32 qsize = sizeof(ColorA) * GLi->QuadVertexs();
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

			memcpy( &(mColors[ lpos * GLi->QuadVertexs() ]), &colors[0], qsize );
		}
	}
}

const ColorA& TextCache::ShadowColor() const {
	return mFontShadowColor;
}

void TextCache::ShadowColor(const ColorA& color) {
	mFontShadowColor = color;
}

std::vector<eeVertexCoords>& TextCache::VertextCoords() {
	return mRenderCoords;
}

std::vector<ColorA>& TextCache::Colors() {
	return mColors;
}

void TextCache::Cache() {
	if ( NULL != mFont && mText.size() ) {
		mFont->CacheWidth( mText, mLinesWidth, mCachedWidth, mNumLines, mLargestLineCharCount );
	}else {
		mCachedWidth = 0;
	}

	mCachedCoords = false;
}

Float TextCache::GetTextWidth() {
	return ( mFlags & FONT_DRAW_VERTICAL ) ? (Float)mFont->GetFontHeight() * (Float)mNumLines : mCachedWidth;
}

Float TextCache::GetTextHeight() {
	return ( mFlags & FONT_DRAW_VERTICAL ) ? mLargestLineCharCount * (Float)mFont->GetFontHeight() : (Float)mFont->GetFontHeight() * (Float)mNumLines;
}

const int& TextCache::GetNumLines() const {
	return mNumLines;
}

const std::vector<Float>& TextCache::LinesWidth() {
	return mLinesWidth;
}

void TextCache::Draw( const Float& X, const Float& Y, const Vector2f& Scale, const Float& Angle, EE_BLEND_MODE Effect ) {
	if ( NULL != mFont ) {
		GlobalBatchRenderer::instance()->Draw();

		if ( Angle != 0.0f || Scale != 1.0f ) {
			mFont->Draw( *this, X, Y, mFlags, Scale, Angle, Effect );
		} else {
			GLi->Translatef( X, Y, 0.f );
	
			mFont->Draw( *this, 0, 0, mFlags, Scale, Angle, Effect );
	
			GLi->Translatef( -X, -Y, 0.f );
		}
	}
}

const bool& TextCache::CachedCoords() const {
	return mCachedCoords;
}

void TextCache::CachedCoords( const bool& cached ) {
	mCachedCoords = cached;
}

const unsigned int& TextCache::CachedVerts() const {
	return mVertexNumCached;
}

void TextCache::CachedVerts( const unsigned int& num ) {
	mVertexNumCached = num;
}

void TextCache::Flags( const Uint32& flags ) {
	if ( mFlags != flags ) {
		mFlags = flags;
		mCachedCoords = false;

		if ( ( mFlags & FONT_DRAW_VERTICAL ) != ( flags & FONT_DRAW_VERTICAL ) ) {
			Cache();
		}
	}
}

const Uint32& TextCache::Flags() const {
	return mFlags;
}

}}
