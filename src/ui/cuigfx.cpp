#include "cuigfx.hpp"

namespace EE { namespace UI {

cUIGfx::cUIGfx( const cUIGfx::CreateParams& Params ) :
	cUIComplexControl( Params ),
	mShape( Params.Shape ),
	mColor( Params.ShapeColor ),
	mRender( Params.ShapeRender ),
	mAlignOffset(0,0)
{
	if ( NULL != mShape && ( ( Flags() & UI_AUTO_SIZE ) || ( Params.Size.x == -1 && Params.Size.y == -1 ) ) )
		Size( mShape->Size() );

	mColor.Alpha = (Uint8)mAlpha;

	AutoSize();
}

cUIGfx::~cUIGfx() {
}

Uint32 cUIGfx::Type() const {
	return UI_TYPE_GFX;
}

bool cUIGfx::IsType( const Uint32& type ) const {
	return cUIGfx::Type() == type ? true : cUIComplexControl::IsType( type );
}

void cUIGfx::Shape( cShape * shape ) {
	mShape = shape;

	AutoSize();
	AutoAlign();
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
		if ( NULL != mShape && 0.f != mAlpha ) {
			eeFloat oDestWidth	= mShape->DestWidth();
			eeFloat oDestHeight	= mShape->DestHeight();
			Int32 oOffX = mShape->OffsetX();
			Int32 oOffY = mShape->OffsetY();

			if ( mFlags & UI_FIT_TO_CONTROL ) {
				mShape->OffsetX( 0 );
				mShape->OffsetY( 0 );

				mShape->DestWidth( (eeFloat)mSize.x );
				mShape->DestHeight( (eeFloat)mSize.y );

				DrawShape();

				mShape->DestWidth( oDestWidth );
				mShape->DestHeight( oDestHeight );

				mShape->OffsetX( oOffX );
				mShape->OffsetY( oOffY );
			} else if ( mFlags & UI_AUTO_FIT ) {
				mShape->OffsetX( 0 );
				mShape->OffsetY( 0 );

				eeFloat Scale1 = mSize.x / oDestWidth;
				eeFloat Scale2 = mSize.y / oDestHeight;

				if ( Scale1 < 1 || Scale2 < 1 ) {
					if ( Scale2 < Scale1 )
						Scale1 = Scale2;

					mShape->DestWidth( oDestWidth * Scale1 );
					mShape->DestHeight( oDestHeight * Scale1 );

					AutoAlign();

					DrawShape();

					mShape->DestWidth( oDestWidth );
					mShape->DestHeight( oDestHeight );

					AutoAlign();
				} else {
					DrawShape();
				}

				mShape->OffsetX( oOffX );
				mShape->OffsetY( oOffY );
			} else {
				DrawShape();
			}
		}
	}
}

void cUIGfx::DrawShape() {
	mShape->Draw( (eeFloat)mScreenPos.x + mAlignOffset.x, (eeFloat)mScreenPos.y + mAlignOffset.y, mColor, 0.f, 1.f, Blend(), mRender );
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

void cUIGfx::AutoAlign() {
	if ( NULL == mShape )
		return;

	if ( HAlignGet( mFlags ) == UI_HALIGN_CENTER ) {
		mAlignOffset.x = mSize.Width() / 2 - mShape->DestWidth() / 2;
	} else if ( FontHAlignGet( mFlags ) == UI_HALIGN_RIGHT ) {
		mAlignOffset.x =  mSize.Width() - mShape->DestWidth();
	} else {
		mAlignOffset.x = 0;
	}

	if ( VAlignGet( mFlags ) == UI_VALIGN_CENTER ) {
		mAlignOffset.y = mSize.Height() / 2 - mShape->DestHeight() / 2;
	} else if ( FontVAlignGet( mFlags ) == UI_VALIGN_BOTTOM ) {
		mAlignOffset.y = mSize.Height() - mShape->DestHeight();
	} else {
		mAlignOffset.y = 0;
	}
}

void cUIGfx::OnSizeChange() {
	AutoSize();
	AutoAlign();
	cUIControlAnim::OnSizeChange();
}

const eeVector2i& cUIGfx::AlignOffset() const {
	return mAlignOffset;
}

}}
