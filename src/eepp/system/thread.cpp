#include <eepp/system/platform/platformimpl.hpp>
#include <eepp/system/thread.hpp>

namespace EE { namespace System {

UintPtr Thread::getCurrentThreadId() {
	return Platform::ThreadImpl::getCurrentThreadId();
}

Thread::Thread() : mThreadImpl( NULL ), mEntryPoint( NULL ) {}

Thread::~Thread() {
	if ( mThreadImpl && mThreadImpl->getId() != Thread::getCurrentThreadId() )
		wait();

	if ( NULL != mEntryPoint )
		delete mEntryPoint;

	eeSAFE_DELETE( mThreadImpl );
}

void Thread::launch() {
	wait();

	mThreadImpl = eeNew( Platform::ThreadImpl, ( this ) );
}

void Thread::wait() {
	if ( mThreadImpl ) {
		mThreadImpl->wait();

		eeSAFE_DELETE( mThreadImpl );
	}
}

void Thread::terminate() {
	if ( mThreadImpl ) {
		mThreadImpl->terminate();

		eeSAFE_DELETE( mThreadImpl );
	}
}

Uint32 Thread::getId() {
	return mThreadImpl->getId();
}

void Thread::run() {
	mEntryPoint->run();
}

}} // namespace EE::System
