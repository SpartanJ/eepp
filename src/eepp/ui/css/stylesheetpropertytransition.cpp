#include <eepp/math/easing.hpp>
#include <eepp/ui/css/stylesheetpropertytransition.hpp>
#include <eepp/ui/uiwidget.hpp>

using namespace EE::Math::easing;

namespace EE { namespace UI { namespace CSS {

bool StyleSheetPropertyTransition::transitionSupported( const PropertyType& type ) {
	switch ( type ) {
	case PropertyType::NumberFloat:
	case PropertyType::NumberInt:
	case PropertyType::NumberLength:
	case PropertyType::Color:
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
	mType( type ) {}

Action* StyleSheetPropertyTransition::clone() const {
	return StyleSheetPropertyTransition::New( mProperty, mStartValue, mEndValue, mDuration,
											   mType );
}

Action* StyleSheetPropertyTransition::reverse() const {
	return StyleSheetPropertyTransition::New( mProperty, mEndValue, mStartValue, mDuration,
											   mType );
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
					StyleSheetProperty( mProperty, String::format( "%fpx", value ) ) );
			} else {
				node->applyProperty( StyleSheetProperty(
					mProperty, String::format( "%dpx", static_cast<int>( value ) ) ) );
			}
			break;
		}
		case PropertyType::Color: {
			break;
		}
		case PropertyType::NumberLength: {
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
			default:
				break;
			}
			Float start = node->convertLength( mStartValue, containerLength );
			Float end = node->convertLength( mEndValue, containerLength );
			Time time =
				mElapsed.asMicroseconds() > mDuration.asMicroseconds() ? mDuration : mElapsed;
			Float value = easingCb[mType]( time.asMilliseconds(), start, end - start,
										   mDuration.asMilliseconds() );
			node->applyProperty( StyleSheetProperty( mProperty, String::format( "%fpx", value ) ) );
			break;
		}
		default:
			break;
		}
	}
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
