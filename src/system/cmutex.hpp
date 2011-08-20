#ifndef EE_SYSTEMCMUTEX_H
#define EE_SYSTEMCMUTEX_H

#include "../base.hpp"

#if EE_PLATFORM == EE_PLATFORM_WIN
#ifndef WIN32_LEAN_AND_MEAN
	#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#elif defined( EE_PLATFORM_POSIX )
#include <pthread.h>
#endif

namespace EE { namespace System {

/** Simple mutex class */
class EE_API cMutex {
	public:
		cMutex();

		~cMutex();

		/** Lock the mutex */
		void Lock();

		/** Unlock the mutex */
		void Unlock();
	private:
		#if EE_PLATFORM == EE_PLATFORM_WIN
		CRITICAL_SECTION mMutex;
		#elif defined( EE_PLATFORM_POSIX )
		pthread_mutex_t mMutex;
		#endif
};

}}

#endif
