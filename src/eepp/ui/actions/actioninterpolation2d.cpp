#include <eepp/ui/actions/actioninterpolation2d.hpp>
#include <eepp/ui/uinode.hpp>

namespace EE { namespace UI { namespace Action {

ActionInterpolation2d::ActionInterpolation2d()
{}

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

Interpolation2d * ActionInterpolation2d::getInterpolation() {
	return &mInterpolation;
}

}}}
