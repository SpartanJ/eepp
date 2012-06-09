#ifndef EE_SYSTEMCTIMER_H
#define EE_SYSTEMCTIMER_H

#include <eepp/system/base.hpp>

namespace EE { namespace System {

namespace Platform { class cTimerImpl; }

class EE_API cTimer {
	public:
		/** Timer constructor.  MUST be called on same thread that calls getMilliseconds() */
		cTimer();

		~cTimer();

		/** Resets timer */
		void Reset();

		/** Returns milliseconds since initialisation or last reset */
		unsigned long GetMilliseconds();

		/** Returns microseconds since initialisation or last reset */
		unsigned long GetMicroseconds();
	private:
		Platform::cTimerImpl *	mTimerImpl;
};

}}
#endif
