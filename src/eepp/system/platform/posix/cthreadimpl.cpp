#include <eepp/system/platform/posix/cthreadimpl.hpp>
#include <eepp/system/cthread.hpp>
#include <iostream>

namespace EE { namespace System { namespace Platform {

#if defined( EE_PLATFORM_POSIX )

Uint32 cThreadImpl::GetCurrentThreadId() {
	return (Uint32)pthread_self();
}

cThreadImpl::cThreadImpl( cThread * owner ) :
	mIsActive(false)
{
	mIsActive = pthread_create( &mThread, NULL, &cThreadImpl::EntryPoint, owner ) == 0;

	if ( !mIsActive )
		std::cerr << "Failed to create thread" << std::endl;
}

void cThreadImpl::Wait() {
	if ( mIsActive ) { // Wait for the thread to finish, no timeout

		eeASSERT( pthread_equal( pthread_self(), mThread ) == 0 );

		pthread_join( mThread, NULL );

		mIsActive = false; // Reset the thread state
	}
}

void cThreadImpl::Terminate() {
	if ( mIsActive ) {
		#if EE_PLATFORM != EE_PLATFORM_ANDROID
			pthread_cancel( mThread );
		#else
			pthread_kill( mThread , SIGUSR1 );
		#endif

		mIsActive = false;
	}
}

Uint32 cThreadImpl::Id() {
	return (Uint32)mThread;
}

void * cThreadImpl::EntryPoint( void * userData ) {
	// The Thread instance is stored in the user data
	cThread * owner = static_cast<cThread*>( userData );

	// Tell the thread to handle cancel requests immediatly
	#ifdef PTHREAD_CANCEL_ASYNCHRONOUS
		pthread_setcanceltype( PTHREAD_CANCEL_ASYNCHRONOUS, NULL );
	#endif

	// Forward to the owner
	owner->Run();

	return NULL;
}

#endif

}}}
