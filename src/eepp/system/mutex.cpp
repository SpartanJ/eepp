#include <eepp/system/mutex.hpp>
#include <eepp/system/platform/platformimpl.hpp>

namespace EE { namespace System {

Mutex::Mutex() : mMutexImpl( new Platform::MutexImpl() ) {}

Mutex::~Mutex() {
	delete mMutexImpl;
}

void Mutex::lock() {
	mMutexImpl->lock();
}

void Mutex::unlock() {
	mMutexImpl->unlock();
}

int Mutex::tryLock() {
	return mMutexImpl->tryLock();
}

}} // namespace EE::System
