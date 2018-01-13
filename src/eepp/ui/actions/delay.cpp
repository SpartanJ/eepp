#include <eepp/ui/actions/delay.hpp>

namespace EE { namespace UI { namespace Action {

Delay * Delay::New(const Time & time) {
	return eeNew( Delay, ( time ) );
}

void Delay::start() {
	mClock.restart();

	sendEvent( ActionType::OnStart );
}

void Delay::stop() {
	// 'Cause none of them can stop the time
	sendEvent( ActionType::OnStop );
}

void Delay::update(const Time & time)
{}

bool Delay::isDone() {
	return mClock.getElapsedTime() >= mTime;
}

UIAction *Delay::clone() const {
	return New( mTime );
}

UIAction *Delay::reverse() const {
	return NULL; // or a time machine
}

Delay::Delay(const Time & time) :
	mTime( time )
{}

}}}
