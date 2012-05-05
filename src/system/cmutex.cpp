#include "cmutex.hpp"

namespace EE { namespace System {

cMutex::cMutex() :
	mMutexImpl( eeNew( Platform::cMutexImpl, () ) )
{
}

cMutex::~cMutex() {
	eeSAFE_DELETE( mMutexImpl );
}

void cMutex::Lock() {
	mMutexImpl->Lock();
}

void cMutex::Unlock() {
	mMutexImpl->Unlock();
}

}}
