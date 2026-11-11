#include <eepp/system/monotonicclock.hpp>

#include <chrono>

namespace EE { namespace System {

MonotonicClock::TimePoint MonotonicClock::now() noexcept {
	return TimePoint( std::chrono::duration_cast<std::chrono::microseconds>(
						  std::chrono::steady_clock::now().time_since_epoch() )
						  .count() );
}

}} // namespace EE::System
