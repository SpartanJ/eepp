#include <eepp/ui/uisubtexture.hpp>
#include <eepp/graphics/subtexture.hpp>
#include <eepp/helper/pugixml/pugixml.hpp>
#include <eepp/graphics/globaltextureatlas.hpp>

namespace EE { namespace UI {

UISubTexture * UISubTexture::New() {
	return eeNew( UISubTexture, () );
}

UISubTexture::UISubTexture() :
	UIWidget(),
	mSubTexture( NULL ),
	mColor(),
	mRender( RN_NORMAL ),
	mAlignOffset(0,0)
{
	mFlags |= UI_AUTO_SIZE;

	onAutoSize();
}

UISubTexture::~UISubTexture() {
}

Uint32 UISubTexture::getType() const {
	return UI_TYPE_SUBTEXTURE;
}

bool UISubTexture::isType( const Uint32& type ) const {
	return UISubTexture::getType() == type ? true : UIWidget::isType( type );
}

UISubTexture * UISubTexture::setSubTexture( Graphics::SubTexture * subTexture ) {
	mSubTexture = subTexture;

	onAutoSize();

	if ( NULL != mSubTexture && mSize.x == 0 && mSize.y == 0 ) {
		setSize( mSubTexture->getDpSize() );
	}

	autoAlign();

	notifyLayoutAttrChange();

	return this;
}

void UISubTexture::onAutoSize() {
	if ( ( mFlags & UI_AUTO_SIZE ) && Sizei::Zero == mSize ) {
		if ( NULL != mSubTexture ) {
			setSize( mSubTexture->getDpSize() );
		}
	}
}

void UISubTexture::draw() {
	UIControlAnim::draw();

	if ( mVisible ) {
		if ( NULL != mSubTexture && 0.f != mAlpha ) {
			Sizef oDestSize	= mSubTexture->getDestSize();
			Vector2i oOff	= mSubTexture->getOffset();

			if ( mScaleType == UIScaleType::Expand ) {
				mSubTexture->setOffset( Vector2i( 0, 0 ) );
				mSubTexture->setDestSize( Vector2f( mRealSize.x, mRealSize.y ) );

				autoAlign();

				drawSubTexture();

			} else if ( mScaleType == UIScaleType::FitInside ) {
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

void UISubTexture::drawSubTexture() {
	mSubTexture->draw( (Float)mScreenPos.x + mAlignOffset.x, (Float)mScreenPos.y + mAlignOffset.y, mColor, 0.f, Vector2f::One, getBlendMode(), mRender );
}

void UISubTexture::setAlpha( const Float& alpha ) {
	UIControlAnim::setAlpha( alpha );
	mColor.a = (Uint8)alpha;
}

Graphics::SubTexture * UISubTexture::getSubTexture() const {
	return mSubTexture;
}

const ColorA& UISubTexture::getColor() const {
	return mColor;
}

void UISubTexture::setColor( const ColorA& col ) {
	mColor = col;
	setAlpha( col.a );
}

const EE_RENDER_MODE& UISubTexture::getRenderMode() const {
	return mRender;
}

void UISubTexture::setRenderMode( const EE_RENDER_MODE& render ) {
	mRender = render;
}

void UISubTexture::autoAlign() {
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

void UISubTexture::onSizeChange() {
	onAutoSize();
	autoAlign();
	UIControlAnim::onSizeChange();
}

void UISubTexture::onAlignChange() {
	onAutoSize();
	autoAlign();
}

const Vector2i& UISubTexture::getAlignOffset() const {
	return mAlignOffset;
}

void UISubTexture::loadFromXmlNode(const pugi::xml_node & node) {
	UIWidget::loadFromXmlNode( node );

	for (pugi::xml_attribute_iterator ait = node.attributes_begin(); ait != node.attributes_end(); ++ait) {
		std::string name = ait->name();
		String::toLowerInPlace( name );

		if ( "src" == name || "subtexture" == name ) {
			SubTexture * res = NULL;

			if ( NULL != ( res = GlobalTextureAtlas::instance()->getByName( ait->as_string() ) ) ) {
				setSubTexture( res );
			}
		} else if ( "scaletype" == name ) {
			std::string val = ait->as_string();
			String::toLowerInPlace( val );

			if ( "expand" == val ) {
				setScaleType( UIScaleType::Expand );
			} else if ( "fit_inside" == val || "fitinside" == val ) {
				setScaleType( UIScaleType::FitInside );
			} else if ( "none" == val ) {
				setScaleType( UIScaleType::None );
			}
		}
	}
}

Uint32 UISubTexture::getScaleType() const {
	return mScaleType;
}

UISubTexture * UISubTexture::setScaleType(const Uint32& scaleType) {
	mScaleType = scaleType;
	return this;
}

}}
