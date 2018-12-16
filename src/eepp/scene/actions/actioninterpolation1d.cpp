#include <eepp/scene/actions/actioninterpolation1d.hpp>
#include <eepp/scene/node.hpp>

namespace EE { namespace Scene { namespace Actions {

ActionInterpolation1d::ActionInterpolation1d()
{}

void ActionInterpolation1d::setInterpolation( Interpolation1d interpolation ) {
	mInterpolation = interpolation;
}

void ActionInterpolation1d::start() {
	mInterpolation.start();

	onStart();

	sendEvent( ActionType::OnStart );
}

void ActionInterpolation1d::stop() {
	mInterpolation.stop();

	onStop();

	sendEvent( ActionType::OnStop );
}

void ActionInterpolation1d::update( const Time& time ) {
	mInterpolation.update( time );

	onUpdate( time );
}

bool ActionInterpolation1d::isDone() {
	return mInterpolation.ended();
}

Interpolation1d * ActionInterpolation1d::getInterpolation() {
	return &mInterpolation;
}

}}}
