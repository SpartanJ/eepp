#include <eepp/ui/uitextureregion.hpp>
#include <eepp/graphics/textureregion.hpp>
#include <pugixml/pugixml.hpp>
#include <eepp/graphics/globaltextureatlas.hpp>
#include <eepp/graphics/textureatlasmanager.hpp>

namespace EE { namespace UI {

UITextureRegion * UITextureRegion::New() {
	return eeNew( UITextureRegion, () );
}

UITextureRegion::UITextureRegion() :
	UIWidget(),
	mTextureRegion( NULL ),
	mColor(),
	mRender( RENDER_NORMAL ),
	mAlignOffset(0,0)
{
	mFlags |= UI_AUTO_SIZE;

	onAutoSize();
}

UITextureRegion::~UITextureRegion() {
}

Uint32 UITextureRegion::getType() const {
	return UI_TYPE_TEXTUREREGION;
}

bool UITextureRegion::isType( const Uint32& type ) const {
	return UITextureRegion::getType() == type ? true : UIWidget::isType( type );
}

UITextureRegion * UITextureRegion::setTextureRegion( Graphics::TextureRegion * TextureRegion ) {
	mTextureRegion = TextureRegion;

	onAutoSize();

	if ( NULL != mTextureRegion && mDpSize.x == 0 && mDpSize.y == 0 ) {
		setSize( mTextureRegion->getDpSize().asFloat() );
	}

	autoAlign();

	notifyLayoutAttrChange();

	invalidateDraw();

	return this;
}

void UITextureRegion::onAutoSize() {
	if ( NULL != mTextureRegion ) {
		if ( ( mFlags & UI_AUTO_SIZE ) && Sizef::Zero == mDpSize ) {
			setSize( mTextureRegion->getDpSize().asFloat() );
		}

		if ( mLayoutWidthRules == WRAP_CONTENT ) {
			setInternalPixelsWidth( mTextureRegion->getPxSize().getWidth() + mRealPadding.Left + mRealPadding.Right );
		}

		if ( mLayoutHeightRules == WRAP_CONTENT ) {
			setInternalPixelsHeight( mTextureRegion->getPxSize().getHeight() + mRealPadding.Top + mRealPadding.Bottom );
		}
	}
}

void UITextureRegion::draw() {
	if ( mVisible ) {
		UINode::draw();

		if ( NULL != mTextureRegion && 0.f != mAlpha ) {
			Sizef oDestSize	= mTextureRegion->getDestSize();
			Vector2i oOff	= mTextureRegion->getOffset();

			if ( mScaleType == UIScaleType::Expand ) {
				mTextureRegion->setOffset( Vector2i( 0, 0 ) );
				mTextureRegion->setDestSize( Vector2f( (int)mSize.x - mRealPadding.Left - mRealPadding.Right, (int)mSize.y - mRealPadding.Top - mRealPadding.Bottom ) );

				autoAlign();

				drawTextureRegion();

			} else if ( mScaleType == UIScaleType::FitInside ) {
				mTextureRegion->setOffset( Vector2i( 0, 0 ) );

				Sizei pxSize = mTextureRegion->getPxSize();
				Float Scale1 = ( mSize.x - mRealPadding.Left - mRealPadding.Right ) / (Float)pxSize.x;
				Float Scale2 = ( mSize.y - mRealPadding.Top - mRealPadding.Bottom ) / (Float)pxSize.y;

				if ( Scale1 < 1 || Scale2 < 1 ) {
					if ( Scale2 < Scale1 )
						Scale1 = Scale2;

					Sizef dst( pxSize.x * Scale1, pxSize.y * Scale1 );
					mTextureRegion->setDestSize( dst.floor() );

					autoAlign();

					drawTextureRegion();
				} else {					
					mTextureRegion->setDestSize( Vector2f( (Float)pxSize.x, (Float)pxSize.y ) );

					autoAlign();

					drawTextureRegion();
				}
			} else {
				mTextureRegion->setOffset( Vector2i( (Int32)( (Float)oOff.x / mTextureRegion->getPixelDensity() * PixelDensity::getPixelDensity() ),
												  (Int32)( (Float)oOff.y / mTextureRegion->getPixelDensity() * PixelDensity::getPixelDensity() )
										) );

				mTextureRegion->setDestSize( Vector2f( (Float)mTextureRegion->getPxSize().x, (Float)mTextureRegion->getPxSize().y ) );

				autoAlign();

				drawTextureRegion();
			}

			mTextureRegion->setDestSize( oDestSize );
			mTextureRegion->setOffset( oOff );
		}
	}
}

void UITextureRegion::drawTextureRegion() {
	mTextureRegion->draw( (Float)mScreenPosi.x + (int)mAlignOffset.x, (Float)mScreenPosi.y + (int)mAlignOffset.y, mColor, 0.f, Vector2f::One, getBlendMode(), mRender );
}

void UITextureRegion::setAlpha( const Float& alpha ) {
	UIWidget::setAlpha( alpha );
	mColor.a = (Uint8)alpha;
}

Graphics::TextureRegion * UITextureRegion::getTextureRegion() const {
	return mTextureRegion;
}

const Color& UITextureRegion::getColor() const {
	return mColor;
}

void UITextureRegion::setColor( const Color& col ) {
	mColor = col;
	setAlpha( col.a );
}

const RenderMode& UITextureRegion::getRenderMode() const {
	return mRender;
}

void UITextureRegion::setRenderMode( const RenderMode& render ) {
	mRender = render;
	invalidateDraw();
}

void UITextureRegion::autoAlign() {
	if ( NULL == mTextureRegion )
		return;

	if ( HAlignGet( mFlags ) == UI_HALIGN_CENTER ) {
		mAlignOffset.x = ( mSize.getWidth() - mTextureRegion->getDestSize().x ) / 2;
	} else if ( fontHAlignGet( mFlags ) == UI_HALIGN_RIGHT ) {
		mAlignOffset.x =  mSize.getWidth() - mTextureRegion->getDestSize().x - mRealPadding.Right;
	} else {
		mAlignOffset.x = mRealPadding.Left;
	}

	if ( VAlignGet( mFlags ) == UI_VALIGN_CENTER ) {
		mAlignOffset.y = ( mSize.getHeight() - mTextureRegion->getDestSize().y ) / 2;
	} else if ( fontVAlignGet( mFlags ) == UI_VALIGN_BOTTOM ) {
		mAlignOffset.y = mSize.getHeight() - mTextureRegion->getDestSize().y - mRealPadding.Bottom;
	} else {
		mAlignOffset.y = mRealPadding.Top;
	}
}

void UITextureRegion::onSizeChange() {
	onAutoSize();
	autoAlign();
	UIWidget::onSizeChange();
}

void UITextureRegion::onAlignChange() {
	onAutoSize();
	autoAlign();
}

const Vector2f& UITextureRegion::getAlignOffset() const {
	return mAlignOffset;
}

bool UITextureRegion::setAttribute( const NodeAttribute& attribute ) {
	const std::string& name = attribute.getName();

	if ( "src" == name || "textureregion" == name ) {
		Drawable * res = NULL;

		if ( NULL != ( res = TextureAtlasManager::instance()->getTextureRegionByName( attribute.asString() ) ) && res->getDrawableType() == Drawable::TEXTUREREGION ) {
			setTextureRegion( static_cast<TextureRegion*>( res ) );
		}
	} else if ( "scaletype" == name ) {
		std::string val = attribute.asString();
		String::toLowerInPlace( val );

		if ( "expand" == val ) {
			setScaleType( UIScaleType::Expand );
		} else if ( "fit_inside" == val || "fitinside" == val ) {
			setScaleType( UIScaleType::FitInside );
		} else if ( "none" == val ) {
			setScaleType( UIScaleType::None );
		}
	} else if ( "tint" == name ) {
		setColor( attribute.asColor() );
	} else {
		return UIWidget::setAttribute( attribute );
	}

	return true;
}

Uint32 UITextureRegion::getScaleType() const {
	return mScaleType;
}

UITextureRegion * UITextureRegion::setScaleType(const Uint32& scaleType) {
	mScaleType = scaleType;
	invalidateDraw();
	return this;
}

}}
