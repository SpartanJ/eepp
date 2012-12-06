#include "hkmutex.hpp"

namespace HaikuTTF {

#if HK_PLATFORM == HK_PLATFORM_WIN

#ifndef WIN32_LEAN_AND_MEAN
	#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

class hkMutexImpl {
	public:
		hkMutexImpl() {
			InitializeCriticalSection(&mMutex);
		}

		~hkMutexImpl() {
			DeleteCriticalSection(&mMutex);
		}

		void Lock() {
			EnterCriticalSection(&mMutex);
		}

		void Unlock() {
			LeaveCriticalSection(&mMutex);
		}
	protected:
		CRITICAL_SECTION mMutex;
};

#elif defined( HK_PLATFORM_POSIX )

#include <pthread.h>
class hkMutexImpl {
	public:
		hkMutexImpl() {
			pthread_mutex_init(&mMutex, NULL);
		}

		~hkMutexImpl() {
			pthread_mutex_destroy(&mMutex);
		}

		void Lock() {
			pthread_mutex_lock(&mMutex);
		}

		void Unlock() {
			pthread_mutex_unlock(&mMutex);
		}
	protected:
		pthread_mutex_t mMutex;
};

#endif

hkMutex::hkMutex() :
	mImpl( hkNew( hkMutexImpl, () ) )
{
}

hkMutex::~hkMutex() {
	hkSAFE_DELETE( mImpl )
}

void hkMutex::Lock() {
	mImpl->Lock();
}

void hkMutex::Unlock() {
	mImpl->Unlock();
}

}


 
