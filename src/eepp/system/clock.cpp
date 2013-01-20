#include <eepp/system/clock.hpp> 
#include <eepp/system/cmutex.hpp>

namespace EE { namespace System {

cLock::cLock( cMutex& mutex ) :
	mMutex( mutex )
{
	mMutex.Lock();
}

cLock::~cLock() {
	mMutex.Unlock();
}

}}
