#include <eepp/ui/cuisprite.hpp>
#include <eepp/graphics/csprite.hpp>

namespace EE { namespace UI {

cUISprite::cUISprite( const cUISprite::CreateParams& Params ) :
	cUIComplexControl( Params ),
	mSprite( Params.Sprite ),
	mRender( Params.SpriteRender ),
	mAlignOffset(0,0),
	mSubTextureLast(NULL)
{
	if ( Params.DeallocSprite )
		mControlFlags |= UI_CTRL_FLAG_FREE_USE;

	if ( ( Flags() & UI_AUTO_SIZE ) || ( Params.Size.x == -1 && Params.Size.y == -1 ) ) {
		if ( NULL != mSprite && NULL != mSprite->GetCurrentSubTexture() ) {
			Size( mSprite->GetCurrentSubTexture()->Size() );
		}
	}
}

cUISprite::~cUISprite() {
	if ( DeallocSprite() )
		eeSAFE_DELETE( mSprite );
}

Uint32 cUISprite::Type() const {
	return UI_TYPE_SPRITE;
}

bool cUISprite::IsType( const Uint32& type ) const {
	return cUISprite::Type() == type ? true : cUIComplexControl::IsType( type );
}

Uint32 cUISprite::DeallocSprite() {
	return mControlFlags & UI_CTRL_FLAG_FREE_USE;
}

void cUISprite::Sprite( cSprite * sprite ) {
	if ( DeallocSprite() )
		eeSAFE_DELETE( mSprite );

	mSprite = sprite;
	
	UpdateSize();
}

void cUISprite::Draw() {
	cUIControlAnim::Draw();

	if ( mVisible ) {
		if ( NULL != mSprite && 0.f != mAlpha ) {
			CheckSubTextureUpdate();
			mSprite->Position( (Float)( mScreenPos.x + mAlignOffset.x ), (Float)( mScreenPos.y + mAlignOffset.y ) );
			mSprite->Draw( Blend(), mRender );
		}
	}
}

void cUISprite::CheckSubTextureUpdate() {
	if ( NULL != mSprite && NULL != mSprite->GetCurrentSubTexture() && mSprite->GetCurrentSubTexture() != mSubTextureLast ) {
		UpdateSize();
		AutoAlign();
		mSubTextureLast = mSprite->GetCurrentSubTexture();
	}
}

void cUISprite::Alpha( const Float& alpha ) {
	cUIControlAnim::Alpha( alpha );
	
	if ( NULL != mSprite )
		mSprite->Alpha( alpha );
}

cSprite * cUISprite::Sprite() const {
	return mSprite;
}

ColorA cUISprite::Color() const {
	if ( NULL != mSprite )
		return mSprite->Color();

	return ColorA();
}

void cUISprite::Color( const ColorA& color ) {
	if ( NULL != mSprite )
		mSprite->Color( color );
	
	Alpha( color.A() );
}

const EE_RENDER_MODE& cUISprite::RenderMode() const {
	return mRender;
}

void cUISprite::RenderMode( const EE_RENDER_MODE& render ) {
	mRender = render;
}

void cUISprite::UpdateSize() {
	if ( Flags() & UI_AUTO_SIZE ) {
		if ( NULL != mSprite ) {
			if ( NULL != mSprite->GetCurrentSubTexture() && mSprite->GetCurrentSubTexture()->Size() != mSize )
				Size( mSprite->GetCurrentSubTexture()->Size() );
		}
	}
}

void cUISprite::AutoAlign() {
	if ( NULL == mSprite || NULL == mSprite->GetCurrentSubTexture() )
		return;

	cSubTexture * tSubTexture = mSprite->GetCurrentSubTexture();

	if ( HAlignGet( mFlags ) == UI_HALIGN_CENTER ) {
		mAlignOffset.x = mSize.Width() / 2 - tSubTexture->Size().Width() / 2;
	} else if ( FontHAlignGet( mFlags ) == UI_HALIGN_RIGHT ) {
		mAlignOffset.x =  mSize.Width() - tSubTexture->Size().Width();
	} else {
		mAlignOffset.x = 0;
	}

	if ( VAlignGet( mFlags ) == UI_VALIGN_CENTER ) {
		mAlignOffset.y = mSize.Height() / 2 - tSubTexture->Size().Height() / 2;
	} else if ( FontVAlignGet( mFlags ) == UI_VALIGN_BOTTOM ) {
		mAlignOffset.y = mSize.Height() - tSubTexture->Size().Height();
	} else {
		mAlignOffset.y = 0;
	}
}

const eeVector2i& cUISprite::AlignOffset() const {
	return mAlignOffset;
}

void cUISprite::OnSizeChange() {
	AutoAlign();
}

}}
