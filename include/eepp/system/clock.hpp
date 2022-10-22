#ifndef EE_SYSTEMCTIMER_H
#define EE_SYSTEMCTIMER_H

#include <eepp/system/time.hpp>

namespace EE { namespace System {

namespace Platform {
class ClockImpl;
}

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
	Time getElapsed();

  private:
	Platform::ClockImpl* mClockImpl;
};

}} // namespace EE::System
#endif
