#ifndef EE_SYSTEMCCLOCKIMPLWIN_H
#define EE_SYSTEMCCLOCKIMPLWIN_H

#include <eepp/core.hpp>

#if EE_PLATFORM == EE_PLATFORM_WIN

#ifndef WIN32_LEAN_AND_MEAN
	#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
	#define NOMINMAX
#endif
#include <windows.h>

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
