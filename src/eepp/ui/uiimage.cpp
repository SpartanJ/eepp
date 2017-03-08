#include <eepp/ui/uiimage.hpp>
#include <eepp/graphics/subtexture.hpp>

namespace EE { namespace UI {

UIImage * UIImage::New() {
	return eeNew( UIImage, () );
}

UIImage::UIImage() :
	UIWidget(),
	mSubTexture( NULL ),
	mColor(),
	mRender( RN_NORMAL ),
	mAlignOffset(0,0)
{
	mFlags |= UI_AUTO_SIZE;

	onAutoSize();
}

UIImage::~UIImage() {
}

Uint32 UIImage::getType() const {
	return UI_TYPE_IMAGE;
}

bool UIImage::isType( const Uint32& type ) const {
	return UIImage::getType() == type ? true : UIWidget::isType( type );
}

void UIImage::setSubTexture( Graphics::SubTexture * subTexture ) {
	mSubTexture = subTexture;

	onAutoSize();

	if ( NULL != mSubTexture && mSize.x == 0 && mSize.y == 0 ) {
		setSize( mSubTexture->getDpSize() );
	}

	autoAlign();
}

void UIImage::onAutoSize() {
	if ( ( mFlags & UI_AUTO_SIZE ) && Sizei::Zero == mSize ) {
		if ( NULL != mSubTexture ) {
			setSize( mSubTexture->getDpSize() );
		}
	}
}

void UIImage::draw() {
	UIControlAnim::draw();

	if ( mVisible ) {
		if ( NULL != mSubTexture && 0.f != mAlpha ) {
			Sizef oDestSize	= mSubTexture->getDestSize();
			Vector2i oOff	= mSubTexture->getOffset();

			if ( mFlags & UI_FIT_TO_CONTROL ) {
				mSubTexture->setOffset( Vector2i( 0, 0 ) );
				mSubTexture->setDestSize( Vector2f( mRealSize.x, mRealSize.y ) );

				autoAlign();

				drawSubTexture();

			} else if ( mFlags & UI_AUTO_FIT ) {
				mSubTexture->setOffset( Vector2i( 0, 0 ) );

				Sizei pxSize = mSubTexture->getPxSize();
				Float Scale1 = mRealSize.x / (Float)pxSize.x;
				Float Scale2 = mRealSize.y / (Float)pxSize.y;

				if ( Scale1 < 1 || Scale2 < 1 ) {
					if ( Scale2 < Scale1 )
						Scale1 = Scale2;

					mSubTexture->setDestSize( Sizef( pxSize.x * Scale1, pxSize.y * Scale1 ) );

					autoAlign();

					drawSubTexture();
				} else {					
					mSubTexture->setDestSize( Vector2f( (Float)pxSize.x, (Float)pxSize.y ) );

					autoAlign();

					drawSubTexture();
				}
			} else {
				Sizei realOffSet = mSubTexture->getOffset();

				mSubTexture->setOffset( Vector2i( (Int32)( (Float)realOffSet.x / mSubTexture->getPixelDensity() * PixelDensity::getPixelDensity() ),
												  (Int32)( (Float)realOffSet.y / mSubTexture->getPixelDensity() * PixelDensity::getPixelDensity() )
										) );

				mSubTexture->setDestSize( Vector2f( (Float)mSubTexture->getPxSize().x, (Float)mSubTexture->getPxSize().y ) );

				autoAlign();

				drawSubTexture();

				mSubTexture->setOffset( realOffSet );
			}

			mSubTexture->setDestSize( oDestSize );
			mSubTexture->setOffset( oOff );
		}
	}
}

void UIImage::drawSubTexture() {
	mSubTexture->draw( (Float)mScreenPos.x + mAlignOffset.x, (Float)mScreenPos.y + mAlignOffset.y, mColor, 0.f, Vector2f::One, getBlendMode(), mRender );
}

void UIImage::setAlpha( const Float& alpha ) {
	UIControlAnim::setAlpha( alpha );
	mColor.Alpha = (Uint8)alpha;
}

Graphics::SubTexture * UIImage::getSubTexture() const {
	return mSubTexture;
}

const ColorA& UIImage::getColor() const {
	return mColor;
}

void UIImage::setColor( const ColorA& col ) {
	mColor = col;
	setAlpha( col.a() );
}

const EE_RENDER_MODE& UIImage::getRenderMode() const {
	return mRender;
}

void UIImage::setRenderMode( const EE_RENDER_MODE& render ) {
	mRender = render;
}

void UIImage::autoAlign() {
	if ( NULL == mSubTexture )
		return;

	if ( HAlignGet( mFlags ) == UI_HALIGN_CENTER ) {
		mAlignOffset.x = mRealSize.getWidth() / 2 - mSubTexture->getDestSize().x / 2;
	} else if ( fontHAlignGet( mFlags ) == UI_HALIGN_RIGHT ) {
		mAlignOffset.x =  mRealSize.getWidth() - mSubTexture->getDestSize().x;
	} else {
		mAlignOffset.x = 0;
	}

	if ( VAlignGet( mFlags ) == UI_VALIGN_CENTER ) {
		mAlignOffset.y = mRealSize.getHeight() / 2 - mSubTexture->getDestSize().y / 2;
	} else if ( fontVAlignGet( mFlags ) == UI_VALIGN_BOTTOM ) {
		mAlignOffset.y = mRealSize.getHeight() - mSubTexture->getDestSize().y;
	} else {
		mAlignOffset.y = 0;
	}
}

void UIImage::onSizeChange() {
	onAutoSize();
	autoAlign();
	UIControlAnim::onSizeChange();
}

void UIImage::onAlignChange() {
	onAutoSize();
	autoAlign();
}

const Vector2i& UIImage::getAlignOffset() const {
	return mAlignOffset;
}

}}
