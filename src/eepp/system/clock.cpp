#include <eepp/system/clock.hpp>

namespace EE { namespace System {

Clock::Clock() {
	restart();
}

Clock::~Clock() {}

void Clock::restart() {
	mRefPoint = ClockImpl::now();
}

Time Clock::getElapsedTime() const {
	return Microseconds(
		std::chrono::duration_cast<std::chrono::microseconds>( ClockImpl::now() - mRefPoint )
			.count() );
}

Time Clock::getElapsedTimeAndReset() {
	Time r = getElapsedTime();
	restart();
	return r;
}

}} // namespace EE::System
