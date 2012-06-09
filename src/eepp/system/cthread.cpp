#include <eepp/system/cthread.hpp>
#include <eepp/system/platform/platformimpl.hpp>

namespace EE { namespace System {

cThread::cThread() :
	mThreadImpl(NULL),
	mEntryPoint(NULL)
{
}

cThread::~cThread() {
	Wait();

	eeSAFE_DELETE( mEntryPoint );
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

void cThread::Run() {
	mEntryPoint->Run();
}

}}
