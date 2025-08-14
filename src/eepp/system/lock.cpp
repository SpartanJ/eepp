#include <eepp/system/lock.hpp>
#include <eepp/system/mutex.hpp>
#ifdef EE_REGISTER_SLOW_LOCKS
#include <eepp/window/engine.hpp>
#endif

namespace EE { namespace System {

Lock::Lock( Mutex& mutex ) : mMutex( mutex ) {
	mMutex.lock();
}

Lock::~Lock() {
	mMutex.unlock();
#ifdef EE_REGISTER_SLOW_LOCKS
	if ( EE::Window::Engine::isMainThread() && mClock.getElapsedTime().asMilliseconds() > 100.f ) {
		Log::info( "Something locked the main thread for too long! It was locked for: %s",
				   mClock.getElapsedTime().toString() );
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
