#include <eepp/system/lock.hpp>
#include <eepp/system/mutex.hpp>
#ifdef EE_DEBUG
#include <eepp/window/engine.hpp>
#endif

namespace EE { namespace System {

Lock::Lock( Mutex& mutex ) : mMutex( mutex ) {
	mMutex.lock();
}

Lock::~Lock() {
	mMutex.unlock();
#ifdef EE_DEBUG
	if ( EE::Window::Engine::isMainThread() && mClock.getElapsedTime().asMilliseconds() > 100.f ) {
		eeASSERT( false );
	}
#endif
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
