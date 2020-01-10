#include <eepp/scene/actions/actioninterpolation2d.hpp>
#include <eepp/scene/node.hpp>

namespace EE { namespace Scene { namespace Actions {

ActionInterpolation2d::ActionInterpolation2d() {}

void ActionInterpolation2d::setInterpolation( Interpolation2d interpolation ) {
	mInterpolation = interpolation;
}

void ActionInterpolation2d::start() {
	mInterpolation.start();

	onStart();

	sendEvent( ActionType::OnStart );
}

void ActionInterpolation2d::stop() {
	mInterpolation.stop();

	onStop();

	sendEvent( ActionType::OnStop );
}

void ActionInterpolation2d::update( const Time& time ) {
	mInterpolation.update( time );

	onUpdate( time );
}

bool ActionInterpolation2d::isDone() {
	return mInterpolation.ended();
}

Float ActionInterpolation2d::getCurrentProgress() {
	return mInterpolation.getCurrentProgress();
}

Interpolation2d* ActionInterpolation2d::getInterpolation() {
	return &mInterpolation;
}

}}} // namespace EE::Scene::Actions
