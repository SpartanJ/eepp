#include <eepp/scene/actions/tint.hpp>
#include <eepp/ui/uiwidget.hpp>
#include <eepp/ui/uitextview.hpp>
using namespace EE::UI;

namespace EE { namespace Scene { namespace Actions {

Tint * Tint::New( const Color& start, const Color& end, const bool& interpolateAlpha, const Time& duration, const Ease::Interpolation& type, const TintType& colorInterpolationType ) {
	return eeNew( Tint, ( start, end, interpolateAlpha, duration, type, colorInterpolationType ) );
}

Tint::Tint()
{}

Interpolation1d Tint::getInterpolationA() const {
	return mInterpolationA;
}

void Tint::setInterpolationA(const Interpolation1d & interpolationA) {
	mInterpolationA = interpolationA;
}

Interpolation1d Tint::getInterpolationB() const {
	return mInterpolationB;
}

void Tint::setInterpolationB(const Interpolation1d& interpolationB) {
	mInterpolationB = interpolationB;
}

Interpolation1d Tint::getInterpolationG() const {
	return mInterpolationG;
}

void Tint::setInterpolationG(const Interpolation1d & interpolationG) {
	mInterpolationG = interpolationG;
}

Interpolation1d Tint::getInterpolationR() const {
	return mInterpolationR;
}

void Tint::setInterpolationR(const Interpolation1d & interpolationR) {
	mInterpolationR = interpolationR;
}

Tint::Tint( const Color& start, const Color & end, const bool& interpolateAlpha, const Time& duration, const Ease::Interpolation& type, const TintType& colorInterpolationType ) :
	mColorInterpolationType( colorInterpolationType ),
	mInterpolateAlpha( interpolateAlpha )
{
	mInterpolationR.clear().add( start.r, duration ).add( end.r ).setType( type );
	mInterpolationG.clear().add( start.g, duration ).add( end.g ).setType( type );
	mInterpolationB.clear().add( start.b, duration ).add( end.b ).setType( type );

	if ( interpolateAlpha )
		mInterpolationA.clear().add( start.a, duration ).add( end.a ).setType( type );
}

void Tint::start() {
	mInterpolationR.start();
	mInterpolationG.start();
	mInterpolationB.start();

	if ( mInterpolateAlpha )
		mInterpolationA.start();

	onStart();

	sendEvent( ActionType::OnStart );
}

void Tint::stop() {
	mInterpolationR.stop();
	mInterpolationG.stop();
	mInterpolationB.stop();

	if ( mInterpolateAlpha )
		mInterpolationA.stop();

	onStop();

	sendEvent( ActionType::OnStop );
}

void Tint::update( const Time& time ) {
	mInterpolationR.update( time );
	mInterpolationG.update( time );
	mInterpolationB.update( time );
	mInterpolationA.update( time );

	onUpdate( time );
}

bool Tint::isDone() {
	return mInterpolationR.ended() &&
			mInterpolationG.ended() &&
			mInterpolationB.ended() && (
			!mInterpolateAlpha ||
			mInterpolationA.ended() );
}


void Tint::onStart() {
	if ( NULL != mNode && mNode->isWidget() ) {
		onUpdate( Time::Zero );
	}
}

Action * Tint::clone() const {
	Tint * action = eeNew( Tint, () );
	action->setInterpolationR( mInterpolationR );
	action->setInterpolationG( mInterpolationG );
	action->setInterpolationB( mInterpolationB );
	action->setInterpolationA( mInterpolationA );
	return action;
}

Action * Tint::reverse() const {
	return NULL;
}

void Tint::onUpdate( const Time& ) {
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
				widget->setSkinColor( widget->getStyleState(),
						Color( mInterpolationR.getPosition(),
							  mInterpolationG.getPosition(),
							  mInterpolationB.getPosition(),
							  mInterpolateAlpha ? mInterpolationA.getPosition() : 255
				) );

				break;
			}
			case Border:
			{
				widget->setBorderColor(
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
			case TextShadow:
			{
				if ( widget->isType( UI_TYPE_TEXTVIEW ) ) {
					UITextView * textView = static_cast<UITextView*>( widget );

					textView->setFontShadowColor( Color( mInterpolationR.getPosition(),
												   mInterpolationG.getPosition(),
												   mInterpolationB.getPosition(),
												   mInterpolateAlpha ? mInterpolationA.getPosition() : 255 ) );
				}

				break;
			}
			case TextOutline:
			{
				if ( widget->isType( UI_TYPE_TEXTVIEW ) ) {
					UITextView * textView = static_cast<UITextView*>( widget );

					textView->setOutlineColor( Color( mInterpolationR.getPosition(),
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
