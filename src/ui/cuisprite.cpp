#include "cuisprite.hpp"

namespace EE { namespace UI {

cUISprite::cUISprite( const cUISprite::CreateParams& Params ) :
	cUIControlAnim( Params ),
	mSprite( Params.Sprite ),
	mRender( Params.SpriteRender )
{
	mType |= UI_TYPE_GET(UI_TYPE_SPRITE);

	if ( ( Flags() & UI_AUTO_SIZE ) || ( Params.Size.x == -1 && Params.Size.y == -1 ) ) {
		if ( NULL != mSprite && NULL != mSprite->GetCurrentShape() ) {
			Size( mSprite->GetCurrentShape()->Size() );
		}
	}
}

cUISprite::~cUISprite() {
}

void cUISprite::Sprite( cSprite * sprite ) {
	mSprite = sprite;
	
	UpdateSize();
}

void cUISprite::Draw() {
	cUIControlAnim::Draw();

	if ( mVisible ) {
		if ( NULL != mSprite && 0.f != mAlpha ) {
			UpdateSize();
			mSprite->Position( (eeFloat)mScreenPos.x, (eeFloat)mScreenPos.y );
			mSprite->Draw( mBlend, mRender );
		}
	}
}

void cUISprite::Alpha( const eeFloat& alpha ) {
	cUIControlAnim::Alpha( alpha );
	
	if ( NULL != mSprite )
		mSprite->Alpha( alpha );
}

cSprite * cUISprite::Sprite() const {
	return mSprite;
}

eeRGBA cUISprite::Color() const {
	if ( NULL != mSprite )
		return mSprite->Color();

	return eeRGBA();
}

void cUISprite::Color( const eeRGBA& color ) {
	if ( NULL != mSprite )
		mSprite->Color( color );
	
	Alpha( color.A() );
}

const EE_RENDERTYPE& cUISprite::RenderType() const {
	return mRender;
}

void cUISprite::RenderType( const EE_RENDERTYPE& render ) {
	mRender = render;
}

void cUISprite::UpdateSize() {
	if ( Flags() & UI_AUTO_SIZE ) {
		if ( NULL != mSprite ) {
			if ( NULL != mSprite->GetCurrentShape() && mSprite->GetCurrentShape()->Size() != mSize )
				Size( mSprite->GetCurrentShape()->Size() );
		}
	}
}

}}
