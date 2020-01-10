#include <eepp/system/platform/win/clockimpl.hpp>

#if EE_PLATFORM == EE_PLATFORM_WIN
#include <algorithm>

namespace EE { namespace System { namespace Platform {

ClockImpl::ClockImpl() : mTimerMask( 0 ) {}

ClockImpl::~ClockImpl() {}

void ClockImpl::restart() {
	// Get the current process core mask
	DWORD procMask;
	DWORD sysMask;

#if _MSC_VER >= 1400 && defined( _M_X64 )
	GetProcessAffinityMask( GetCurrentProcess(), (PDWORD_PTR)&procMask, (PDWORD_PTR)&sysMask );
#else
	GetProcessAffinityMask( GetCurrentProcess(), &procMask, &sysMask );
#endif

	// Find the lowest core that this process uses
	if ( mTimerMask == 0 ) {
		mTimerMask = 1;
		while ( ( mTimerMask & procMask ) == 0 ) {
			mTimerMask <<= 1;
		}
	}

	HANDLE thread = GetCurrentThread();

	// Set affinity to the first core
	DWORD_PTR oldMask = SetThreadAffinityMask( thread, mTimerMask );

	// Get the constant frequency
	QueryPerformanceFrequency( &mFrequency );

	// Query the timer
	QueryPerformanceCounter( &mStartTime );
	mStartTick = GetTickCount();

	// Reset affinity
	SetThreadAffinityMask( thread, oldMask );

	mLastTime = 0;
}

unsigned long ClockImpl::getElapsedTime() {
	LARGE_INTEGER curTime;

	HANDLE thread = GetCurrentThread();

	// Set affinity to the first core
	DWORD_PTR oldMask = SetThreadAffinityMask( thread, mTimerMask );

	// Query the timer
	QueryPerformanceCounter( &curTime );

	// Reset affinity
	SetThreadAffinityMask( thread, oldMask );

	LONGLONG newTime = curTime.QuadPart - mStartTime.QuadPart;

	// get milliseconds to check against GetTickCount
	unsigned long newTicks = (unsigned long)( 1000 * newTime / mFrequency.QuadPart );

	// detect and compensate for performance counter leaps
	// (surprisingly common, see Microsoft KB: Q274323)
	unsigned long check = GetTickCount() - mStartTick;
	signed long msecOff = (signed long)( newTicks - check );
	if ( msecOff < -100 || msecOff > 100 ) {
		// We must keep the timer running forward :)
		LONGLONG adjust = std::min( msecOff * mFrequency.QuadPart / 1000, newTime - mLastTime );
		mStartTime.QuadPart += adjust;
		newTime -= adjust;
	}

	// Record last time for adjust
	mLastTime = newTime;

	// scale by 1000000 for microseconds
	unsigned long newMicro = (unsigned long)( 1000000 * newTime / mFrequency.QuadPart );

	return newMicro;
}

bool ClockImpl::isPO2( Uint32 n ) {
	return ( n & ( n - 1 ) ) == 0;
}

}}} // namespace EE::System::Platform

#endif
