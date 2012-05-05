#include "ctimerimpl.hpp"

#if defined( EE_PLATFORM_POSIX )

namespace EE { namespace System { namespace Platform { 

cTimerImpl::cTimerImpl() {
}

cTimerImpl::~cTimerImpl() {
}

void cTimerImpl::Reset() {
	mZeroClock = clock();
	gettimeofday(&mStart, NULL);
}

unsigned long cTimerImpl::GetMilliseconds() {
	struct timeval now;
	gettimeofday(&now, NULL);
	return (now.tv_sec-mStart.tv_sec)*1000+(now.tv_usec-mStart.tv_usec)/1000;
}

unsigned long cTimerImpl::GetMicroseconds() {
	struct timeval now;
	gettimeofday(&now, NULL);
	return (now.tv_sec-mStart.tv_sec)*1000000+(now.tv_usec-mStart.tv_usec);
}

}}}

#endif
