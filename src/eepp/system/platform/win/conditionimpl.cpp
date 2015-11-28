#include <eepp/system/platform/win/conditionimpl.hpp>

#if EE_PLATFORM == EE_PLATFORM_WIN

#include <iostream>

namespace EE { namespace System { namespace Platform {

ConditionImpl::ConditionImpl(int var) :
	mIsValid( true ),
	mConditionnedVar( var ),
	mMutex()
{
	mCond = CreateEvent(NULL, FALSE, FALSE, NULL);
	
	if ( mCond == NULL )
		std::cerr << "ConditionImpl() - CreateEvent() error\n";
}

ConditionImpl::~ConditionImpl() {
	CloseHandle( mCond );
}

void ConditionImpl::Lock() {
	mMutex.Lock();
}

void ConditionImpl::Unlock() {
	mMutex.Unlock();
}

bool ConditionImpl::WaitAndRetain(int value) {
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

void ConditionImpl::Release( int value ) {
	mConditionnedVar = value;
	mMutex.Unlock();
	
	Signal();
}

void ConditionImpl::SetValue( int value ) {
	// Make sure the Condition's value is not modified while retained
	mMutex.Lock();
	mConditionnedVar = value;
	mMutex.Unlock();
	
	Signal();
}

int ConditionImpl::Value() const {
	return mConditionnedVar;
}

void ConditionImpl::Signal() {
	SetEvent( mCond );
}

void ConditionImpl::Invalidate() {
	if ( mIsValid ) {
		mIsValid = false;
		
		Signal();
	}
}

void ConditionImpl::Restore() {
	if ( !mIsValid ) {
		mIsValid = true;
	}
}

}}}

#endif
