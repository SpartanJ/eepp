#ifndef EE_SYSTEMCTIMER_H
#define EE_SYSTEMCTIMER_H

#include "base.hpp"

#if EE_PLATFORM == EE_PLATFORM_WIN32
	#ifndef WIN32_LEAN_AND_MEAN
		#define WIN32_LEAN_AND_MEAN
	#endif

	#include <windows.h>

	//Get around Windows hackery
	#ifdef max
	#  undef max
	#endif
	#ifdef min
	#  undef min
	#endif

	#include <ctime>
#else
	#include <sys/time.h>
#endif

namespace EE { namespace System {

class EE_API cTimer {
	private:
		#if EE_PLATFORM == EE_PLATFORM_WIN32
			clock_t mZeroClock;

			unsigned long mStartTick;
			LONGLONG mLastTime;
			LARGE_INTEGER mStartTime;
			LARGE_INTEGER mFrequency;

			unsigned long mTimerMask;

			bool isPO2(Uint32 n) {
				return (n & (n-1)) == 0;
			}
		#else
			struct timeval start;
			clock_t zeroClock;
		#endif
	public:
		/** Timer constructor.  MUST be called on same thread that calls getMilliseconds() */
		cTimer();
		~cTimer();

		/** Method for setting a specific option of the Timer. These options are usually
		specific for a certain implementation of the Timer class, and may (and probably
		will) not exist across different implementations.  reset() must be called after
		all setOption() calls.
		@par
		Current options supported are:
		<ul><li>"QueryAffinityMask" (DWORD): Set the thread affinity mask to be used
		to check the timer. If 'reset' has been called already this mask should
		overlap with the process mask that was in force at that point, and should
		be a power of two (a single core).</li></ul>
		@param
			strKey The name of the option to set
		@param
			pValue A pointer to the value - the size should be calculated by the timer
			based on the key
		@return
			On success, true is returned.
		@par
			On failure, false is returned.
		*/
        bool setOption( const std::string strKey, const void* pValue );

		/** Resets timer */
		void reset();

		/** Returns milliseconds since initialisation or last reset */
		unsigned long getMilliseconds();

		/** Returns microseconds since initialisation or last reset */
		unsigned long getMicroseconds();

		/** Returns milliseconds since initialisation or last reset, only CPU time measured */
		unsigned long getMillisecondsCPU();

		/** Returns microseconds since initialisation or last reset, only CPU time measured */
		unsigned long getMicrosecondsCPU();
	};
}}
#endif
