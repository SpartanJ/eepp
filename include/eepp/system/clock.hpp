#ifndef EE_SYSTEMCTIMER_H
#define EE_SYSTEMCTIMER_H

#include <chrono>
#include <eepp/system/time.hpp>

namespace EE { namespace System {

class EE_API Clock {
  public:
	/** Clock constructor. Must be called from the same thread that calls GetElapsedTime() */
	Clock();

	~Clock();

	/** Restarts the timer */
	void restart();

	/** @returns time since initialisation or last reset */
	Time getElapsedTime() const;

	/** Time in time elapsed between this call and the last call to Elapsed()
	 * This is the equivalent to call GetElapsedTime() and then Restart().
	 */
	Time getElapsedTimeAndReset();

  private:
	using ClockImpl =
		std::conditional_t<std::chrono::high_resolution_clock::is_steady,
						   std::chrono::high_resolution_clock, std::chrono::steady_clock>;

	ClockImpl::time_point mRefPoint{ ClockImpl::now() }; //!< Time of last reset
};

}} // namespace EE::System
#endif
