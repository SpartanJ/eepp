#include <eepp/ui/uiloader.hpp>
#include <eepp/graphics/renderer/renderer.hpp>
#include <pugixml/pugixml.hpp>

namespace EE { namespace UI {

UILoader * UILoader::New() {
	return eeNew( UILoader, () );
}

UILoader::UILoader() :
	UIWidget(),
	mRadius(0),
	mOutlineThickness( PixelDensity::dpToPx(8) ),
	mColor( Color::Green ),
	mArcAngle(0),
	mArcStartAngle(0),
	mProgress(0),
	mMaxProgress(100),
	mAnimationSpeed(0.5f),
	mOp(1),
	mIndeterminate(true)
{
	mArc.setFillMode( DRAW_FILL );
	mCircle.setFillMode( DRAW_FILL );
	setFillColor( mColor );
}

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

	ClippingMask * clippingMask = Renderer::instance()->getClippingMask();
	clippingMask->setMaskMode( ClippingMask::Exclusive );
	clippingMask->clearMasks();
	clippingMask->appendMask( mCircle );
	clippingMask->stencilMaskEnable();

	mArc.draw();

	clippingMask->stencilMaskDisable();
}

void UILoader::update( const Time& time ) {
	UIWidget::update( time );

	if ( mIndeterminate ) {
		mArcAngle += time.asMilliseconds() * mAnimationSpeed * mOp;
		mArcStartAngle += time.asMilliseconds() * (mAnimationSpeed*1.5f);

		if ( mOp == 1 && mArcAngle > 340 ) {
			mOp = -1;
		} else if ( mOp == -1 && mArcAngle < 20 ) {
			mOp = 1;
		}

		mArc.setArcAngle( mArcAngle );
		mArc.setArcStartAngle( mArcStartAngle );
	} else {
		mArcStartAngle += time.asMilliseconds() * (mAnimationSpeed*1.5f);
		mArc.setArcStartAngle( mArcStartAngle );
	}
}

UILoader * UILoader::setOutlineThickness( const Float& thickness ) {
	mOutlineThickness = thickness;
	mCircle.setRadius( PixelDensity::dpToPx( mRadius ) - PixelDensity::dpToPx( mOutlineThickness ) );
	return this;
}

const Float& UILoader::getOutlineThickness() const {
	return mOutlineThickness;
}

UILoader * UILoader::setRadius( const Float& radius ) {
	mRadius = radius;
	Float rRadius = PixelDensity::dpToPx( radius );
	mCircle.setRadius( rRadius - PixelDensity::dpToPx( mOutlineThickness ) );
	mArc.setRadius( rRadius );
	return this;
}

const Float& UILoader::getRadius() const {
	return mRadius;
}

UILoader * UILoader::setFillColor( const Color& color ) {
	mColor = color;
	mArc.setColor( mColor );
	mCircle.setColor( mColor );
	return this;
}

const Color& UILoader::getFillColor() const {
	return mColor;
}

void UILoader::onSizeChange() {
	if ( mRadius == 0 ) {
		setRadius( eemin( mDpSize.x, mDpSize.y ) / 2.f );
	}
}

const bool& UILoader::isIndeterminate() const {
	return mIndeterminate;
}

UILoader * UILoader::setIndeterminate( const bool& indeterminate ) {
	mIndeterminate = indeterminate;
	return this;
}

UILoader * UILoader::setProgress( const Float& progress ) {
	mProgress = eemax( 0.f, eemin( progress, mMaxProgress ) );

	if ( !mIndeterminate ) {
		mArcAngle = progress / mMaxProgress * 360.f;
		mArc.setArcAngle( mArcAngle );
	}

	return this;
}

const Float& UILoader::getProgress() const {
	return mProgress;
}

const Float& UILoader::getMaxProgress() const {
	return mMaxProgress;
}

UILoader * UILoader::setMaxProgress( const Float& maxProgress ) {
	mMaxProgress = maxProgress;
	return this;
}

const Float& UILoader::getAnimationSpeed() const {
	return mAnimationSpeed;
}

UILoader * UILoader::setAnimationSpeed( const Float& animationSpeed ) {
	mAnimationSpeed = animationSpeed;
	return this;
}

Float UILoader::getArcStartAngle() const {
	return mArcStartAngle;
}

UILoader * UILoader::setArcStartAngle( const Float& arcStartAngle ) {
	mArcStartAngle = arcStartAngle;
	return this;
}

void UILoader::loadFromXmlNode(const pugi::xml_node & node) {
	beginPropertiesTransaction();

	UIWidget::loadFromXmlNode( node );

	for (pugi::xml_attribute_iterator ait = node.attributes_begin(); ait != node.attributes_end(); ++ait) {
		std::string name = ait->name();
		String::toLowerInPlace( name );

		if ( "indeterminate" == name ) {
			setIndeterminate( ait->as_bool() );
		} else if ( "maxprogress" == name ) {
			setMaxProgress( ait->as_float() );
		} else if ( "progress" == name ) {
			setProgress( ait->as_float() );
		} else if ( "fillcolor" == name ) {
			setFillColor( Color::fromString( ait->as_string() ) );
		} else if ( "radius" == name ) {
			setRadius( ait->as_float() );
		} else if ( "outlinethickness" == name ) {
			setOutlineThickness( ait->as_float() );
		} else if ( "animationspeed" == name ) {
			setAnimationSpeed( ait->as_float() );
		} else if ( "arcstartangle" == name ) {
			setArcStartAngle( ait->as_float() );
		}
	}

	endPropertiesTransaction();
}

}}
