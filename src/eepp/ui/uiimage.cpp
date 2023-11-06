#include <eepp/graphics/drawable.hpp>
#include <eepp/graphics/drawablesearcher.hpp>
#include <eepp/graphics/sprite.hpp>
#include <eepp/ui/css/drawableimageparser.hpp>
#include <eepp/ui/css/propertydefinition.hpp>
#include <eepp/ui/uiicon.hpp>
#include <eepp/ui/uiimage.hpp>
#include <eepp/ui/uiscenenode.hpp>

namespace EE { namespace UI {

UIImage* UIImage::New() {
	return eeNew( UIImage, () );
}

UIImage* UIImage::NewWithTag( const std::string& tag ) {
	return eeNew( UIImage, ( tag ) );
}

UIImage::UIImage( const std::string& tag ) :
	UIWidget( tag ),
	mScaleType( UIScaleType::None ),
	mDrawable( NULL ),
	mColor(),
	mAlignOffset( 0, 0 ),
	mResourceChangeCb( 0 ),
	mDrawableOwner( false ) {
	mFlags |= UI_AUTO_SIZE;

	applyDefaultTheme();
}

UIImage::UIImage() : UIImage( "image" ) {}

UIImage::~UIImage() {
	safeDeleteDrawable();
}

Uint32 UIImage::getType() const {
	return UI_TYPE_IMAGE;
}

bool UIImage::isType( const Uint32& type ) const {
	return UIImage::getType() == type ? true : UIWidget::isType( type );
}

UIImage* UIImage::setDrawable( Drawable* drawable, bool ownIt ) {
	if ( drawable == mDrawable )
		return this;

	safeDeleteDrawable();

	mDrawable = drawable;
	mDrawableOwner = ownIt;
	sendCommonEvent( Event::OnResourceChange );

	if ( NULL != mDrawable && mDrawable->isDrawableResource() ) {
		mResourceChangeCb = static_cast<DrawableResource*>( mDrawable )
								->pushResourceChangeCallback( [this]( auto, auto event, auto res ) {
									onDrawableResourceEvent( event, res );
								} );
	}

	onAutoSize();

	autoAlign();

	invalidateDraw();

	return this;
}

void UIImage::onAutoSize() {
	if ( NULL != mDrawable ) {
		if ( ( mFlags & UI_AUTO_SIZE ) && Sizef::Zero == getSize() )
			setInternalSize( mDrawable->getSize().asInt().asFloat() );

		Sizef size( getSize() );

		if ( mWidthPolicy == SizePolicy::WrapContent )
			size.x = ( (int)mDrawable->getSize().getWidth() + mPadding.Left + mPadding.Right );

		if ( mHeightPolicy == SizePolicy::WrapContent )
			size.y = ( (int)mDrawable->getSize().getHeight() + mPadding.Top + mPadding.Bottom );

		setSize( size );
	}
}

void UIImage::calcDestSize() {
	if ( mScaleType == UIScaleType::Expand ) {
		mDestSize = Sizef( mSize.x - mPaddingPx.Left - mPaddingPx.Right,
						   mSize.y - mPaddingPx.Top - mPaddingPx.Bottom );
	} else if ( mScaleType == UIScaleType::FitInside ) {
		if ( NULL == mDrawable )
			return;

		Sizef pxSize( mDrawable->getPixelsSize() );
		Float Scale1 = ( mSize.x - mPaddingPx.Left - mPaddingPx.Right ) / pxSize.x;
		Float Scale2 = ( mSize.y - mPaddingPx.Top - mPaddingPx.Bottom ) / pxSize.y;

		if ( Scale1 < 1 || Scale2 < 1 ) {
			if ( Scale2 < Scale1 )
				Scale1 = Scale2;

			mDestSize = Sizef( pxSize.x * Scale1, pxSize.y * Scale1 );
		} else {
			mDestSize = pxSize;
		}
	} else if ( mDrawable ) {
		mDestSize = mDrawable->getSize();
	}

	mDestSize = mDestSize.floor();

	autoAlign();
}

void UIImage::draw() {
	UINode::draw();

	if ( mVisible ) {
		if ( NULL != mDrawable && 0.f != mAlpha ) {
			calcDestSize();

			mDrawable->setColor( mColor );
			mDrawable->draw( Vector2f( (Float)mScreenPosi.x + (int)mAlignOffset.x,
									   (Float)mScreenPosi.y + (int)mAlignOffset.y ),
							 mDestSize );
			mDrawable->clearColor();
		}
	}
}

void UIImage::setAlpha( const Float& alpha ) {
	UINode::setAlpha( alpha );
	mColor.a = (Uint8)alpha;
}

Drawable* UIImage::getDrawable() const {
	return mDrawable;
}

const Color& UIImage::getColor() const {
	return mColor;
}

UIImage* UIImage::setColor( const Color& col ) {
	if ( mColor != col ) {
		mColor = col;
		setAlpha( col.a );
		invalidateDraw();
	}
	return this;
}

void UIImage::autoAlign() {
	if ( NULL == mDrawable )
		return;

	if ( Font::getHorizontalAlign( mFlags ) == UI_HALIGN_CENTER ) {
		mAlignOffset.x = ( mSize.getWidth() - mDestSize.x ) / 2;
	} else if ( Font::getHorizontalAlign( mFlags ) == UI_HALIGN_RIGHT ) {
		mAlignOffset.x = mSize.getWidth() - mDestSize.x - mPaddingPx.Right;
	} else {
		mAlignOffset.x = mPaddingPx.Left;
	}

	if ( Font::getVerticalAlign( mFlags ) == UI_VALIGN_CENTER ) {
		mAlignOffset.y = ( mSize.getHeight() - mDestSize.y ) / 2;
	} else if ( Font::getVerticalAlign( mFlags ) == UI_VALIGN_BOTTOM ) {
		mAlignOffset.y = mSize.getHeight() - mDestSize.y - mPaddingPx.Bottom;
	} else {
		mAlignOffset.y = mPaddingPx.Top;
	}
}

void UIImage::safeDeleteDrawable() {
	if ( NULL != mDrawable && mDrawable->isDrawableResource() ) {
		static_cast<DrawableResource*>( mDrawable )->popResourceChangeCallback( mResourceChangeCb );
		mResourceChangeCb = 0;
	}

	if ( NULL != mDrawable && mDrawableOwner ) {
		eeSAFE_DELETE( mDrawable );

		mDrawableOwner = false;
	}
}

void UIImage::onDrawableResourceEvent( DrawableResource::Event event, DrawableResource* ) {
	if ( event == DrawableResource::Change ) {
		invalidateDraw();
	} else if ( event == DrawableResource::Unload ) {
		mDrawable = NULL;
	}
}

void UIImage::onSizeChange() {
	onAutoSize();
	calcDestSize();
	UIWidget::onSizeChange();
}

void UIImage::onAlignChange() {
	UIWidget::onAlignChange();
	onAutoSize();
	calcDestSize();
}

const Vector2f& UIImage::getAlignOffset() const {
	return mAlignOffset;
}

std::string UIImage::getPropertyString( const PropertyDefinition* propertyDef,
										const Uint32& propertyIndex ) const {
	if ( NULL == propertyDef )
		return "";

	switch ( propertyDef->getPropertyId() ) {
		case PropertyId::Src:
			// TODO: Implement src.
			return "";
		case PropertyId::Icon:
			// TODO: Implement icon.
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

std::vector<PropertyId> UIImage::getPropertiesImplemented() const {
	auto props = UIWidget::getPropertiesImplemented();
	auto local = { PropertyId::ScaleType, PropertyId::Tint };
	props.insert( props.end(), local.begin(), local.end() );
	return props;
}

bool UIImage::applyProperty( const StyleSheetProperty& attribute ) {
	if ( !checkPropertyDefinition( attribute ) )
		return false;

	switch ( attribute.getPropertyDefinition()->getPropertyId() ) {
		case PropertyId::Src: {
			std::string path( attribute.getValue() );
			bool ownIt;
			Drawable* createdDrawable =
				CSS::StyleSheetSpecification::instance()->getDrawableImageParser().createDrawable(
					path, mSize, ownIt, this );
			if ( createdDrawable ) {
				setDrawable( createdDrawable, ownIt );
			} else {
				Drawable* res = NULL;
				if ( NULL != ( res = DrawableSearcher::searchByName( path ) ) )
					setDrawable( res, res->getDrawableType() == Drawable::SPRITE );
			}
			break;
		}
		case PropertyId::Icon: {
			std::string val = attribute.asString();
			Drawable* icon = NULL;
			bool ownIt;
			UIIcon* iconF = getUISceneNode()->findIcon( val );
			if ( iconF ) {
				setDrawable(
					iconF->getSize( mSize.getHeight() - mPaddingPx.Top - mPadding.Bottom ) );
			} else if ( NULL !=
						( icon = CSS::StyleSheetSpecification::instance()
									 ->getDrawableImageParser()
									 .createDrawable( val, getPixelsSize(), ownIt, this ) ) ) {
				setDrawable( icon, ownIt );
			}
			break;
		}
		case PropertyId::ScaleType: {
			std::string val = attribute.asString();
			String::toLowerInPlace( val );
			if ( "expand" == val ) {
				setScaleType( UIScaleType::Expand );
			} else if ( "fit-inside" == val || "fit_inside" == val || "fitinside" == val ) {
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

const UIScaleType& UIImage::getScaleType() const {
	return mScaleType;
}

UIImage* UIImage::setScaleType( const UIScaleType& scaleType ) {
	mScaleType = scaleType;
	calcDestSize();
	invalidateDraw();
	return this;
}

}} // namespace EE::UI
