#include <eepp/system/lock.hpp> 
#include <eepp/system/mutex.hpp>

namespace EE { namespace System {

Lock::Lock( Mutex& mutex ) :
	mMutex( mutex )
{
	mMutex.Lock();
}

Lock::~Lock() {
	mMutex.Unlock();
}

}}
