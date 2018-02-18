#include <eepp/scene/actions/marginmove.hpp>
#include <eepp/ui/uinode.hpp>
#include <eepp/ui/uiwidget.hpp>
using namespace EE::UI;

namespace EE { namespace Scene { namespace Actions {

MarginMove * MarginMove::New( const Rect & start, const Rect & end, const Time& duration, const Ease::Interpolation& type ) {
	return eeNew( MarginMove, ( start, end, duration, type ) );
}

MarginMove::MarginMove()
{}

Interpolation1d MarginMove::getInterpolationBottom() const {
	return mInterpolationBottom;
}

void MarginMove::setInterpolationBottom(const Interpolation1d & interpolationBottom) {
	mInterpolationBottom = interpolationBottom;
}

Interpolation1d MarginMove::getInterpolationTop() const {
	return mInterpolationTop;
}

void MarginMove::setInterpolationTop(const Interpolation1d & interpolationTop) {
	mInterpolationTop = interpolationTop;
}

Interpolation1d MarginMove::getInterpolationRight() const {
	return mInterpolationRight;
}

void MarginMove::setInterpolationRight(const Interpolation1d & interpolationRight) {
	mInterpolationRight = interpolationRight;
}

Interpolation1d MarginMove::getInterpolationLeft() const {
	return mInterpolationLeft;
}

void MarginMove::setInterpolationLeft(const Interpolation1d & interpolationLeft) {
	mInterpolationLeft = interpolationLeft;
}

MarginMove::MarginMove( const Rect& start, const Rect & end, const Time& duration, const Ease::Interpolation& type ) {
	mInterpolationLeft.clear().add( start.Left, duration ).add( end.Left ).setType( type );
	mInterpolationRight.clear().add( start.Right, duration ).add( end.Right ).setType( type );
	mInterpolationTop.clear().add( start.Top, duration ).add( end.Top ).setType( type );
	mInterpolationBottom.clear().add( start.Bottom, duration ).add( end.Bottom ).setType( type );
}

void MarginMove::start() {
	mInterpolationLeft.start();
	mInterpolationRight.start();
	mInterpolationTop.start();
	mInterpolationBottom.start();

	onStart();

	sendEvent( ActionType::OnStart );
}

void MarginMove::stop() {
	mInterpolationLeft.stop();
	mInterpolationRight.stop();
	mInterpolationTop.stop();
	mInterpolationBottom.stop();

	onStop();

	sendEvent( ActionType::OnStop );
}

void MarginMove::update( const Time& time ) {
	mInterpolationLeft.update( time );
	mInterpolationRight.update( time );
	mInterpolationTop.update( time );
	mInterpolationBottom.update( time );

	onUpdate( time );
}

bool MarginMove::isDone() {
	return mInterpolationLeft.ended() &&
			mInterpolationRight.ended() &&
			mInterpolationTop.ended() &&
			mInterpolationBottom.ended();
}


void MarginMove::onStart() {
	if ( NULL != mNode && mNode->isWidget() ) {
		onUpdate( Time::Zero );
	}
}

void MarginMove::onUpdate( const Time& time ) {
	if ( NULL != mNode && mNode->isWidget() ) {
		static_cast<UIWidget*>( mNode )->setLayoutMargin(
											Rect( mInterpolationLeft.getPosition(),
												  mInterpolationTop.getPosition(),
												  mInterpolationRight.getPosition(),
												  mInterpolationBottom.getPosition()
											) );
	}
}

Action * MarginMove::clone() const {
	MarginMove * action = eeNew( MarginMove, () );
	action->setInterpolationLeft( mInterpolationLeft );
	action->setInterpolationRight( mInterpolationRight );
	action->setInterpolationTop( mInterpolationTop );
	action->setInterpolationBottom( mInterpolationBottom );
	return action;
}

Action * MarginMove::reverse() const {
	return NULL;
}

}}} 
