#include <eepp/math/easing.hpp>
#include <eepp/ui/css/stylesheetpropertytransition.hpp>
#include <eepp/ui/uinodedrawable.hpp>
#include <eepp/ui/uiwidget.hpp>

using namespace EE::Math::easing;

namespace EE { namespace UI { namespace CSS {

bool StyleSheetPropertyTransition::transitionSupported( const PropertyType& type ) {
	switch ( type ) {
		case PropertyType::NumberFloat:
		case PropertyType::NumberInt:
		case PropertyType::NumberLength:
		case PropertyType::Color:
		case PropertyType::Vector2:
		case PropertyType::BackgroundSize:
		case PropertyType::ForegroundSize:
			return true;
		default:
			return false;
	}
}

StyleSheetPropertyTransition* StyleSheetPropertyTransition::New( const PropertyDefinition* property,
																 const std::string& startValue,
																 const std::string& endValue,
																 const Time& duration,
																 const Ease::Interpolation& type ) {
	return eeNew( StyleSheetPropertyTransition,
				  ( property, startValue, endValue, duration, type ) );
}

StyleSheetPropertyTransition::StyleSheetPropertyTransition( const PropertyDefinition* property,
															const std::string& startValue,
															const std::string& endValue,
															const Time& duration,
															const Ease::Interpolation& type ) :
	mProperty( property ),
	mStartValue( startValue ),
	mEndValue( endValue ),
	mDuration( duration ),
	mType( type ) {
	mId = ID;
}

Action* StyleSheetPropertyTransition::clone() const {
	return StyleSheetPropertyTransition::New( mProperty, mStartValue, mEndValue, mDuration, mType );
}

Action* StyleSheetPropertyTransition::reverse() const {
	return StyleSheetPropertyTransition::New( mProperty, mEndValue, mStartValue, mDuration, mType );
}

void StyleSheetPropertyTransition::start() {
	onStart();

	sendEvent( ActionType::OnStart );
}

void StyleSheetPropertyTransition::stop() {
	onStop();

	sendEvent( ActionType::OnStop );
}

void StyleSheetPropertyTransition::update( const Time& time ) {
	mElapsed += time;

	onUpdate( time );
}

bool StyleSheetPropertyTransition::isDone() {
	return mElapsed.asMicroseconds() >= mDuration.asMicroseconds();
}

Float StyleSheetPropertyTransition::getCurrentProgress() {
	return mElapsed.asMilliseconds() / mDuration.asMilliseconds();
}

void StyleSheetPropertyTransition::onStart() {
	onUpdate( Time::Zero );
}

void StyleSheetPropertyTransition::onUpdate( const Time& ) {
	if ( NULL != mNode && mNode->isWidget() ) {
		UIWidget* node = mNode->asType<UIWidget>();

		switch ( mProperty->getType() ) {
			case PropertyType::NumberFloat:
			case PropertyType::NumberInt: {
				Float start = node->convertLength( mStartValue, 0 );
				Float end = node->convertLength( mEndValue, 0 );
				Time time =
					mElapsed.asMicroseconds() > mDuration.asMicroseconds() ? mDuration : mElapsed;
				Float value = easingCb[mType]( time.asMilliseconds(), start, end - start,
											   mDuration.asMilliseconds() );
				if ( mProperty->getType() == PropertyType::NumberFloat ) {
					node->applyProperty(
						StyleSheetProperty( mProperty, String::fromFloat( value ) ) );
				} else {
					node->applyProperty( StyleSheetProperty(
						mProperty, String::format( "%d", static_cast<int>( value ) ) ) );
				}
				break;
			}
			case PropertyType::Color: {
				Color startColor( mStartValue );
				Color endColor( mEndValue );
				Time time =
					mElapsed.asMicroseconds() > mDuration.asMicroseconds() ? mDuration : mElapsed;
				Float progress =
					easingCb[mType]( time.asMilliseconds(), 0, 1, mDuration.asMilliseconds() );
				Color resColor( startColor );
				resColor.r = static_cast<Uint8>( eemin(
					static_cast<Int32>( startColor.r + ( endColor.r - startColor.r ) * progress ),
					255 ) );
				resColor.g = static_cast<Uint8>( eemin(
					static_cast<Int32>( startColor.g + ( endColor.g - startColor.g ) * progress ),
					255 ) );
				resColor.b = static_cast<Uint8>( eemin(
					static_cast<Int32>( startColor.b + ( endColor.b - startColor.b ) * progress ),
					255 ) );
				resColor.a = static_cast<Uint8>( eemin(
					static_cast<Int32>( startColor.a + ( endColor.a - startColor.a ) * progress ),
					255 ) );
				node->applyProperty( StyleSheetProperty( mProperty, resColor.toHexString() ) );
				break;
			}
			case PropertyType::NumberLength: {
				Float containerLength = getContainerLength( node );
				Float start = node->convertLength( mStartValue, containerLength );
				Float end = node->convertLength( mEndValue, containerLength );
				Time time =
					mElapsed.asMicroseconds() > mDuration.asMicroseconds() ? mDuration : mElapsed;
				Float value = easingCb[mType]( time.asMilliseconds(), start, end - start,
											   mDuration.asMilliseconds() );
				node->applyProperty(
					StyleSheetProperty( mProperty, String::fromFloat( value, "px" ) ) );

				if ( isDone() ) {
					node->applyProperty( StyleSheetProperty( mProperty, mEndValue ) );
				}
				break;
			}
			case PropertyType::Vector2: {
				Vector2f start( StyleSheetProperty( mProperty, mStartValue ).asVector2f() );
				Vector2f end( StyleSheetProperty( mProperty, mEndValue ).asVector2f() );
				Time time =
					mElapsed.asMicroseconds() > mDuration.asMicroseconds() ? mDuration : mElapsed;
				Float x = easingCb[mType]( time.asMilliseconds(), start.x, end.x - start.x,
										   mDuration.asMilliseconds() );
				Float y = easingCb[mType]( time.asMilliseconds(), start.y, end.y - start.y,
										   mDuration.asMilliseconds() );
				node->applyProperty( StyleSheetProperty( mProperty, String::fromFloat( x ) + " " +
																		String::fromFloat( y ) ) );
				if ( isDone() ) {
					node->applyProperty( StyleSheetProperty( mProperty, mEndValue ) );
				}
				break;
			}
			case PropertyType::BackgroundSize: {
				Sizef start(
					node->getBackground()->getLayer( 0 )->calcDrawableSize( mStartValue ) );
				Sizef end( node->getBackground()->getLayer( 0 )->calcDrawableSize( mEndValue ) );
				Time time =
					mElapsed.asMicroseconds() > mDuration.asMicroseconds() ? mDuration : mElapsed;
				Float x = easingCb[mType]( time.asMilliseconds(), start.x, end.x - start.x,
										   mDuration.asMilliseconds() );
				Float y = easingCb[mType]( time.asMilliseconds(), start.y, end.y - start.y,
										   mDuration.asMilliseconds() );
				node->applyProperty(
					StyleSheetProperty( mProperty, String::fromFloat( x, "px" ) + " " +
													   String::fromFloat( y, "px" ) ) );
				if ( isDone() ) {
					node->applyProperty( StyleSheetProperty( mProperty, mEndValue ) );
				}
				break;
			}
			case PropertyType::ForegroundSize: {
				Sizef start(
					node->getForeground()->getLayer( 0 )->calcDrawableSize( mStartValue ) );
				Sizef end( node->getForeground()->getLayer( 0 )->calcDrawableSize( mEndValue ) );
				Time time =
					mElapsed.asMicroseconds() > mDuration.asMicroseconds() ? mDuration : mElapsed;
				Float x = easingCb[mType]( time.asMilliseconds(), start.x, end.x - start.x,
										   mDuration.asMilliseconds() );
				Float y = easingCb[mType]( time.asMilliseconds(), start.y, end.y - start.y,
										   mDuration.asMilliseconds() );
				node->applyProperty(
					StyleSheetProperty( mProperty, String::fromFloat( x, "px" ) + " " +
													   String::fromFloat( y, "px" ) ) );
				if ( isDone() ) {
					node->applyProperty( StyleSheetProperty( mProperty, mEndValue ) );
				}
				break;
			}
			default:
				break;
		}
	}
}

Float StyleSheetPropertyTransition::getContainerLength( UIWidget* node ) {
	Float containerLength = 0;
	switch ( mProperty->getRelativeTarget() ) {
		case PropertyRelativeTarget::ContainingBlockWidth:
			containerLength = node->getParent()->getPixelsSize().getWidth();
			break;
		case PropertyRelativeTarget::ContainingBlockHeight:
			containerLength = node->getParent()->getPixelsSize().getHeight();
		case PropertyRelativeTarget::LocalBlockWidth:
			containerLength = node->getPixelsSize().getWidth();
			break;
		case PropertyRelativeTarget::LocalBlockHeight:
			containerLength = node->getPixelsSize().getHeight();
			break;
		case PropertyRelativeTarget::BackgroundWidth:
			containerLength = node->getPixelsSize().getWidth() -
							  node->getBackground()->getLayer( 0 )->getDrawableSize().getWidth();
			break;
		case PropertyRelativeTarget::BackgroundHeight:
			containerLength = node->getPixelsSize().getHeight() -
							  node->getBackground()->getLayer( 0 )->getDrawableSize().getHeight();
			break;
		case PropertyRelativeTarget::ForegroundWidth:
			containerLength = node->getPixelsSize().getWidth() -
							  node->getForeground()->getLayer( 0 )->getDrawableSize().getWidth();
			break;
		case PropertyRelativeTarget::ForegroundHeight:
			containerLength = node->getPixelsSize().getHeight() -
							  node->getForeground()->getLayer( 0 )->getDrawableSize().getHeight();
			break;
		default:
			break;
	}
	return containerLength;
}

const Time& StyleSheetPropertyTransition::getElapsed() const {
	return mElapsed;
}

const Ease::Interpolation& StyleSheetPropertyTransition::getType() const {
	return mType;
}

const Time& StyleSheetPropertyTransition::getDuration() const {
	return mDuration;
}

const std::string& StyleSheetPropertyTransition::getEndValue() const {
	return mEndValue;
}

const std::string& StyleSheetPropertyTransition::getStartValue() const {
	return mStartValue;
}

const std::string& StyleSheetPropertyTransition::getPropertyName() const {
	return mProperty->getName();
}

}}} // namespace EE::UI::CSS
