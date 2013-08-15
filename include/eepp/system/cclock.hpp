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

		/** Resets timer */
		void Reset();

		/** Returns microseconds since initialisation or last reset */
		cTime GetElapsedTime();

		/** Time in milliseconds elapsed between this call and the last call to Elapsed() */
		eeDouble Elapsed();
	private:
		Platform::cClockImpl *	mClockImpl;
};

}}
#endif
