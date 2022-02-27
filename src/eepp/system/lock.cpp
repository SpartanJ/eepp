#include <eepp/system/lock.hpp>
#include <eepp/system/mutex.hpp>

namespace EE { namespace System {

Lock::Lock( Mutex& mutex ) : mMutex( mutex ) {
	mMutex.lock();
}

Lock::~Lock() {
	mMutex.unlock();
}

ConditionalLock::ConditionalLock( bool condition, Mutex* mutex ) :
	mMutex( mutex ), mCondition( condition ) {
	if ( mMutex && mCondition )
		mMutex->lock();
}

ConditionalLock::~ConditionalLock() {
	if ( mMutex && mCondition )
		mMutex->unlock();
}

}} // namespace EE::System
