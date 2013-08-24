#ifndef EE_SYSTEMCTIMER_H
#define EE_SYSTEMCTIMER_H

#include <eepp/system/base.hpp>
#include <eepp/system/ctime.hpp>

namespace EE { namespace System {

namespace Platform { class cClockImpl; }

class EE_API cClock {
	public:
		/** Clock constructor. Must be called from the same thread that calls GetElapsedTime() */
		cClock();

		~cClock();

		/** Restarts the timer */
		void Restart();

		/** @returns time since initialisation or last reset */
		cTime GetElapsedTime() const;

		/** Time in time elapsed between this call and the last call to Elapsed()
		* This is the equivalent to call GetElapsedTime() and then Restart().
		*/
		cTime Elapsed();
	private:
		Platform::cClockImpl *	mClockImpl;
};

}}
#endif
