#ifndef EE_SYSTEMCTIMER_H
#define EE_SYSTEMCTIMER_H

#include <eepp/config.hpp>
#include <eepp/system/monotonicclock.hpp>

namespace EE { namespace System {

class EE_API Clock {
  public:
	/** Starts an elapsed-time clock. Concurrent reads are safe when no thread restarts it. */
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
	MonotonicClock::TimePoint mRefPoint; //!< Instant of the last reset
};

}} // namespace EE::System
#endif
