#include <eepp/system/platform/posix/clockimpl.hpp>

#if defined( EE_PLATFORM_POSIX )

namespace EE { namespace System { namespace Platform { 

ClockImpl::ClockImpl() {
}

ClockImpl::~ClockImpl() {
}

void ClockImpl::Restart() {
#ifdef EE_HAVE_CLOCK_GETTIME
	clock_gettime( CLOCK_MONOTONIC, &mStart );
#else
	gettimeofday( &mStart, NULL );
#endif
}

unsigned long ClockImpl::GetElapsedTime() {
#ifdef EE_HAVE_CLOCK_GETTIME
	timespec time;
	clock_gettime( CLOCK_MONOTONIC, &time );
	return ( static_cast<Uint64>( time.tv_sec - mStart.tv_sec ) * 1000000 + ( time.tv_nsec - mStart.tv_nsec ) / 1000 );
#else
	struct timeval now;
	gettimeofday( &now, NULL );
	return ( now.tv_sec - mStart.tv_sec ) * 1000000 + ( now.tv_usec - mStart.tv_usec );
#endif
}

}}}

#endif
