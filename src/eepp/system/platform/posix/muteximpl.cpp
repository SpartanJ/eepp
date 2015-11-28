#include <eepp/system/platform/posix/muteximpl.hpp>

#if defined( EE_PLATFORM_POSIX )

#include <iostream>

namespace EE { namespace System { namespace Platform {

MutexImpl::MutexImpl() {
	pthread_mutexattr_t attributes;
	pthread_mutexattr_init(&attributes);
	pthread_mutexattr_settype(&attributes, PTHREAD_MUTEX_RECURSIVE);
	if ( 0 != pthread_mutex_init(&mMutex, &attributes) )
		std::cerr << "MutexImpl::MutexImpl(): pthread_mutex_init() error\n";
}

MutexImpl::~MutexImpl() {
	if ( 0 != pthread_mutex_destroy( &mMutex ) )
		std::cerr << "MutexImpl::~MutexImpl(): pthread_mutex_destroy() error\n";
}

void MutexImpl::Lock() {
	pthread_mutex_lock(&mMutex);
}

void MutexImpl::Unlock() {
	pthread_mutex_unlock(&mMutex);
}

int MutexImpl::TryLock() {
	return pthread_mutex_trylock(&mMutex);
}

}}}

#endif
