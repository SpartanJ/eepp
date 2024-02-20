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

class ClockImpl {
  public:
	ClockImpl();

	~ClockImpl();

	void restart();

	unsigned long getElapsedTime();

  private:
	unsigned long mStartTick;
	LONGLONG mLastTime;
	LARGE_INTEGER mStartTime;
	LARGE_INTEGER mFrequency;
	unsigned long mTimerMask;
};

}}} // namespace EE::System::Platform

#endif

#endif
