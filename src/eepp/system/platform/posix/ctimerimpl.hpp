#ifndef EE_SYSTEMCTIMERIMPLPOSIX_H
#define EE_SYSTEMCTIMERIMPLPOSIX_H

#include <eepp/base.hpp>

#if defined( EE_PLATFORM_POSIX )

#ifdef EE_HAVE_CLOCK_GETTIME
	#include <time.h>
#else
	#include <sys/time.h>
#endif

namespace EE { namespace System { namespace Platform {

class cTimerImpl {
	public:
		cTimerImpl();

		~cTimerImpl();

		void Reset();

		unsigned long GetMilliseconds();

		unsigned long GetMicroseconds();
	private:
		#ifdef EE_HAVE_CLOCK_GETTIME
		struct timespec mStart;
		#else
		struct timeval mStart;
		#endif
	};
}}}

#endif

#endif
