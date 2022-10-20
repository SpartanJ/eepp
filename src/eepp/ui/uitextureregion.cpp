#include <eepp/graphics/globaltextureatlas.hpp>
#include <eepp/graphics/textureatlasmanager.hpp>
#include <eepp/graphics/textureregion.hpp>
#include <eepp/ui/css/propertydefinition.hpp>
#include <eepp/ui/uitextureregion.hpp>

namespace EE { namespace UI {

UITextureRegion* UITextureRegion::New() {
	return eeNew( UITextureRegion, () );
}

UITextureRegion::UITextureRegion() :
	UIWidget( "textureregion" ),
	mTextureRegion( NULL ),
	mColor(),
	mRender( RENDER_NORMAL ),
	mAlignOffset( 0, 0 ) {
	mFlags |= UI_AUTO_SIZE;

	onAutoSize();
}

UITextureRegion::~UITextureRegion() {}

Uint32 UITextureRegion::getType() const {
	return UI_TYPE_TEXTUREREGION;
}

bool UITextureRegion::isType( const Uint32& type ) const {
	return UITextureRegion::getType() == type ? true : UIWidget::isType( type );
}

UITextureRegion* UITextureRegion::setTextureRegion( Graphics::TextureRegion* TextureRegion ) {
	mTextureRegion = TextureRegion;

	onAutoSize();

	if ( NULL != mTextureRegion && getSize().getWidth() == 0 && getSize().getHeight() == 0 ) {
		setSize( mTextureRegion->getDpSize().asFloat() );
	}

	autoAlign();

	invalidateDraw();

	return this;
}

void UITextureRegion::onAutoSize() {
	if ( NULL != mTextureRegion ) {
		if ( ( mFlags & UI_AUTO_SIZE ) && Sizef::Zero == getSize() ) {
			setInternalSize( mTextureRegion->getDpSize().asFloat() );
			autoAlign();
		}

		if ( mWidthPolicy == SizePolicy::WrapContent || mHeightPolicy == SizePolicy::WrapContent ) {
			if ( mWidthPolicy == SizePolicy::WrapContent ) {
				setInternalPixelsWidth( mTextureRegion->getPixelsSize().getWidth() +
										mPaddingPx.Left + mPaddingPx.Right );
			}

			if ( mHeightPolicy == SizePolicy::WrapContent ) {
				setInternalPixelsHeight( mTextureRegion->getPixelsSize().getHeight() +
										 mPaddingPx.Top + mPaddingPx.Bottom );
			}

			autoAlign();
		}
	}
}

void UITextureRegion::draw() {
	if ( mVisible ) {
		UINode::draw();

		if ( NULL != mTextureRegion && 0.f != mAlpha ) {
			Sizef oDestSize = mTextureRegion->getDestSize();
			Vector2i oOff = mTextureRegion->getOffset();

			if ( mScaleType == UIScaleType::Expand ) {
				mTextureRegion->setOffset( Vector2i( 0, 0 ) );
				mTextureRegion->setDestSize(
					Vector2f( (int)mSize.x - mPaddingPx.Left - mPaddingPx.Right,
							  (int)mSize.y - mPaddingPx.Top - mPaddingPx.Bottom ) );

				autoAlign();

				drawTextureRegion();

			} else if ( mScaleType == UIScaleType::FitInside ) {
				mTextureRegion->setOffset( Vector2i( 0, 0 ) );

				Sizef pxSize = mTextureRegion->getPixelsSize();
				Float Scale1 = ( mSize.x - mPaddingPx.Left - mPaddingPx.Right ) / (Float)pxSize.x;
				Float Scale2 = ( mSize.y - mPaddingPx.Top - mPaddingPx.Bottom ) / (Float)pxSize.y;

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
				mTextureRegion->setOffset(
					Vector2i( (Int32)( (Float)oOff.x / mTextureRegion->getPixelDensity() *
									   PixelDensity::getPixelDensity() ),
							  (Int32)( (Float)oOff.y / mTextureRegion->getPixelDensity() *
									   PixelDensity::getPixelDensity() ) ) );

				mTextureRegion->setDestSize( Vector2f( (Float)mTextureRegion->getPixelsSize().x,
													   (Float)mTextureRegion->getPixelsSize().y ) );

				autoAlign();

				drawTextureRegion();
			}

			mTextureRegion->setDestSize( oDestSize );
			mTextureRegion->setOffset( oOff );
		}
	}
}

void UITextureRegion::drawTextureRegion() {
	mTextureRegion->draw( (Float)mScreenPosi.x + (int)mAlignOffset.x,
						  (Float)mScreenPosi.y + (int)mAlignOffset.y, mColor, 0.f, Vector2f::One,
						  getBlendMode(), mRender );
}

void UITextureRegion::setAlpha( const Float& alpha ) {
	UIWidget::setAlpha( alpha );
	mColor.a = (Uint8)alpha;
}

Graphics::TextureRegion* UITextureRegion::getTextureRegion() const {
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

	if ( Font::getHorizontalAlign( mFlags ) == UI_HALIGN_CENTER ) {
		mAlignOffset.x = ( mSize.getWidth() - mTextureRegion->getDestSize().x ) / 2;
	} else if ( Font::getHorizontalAlign( mFlags ) == UI_HALIGN_RIGHT ) {
		mAlignOffset.x = mSize.getWidth() - mTextureRegion->getDestSize().x - mPaddingPx.Right;
	} else {
		mAlignOffset.x = mPaddingPx.Left;
	}

	if ( Font::getVerticalAlign( mFlags ) == UI_VALIGN_CENTER ) {
		mAlignOffset.y = ( mSize.getHeight() - mTextureRegion->getDestSize().y ) / 2;
	} else if ( Font::getVerticalAlign( mFlags ) == UI_VALIGN_BOTTOM ) {
		mAlignOffset.y = mSize.getHeight() - mTextureRegion->getDestSize().y - mPaddingPx.Bottom;
	} else {
		mAlignOffset.y = mPaddingPx.Top;
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

std::string UITextureRegion::getPropertyString( const PropertyDefinition* propertyDef,
												const Uint32& propertyIndex ) const {
	if ( NULL == propertyDef )
		return "";

	switch ( propertyDef->getPropertyId() ) {
		case PropertyId::Src:
			// TODO: Implement src.
			return "";
		case PropertyId::ScaleType:
			return getScaleType() == UIScaleType::FitInside
					   ? "fit-inside"
					   : ( getScaleType() == UIScaleType::Expand ? "expand" : "none" );
		case PropertyId::Tint:
			return getColor().toHexString();
		default:
			return UIWidget::getPropertyString( propertyDef, propertyIndex );
	}
}

std::vector<PropertyId> UITextureRegion::getPropertiesImplemented() const {
	auto props = UIWidget::getPropertiesImplemented();
	auto local = { PropertyId::Src, PropertyId::ScaleType, PropertyId::Tint };
	props.insert( props.end(), local.begin(), local.end() );
	return props;
}

bool UITextureRegion::applyProperty( const StyleSheetProperty& attribute ) {
	if ( !checkPropertyDefinition( attribute ) )
		return false;

	switch ( attribute.getPropertyDefinition()->getPropertyId() ) {
		case PropertyId::Src: {
			Drawable* res = NULL;
			std::string name( attribute.asString() );

			if ( String::startsWith( name, "@textureregion/" ) ) {
				name = name.substr( 12 );
			}

			if ( NULL !=
					 ( res = TextureAtlasManager::instance()->getTextureRegionByName( name ) ) &&
				 res->getDrawableType() == Drawable::TEXTUREREGION ) {
				setTextureRegion( static_cast<TextureRegion*>( res ) );
			}
			break;
		}
		case PropertyId::ScaleType: {
			std::string val = attribute.asString();
			String::toLowerInPlace( val );

			if ( "expand" == val ) {
				setScaleType( UIScaleType::Expand );
			} else if ( "fit_inside" == val || "fitinside" == val ) {
				setScaleType( UIScaleType::FitInside );
			} else if ( "none" == val ) {
				setScaleType( UIScaleType::None );
			}
			break;
		}
		case PropertyId::Tint:
			setColor( attribute.asColor() );
			break;
		default:
			return UIWidget::applyProperty( attribute );
	}

	return true;
}

const UIScaleType& UITextureRegion::getScaleType() const {
	return mScaleType;
}

UITextureRegion* UITextureRegion::setScaleType( const UIScaleType& scaleType ) {
	mScaleType = scaleType;
	invalidateDraw();
	return this;
}

}} // namespace EE::UI
