#include <eepp/system/thread.hpp>
#include <eepp/system/platform/platformimpl.hpp>

namespace EE { namespace System {

Uint32 Thread::GetCurrentThreadId() {
	return Platform::ThreadImpl::GetCurrentThreadId();
}

Thread::Thread() :
	mThreadImpl(NULL),
	mEntryPoint(NULL)
{
}

Thread::~Thread() {
	Wait();

	if ( NULL != mEntryPoint )
		delete mEntryPoint;
}

void Thread::Launch() {
	Wait();

	mThreadImpl = eeNew( Platform::ThreadImpl, ( this ) );
}

void Thread::Wait() {
	if ( mThreadImpl ) {
		mThreadImpl->Wait();

		eeSAFE_DELETE( mThreadImpl );
	}
}

void Thread::Terminate() {
	if ( mThreadImpl ) {
		mThreadImpl->Terminate();

		eeSAFE_DELETE( mThreadImpl );
	}
}

Uint32 Thread::Id() {
	return mThreadImpl->Id();
}

void Thread::Run() {
	mEntryPoint->Run();
}

}}
