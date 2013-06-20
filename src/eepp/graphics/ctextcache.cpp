#include <eepp/graphics/ctextcache.hpp>
#include <eepp/graphics/cfont.hpp>
#include <eepp/graphics/cglobalbatchrenderer.hpp>
#include <eepp/graphics/renderer/cgl.hpp>

namespace EE { namespace Graphics {

cTextCache::cTextCache() :
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

cTextCache::cTextCache( cFont * font, const String& text, eeColorA FontColor, eeColorA FontShadowColor ) :
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

cTextCache::~cTextCache() {
}

void cTextCache::Create( cFont * font, const String& text, eeColorA FontColor, eeColorA FontShadowColor ) {
	mFont = font;
	mText = text;
	UpdateCoords();
	Color( FontColor );
	ShadowColor( FontShadowColor );
	Cache();
}

cFont * cTextCache::Font() const {
	return mFont;
}

void cTextCache::Font( cFont * font ) {
	mFont = font;
	Cache();
}

String& cTextCache::Text() {
	return mText;
}

void cTextCache::UpdateCoords() {
	Uint32 size = (Uint32)mText.size() * EE_QUAD_VERTEX;
	
	mRenderCoords.resize( size );
	mColors.resize( size, mFontColor );
}

void cTextCache::Text( const String& text ) {
	bool needUpdate = false;

	if ( mText.size() != text.size() )
		needUpdate = true;

	mText = text;

	if ( needUpdate )
		UpdateCoords();

	Cache();
}

const eeColorA& cTextCache::Color() const {
	return mFontColor;
}

void cTextCache::Color( const eeColorA& color ) {
	if ( mFontColor != color ) {
		mFontColor = color;

		mColors.assign( mText.size() * EE_QUAD_VERTEX, mFontColor );
	}
}

void cTextCache::Color( const eeColorA& color, Uint32 from, Uint32 to ) {
	std::size_t s = mText.size();

	if ( to >= s ) { to = s - 1; }

	if ( from <= to && from < s && to < s ) {
		for ( Uint32 i = from * EE_QUAD_VERTEX; i < to * EE_QUAD_VERTEX; i++ ) {
			mColors[ i ] = color;
		}
	}
}

const eeColorA& cTextCache::ShadowColor() const {
	return mFontShadowColor;
}

void cTextCache::ShadowColor(const eeColorA& color) {
	mFontShadowColor = color;
}

std::vector<eeVertexCoords>& cTextCache::VertextCoords() {
	return mRenderCoords;
}

std::vector<eeColorA>& cTextCache::Colors() {
	return mColors;
}

void cTextCache::Cache() {
	if ( NULL != mFont && mText.size() ) {
		mFont->CacheWidth( mText, mLinesWidth, mCachedWidth, mNumLines, mLargestLineCharCount );
	}else {
		mCachedWidth = 0;
	}

	mCachedCoords = false;
}

eeFloat cTextCache::GetTextWidth() {
	return ( mFlags & FONT_DRAW_VERTICAL ) ? (eeFloat)mFont->GetFontHeight() * (eeFloat)mNumLines : mCachedWidth;
}

eeFloat cTextCache::GetTextHeight() {
	return ( mFlags & FONT_DRAW_VERTICAL ) ? mLargestLineCharCount * (eeFloat)mFont->GetFontHeight() : (eeFloat)mFont->GetFontHeight() * (eeFloat)mNumLines;
}

const eeInt& cTextCache::GetNumLines() const {
	return mNumLines;
}

const std::vector<eeFloat>& cTextCache::LinesWidth() {
	return mLinesWidth;
}

void cTextCache::Draw( const eeFloat& X, const eeFloat& Y, const eeFloat& Scale, const eeFloat& Angle, EE_BLEND_MODE Effect ) {
	if ( NULL != mFont ) {
		cGlobalBatchRenderer::instance()->Draw();

		if ( Angle != 0.0f || Scale != 1.0f ) {
			mFont->Draw( *this, X, Y, mFlags, Scale, Angle, Effect );
		} else {
			GLi->Translatef( X, Y, 0.f );
	
			mFont->Draw( *this, 0, 0, mFlags, Scale, Angle, Effect );
	
			GLi->Translatef( -X, -Y, 0.f );
		}
	}
}

const bool& cTextCache::CachedCoords() const {
	return mCachedCoords;
}

void cTextCache::CachedCoords( const bool& cached ) {
	mCachedCoords = cached;
}

const eeUint& cTextCache::CachedVerts() const {
	return mVertexNumCached;
}

void cTextCache::CachedVerts( const eeUint& num ) {
	mVertexNumCached = num;
}

void cTextCache::Flags( const Uint32& flags ) {
	if ( mFlags != flags ) {
		mFlags = flags;
		mCachedCoords = false;

		if ( ( mFlags & FONT_DRAW_VERTICAL ) != ( flags & FONT_DRAW_VERTICAL ) ) {
			Cache();
		}
	}
}

const Uint32& cTextCache::Flags() const {
	return mFlags;
}

}}
