#include <eepp/graphics/renderer/renderer.hpp>
#include <eepp/scene/scenenode.hpp>
#include <eepp/ui/css/propertydefinition.hpp>
#include <eepp/ui/uiloader.hpp>

namespace EE { namespace UI {

UILoader* UILoader::New() {
	return eeNew( UILoader, () );
}

UILoader::UILoader() :
	UIWidget( "loader" ),
	mRadius( 0 ),
	mOutlineThickness( PixelDensity::dpToPx( 8 ) ),
	mColor( Color::White ),
	mArcAngle( 0 ),
	mArcStartAngle( 0 ),
	mProgress( 0 ),
	mMaxProgress( 100 ),
	mAnimationSpeed( 0.5f ),
	mOp( 1 ),
	mIndeterminate( true ) {
	subscribeScheduledUpdate();

	mArc.setFillMode( DRAW_FILL );
	mCircle.setFillMode( DRAW_FILL );
	setFillColor( Color::White );
}

UILoader::~UILoader() {}

Uint32 UILoader::getType() const {
	return UI_TYPE_LOADER;
}

bool UILoader::isType( const Uint32& type ) const {
	return UILoader::getType() == type ? true : UIWidget::isType( type );
}

void UILoader::draw() {
	UIWidget::draw();

	Rectf rect( Vector2f( mScreenPosi.x, mScreenPosi.y ), Sizef( (int)mSize.x, (int)mSize.y ) );
	mArc.setPosition( rect.getCenter() );
	mCircle.setPosition( rect.getCenter() );

	ClippingMask* clippingMask = Renderer::instance()->getClippingMask();

	if ( mCircle.getRadius() > 0 ) {
		clippingMask->setMaskMode( ClippingMask::Exclusive );
		clippingMask->clearMasks();
		clippingMask->appendMask( mCircle );
		clippingMask->stencilMaskEnable();
	}

	mArc.draw();

	if ( mCircle.getRadius() > 0 ) {
		clippingMask->stencilMaskDisable();
	}
}

void UILoader::scheduledUpdate( const Time& time ) {
	if ( !mVisible )
		return;

	if ( mVisible && isMeOrParentTreeVisible() && mAnimationSpeed != 0.f )
		invalidateDraw();

	if ( mIndeterminate ) {
		mArcAngle += time.asMilliseconds() * mAnimationSpeed * mOp;
		mArcStartAngle += time.asMilliseconds() * ( mAnimationSpeed * 1.5f );

		if ( mOp == 1 && mArcAngle > 340 ) {
			mOp = -1;
			mArcAngle = 340;
		} else if ( mOp == -1 && mArcAngle < 20 ) {
			mOp = 1;
			mArcAngle = 20;
		}

		if ( mArcStartAngle > 360 )
			mArcStartAngle = fmod( mArcStartAngle, 360 );

		mArc.setArcAngle( mArcAngle );
		mArc.setArcStartAngle( mArcStartAngle );
	} else {
		mArcStartAngle += time.asMilliseconds() * ( mAnimationSpeed * 1.5f );
		mArc.setArcStartAngle( mArcStartAngle );
	}
}

UILoader* UILoader::setOutlineThickness( const Float& thickness ) {
	if ( thickness != mOutlineThickness ) {
		mOutlineThickness = thickness;
		mCircle.setRadius( PixelDensity::dpToPx( mRadius ) -
						   PixelDensity::dpToPx( mOutlineThickness ) );
		invalidateDraw();
	}
	return this;
}

const Float& UILoader::getOutlineThickness() const {
	return mOutlineThickness;
}

UILoader* UILoader::setRadius( const Float& radius ) {
	if ( radius != mRadius ) {
		mRadius = radius;
		Float rRadius = PixelDensity::dpToPx( radius );
		mCircle.setRadius( rRadius - PixelDensity::dpToPx( mOutlineThickness ) );
		mArc.setRadius( rRadius );
		invalidateDraw();
	}
	return this;
}

const Float& UILoader::getRadius() const {
	return mRadius;
}

UILoader* UILoader::setFillColor( const Color& color ) {
	if ( color != mColor ) {
		mColor = color;
		mArc.setColor( mColor );
		mCircle.setColor( mColor );
		invalidateDraw();
	}
	return this;
}

const Color& UILoader::getFillColor() const {
	return mColor;
}

void UILoader::onSizeChange() {
	UIWidget::onSizeChange();

	updateRadius();
}

void UILoader::onPaddingChange() {
	mRadius = 0;

	onSizeChange();

	UIWidget::onPaddingChange();
}

void UILoader::updateRadius() {
	if ( mRadius == 0 ) {
		setRadius( eemin( getSize().getWidth() - mPadding.Left - mPadding.Right,
						  getSize().getHeight() - mPadding.Top - mPadding.Bottom ) /
				   2.f );
	}
}

void UILoader::onAutoSize() {
	if ( mWidthPolicy == SizePolicy::WrapContent || mHeightPolicy == SizePolicy::WrapContent ) {
		Sizef minSize( getSize() );

		if ( mWidthPolicy == SizePolicy::WrapContent ) {
			minSize.x = eemax( minSize.x, 64.f );
		}

		if ( mHeightPolicy == SizePolicy::WrapContent ) {
			minSize.y = eemax( minSize.y, 64.f );
		}

		setInternalSize( minSize );

		updateRadius();
	}
}

const bool& UILoader::isIndeterminate() const {
	return mIndeterminate;
}

UILoader* UILoader::setIndeterminate( const bool& indeterminate ) {
	if ( indeterminate != mIndeterminate ) {
		mIndeterminate = indeterminate;
		invalidateDraw();
	}
	return this;
}

UILoader* UILoader::setProgress( const Float& progress ) {
	if ( progress != mProgress ) {
		mProgress = eemax( 0.f, eemin( progress, mMaxProgress ) );

		if ( !mIndeterminate ) {
			mArcAngle = progress / mMaxProgress * 360.f;
			mArc.setArcAngle( mArcAngle );
		}

		invalidateDraw();
	}
	return this;
}

const Float& UILoader::getProgress() const {
	return mProgress;
}

const Float& UILoader::getMaxProgress() const {
	return mMaxProgress;
}

UILoader* UILoader::setMaxProgress( const Float& maxProgress ) {
	if ( maxProgress != mMaxProgress ) {
		mMaxProgress = maxProgress;
		invalidateDraw();
	}
	return this;
}

const Float& UILoader::getAnimationSpeed() const {
	return mAnimationSpeed;
}

UILoader* UILoader::setAnimationSpeed( const Float& animationSpeed ) {
	if ( animationSpeed != mAnimationSpeed ) {
		mAnimationSpeed = animationSpeed;
		invalidateDraw();
	}
	return this;
}

std::string UILoader::getPropertyString( const PropertyDefinition* propertyDef,
										 const Uint32& propertyIndex ) const {
	if ( NULL == propertyDef )
		return "";

	switch ( propertyDef->getPropertyId() ) {
		case PropertyId::Indeterminate:
			return isIndeterminate() ? "true" : "false";
		case PropertyId::MaxProgress:
			return String::fromFloat( getMaxProgress() );
		case PropertyId::Progress:
			return String::fromFloat( getProgress() );
		case PropertyId::FillColor:
			return getFillColor().toHexString();
		case PropertyId::Radius:
			return String::fromFloat( getRadius(), "dp" );
		case PropertyId::OutlineThickness:
			return String::fromFloat( getOutlineThickness(), "dp" );
		case PropertyId::AnimationSpeed:
			return String::fromFloat( getAnimationSpeed() );
		case PropertyId::ArcStartAngle:
			return String::fromFloat( getArcStartAngle() );
		default:
			return UIWidget::getPropertyString( propertyDef, propertyIndex );
	}
}

std::vector<PropertyId> UILoader::getPropertiesImplemented() const {
	auto props = UIWidget::getPropertiesImplemented();
	auto local = { PropertyId::Indeterminate,  PropertyId::MaxProgress,
				   PropertyId::Progress,	   PropertyId::FillColor,
				   PropertyId::Radius,		   PropertyId::OutlineThickness,
				   PropertyId::AnimationSpeed, PropertyId::ArcStartAngle };
	props.insert( props.end(), local.begin(), local.end() );
	return props;
}

bool UILoader::applyProperty( const StyleSheetProperty& attribute ) {
	if ( !checkPropertyDefinition( attribute ) )
		return false;

	switch ( attribute.getPropertyDefinition()->getPropertyId() ) {
		case PropertyId::Indeterminate:
			setIndeterminate( attribute.asBool() );
			break;
		case PropertyId::MaxProgress:
			setMaxProgress( attribute.asFloat() );
			break;
		case PropertyId::Progress:
			setProgress( attribute.asFloat() );
			break;
		case PropertyId::FillColor:
			setFillColor( attribute.asColor() );
			break;
		case PropertyId::Radius:
			setRadius( attribute.asDpDimension( this ) );
			break;
		case PropertyId::OutlineThickness:
			setOutlineThickness( attribute.asDpDimension( this ) );
			break;
		case PropertyId::AnimationSpeed:
			setAnimationSpeed( attribute.asFloat() );
			break;
		case PropertyId::ArcStartAngle:
			setArcStartAngle( attribute.asFloat() );
			break;
		default:
			return UIWidget::applyProperty( attribute );
	}

	return true;
}

Float UILoader::getArcStartAngle() const {
	return mArcStartAngle;
}

UILoader* UILoader::setArcStartAngle( const Float& arcStartAngle ) {
	if ( arcStartAngle != mArcStartAngle ) {
		mArcStartAngle = arcStartAngle;
		invalidateDraw();
	}
	return this;
}

}} // namespace EE::UI
