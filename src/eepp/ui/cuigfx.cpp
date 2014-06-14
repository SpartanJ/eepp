#include <eepp/ui/cuigfx.hpp>
#include <eepp/graphics/csubtexture.hpp>

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
			eeSizef oDestSize	= mSubTexture->DestSize();
			eeVector2i oOff		= mSubTexture->Offset();

			if ( mFlags & UI_FIT_TO_CONTROL ) {
				mSubTexture->Offset( eeVector2i( 0, 0 ) );
				mSubTexture->DestSize( eeVector2f( mSize.x, mSize.y ) );

				DrawSubTexture();

				mSubTexture->DestSize( oDestSize );
				mSubTexture->Offset( oOff );
			} else if ( mFlags & UI_AUTO_FIT ) {
				mSubTexture->Offset( eeVector2i( 0, 0 ) );

				Float Scale1 = mSize.x / oDestSize.x;
				Float Scale2 = mSize.y / oDestSize.y;

				if ( Scale1 < 1 || Scale2 < 1 ) {
					if ( Scale2 < Scale1 )
						Scale1 = Scale2;

					mSubTexture->DestSize( eeSizef( oDestSize.x * Scale1, oDestSize.y * Scale1 ) );

					AutoAlign();

					DrawSubTexture();

					mSubTexture->DestSize( oDestSize );

					AutoAlign();
				} else {
					DrawSubTexture();
				}

				mSubTexture->Offset( oOff );
			} else {
				AutoAlign();

				DrawSubTexture();
			}
		}
	}
}

void cUIGfx::DrawSubTexture() {
	mSubTexture->Draw( (Float)mScreenPos.x + mAlignOffset.x, (Float)mScreenPos.y + mAlignOffset.y, mColor, 0.f, eeVector2f::One, Blend(), mRender );
}

void cUIGfx::Alpha( const Float& alpha ) {
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

const EE_RENDER_MODE& cUIGfx::RenderMode() const {
	return mRender;
}

void cUIGfx::RenderMode( const EE_RENDER_MODE& render ) {
	mRender = render;
}

void cUIGfx::AutoAlign() {
	if ( NULL == mSubTexture )
		return;

	if ( HAlignGet( mFlags ) == UI_HALIGN_CENTER ) {
		mAlignOffset.x = mSize.Width() / 2 - mSubTexture->DestSize().x / 2;
	} else if ( FontHAlignGet( mFlags ) == UI_HALIGN_RIGHT ) {
		mAlignOffset.x =  mSize.Width() - mSubTexture->DestSize().x;
	} else {
		mAlignOffset.x = 0;
	}

	if ( VAlignGet( mFlags ) == UI_VALIGN_CENTER ) {
		mAlignOffset.y = mSize.Height() / 2 - mSubTexture->DestSize().y / 2;
	} else if ( FontVAlignGet( mFlags ) == UI_VALIGN_BOTTOM ) {
		mAlignOffset.y = mSize.Height() - mSubTexture->DestSize().y;
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
