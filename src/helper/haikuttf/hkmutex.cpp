#include "hkmutex.hpp"

namespace HaikuTTF {

hkMutex::hkMutex() {
	#if HK_PLATFORM == HK_PLATFORM_WIN
	InitializeCriticalSection(&mMutex);
	#elif defined( HK_PLATFORM_UNIX )
	pthread_mutex_init(&mMutex, NULL);
	#else
	mMutex = SDL_CreateMutex();
	#endif
}

hkMutex::~hkMutex() {
	#if HK_PLATFORM == HK_PLATFORM_WIN
	DeleteCriticalSection(&mMutex);
	#elif defined( HK_PLATFORM_UNIX )
	pthread_mutex_destroy(&mMutex);
	#else
	SDL_DestroyMutex(mMutex);
	#endif
}

void hkMutex::Lock() {
	#if HK_PLATFORM == HK_PLATFORM_WIN
	EnterCriticalSection(&mMutex);
	#elif defined( HK_PLATFORM_UNIX )
	pthread_mutex_lock(&mMutex);
	#else
	SDL_LockMutex(mMutex)
	#endif
}

void hkMutex::Unlock() {
	#if HK_PLATFORM == HK_PLATFORM_WIN
	LeaveCriticalSection(&mMutex);
	#elif defined( HK_PLATFORM_UNIX )
	pthread_mutex_unlock(&mMutex);
	#else
	SDL_UnlockMutex(mMutex)
	#endif
}

}


 
