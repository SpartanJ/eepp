#include <eepp/system/platform/posix/threadimpl.hpp>
#include <eepp/system/thread.hpp>
#include <iostream>

namespace EE { namespace System { namespace Platform {

#if defined( EE_PLATFORM_POSIX )

UintPtr ThreadImpl::getCurrentThreadId() {
	return (UintPtr)pthread_self();
}

ThreadImpl::ThreadImpl( Thread * owner ) :
	mIsActive(false)
{
	mIsActive = pthread_create( &mThread, NULL, &ThreadImpl::entryPoint, owner ) == 0;

	if ( !mIsActive )
		std::cerr << "Failed to create thread" << std::endl;
}

void ThreadImpl::wait() {
	if ( mIsActive ) { // Wait for the thread to finish, no timeout

		eeASSERT( pthread_equal( pthread_self(), mThread ) == 0 );

		pthread_join( mThread, NULL );

		mIsActive = false; // Reset the thread state
	}
}

void ThreadImpl::terminate() {
	if ( mIsActive ) {
		#if EE_PLATFORM != EE_PLATFORM_ANDROID
			pthread_cancel( mThread );
		#else
			pthread_kill( mThread , SIGUSR1 );
		#endif

		mIsActive = false;
	}
}

UintPtr ThreadImpl::getId() {
	return (UintPtr)mThread;
}

void * ThreadImpl::entryPoint( void * userData ) {
	// The Thread instance is stored in the user data
	Thread * owner = static_cast<Thread*>( userData );

	// Tell the thread to handle cancel requests immediatly
	#ifdef PTHREAD_CANCEL_ASYNCHRONOUS
		pthread_setcanceltype( PTHREAD_CANCEL_ASYNCHRONOUS, NULL );
	#endif

	// Forward to the owner
	owner->run();

	return NULL;
}

#endif

}}}
