#include <eepp/scene/actions/marginmove.hpp>
#include <eepp/ui/uiwidget.hpp>
using namespace EE::UI;

namespace EE { namespace Scene { namespace Actions {

MarginMove* MarginMove::New( const Rect& start, const Rect& end, const Time& duration,
							 const Ease::Interpolation& type, const Uint32& interpolateFlag ) {
	return eeNew( MarginMove, ( start, end, duration, type, interpolateFlag ) );
}

MarginMove::MarginMove() {}

Interpolation1d MarginMove::getInterpolationBottom() const {
	return mInterpolationBottom;
}

void MarginMove::setInterpolationBottom( const Interpolation1d& interpolationBottom ) {
	mInterpolationBottom = interpolationBottom;
}

Interpolation1d MarginMove::getInterpolationTop() const {
	return mInterpolationTop;
}

void MarginMove::setInterpolationTop( const Interpolation1d& interpolationTop ) {
	mInterpolationTop = interpolationTop;
}

Interpolation1d MarginMove::getInterpolationRight() const {
	return mInterpolationRight;
}

void MarginMove::setInterpolationRight( const Interpolation1d& interpolationRight ) {
	mInterpolationRight = interpolationRight;
}

Interpolation1d MarginMove::getInterpolationLeft() const {
	return mInterpolationLeft;
}

void MarginMove::setInterpolationLeft( const Interpolation1d& interpolationLeft ) {
	mInterpolationLeft = interpolationLeft;
}

MarginMove::MarginMove( const Rect& start, const Rect& end, const Time& duration,
						const Ease::Interpolation& type, const Uint32& interpolateFlag ) :
	mFlags( interpolateFlag ) {
	if ( mFlags & InterpolateFlag::Left )
		mInterpolationLeft.clear().add( start.Left, duration ).add( end.Left ).setType( type );

	if ( mFlags & InterpolateFlag::Right )
		mInterpolationRight.clear().add( start.Right, duration ).add( end.Right ).setType( type );

	if ( mFlags & InterpolateFlag::Top )
		mInterpolationTop.clear().add( start.Top, duration ).add( end.Top ).setType( type );

	if ( mFlags & InterpolateFlag::Bottom )
		mInterpolationBottom.clear()
			.add( start.Bottom, duration )
			.add( end.Bottom )
			.setType( type );
}

void MarginMove::start() {
	if ( mFlags & InterpolateFlag::Left )
		mInterpolationLeft.start();

	if ( mFlags & InterpolateFlag::Right )
		mInterpolationRight.start();

	if ( mFlags & InterpolateFlag::Top )
		mInterpolationTop.start();

	if ( mFlags & InterpolateFlag::Bottom )
		mInterpolationBottom.start();

	onStart();

	sendEvent( ActionType::OnStart );
}

void MarginMove::stop() {
	if ( mFlags & InterpolateFlag::Left )
		mInterpolationLeft.stop();

	if ( mFlags & InterpolateFlag::Right )
		mInterpolationRight.stop();

	if ( mFlags & InterpolateFlag::Top )
		mInterpolationTop.stop();

	if ( mFlags & InterpolateFlag::Bottom )
		mInterpolationBottom.stop();

	onStop();

	sendEvent( ActionType::OnStop );
}

void MarginMove::update( const Time& time ) {
	if ( mFlags & InterpolateFlag::Left )
		mInterpolationLeft.update( time );

	if ( mFlags & InterpolateFlag::Right )
		mInterpolationRight.update( time );

	if ( mFlags & InterpolateFlag::Top )
		mInterpolationTop.update( time );

	if ( mFlags & InterpolateFlag::Bottom )
		mInterpolationBottom.update( time );

	onUpdate( time );
}

bool MarginMove::isDone() {
	return ( ( mFlags & InterpolateFlag::Left ) ? mInterpolationLeft.ended() : true ) &&
		   ( ( mFlags & InterpolateFlag::Right ) ? mInterpolationRight.ended() : true ) &&
		   ( ( mFlags & InterpolateFlag::Top ) ? mInterpolationTop.ended() : true ) &&
		   ( ( mFlags & InterpolateFlag::Bottom ) ? mInterpolationBottom.ended() : true );
}

void MarginMove::onStart() {
	if ( NULL != mNode && mNode->isWidget() ) {
		onUpdate( Time::Zero );
	}
}

void MarginMove::onUpdate( const Time& ) {
	if ( NULL != mNode && mNode->isWidget() ) {
		UIWidget* widget = static_cast<UIWidget*>( mNode );

		widget->setLayoutMargin(
			Rectf( ( mFlags & InterpolateFlag::Left ) ? mInterpolationLeft.getPosition()
													  : widget->getLayoutMargin().Left,
				   ( mFlags & InterpolateFlag::Top ) ? mInterpolationTop.getPosition()
													 : widget->getLayoutMargin().Top,
				   ( mFlags & InterpolateFlag::Right ) ? mInterpolationRight.getPosition()
													   : widget->getLayoutMargin().Right,
				   ( mFlags & InterpolateFlag::Bottom ) ? mInterpolationBottom.getPosition()
														: widget->getLayoutMargin().Bottom ) );
	}
}

Action* MarginMove::clone() const {
	MarginMove* action = eeNew( MarginMove, () );
	action->mFlags = mFlags;
	action->setInterpolationLeft( mInterpolationLeft );
	action->setInterpolationRight( mInterpolationRight );
	action->setInterpolationTop( mInterpolationTop );
	action->setInterpolationBottom( mInterpolationBottom );
	return action;
}

Action* MarginMove::reverse() const {
	return NULL;
}

Float MarginMove::getCurrentProgress() {
	return mInterpolationLeft.getCurrentProgress();
}

Time MarginMove::getTotalTime() {
	return mInterpolationLeft.getDuration();
}

}}} // namespace EE::Scene::Actions
