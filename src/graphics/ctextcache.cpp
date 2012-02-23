#include "ctextcache.hpp"
#include "cfont.hpp"

namespace EE { namespace Graphics {

cTextCache::cTextCache() :
	mFont(NULL),
	mCachedWidth(0.f),
	mNumLines(1),
	mFontColor(255,255,255,255),
	mFontShadowColor(0,0,0,255),
	mFlags(0),
	mCachedCoords(false)
{
}

cTextCache::cTextCache( cFont * font, const String& text, eeColorA FontColor, eeColorA FontShadowColor ) :
	mText( text ),
	mFont( font ),
	mCachedWidth(0.f),
	mNumLines(1)
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

void cTextCache::Color(const eeColorA& Color) {
	if ( mFontColor != Color ) {
		mFontColor = Color;

		mColors.assign( mText.size() * EE_QUAD_VERTEX, mFontColor );
	}
}

const eeColorA& cTextCache::ShadowColor() const {
	return mFontShadowColor;
}

void cTextCache::ShadowColor(const eeColorA& Color) {
	mFontShadowColor = Color;
}

std::vector<eeVertexCoords>& cTextCache::VertextCoords() {
	return mRenderCoords;
}

std::vector<eeColorA>& cTextCache::Colors() {
	return mColors;
}

void cTextCache::Cache() {
	if ( NULL != mFont && mText.size() ) {
		mFont->CacheWidth( mText, mLinesWidth, mCachedWidth, mNumLines );
	}else {
		mCachedWidth = 0;
	}

	mCachedCoords = false;
}

eeFloat cTextCache::GetTextWidth() {
	return mCachedWidth;
}

eeFloat cTextCache::GetTextHeight() {
	return (eeFloat)mFont->GetFontHeight() * (eeFloat)mNumLines;
}

const eeInt& cTextCache::GetNumLines() const {
	return mNumLines;
}

const std::vector<eeFloat>& cTextCache::LinesWidth() {
	return mLinesWidth;
}

void cTextCache::Draw( const eeFloat& X, const eeFloat& Y, const Uint32& Flags, const eeFloat& Scale, const eeFloat& Angle, EE_PRE_BLEND_FUNC Effect ) {
	if ( NULL != mFont ) {
		if ( mFlags != Flags ) {
			mFlags = Flags;
			mCachedCoords = false;
		}
		
		cGlobalBatchRenderer::instance()->Draw();

		if ( Angle != 0.0f || Scale != 1.0f ) {
			mFont->Draw( *this, X, Y, Flags, Scale, Angle, Effect );
		} else {
			GLi->Translatef( X, Y, 0.f );
	
			mFont->Draw( *this, 0, 0, Flags, Scale, Angle, Effect );
	
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

}}
