#include <eepp/system/platform/platformimpl.hpp>
#include <eepp/system/thread.hpp>

namespace EE { namespace System {

Uint32 Thread::getCurrentThreadId() {
	return Platform::ThreadImpl::getCurrentThreadId();
}

Thread::Thread() : mThreadImpl( NULL ), mEntryPoint( NULL ) {}

Thread::~Thread() {
	wait();

	if ( NULL != mEntryPoint )
		delete mEntryPoint;
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
