#include <eepp/system/platform/win/cconditionimpl.hpp>

namespace EE { namespace System { namespace Platform {

cConditionImpl::cConditionImpl(int var) :
	mIsValid( true ),
	mConditionnedVar( var ),
	mMutex()
{
	mCond = CreateEvent(NULL, FALSE, FALSE, NULL);
	
	if ( mCond == NULL )
		std::cerr << "ConditionImpl() - CreateEvent() error\n";
}

cConditionImpl::~cConditionImpl() {
	CloseHandle( mCond );
}

void cConditionImpl::Lock() {
	mMutex.Lock();
}

void cConditionImpl::Unlock() {
	mMutex.Unlock();
}
	
bool cConditionImpl::WaitAndRetain(int value) {
	mMutex.Lock();
	
	while ( mConditionnedVar != value && mIsValid ) {
		mMutex.Unlock();
		
		WaitForSingleObject( mCond, INFINITE );
		
		mMutex.Lock();
	}
	
	if ( mIsValid ) {
		return true;
	}
	
	mMutex.Unlock();
	return false;
}

void cConditionImpl::Release( int value ) {
	mConditionnedVar = value;
	mMutex.Unlock();
	
	Signal();
}

void cConditionImpl::SetValue( int value ) {
	// Make sure the Condition's value is not modified while retained
	mMutex.Lock();
	mConditionnedVar = value;
	mMutex.Unlock();
	
	Signal();
}

int cConditionImpl::Value() const {
	return mConditionnedVar;
}

void cConditionImpl::Signal() {
	SetEvent( mCond );
}

void cConditionImpl::Invalidate() {
	if ( mIsValid ) {
		mIsValid = false;
		
		Signal();
	}
}

void cConditionImpl::Restore() {
	if ( !mIsValid ) {
		mIsValid = true;
	}
}

}}}
