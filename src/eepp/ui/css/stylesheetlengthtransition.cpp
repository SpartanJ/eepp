#include <eepp/ui/css/stylesheetlengthtransition.hpp>
#include <eepp/ui/uiwidget.hpp>
#include <eepp/math/easing.hpp>

using namespace EE::Math::easing;

namespace EE { namespace UI { namespace CSS {

StyleSheetLengthTransition * StyleSheetLengthTransition::New( const std::string& propertyName, const std::string& startValue, const std::string& endValue, const Time& duration, const Ease::Interpolation& type ) {
	return eeNew( StyleSheetLengthTransition, ( propertyName, startValue, endValue, duration, type ) );
}

StyleSheetLengthTransition::StyleSheetLengthTransition( const std::string& propertyName, const std::string& startValue, const std::string& endValue, const Time& duration, const Ease::Interpolation& type ) :
	mPropertyName( propertyName ),
	mStartValue( startValue ),
	mEndValue( endValue ),
	mDuration( duration ),
	mType( type )
{}

Action * StyleSheetLengthTransition::clone() const {
	return &StyleSheetLengthTransition::New( mPropertyName, mStartValue, mEndValue, mDuration, mType )
			->setContainerLength( mContainerLength )
			.setContainerLengthFunction( mContainerLengthFunction );
}

Action * StyleSheetLengthTransition::reverse() const {
	return &StyleSheetLengthTransition::New( mPropertyName, mEndValue, mStartValue, mDuration, mType )
			->setContainerLength( mContainerLength )
			.setContainerLengthFunction( mContainerLengthFunction );
}

void StyleSheetLengthTransition::start() {
	onStart();

	sendEvent( ActionType::OnStart );
}

void StyleSheetLengthTransition::stop() {
	onStop();

	sendEvent( ActionType::OnStop );
}

void StyleSheetLengthTransition::update( const Time& time ) {
	mElapsed += time;

	onUpdate( time );
}

bool StyleSheetLengthTransition::isDone() {
	return mElapsed.asMicroseconds() >= mDuration.asMicroseconds();
}

Float StyleSheetLengthTransition::getCurrentProgress() {
	return mElapsed.asMilliseconds() / mDuration.asMilliseconds();
}

void StyleSheetLengthTransition::onStart() {
	onUpdate( Time::Zero );
}

void StyleSheetLengthTransition::onUpdate( const Time& ) {
	if ( NULL != mNode && mNode->isWidget() ) {
		UIWidget * node = mNode->asType<UIWidget>();
		Float start = node->convertLength( mStartValue, mContainerLengthFunction ? mContainerLengthFunction() : mContainerLength );
		Float end = node->convertLength( mEndValue, mContainerLengthFunction ? mContainerLengthFunction() :  mContainerLength );
		Time time = mElapsed.asMicroseconds() > mDuration.asMicroseconds() ? mDuration : mElapsed;
		Float value = easingCb[ mType ]( time.asMilliseconds(), start, end - start, mDuration.asMilliseconds() );
		node->setStyleSheetProperty( mPropertyName, String::format( "%.f", value ) );
	}
}

const Float& StyleSheetLengthTransition::getContainerLength() const {
	return mContainerLength;
}

StyleSheetLengthTransition& StyleSheetLengthTransition::setContainerLength( const Float& containerLength ) {
	mContainerLength = containerLength;
	return *this;
}

StyleSheetLengthTransition& StyleSheetLengthTransition::setContainerLengthFunction( const StyleSheetLengthTransition::ContainerLengthProvider& containerLengthProvider ) {
	mContainerLengthFunction = containerLengthProvider;
	return *this;
}

const Time& StyleSheetLengthTransition::getElapsed() const {
	return mElapsed;
}

const Ease::Interpolation& StyleSheetLengthTransition::getType() const {
	return mType;
}

const Time& StyleSheetLengthTransition::getDuration() const {
	return mDuration;
}

const std::string& StyleSheetLengthTransition::getEndValue() const {
	return mEndValue;
}

const std::string& StyleSheetLengthTransition::getStartValue() const {
	return mStartValue;
}

const std::string& StyleSheetLengthTransition::getPropertyName() const {
	return mPropertyName;
}

}}}
