#include <eepp/system/cmutex.hpp>
#include <eepp/system/platform/platformimpl.hpp>

namespace EE { namespace System {

cMutex::cMutex() :
	mMutexImpl( new Platform::cMutexImpl() )
{
}

cMutex::~cMutex() {
	delete mMutexImpl;
}

void cMutex::Lock() {
	mMutexImpl->Lock();
}

void cMutex::Unlock() {
	mMutexImpl->Unlock();
}

int cMutex::TryLock() {
	return mMutexImpl->TryLock();
}

}}
