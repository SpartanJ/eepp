#include <eepp/system/clock.hpp>

#include <algorithm>
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
	// A clock started on one CPU and read on another can appear to run backwards where the
	// steady clock is not synchronized across CPUs (Haiku on KVM: several ms). Elapsed time is
	// never negative, and zero-delay actions must not wait for an extra frame.
	return Microseconds( std::max<Int64>( 0, nowMicroseconds() - mRefPoint ) );
}

Time Clock::getElapsedTimeAndReset() {
	Time r = getElapsedTime();
	restart();
	return r;
}

}} // namespace EE::System
