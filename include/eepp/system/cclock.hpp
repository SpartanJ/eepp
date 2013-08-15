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
		void Restart();

		/** Returns microseconds since initialisation or last reset */
		cTime GetElapsedTime() const;

		/** Time in milliseconds elapsed between this call and the last call to Elapsed()
		* This is the equivalent to call GetElapsedTime() and then Reset().
		*/
		cTime Elapsed();
	private:
		Platform::cClockImpl *	mClockImpl;
};

}}
#endif
