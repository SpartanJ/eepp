#include "cuigfx.hpp"

namespace EE { namespace UI {

cUIGfx::cUIGfx( const cUIGfx::CreateParams& Params ) :
	cUIComplexControl( Params ),
	mShape( Params.Shape ),
	mColor( Params.ShapeColor ),
	mRender( Params.ShapeRender ),
	mAlignOffset(0,0)
{
	mType = UI_TYPE_GFX;

	if ( NULL != mShape && ( ( Flags() & UI_AUTO_SIZE ) || ( Params.Size.x == -1 && Params.Size.y == -1 ) ) )
		Size( mShape->Size() );

	mColor.Alpha = (Uint8)mAlpha;

	AutoSize();
}

cUIGfx::~cUIGfx() {
}

void cUIGfx::Shape( cShape * shape ) {
	mShape = shape;
	
	AutoSize();
}

void cUIGfx::AutoSize() {
	if ( mFlags & UI_AUTO_SIZE ) {
		if ( NULL != mShape ) {
			Size( mShape->Size() );
		} else {
			Size( eeSize( 0, 0 ) );
		}
	}
}

void cUIGfx::Draw() {
	cUIControlAnim::Draw();

	if ( mVisible ) {
		if ( NULL != mShape && 0.f != mAlpha )
			mShape->Draw( (eeFloat)mScreenPos.x + mAlignOffset.x, (eeFloat)mScreenPos.y + mAlignOffset.y, mColor, 0.f, 1.f, Blend(), mRender );
	}
}

void cUIGfx::Alpha( const eeFloat& alpha ) {
	cUIControlAnim::Alpha( alpha );
	mColor.Alpha = (Uint8)alpha;
}

cShape * cUIGfx::Shape() const {
	return mShape;
}

const eeColorA& cUIGfx::Color() const {
	return mColor;
}

void cUIGfx::Color( const eeColorA& color ) {
	mColor = color;
	Alpha( color.A() );
}

const EE_RENDERTYPE& cUIGfx::RenderType() const {
	return mRender;
}

void cUIGfx::RenderType( const EE_RENDERTYPE& render ) {
	mRender = render;
}

void cUIGfx::FitToControl() {
	if ( NULL != mShape && mFlags & UI_FIT_TO_CONTROL ) {
		mShape->DestWidth( (eeFloat)mSize.x );
		mShape->DestHeight( (eeFloat)mSize.y );
	}
}

void cUIGfx::AutoAlign() {
	if ( NULL == mShape )
		return;

	if ( HAlignGet( mFlags ) == UI_HALIGN_CENTER ) {
		mAlignOffset.x = mSize.Width() / 2 - mShape->Size().Width() / 2;
	} else if ( FontHAlignGet( mFlags ) == UI_HALIGN_RIGHT ) {
		mAlignOffset.x =  mSize.Width() - mShape->Size().Width();
	} else {
		mAlignOffset.x = 0;
	}

	if ( VAlignGet( mFlags ) == UI_VALIGN_CENTER ) {
		mAlignOffset.y = mSize.Height() / 2 - mShape->Size().Height() / 2;
	} else if ( FontVAlignGet( mFlags ) == UI_VALIGN_BOTTOM ) {
		mAlignOffset.y = mSize.Height() - mShape->Size().Height();
	} else {
		mAlignOffset.y = 0;
	}
}

void cUIGfx::OnSizeChange() {
	FitToControl();
	AutoSize();
	AutoAlign();
	cUIControlAnim::OnSizeChange();
}

const eeVector2i& cUIGfx::AlignOffset() const {
	return mAlignOffset;
}

}}
