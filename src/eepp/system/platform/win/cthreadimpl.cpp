#include <eepp/system/platform/win/cthreadimpl.hpp>
#include <eepp/system/cthread.hpp>

namespace EE { namespace System { namespace Platform {

#if EE_PLATFORM == EE_PLATFORM_WIN

cThreadImpl::cThreadImpl( cThread * owner ) {
	mThread = reinterpret_cast<HANDLE>( _beginthreadex( NULL, 0, &cThreadImpl::EntryPoint, owner, 0, &mThreadId ) );

	if ( !mThread )
		std::cerr << "Failed to create thread" << std::endl;
}

void cThreadImpl::Wait() {
	if ( mThread ) { // Wait for the thread to finish, no timeout
	
		eeASSERT( mThreadId != GetCurrentThreadId() ); // A thread cannot wait for itself!

		WaitForSingleObject( mThread, INFINITE );
	}
}

void cThreadImpl::Terminate() {
	if ( mThread ) {
		TerminateThread( mThread, 0 );
	}
}

unsigned int __stdcall cThreadImpl::EntryPoint( void * userData ) {
	// The Thread instance is stored in the user data
	cThread * owner = static_cast<cThread*>( userData );

	// Forward to the owner
	owner->Run();

	// Optional, but it is cleaner
	_endthreadex(0);

	return 0;
}

#endif

}}}
