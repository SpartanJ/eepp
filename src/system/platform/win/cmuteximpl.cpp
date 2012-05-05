#include "cmuteximpl.hpp"

#if EE_PLATFORM == EE_PLATFORM_WIN

namespace EE { namespace System { namespace Platform {

cMutexImpl::cMutexImpl() {
	InitializeCriticalSection(&mMutex);
}

cMutexImpl::~cMutexImpl() {
	DeleteCriticalSection(&mMutex);
}

void cMutexImpl::Lock() {
	EnterCriticalSection(&mMutex);
}

void cMutexImpl::Unlock() {
	LeaveCriticalSection(&mMutex);
}

}}}

#endif
