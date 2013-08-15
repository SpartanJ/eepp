#ifndef EE_SYSTEMCCLOCKIMPLWIN_H
#define EE_SYSTEMCCLOCKIMPLWIN_H

#include <eepp/base.hpp>

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

class cClockImpl {
	public:
		cClockImpl();

		~cClockImpl();

		void Restart();

		unsigned long GetElapsedTime();
	private:
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
