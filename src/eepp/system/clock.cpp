#include <eepp/system/clock.hpp>
#include <eepp/system/platform/platformimpl.hpp>

namespace EE { namespace System {

Clock::Clock() : mClockImpl( new Platform::ClockImpl() ) {
	restart();
}

Clock::~Clock() {
	delete mClockImpl;
}

void Clock::restart() {
	mClockImpl->restart();
}

Time Clock::getElapsedTime() const {
	return Microseconds( mClockImpl->getElapsedTime() );
}

Time Clock::getElapsed() {
	Time r = getElapsedTime();
	restart();
	return r;
}

}} // namespace EE::System
