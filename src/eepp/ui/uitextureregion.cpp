#include <eepp/ui/uitextureregion.hpp>
#include <eepp/graphics/textureregion.hpp>
#include <pugixml/pugixml.hpp>
#include <eepp/graphics/globaltextureatlas.hpp>

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

	if ( NULL != mTextureRegion && mSize.x == 0 && mSize.y == 0 ) {
		setSize( mTextureRegion->getDpSize() );
	}

	autoAlign();

	notifyLayoutAttrChange();

	invalidateDraw();

	return this;
}

void UITextureRegion::onAutoSize() {
	if ( ( mFlags & UI_AUTO_SIZE ) && Sizei::Zero == mSize ) {
		if ( NULL != mTextureRegion ) {
			setSize( mTextureRegion->getDpSize() );
		}
	}
}

void UITextureRegion::draw() {
	UINode::draw();

	if ( mVisible ) {
		if ( NULL != mTextureRegion && 0.f != mAlpha ) {
			Sizef oDestSize	= mTextureRegion->getDestSize();
			Vector2i oOff	= mTextureRegion->getOffset();

			if ( mScaleType == UIScaleType::Expand ) {
				mTextureRegion->setOffset( Vector2i( 0, 0 ) );
				mTextureRegion->setDestSize( Vector2f( mRealSize.x, mRealSize.y ) );

				autoAlign();

				drawTextureRegion();

			} else if ( mScaleType == UIScaleType::FitInside ) {
				mTextureRegion->setOffset( Vector2i( 0, 0 ) );

				Sizei pxSize = mTextureRegion->getPxSize();
				Float Scale1 = mRealSize.x / (Float)pxSize.x;
				Float Scale2 = mRealSize.y / (Float)pxSize.y;

				if ( Scale1 < 1 || Scale2 < 1 ) {
					if ( Scale2 < Scale1 )
						Scale1 = Scale2;

					mTextureRegion->setDestSize( Sizef( pxSize.x * Scale1, pxSize.y * Scale1 ) );

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
	mTextureRegion->draw( (Float)mScreenPos.x + mAlignOffset.x, (Float)mScreenPos.y + mAlignOffset.y, mColor, 0.f, Vector2f::One, getBlendMode(), mRender );
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
		mAlignOffset.x = mRealSize.getWidth() / 2 - mTextureRegion->getDestSize().x / 2;
	} else if ( fontHAlignGet( mFlags ) == UI_HALIGN_RIGHT ) {
		mAlignOffset.x =  mRealSize.getWidth() - mTextureRegion->getDestSize().x;
	} else {
		mAlignOffset.x = 0;
	}

	if ( VAlignGet( mFlags ) == UI_VALIGN_CENTER ) {
		mAlignOffset.y = mRealSize.getHeight() / 2 - mTextureRegion->getDestSize().y / 2;
	} else if ( fontVAlignGet( mFlags ) == UI_VALIGN_BOTTOM ) {
		mAlignOffset.y = mRealSize.getHeight() - mTextureRegion->getDestSize().y;
	} else {
		mAlignOffset.y = 0;
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

const Vector2i& UITextureRegion::getAlignOffset() const {
	return mAlignOffset;
}

void UITextureRegion::loadFromXmlNode(const pugi::xml_node & node) {
	beginPropertiesTransaction();

	UIWidget::loadFromXmlNode( node );

	for (pugi::xml_attribute_iterator ait = node.attributes_begin(); ait != node.attributes_end(); ++ait) {
		std::string name = ait->name();
		String::toLowerInPlace( name );

		if ( "src" == name || "textureregion" == name || "subtexture" == name ) {
			DrawableResource * res = NULL;

			if ( NULL != ( res = GlobalTextureAtlas::instance()->getByName( ait->as_string() ) ) && res->getDrawableType() == Drawable::TEXTUREREGION ) {
				setTextureRegion( static_cast<TextureRegion*>( res ) );
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

	endPropertiesTransaction();
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
