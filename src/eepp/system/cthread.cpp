#include <eepp/system/cthread.hpp>
#include <eepp/system/platform/platformimpl.hpp>

namespace EE { namespace System {

Uint32 cThread::GetCurrentThreadId() {
	return Platform::cThreadImpl::GetCurrentThreadId();
}

cThread::cThread() :
	mThreadImpl(NULL),
	mEntryPoint(NULL)
{
}

cThread::~cThread() {
	Wait();

	if ( NULL != mEntryPoint )
		delete mEntryPoint;
}

void cThread::Launch() {
	Wait();

	mThreadImpl = eeNew( Platform::cThreadImpl, ( this ) );
}

void cThread::Wait() {
	if ( mThreadImpl ) {
		mThreadImpl->Wait();

		eeSAFE_DELETE( mThreadImpl );
	}
}

void cThread::Terminate() {
	if ( mThreadImpl ) {
		mThreadImpl->Terminate();

		eeSAFE_DELETE( mThreadImpl );
	}
}

Uint32 cThread::Id() {
	return mThreadImpl->Id();
}

void cThread::Run() {
	mEntryPoint->Run();
}

}}
