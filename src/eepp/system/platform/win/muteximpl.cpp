#include <eepp/system/platform/win/muteximpl.hpp>

#if EE_PLATFORM == EE_PLATFORM_WIN

namespace EE { namespace System { namespace Platform {

MutexImpl::MutexImpl() {
	InitializeCriticalSection( &mMutex );
}

MutexImpl::~MutexImpl() {
	DeleteCriticalSection( &mMutex );
}

void MutexImpl::lock() {
	EnterCriticalSection( &mMutex );
}

void MutexImpl::unlock() {
	LeaveCriticalSection( &mMutex );
}

int MutexImpl::tryLock() {
	return TryEnterCriticalSection( &mMutex );
}

}}} // namespace EE::System::Platform

#endif
