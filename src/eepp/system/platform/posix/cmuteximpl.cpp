#include <eepp/system/platform/posix/cmuteximpl.hpp>

#if defined( EE_PLATFORM_POSIX )

namespace EE { namespace System { namespace Platform {

cMutexImpl::cMutexImpl() {
	pthread_mutexattr_t attributes;
	pthread_mutexattr_init(&attributes);
	pthread_mutexattr_settype(&attributes, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&mMutex, &attributes);
}

cMutexImpl::~cMutexImpl() {
	pthread_mutex_destroy(&mMutex);
}

void cMutexImpl::Lock() {
	pthread_mutex_lock(&mMutex);
}

void cMutexImpl::Unlock() {
	pthread_mutex_unlock(&mMutex);
}

int cMutexImpl::TryLock() {
	return pthread_mutex_trylock(&mMutex);
}

}}}

#endif
