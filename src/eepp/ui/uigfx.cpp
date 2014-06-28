#include <eepp/ui/uigfx.hpp>
#include <eepp/graphics/subtexture.hpp>

namespace EE { namespace UI {

UIGfx::UIGfx( const UIGfx::CreateParams& Params ) :
	UIComplexControl( Params ),
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

UIGfx::~UIGfx() {
}

Uint32 UIGfx::Type() const {
	return UI_TYPE_GFX;
}

bool UIGfx::IsType( const Uint32& type ) const {
	return UIGfx::Type() == type ? true : UIComplexControl::IsType( type );
}

void UIGfx::SubTexture( Graphics::SubTexture * subTexture ) {
	mSubTexture = subTexture;

	AutoSize();
	AutoAlign();
}

void UIGfx::AutoSize() {
	if ( mFlags & UI_AUTO_SIZE ) {
		if ( NULL != mSubTexture ) {
			Size( mSubTexture->Size() );
		} else {
			Size( Sizei( 0, 0 ) );
		}
	}
}

void UIGfx::Draw() {
	UIControlAnim::Draw();

	if ( mVisible ) {
		if ( NULL != mSubTexture && 0.f != mAlpha ) {
			Sizef oDestSize	= mSubTexture->DestSize();
			Vector2i oOff		= mSubTexture->Offset();

			if ( mFlags & UI_FIT_TO_CONTROL ) {
				mSubTexture->Offset( Vector2i( 0, 0 ) );
				mSubTexture->DestSize( Vector2f( mSize.x, mSize.y ) );

				DrawSubTexture();

				mSubTexture->DestSize( oDestSize );
				mSubTexture->Offset( oOff );
			} else if ( mFlags & UI_AUTO_FIT ) {
				mSubTexture->Offset( Vector2i( 0, 0 ) );

				Float Scale1 = mSize.x / oDestSize.x;
				Float Scale2 = mSize.y / oDestSize.y;

				if ( Scale1 < 1 || Scale2 < 1 ) {
					if ( Scale2 < Scale1 )
						Scale1 = Scale2;

					mSubTexture->DestSize( Sizef( oDestSize.x * Scale1, oDestSize.y * Scale1 ) );

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

void UIGfx::DrawSubTexture() {
	mSubTexture->Draw( (Float)mScreenPos.x + mAlignOffset.x, (Float)mScreenPos.y + mAlignOffset.y, mColor, 0.f, Vector2f::One, Blend(), mRender );
}

void UIGfx::Alpha( const Float& alpha ) {
	UIControlAnim::Alpha( alpha );
	mColor.Alpha = (Uint8)alpha;
}

Graphics::SubTexture * UIGfx::SubTexture() const {
	return mSubTexture;
}

const ColorA& UIGfx::Color() const {
	return mColor;
}

void UIGfx::Color( const ColorA& color ) {
	mColor = color;
	Alpha( color.A() );
}

const EE_RENDER_MODE& UIGfx::RenderMode() const {
	return mRender;
}

void UIGfx::RenderMode( const EE_RENDER_MODE& render ) {
	mRender = render;
}

void UIGfx::AutoAlign() {
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

void UIGfx::OnSizeChange() {
	AutoSize();
	AutoAlign();
	UIControlAnim::OnSizeChange();
}

const Vector2i& UIGfx::AlignOffset() const {
	return mAlignOffset;
}

}}
