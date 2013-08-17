#include <eepp/system/platform/posix/cmuteximpl.hpp>

#if defined( EE_PLATFORM_POSIX )

#include <iostream>

namespace EE { namespace System { namespace Platform {

cMutexImpl::cMutexImpl() {
	pthread_mutexattr_t attributes;
	pthread_mutexattr_init(&attributes);
	pthread_mutexattr_settype(&attributes, PTHREAD_MUTEX_RECURSIVE);
	if ( 0 != pthread_mutex_init(&mMutex, &attributes) )
		std::cerr << "cMutexImpl::cMutexImpl(): pthread_mutex_init() error\n";
}

cMutexImpl::~cMutexImpl() {
	if ( 0 != pthread_mutex_destroy( &mMutex ) )
		std::cerr << "cMutexImpl::~cMutexImpl(): pthread_mutex_destroy() error\n";
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
