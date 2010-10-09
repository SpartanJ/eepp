#include "ctimer.hpp"

namespace EE { namespace System {

#if EE_PLATFORM == EE_PLATFORM_WIN
cTimer::cTimer() : mTimerMask( 0 ) {
#else
cTimer::cTimer() {
#endif
	reset();
}

cTimer::~cTimer() {
}

bool cTimer::setOption( const std::string key, const void * val ) {
	#if EE_PLATFORM == EE_PLATFORM_WIN
	if ( key.compare("QueryAffinityMask") == 0 ) {		// Telling timer what core to use for a timer read
		DWORD newTimerMask = * static_cast < const DWORD * > ( val );

		// Get the current process core mask
		DWORD procMask;
		DWORD sysMask;

		#if _MSC_VER >= 1400 && defined (_M_X64)
		GetProcessAffinityMask(GetCurrentProcess(), (PDWORD_PTR)&procMask, (PDWORD_PTR)&sysMask);
		#else
		GetProcessAffinityMask(GetCurrentProcess(), &procMask, &sysMask);
		#endif

		// If new mask is 0, then set to default behavior, otherwise check
		// to make sure new timer core mask overlaps with process core mask
		// and that new timer core mask is a power of 2 (i.e. a single core)
		if( ( newTimerMask == 0 ) || ( ( ( newTimerMask & procMask ) != 0 ) && isPO2( newTimerMask ) ) ) {
			mTimerMask = newTimerMask;
			return true;
		}
	}
	#endif
	return false;
}

void cTimer::reset() {
#if EE_PLATFORM == EE_PLATFORM_WIN
	// Get the current process core mask
	DWORD procMask;
	DWORD sysMask;

	#if _MSC_VER >= 1400 && defined (_M_X64)
	GetProcessAffinityMask(GetCurrentProcess(), (PDWORD_PTR)&procMask, (PDWORD_PTR)&sysMask);
	#else
	GetProcessAffinityMask(GetCurrentProcess(), &procMask, &sysMask);
	#endif

	// Find the lowest core that this process uses
	if( mTimerMask == 0 )
	{
		mTimerMask = 1;
		while( ( mTimerMask & procMask ) == 0 )
		{
			mTimerMask <<= 1;
		}
	}

	HANDLE thread = GetCurrentThread();

	// Set affinity to the first core
	DWORD_PTR oldMask = SetThreadAffinityMask(thread, mTimerMask);

	// Get the constant frequency
	QueryPerformanceFrequency(&mFrequency);

	// Query the timer
	QueryPerformanceCounter(&mStartTime);
	mStartTick = GetTickCount();

	// Reset affinity
	SetThreadAffinityMask(thread, oldMask);

	mLastTime = 0;
	mZeroClock = clock();
#else
	mZeroClock = clock();
	gettimeofday(&start, NULL);
#endif
}

unsigned long cTimer::getMilliseconds() {
#if EE_PLATFORM == EE_PLATFORM_WIN
	LARGE_INTEGER curTime;

	HANDLE thread = GetCurrentThread();

	// Set affinity to the first core
	DWORD_PTR oldMask = SetThreadAffinityMask(thread, mTimerMask);

	// Query the cTimer
	QueryPerformanceCounter(&curTime);

	// Reset affinity
	SetThreadAffinityMask(thread, oldMask);

	LONGLONG newTime = curTime.QuadPart - mStartTime.QuadPart;

	// scale by 1000 for milliseconds
	unsigned long newTicks = (unsigned long) (1000 * newTime / mFrequency.QuadPart);

	// detect and compensate for performance counter leaps
	// (surprisingly common, see Microsoft KB: Q274323)
	unsigned long check = GetTickCount() - mStartTick;
	signed long msecOff = (signed long)(newTicks - check);
	if (msecOff < -100 || msecOff > 100) {
		// We must keep the cTimer running forward :)
		LONGLONG adjust = (std::min)(msecOff * mFrequency.QuadPart / 1000, newTime - mLastTime);
		mStartTime.QuadPart += adjust;
		newTime -= adjust;

		// Re-calculate milliseconds
		newTicks = (unsigned long) (1000 * newTime / mFrequency.QuadPart);
	}

	// Record last time for adjust
	mLastTime = newTime;

	return newTicks;
#else
	struct timeval now;
	gettimeofday(&now, NULL);
	return (now.tv_sec-start.tv_sec)*1000+(now.tv_usec-start.tv_usec)/1000;
#endif
}

unsigned long cTimer::getMicroseconds() {
#if EE_PLATFORM == EE_PLATFORM_WIN
	LARGE_INTEGER curTime;

	HANDLE thread = GetCurrentThread();

	// Set affinity to the first core
	DWORD_PTR oldMask = SetThreadAffinityMask(thread, mTimerMask);

	// Query the timer
	QueryPerformanceCounter(&curTime);

	// Reset affinity
	SetThreadAffinityMask(thread, oldMask);

	LONGLONG newTime = curTime.QuadPart - mStartTime.QuadPart;

	// get milliseconds to check against GetTickCount
	unsigned long newTicks = (unsigned long) (1000 * newTime / mFrequency.QuadPart);

	// detect and compensate for performance counter leaps
	// (surprisingly common, see Microsoft KB: Q274323)
	unsigned long check = GetTickCount() - mStartTick;
	signed long msecOff = (signed long)(newTicks - check);
	if (msecOff < -100 || msecOff > 100) {
		// We must keep the timer running forward :)
		LONGLONG adjust = (std::min)(msecOff * mFrequency.QuadPart / 1000, newTime - mLastTime);
		mStartTime.QuadPart += adjust;
		newTime -= adjust;
	}

	// Record last time for adjust
	mLastTime = newTime;

	// scale by 1000000 for microseconds
	unsigned long newMicro = (unsigned long) (1000000 * newTime / mFrequency.QuadPart);

	return newMicro;
#else
	struct timeval now;
	gettimeofday(&now, NULL);
	return (now.tv_sec-start.tv_sec)*1000000+(now.tv_usec-start.tv_usec);
#endif
}

unsigned long cTimer::getMillisecondsCPU() {
	clock_t newClock = clock();
	return (unsigned long)( (eeDouble)( newClock - mZeroClock ) / ( (eeDouble)CLOCKS_PER_SEC / 1000.0 ) );
}

unsigned long cTimer::getMicrosecondsCPU() {
	clock_t newClock = clock();
	return (unsigned long)( (eeDouble)( newClock - mZeroClock ) / ( (eeDouble)CLOCKS_PER_SEC / 1000000.0 ) );
}

}}

