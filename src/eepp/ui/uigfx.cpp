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
	if ( NULL != mSubTexture && ( ( getFlags() & UI_AUTO_SIZE ) || ( Params.Size.x == -1 && Params.Size.y == -1 ) ) )
		setSize( mSubTexture->getSize() );

	mColor.Alpha = (Uint8)mAlpha;

	autoSize();
}

UIGfx::~UIGfx() {
}

Uint32 UIGfx::getType() const {
	return UI_TYPE_GFX;
}

bool UIGfx::isType( const Uint32& type ) const {
	return UIGfx::getType() == type ? true : UIComplexControl::isType( type );
}

void UIGfx::subTexture( Graphics::SubTexture * subTexture ) {
	mSubTexture = subTexture;

	autoSize();
	autoAlign();
}

void UIGfx::autoSize() {
	if ( mFlags & UI_AUTO_SIZE ) {
		if ( NULL != mSubTexture ) {
			setSize( mSubTexture->getSize() );
		} else {
			setSize( Sizei( 0, 0 ) );
		}
	}
}

void UIGfx::draw() {
	UIControlAnim::draw();

	if ( mVisible ) {
		if ( NULL != mSubTexture && 0.f != mAlpha ) {
			Sizef oDestSize	= mSubTexture->getDestSize();
			Vector2i oOff		= mSubTexture->getOffset();

			if ( mFlags & UI_FIT_TO_CONTROL ) {
				mSubTexture->setOffset( Vector2i( 0, 0 ) );
				mSubTexture->setDestSize( Vector2f( mSize.x, mSize.y ) );

				drawSubTexture();

				mSubTexture->setDestSize( oDestSize );
				mSubTexture->setOffset( oOff );
			} else if ( mFlags & UI_AUTO_FIT ) {
				mSubTexture->setOffset( Vector2i( 0, 0 ) );

				Float Scale1 = mSize.x / oDestSize.x;
				Float Scale2 = mSize.y / oDestSize.y;

				if ( Scale1 < 1 || Scale2 < 1 ) {
					if ( Scale2 < Scale1 )
						Scale1 = Scale2;

					mSubTexture->setDestSize( Sizef( oDestSize.x * Scale1, oDestSize.y * Scale1 ) );

					autoAlign();

					drawSubTexture();

					mSubTexture->setDestSize( oDestSize );

					autoAlign();
				} else {
					drawSubTexture();
				}

				mSubTexture->setOffset( oOff );
			} else {
				autoAlign();

				drawSubTexture();
			}
		}
	}
}

void UIGfx::drawSubTexture() {
	mSubTexture->draw( (Float)mScreenPos.x + mAlignOffset.x, (Float)mScreenPos.y + mAlignOffset.y, mColor, 0.f, Vector2f::One, getBlendMode(), mRender );
}

void UIGfx::setAlpha( const Float& alpha ) {
	UIControlAnim::setAlpha( alpha );
	mColor.Alpha = (Uint8)alpha;
}

Graphics::SubTexture * UIGfx::subTexture() const {
	return mSubTexture;
}

const ColorA& UIGfx::color() const {
	return mColor;
}

void UIGfx::color( const ColorA& col ) {
	mColor = col;
	setAlpha( col.a() );
}

const EE_RENDER_MODE& UIGfx::renderMode() const {
	return mRender;
}

void UIGfx::renderMode( const EE_RENDER_MODE& render ) {
	mRender = render;
}

void UIGfx::autoAlign() {
	if ( NULL == mSubTexture )
		return;

	if ( HAlignGet( mFlags ) == UI_HALIGN_CENTER ) {
		mAlignOffset.x = mSize.getWidth() / 2 - mSubTexture->getDestSize().x / 2;
	} else if ( FontHAlignGet( mFlags ) == UI_HALIGN_RIGHT ) {
		mAlignOffset.x =  mSize.getWidth() - mSubTexture->getDestSize().x;
	} else {
		mAlignOffset.x = 0;
	}

	if ( VAlignGet( mFlags ) == UI_VALIGN_CENTER ) {
		mAlignOffset.y = mSize.getHeight() / 2 - mSubTexture->getDestSize().y / 2;
	} else if ( FontVAlignGet( mFlags ) == UI_VALIGN_BOTTOM ) {
		mAlignOffset.y = mSize.getHeight() - mSubTexture->getDestSize().y;
	} else {
		mAlignOffset.y = 0;
	}
}

void UIGfx::onSizeChange() {
	autoSize();
	autoAlign();
	UIControlAnim::onSizeChange();
}

const Vector2i& UIGfx::alignOffset() const {
	return mAlignOffset;
}

}}
