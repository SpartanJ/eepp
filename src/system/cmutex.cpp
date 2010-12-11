#include "cmutex.hpp"

namespace EE { namespace System {

cMutex::cMutex() {
	#if EE_PLATFORM == EE_PLATFORM_WIN
	InitializeCriticalSection(&mMutex);
	#elif defined( EE_PLATFORM_UNIX )
	pthread_mutex_init(&mMutex, NULL);
	#endif
}

cMutex::~cMutex() {
	#if EE_PLATFORM == EE_PLATFORM_WIN
	DeleteCriticalSection(&mMutex);
	#elif defined( EE_PLATFORM_UNIX )
	pthread_mutex_destroy(&mMutex);
	#endif
}

void cMutex::Lock() {
	#if EE_PLATFORM == EE_PLATFORM_WIN
	EnterCriticalSection(&mMutex);
	#elif defined( EE_PLATFORM_UNIX )
	pthread_mutex_lock(&mMutex);
	#endif
}

void cMutex::Unlock() {
	#if EE_PLATFORM == EE_PLATFORM_WIN
	LeaveCriticalSection(&mMutex);
	#elif defined( EE_PLATFORM_UNIX )
	pthread_mutex_unlock(&mMutex);
	#endif
}

}}
