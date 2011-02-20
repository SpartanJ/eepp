#include "cthread.hpp"

namespace EE { namespace System {

cThread::cThread() :
	mIsActive(false),
	mFunction(NULL),
	mUserData(NULL)
{
}

cThread::cThread( cThread::FuncType Function, void* UserData ) :
	mIsActive(false),
	mFunction(Function),
	mUserData(UserData)
{
}

cThread::~cThread() {
	Wait();
}

void cThread::Launch() {
	mIsActive = true;

	#if EE_PLATFORM == EE_PLATFORM_WIN

	mThread = reinterpret_cast<HANDLE>( _beginthreadex( NULL, 0, &cThread::EntryPoint, this, 0, NULL ) );

	if ( !mThread )
		mIsActive = false;

	#elif defined( EE_PLATFORM_POSIX )

	mIsActive = pthread_create( &mThread, NULL, &cThread::EntryPoint, this ) == 0;

	#endif

	if ( !mIsActive ) {
		std::cerr << "Failed to create thread" << std::endl;

		mIsActive = false;
	}
}

void cThread::Wait() {
	if ( mIsActive ) { // Wait for the thread to finish, no timeout
		#if EE_PLATFORM == EE_PLATFORM_WIN

		WaitForSingleObject( mThread, INFINITE );

		#elif defined( EE_PLATFORM_POSIX )

		pthread_join(mThread, NULL);

		#endif

		mIsActive = false; // Reset the thread state
	}
}

void cThread::Terminate() {
	if ( mIsActive ) {
		#if EE_PLATFORM == EE_PLATFORM_WIN

		TerminateThread( mThread, 0 );

		#elif defined( EE_PLATFORM_POSIX )

		pthread_cancel( mThread );

		#endif

		mIsActive = false;
	}
}

void cThread::Run() {
	if ( mFunction )
		mFunction( mUserData );
}

#if EE_PLATFORM == EE_PLATFORM_WIN
unsigned int __stdcall cThread::EntryPoint( void * userData ) {
	// The Thread instance is stored in the user data
	cThread * owner = static_cast<cThread*>( userData );

	// Forward to the owner
	owner->Run();

	// Optional, but it is cleaner
	_endthreadex(0);

	return 0;
}

#elif defined( EE_PLATFORM_POSIX )

void * cThread::EntryPoint( void * userData ) {
	// The Thread instance is stored in the user data
	cThread * owner = static_cast<cThread*>( userData );

	// Tell the thread to handle cancel requests immediatly
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

	// Forward to the owner
	owner->Run();

	return NULL;
}

#endif

}}
