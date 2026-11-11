#include <eepp/system/clock.hpp>

#include <algorithm>

namespace EE { namespace System {

Clock::Clock() {
	restart();
}

Clock::~Clock() {}

void Clock::restart() {
	mRefPoint = MonotonicClock::now();
}

Time Clock::getElapsedTime() const {
	// A clock started on one CPU and read on another can appear to run backwards where the
	// steady clock is not synchronized across CPUs (Haiku on KVM: several ms). Elapsed time is
	// never negative, and zero-delay actions must not wait for an extra frame.
	return std::max( Time::Zero, MonotonicClock::now() - mRefPoint );
}

Time Clock::getElapsedTimeAndReset() {
	Time r = getElapsedTime();
	restart();
	return r;
}

}} // namespace EE::System
