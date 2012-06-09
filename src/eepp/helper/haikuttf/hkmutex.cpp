#include "hkmutex.hpp"

namespace HaikuTTF {

hkMutex::hkMutex() {
	#if HK_PLATFORM == HK_PLATFORM_WIN
	InitializeCriticalSection(&mMutex);
	#elif defined( HK_PLATFORM_POSIX )
	pthread_mutex_init(&mMutex, NULL);
	#endif
}

hkMutex::~hkMutex() {
	#if HK_PLATFORM == HK_PLATFORM_WIN
	DeleteCriticalSection(&mMutex);
	#elif defined( HK_PLATFORM_POSIX )
	pthread_mutex_destroy(&mMutex);
	#endif
}

void hkMutex::Lock() {
	#if HK_PLATFORM == HK_PLATFORM_WIN
	EnterCriticalSection(&mMutex);
	#elif defined( HK_PLATFORM_POSIX )
	pthread_mutex_lock(&mMutex);
	#endif
}

void hkMutex::Unlock() {
	#if HK_PLATFORM == HK_PLATFORM_WIN
	LeaveCriticalSection(&mMutex);
	#elif defined( HK_PLATFORM_POSIX )
	pthread_mutex_unlock(&mMutex);
	#endif
}

}


 
