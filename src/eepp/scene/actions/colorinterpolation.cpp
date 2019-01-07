#include <eepp/scene/actions/colorinterpolation.hpp>
#include <eepp/ui/uiwidget.hpp>
#include <eepp/ui/uitextview.hpp>
using namespace EE::UI;

namespace EE { namespace Scene { namespace Actions {

ColorInterpolation * ColorInterpolation::New( const Color& start, const Color& end, const bool& interpolateAlpha, const Time& duration, const Ease::Interpolation& type, const ColorInterpolationType& colorInterpolationType ) {
	return eeNew( ColorInterpolation, ( start, end, interpolateAlpha, duration, type, colorInterpolationType ) );
}

ColorInterpolation::ColorInterpolation()
{}

Interpolation1d ColorInterpolation::getInterpolationA() const {
	return mInterpolationA;
}

void ColorInterpolation::setInterpolationA(const Interpolation1d & interpolationA) {
	mInterpolationA = interpolationA;
}

Interpolation1d ColorInterpolation::getInterpolationB() const {
	return mInterpolationB;
}

void ColorInterpolation::setInterpolationB(const Interpolation1d& interpolationB) {
	mInterpolationB = interpolationB;
}

Interpolation1d ColorInterpolation::getInterpolationG() const {
	return mInterpolationG;
}

void ColorInterpolation::setInterpolationG(const Interpolation1d & interpolationG) {
	mInterpolationG = interpolationG;
}

Interpolation1d ColorInterpolation::getInterpolationR() const {
	return mInterpolationR;
}

void ColorInterpolation::setInterpolationR(const Interpolation1d & interpolationR) {
	mInterpolationR = interpolationR;
}

ColorInterpolation::ColorInterpolation( const Color& start, const Color & end, const bool& interpolateAlpha, const Time& duration, const Ease::Interpolation& type, const ColorInterpolationType& colorInterpolationType ) :
	mColorInterpolationType( colorInterpolationType ),
	mInterpolateAlpha( interpolateAlpha )
{
	mInterpolationR.clear().add( start.r, duration ).add( end.r ).setType( type );
	mInterpolationG.clear().add( start.g, duration ).add( end.g ).setType( type );
	mInterpolationB.clear().add( start.b, duration ).add( end.b ).setType( type );

	if ( interpolateAlpha )
		mInterpolationA.clear().add( start.a, duration ).add( end.a ).setType( type );
}

void ColorInterpolation::start() {
	mInterpolationR.start();
	mInterpolationG.start();
	mInterpolationB.start();

	if ( mInterpolateAlpha )
		mInterpolationA.start();

	onStart();

	sendEvent( ActionType::OnStart );
}

void ColorInterpolation::stop() {
	mInterpolationR.stop();
	mInterpolationG.stop();
	mInterpolationB.stop();

	if ( mInterpolateAlpha )
		mInterpolationA.stop();

	onStop();

	sendEvent( ActionType::OnStop );
}

void ColorInterpolation::update( const Time& time ) {
	mInterpolationR.update( time );
	mInterpolationG.update( time );
	mInterpolationB.update( time );
	mInterpolationA.update( time );

	onUpdate( time );
}

bool ColorInterpolation::isDone() {
	return mInterpolationR.ended() &&
			mInterpolationG.ended() &&
			mInterpolationB.ended() && (
			!mInterpolateAlpha ||
			mInterpolationA.ended() );
}


void ColorInterpolation::onStart() {
	if ( NULL != mNode && mNode->isWidget() ) {
		onUpdate( Time::Zero );
	}
}

Action * ColorInterpolation::clone() const {
	ColorInterpolation * action = eeNew( ColorInterpolation, () );
	action->setInterpolationR( mInterpolationR );
	action->setInterpolationG( mInterpolationG );
	action->setInterpolationB( mInterpolationB );
	action->setInterpolationA( mInterpolationA );
	return action;
}

Action * ColorInterpolation::reverse() const {
	return NULL;
}

void ColorInterpolation::onUpdate( const Time& ) {
	if ( NULL != mNode && mNode->isWidget() ) {
		UIWidget * widget = static_cast<UIWidget*>( mNode );

		switch ( mColorInterpolationType ) {
			case Background:
			{
				widget->setBackgroundColor( widget->getStyleState(),
						Color( mInterpolationR.getPosition(),
							  mInterpolationG.getPosition(),
							  mInterpolationB.getPosition(),
							  mInterpolateAlpha ? mInterpolationA.getPosition() : 255
				) );

				break;
			}
			case Foreground:
			{
				widget->setForegroundColor( widget->getStyleState(),
						Color( mInterpolationR.getPosition(),
							  mInterpolationG.getPosition(),
							  mInterpolationB.getPosition(),
							  mInterpolateAlpha ? mInterpolationA.getPosition() : 255
				) );

				break;
			}
			case Skin:
			{
				/*
				widget->setSkinColor( widget->getStyleState(),
						Color( mInterpolationR.getPosition(),
							  mInterpolationG.getPosition(),
							  mInterpolationB.getPosition(),
							  mInterpolateAlpha ? mInterpolationA.getPosition() : 255
				) );
				*/
				break;
			}
			case Border:
			{
				widget->setBorderColor( widget->getStyleState(),
						Color( mInterpolationR.getPosition(),
							  mInterpolationG.getPosition(),
							  mInterpolationB.getPosition(),
							  mInterpolateAlpha ? mInterpolationA.getPosition() : 255
				) );

				break;
			}
			case Text:
			{
				if ( widget->isType( UI_TYPE_TEXTVIEW ) ) {
					UITextView * textView = static_cast<UITextView*>( widget );

					textView->setFontColor( Color( mInterpolationR.getPosition(),
												   mInterpolationG.getPosition(),
												   mInterpolationB.getPosition(),
												   mInterpolateAlpha ? mInterpolationA.getPosition() : 255 ) );
				}

				break;
			}
		}
	}
}

}}}
