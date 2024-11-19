#include <eepp/system/mutex.hpp>

namespace EE { namespace System {

void Mutex::lock() {
	mMutexImpl.lock();
}

void Mutex::unlock() {
	mMutexImpl.unlock();
}

bool Mutex::tryLock() {
	return mMutexImpl.try_lock();
}

}} // namespace EE::System
