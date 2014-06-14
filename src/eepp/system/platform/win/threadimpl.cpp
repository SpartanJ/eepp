#include <eepp/system/platform/win/threadimpl.hpp>
#include <eepp/system/thread.hpp>
#include <iostream>

namespace EE { namespace System { namespace Platform {

#if EE_PLATFORM == EE_PLATFORM_WIN

UintPtr ThreadImpl::GetCurrentThreadId() {
	return (UintPtr)::GetCurrentThreadId();
}

ThreadImpl::ThreadImpl( Thread * owner ) {
	mThread = reinterpret_cast<HANDLE>( _beginthreadex( NULL, 0, &ThreadImpl::EntryPoint, owner, 0, &mThreadId ) );

	if ( !mThread )
		std::cerr << "Failed to create thread" << std::endl;
}

void ThreadImpl::Wait() {
	if ( mThread ) { // Wait for the thread to finish, no timeout
	
		eeASSERT( mThreadId != GetCurrentThreadId() ); // A thread cannot wait for itself!

		WaitForSingleObject( mThread, INFINITE );
	}
}

void ThreadImpl::Terminate() {
	if ( mThread ) {
		TerminateThread( mThread, 0 );
	}
}

UintPtr ThreadImpl::Id() {
	return (UintPtr)mThreadId;
}

unsigned int __stdcall ThreadImpl::EntryPoint( void * userData ) {
	// The Thread instance is stored in the user data
	Thread * owner = static_cast<Thread*>( userData );

	// Forward to the owner
	owner->Run();

	// Optional, but it is cleaner
	_endthreadex(0);

	return 0;
}

#endif

}}}
