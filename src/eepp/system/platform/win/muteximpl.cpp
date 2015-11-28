#include <eepp/system/platform/win/muteximpl.hpp>

#if EE_PLATFORM == EE_PLATFORM_WIN

namespace EE { namespace System { namespace Platform {

MutexImpl::MutexImpl() {
	InitializeCriticalSection(&mMutex);
}

MutexImpl::~MutexImpl() {
	DeleteCriticalSection(&mMutex);
}

void MutexImpl::Lock() {
	EnterCriticalSection(&mMutex);
}

void MutexImpl::Unlock() {
	LeaveCriticalSection(&mMutex);
}

int MutexImpl::TryLock() {
	return TryEnterCriticalSection(&mMutex);
}

}}}

#endif
