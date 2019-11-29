#include <eepp/ui/uiimage.hpp>
#include <eepp/ui/css/propertydefinition.hpp>
#include <eepp/graphics/drawable.hpp>
#include <eepp/graphics/sprite.hpp>
#include <eepp/graphics/drawablesearcher.hpp>
#include <eepp/ui/uithememanager.hpp>

namespace EE { namespace UI {

UIImage * UIImage::New() {
	return eeNew( UIImage, () );
}

UIImage *UIImage::NewWithTag( const std::string& tag ) {
	return eeNew( UIImage, ( tag ) );
}

UIImage::UIImage( const std::string& tag ) :
	UIWidget( tag ),
	mScaleType( UIScaleType::None ),
	mDrawable( NULL ),
	mColor(),
	mAlignOffset(0,0),
	mResourceChangeCb(0),
	mDrawableOwner(false)
{
	mFlags |= UI_AUTO_SIZE;

	onAutoSize();

	applyDefaultTheme();
}

UIImage::UIImage() :
	UIImage( "image" )
{}

UIImage::~UIImage() {
	safeDeleteDrawable();
}

Uint32 UIImage::getType() const {
	return UI_TYPE_IMAGE;
}

bool UIImage::isType( const Uint32& type ) const {
	return UIImage::getType() == type ? true : UIWidget::isType( type );
}

UIImage * UIImage::setDrawable(Drawable * drawable , bool ownIt ) {
	safeDeleteDrawable();

	mDrawable = drawable;
	mDrawableOwner = ownIt;

	if ( NULL != mDrawable && mDrawable->isDrawableResource() ) {
		mResourceChangeCb = static_cast<DrawableResource*>( mDrawable )->pushResourceChangeCallback( cb::Make2( this, &UIImage::onDrawableResourceEvent ) );
	}

	onAutoSize();

	if ( NULL != mDrawable && getSize().getWidth() == 0 && getSize().getHeight() == 0 ) {
		setSize( mDrawable->getSize() );
	}

	autoAlign();

	notifyLayoutAttrChange();

	invalidateDraw();

	return this;
}

void UIImage::onAutoSize() {
	if ( NULL != mDrawable ) {
		if ( ( mFlags & UI_AUTO_SIZE ) && Sizef::Zero == getSize() ) {
			setInternalSize( mDrawable->getSize() );
		}

		if ( mLayoutWidthRules == WRAP_CONTENT ) {
			setInternalPixelsWidth( (int)PixelDensity::dpToPx( mDrawable->getSize().getWidth() ) + mRealPadding.Left + mRealPadding.Right );
		}

		if ( mLayoutHeightRules == WRAP_CONTENT ) {
			setInternalPixelsHeight( (int)PixelDensity::dpToPx( mDrawable->getSize().getHeight() ) + mRealPadding.Top + mRealPadding.Bottom );
		}
	}
}

void UIImage::calcDestSize() {
	if ( mScaleType == UIScaleType::Expand ) {
		mDestSize = Sizef( mSize.x - mRealPadding.Left - mRealPadding.Right, mSize.y - mRealPadding.Top - mRealPadding.Bottom );
	} else if ( mScaleType == UIScaleType::FitInside ) {
		if ( NULL == mDrawable)
			return;

		Sizef pxSize( PixelDensity::dpToPx( mDrawable->getSize() ) );
		Float Scale1 = ( mSize.x - mRealPadding.Left - mRealPadding.Right ) / pxSize.x;
		Float Scale2 = ( mSize.y - mRealPadding.Top - mRealPadding.Bottom ) / pxSize.y;

		if ( Scale1 < 1 || Scale2 < 1 ) {
			if ( Scale2 < Scale1 )
				Scale1 = Scale2;

			mDestSize = Sizef( pxSize.x * Scale1, pxSize.y * Scale1 );
		} else {
			mDestSize = pxSize;
		}
	} else {
		if ( NULL == mDrawable)
			return;

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
			mDrawable->draw( Vector2f( (Float)mScreenPosi.x + (int)mAlignOffset.x, (Float)mScreenPosi.y + (int)mAlignOffset.y ), mDestSize );
			mDrawable->clearColor();
		}
	}
}

void UIImage::setAlpha( const Float& alpha ) {
	UINode::setAlpha( alpha );
	mColor.a = (Uint8)alpha;
}

Drawable * UIImage::getDrawable() const {
	return mDrawable;
}

const Color& UIImage::getColor() const {
	return mColor;
}

UIImage * UIImage::setColor( const Color& col ) {
	mColor = col;
	setAlpha( col.a );
	return this;
}

void UIImage::autoAlign() {
	if ( NULL == mDrawable )
		return;

	if ( HAlignGet( mFlags ) == UI_HALIGN_CENTER ) {
		mAlignOffset.x = ( mSize.getWidth()- mDestSize.x ) / 2;
	} else if ( fontHAlignGet( mFlags ) == UI_HALIGN_RIGHT ) {
		mAlignOffset.x =  mSize.getWidth() - mDestSize.x - mRealPadding.Right;
	} else {
		mAlignOffset.x = mRealPadding.Left;
	}

	if ( VAlignGet( mFlags ) == UI_VALIGN_CENTER ) {
		mAlignOffset.y = ( mSize.getHeight() - mDestSize.y ) / 2;
	} else if ( fontVAlignGet( mFlags ) == UI_VALIGN_BOTTOM ) {
		mAlignOffset.y = mSize.getHeight() - mDestSize.y - mRealPadding.Bottom;
	} else {
		mAlignOffset.y = mRealPadding.Top;
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

void UIImage::onDrawableResourceEvent( DrawableResource::Event event, DrawableResource * ) {
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

bool UIImage::applyProperty( const StyleSheetProperty& attribute, const Uint32& state ) {
	if ( !checkPropertyDefinition( attribute ) ) return false;

	switch ( attribute.getPropertyDefinition()->getPropertyId() ) {
		case PropertyId::Src:
		{
			Drawable * res = NULL;
			if ( NULL != ( res = DrawableSearcher::searchByName( attribute.asString() ) ) ) {
				if ( res->getDrawableType() == Drawable::SPRITE )
					mDrawableOwner = true;

				setDrawable( res );
			}
			break;
		}
		case PropertyId::Icon:
		{
			std::string val = attribute.asString();
			Drawable * icon = NULL;
			if ( NULL != mTheme && NULL != ( icon = mTheme->getIconByName( val ) ) ) {
				setDrawable( icon );
			} else if ( NULL != ( icon = DrawableSearcher::searchByName( val ) ) ) {
				setDrawable( icon );
			}
			break;
		}
		case PropertyId::ScaleType:
		{
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
			return UIWidget::applyProperty( attribute, state );
	}

	return true;
}

Uint32 UIImage::getScaleType() const {
	return mScaleType;
}

UIImage * UIImage::setScaleType(const Uint32& scaleType) {
	mScaleType = scaleType;
	calcDestSize();
	invalidateDraw();
	return this;
}

}}
