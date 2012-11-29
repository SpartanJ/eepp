#include <eepp/ui/cuigfx.hpp>

namespace EE { namespace UI {

cUIGfx::cUIGfx( const cUIGfx::CreateParams& Params ) :
	cUIComplexControl( Params ),
	mSubTexture( Params.SubTexture ),
	mColor( Params.SubTextureColor ),
	mRender( Params.SubTextureRender ),
	mAlignOffset(0,0)
{
	if ( NULL != mSubTexture && ( ( Flags() & UI_AUTO_SIZE ) || ( Params.Size.x == -1 && Params.Size.y == -1 ) ) )
		Size( mSubTexture->Size() );

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

void cUIGfx::SubTexture( cSubTexture * subTexture ) {
	mSubTexture = subTexture;

	AutoSize();
	AutoAlign();
}

void cUIGfx::AutoSize() {
	if ( mFlags & UI_AUTO_SIZE ) {
		if ( NULL != mSubTexture ) {
			Size( mSubTexture->Size() );
		} else {
			Size( eeSize( 0, 0 ) );
		}
	}
}

void cUIGfx::Draw() {
	cUIControlAnim::Draw();

	if ( mVisible ) {
		if ( NULL != mSubTexture && 0.f != mAlpha ) {
			eeFloat oDestWidth	= mSubTexture->DestWidth();
			eeFloat oDestHeight	= mSubTexture->DestHeight();
			Int32 oOffX = mSubTexture->OffsetX();
			Int32 oOffY = mSubTexture->OffsetY();

			if ( mFlags & UI_FIT_TO_CONTROL ) {
				mSubTexture->OffsetX( 0 );
				mSubTexture->OffsetY( 0 );

				mSubTexture->DestWidth( (eeFloat)mSize.x );
				mSubTexture->DestHeight( (eeFloat)mSize.y );

				DrawSubTexture();

				mSubTexture->DestWidth( oDestWidth );
				mSubTexture->DestHeight( oDestHeight );

				mSubTexture->OffsetX( oOffX );
				mSubTexture->OffsetY( oOffY );
			} else if ( mFlags & UI_AUTO_FIT ) {
				mSubTexture->OffsetX( 0 );
				mSubTexture->OffsetY( 0 );

				eeFloat Scale1 = mSize.x / oDestWidth;
				eeFloat Scale2 = mSize.y / oDestHeight;

				if ( Scale1 < 1 || Scale2 < 1 ) {
					if ( Scale2 < Scale1 )
						Scale1 = Scale2;

					mSubTexture->DestWidth( oDestWidth * Scale1 );
					mSubTexture->DestHeight( oDestHeight * Scale1 );

					AutoAlign();

					DrawSubTexture();

					mSubTexture->DestWidth( oDestWidth );
					mSubTexture->DestHeight( oDestHeight );

					AutoAlign();
				} else {
					DrawSubTexture();
				}

				mSubTexture->OffsetX( oOffX );
				mSubTexture->OffsetY( oOffY );
			} else {
				DrawSubTexture();
			}
		}
	}
}

void cUIGfx::DrawSubTexture() {
	mSubTexture->Draw( (eeFloat)mScreenPos.x + mAlignOffset.x, (eeFloat)mScreenPos.y + mAlignOffset.y, mColor, 0.f, 1.f, Blend(), mRender );
}

void cUIGfx::Alpha( const eeFloat& alpha ) {
	cUIControlAnim::Alpha( alpha );
	mColor.Alpha = (Uint8)alpha;
}

cSubTexture * cUIGfx::SubTexture() const {
	return mSubTexture;
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
	if ( NULL == mSubTexture )
		return;

	if ( HAlignGet( mFlags ) == UI_HALIGN_CENTER ) {
		mAlignOffset.x = mSize.Width() / 2 - mSubTexture->DestWidth() / 2;
	} else if ( FontHAlignGet( mFlags ) == UI_HALIGN_RIGHT ) {
		mAlignOffset.x =  mSize.Width() - mSubTexture->DestWidth();
	} else {
		mAlignOffset.x = 0;
	}

	if ( VAlignGet( mFlags ) == UI_VALIGN_CENTER ) {
		mAlignOffset.y = mSize.Height() / 2 - mSubTexture->DestHeight() / 2;
	} else if ( FontVAlignGet( mFlags ) == UI_VALIGN_BOTTOM ) {
		mAlignOffset.y = mSize.Height() - mSubTexture->DestHeight();
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
