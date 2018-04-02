#include <eepp/scene/actions/delay.hpp>

namespace EE { namespace Scene { namespace Actions {

Delay * Delay::New(const Time & time) {
	return eeNew( Delay, ( time ) );
}

void Delay::start() {
	mClock.restart();

	onStart();

	sendEvent( ActionType::OnStart );
}

void Delay::stop() {
	// 'Cause none of them can stop the time
	onStop();

	sendEvent( ActionType::OnStop );
}

void Delay::update(const Time & time)
{}

bool Delay::isDone() {
	return mClock.getElapsedTime() >= mTime;
}

Action *Delay::clone() const {
	return New( mTime );
}

Action *Delay::reverse() const {
	return NULL; // or a time machine
}

Delay::Delay(const Time & time) :
	mTime( time )
{}

}}}
