#ifndef EE_SYSTEMCTIMERIMPLPOSIX_H
#define EE_SYSTEMCTIMERIMPLPOSIX_H

#include "../../../base.hpp"

#if defined( EE_PLATFORM_POSIX )

#include <sys/time.h>

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
		struct timeval		mStart;
	};
}}}

#endif

#endif
