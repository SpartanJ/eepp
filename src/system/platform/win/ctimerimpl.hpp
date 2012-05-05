#ifndef EE_SYSTEMCTIMERIMPLWIN_H
#define EE_SYSTEMCTIMERIMPLWIN_H

#include "../../../base.hpp"

#if EE_PLATFORM == EE_PLATFORM_WIN

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

namespace EE { namespace System { namespace Platform {

class cTimerImpl {
	public:
		cTimerImpl();

		~cTimerImpl();

		void Reset();

		unsigned long GetMilliseconds();

		unsigned long GetMicroseconds();
	private:
		clock_t				mZeroClock;
		unsigned long		mStartTick;
		LONGLONG			mLastTime;
		LARGE_INTEGER		mStartTime;
		LARGE_INTEGER		mFrequency;
		unsigned long		mTimerMask;

		bool isPO2(Uint32 n);
	};
}}}

#endif

#endif
