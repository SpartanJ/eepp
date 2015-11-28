#include <eepp/system/mutex.hpp>
#include <eepp/system/platform/platformimpl.hpp>

namespace EE { namespace System {

Mutex::Mutex() :
	mMutexImpl( new Platform::MutexImpl() )
{
}

Mutex::~Mutex() {
	delete mMutexImpl;
}

void Mutex::Lock() {
	mMutexImpl->Lock();
}

void Mutex::Unlock() {
	mMutexImpl->Unlock();
}

int Mutex::TryLock() {
	return mMutexImpl->TryLock();
}

}}
