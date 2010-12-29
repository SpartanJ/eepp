#include "cuisprite.hpp"

namespace EE { namespace UI {

cUISprite::cUISprite( const cUISprite::CreateParams& Params ) :
	cUIComplexControl( Params ),
	mSprite( Params.Sprite ),
	mRender( Params.SpriteRender ),
	mAlignOffset(0,0),
	mShapeLast(NULL)
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
			CheckShapeUpdate();
			mSprite->Position( (eeFloat)( mScreenPos.x + mAlignOffset.x ), (eeFloat)( mScreenPos.y + mAlignOffset.y ) );
			mSprite->Draw( mBlend, mRender );
		}
	}
}

void cUISprite::CheckShapeUpdate() {
	if ( NULL != mSprite && NULL != mSprite->GetCurrentShape() && mSprite->GetCurrentShape() != mShapeLast ) {
		UpdateSize();
		AutoAlign();
		mShapeLast = mSprite->GetCurrentShape();
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

eeColorA cUISprite::Color() const {
	if ( NULL != mSprite )
		return mSprite->Color();

	return eeColorA();
}

void cUISprite::Color( const eeColorA& color ) {
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

void cUISprite::AutoAlign() {
	if ( NULL == mSprite || NULL == mSprite->GetCurrentShape() )
		return;

	cShape * tShape = mSprite->GetCurrentShape();

	if ( HAlignGet( mFlags ) == UI_HALIGN_CENTER ) {
		mAlignOffset.x = mSize.Width() / 2 - tShape->Size().Width() / 2;
	} else if ( FontHAlignGet( mFlags ) == UI_HALIGN_RIGHT ) {
		mAlignOffset.x =  mSize.Width() - tShape->Size().Width();
	} else {
		mAlignOffset.x = 0;
	}

	if ( VAlignGet( mFlags ) == UI_VALIGN_CENTER ) {
		mAlignOffset.y = mSize.Height() / 2 - tShape->Size().Height() / 2;
	} else if ( FontVAlignGet( mFlags ) == UI_VALIGN_BOTTOM ) {
		mAlignOffset.y = mSize.Height() - tShape->Size().Height();
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
