#include <eepp/system/clock.hpp>

#include <chrono>

namespace EE { namespace System {

using ClockImpl = std::conditional_t<std::chrono::high_resolution_clock::is_steady,
									 std::chrono::high_resolution_clock, std::chrono::steady_clock>;

static Int64 nowMicroseconds() {
	return std::chrono::duration_cast<std::chrono::microseconds>(
			   ClockImpl::now().time_since_epoch() )
		.count();
}

Clock::Clock() {
	restart();
}

Clock::~Clock() {}

void Clock::restart() {
	mRefPoint = nowMicroseconds();
}

Time Clock::getElapsedTime() const {
	return Microseconds( nowMicroseconds() - mRefPoint );
}

Time Clock::getElapsedTimeAndReset() {
	Time r = getElapsedTime();
	restart();
	return r;
}

}} // namespace EE::System
