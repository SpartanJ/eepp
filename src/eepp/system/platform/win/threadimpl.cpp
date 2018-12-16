#include <eepp/system/platform/win/threadimpl.hpp>
#include <eepp/system/thread.hpp>
#include <eepp/core/debug.hpp>
#include <iostream>

namespace EE { namespace System { namespace Platform {

#if EE_PLATFORM == EE_PLATFORM_WIN

UintPtr ThreadImpl::getCurrentThreadId() {
	return (UintPtr)::GetCurrentThreadId();
}

ThreadImpl::ThreadImpl( Thread * owner ) {
	mThread = reinterpret_cast<HANDLE>( _beginthreadex( NULL, 0, &ThreadImpl::entryPoint, owner, 0, &mThreadId ) );

	if ( !mThread )
		std::cerr << "Failed to create thread" << std::endl;
}

ThreadImpl::~ThreadImpl()
{
	if ( mThread ) {
		CloseHandle( mThread );
	}
}

void ThreadImpl::wait() {
	if ( mThread ) { // Wait for the thread to finish, no timeout
	
		eeASSERT( mThreadId != getCurrentThreadId() ); // A thread cannot wait for itself!

		WaitForSingleObject( mThread, INFINITE );
	}
}

void ThreadImpl::terminate() {
	if ( mThread ) {
		TerminateThread( mThread, 0 );
	}
}

UintPtr ThreadImpl::getId() {
	return (UintPtr)mThreadId;
}

unsigned int __stdcall ThreadImpl::entryPoint( void * userData ) {
	// The Thread instance is stored in the user data
	Thread * owner = static_cast<Thread*>( userData );

	// Forward to the owner
	owner->run();

	// Optional, but it is cleaner
	_endthreadex(0);

	return 0;
}

#endif

}}}
