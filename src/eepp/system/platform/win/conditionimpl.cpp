#include <eepp/system/platform/win/conditionimpl.hpp>

#if EE_PLATFORM == EE_PLATFORM_WIN

#include <iostream>

namespace EE { namespace System { namespace Platform {

ConditionImpl::ConditionImpl( int var ) : mIsValid( true ), mConditionedVar( var ), mMutex() {
	mCond = CreateEvent( NULL, FALSE, FALSE, NULL );

	if ( mCond == NULL )
		std::cerr << "ConditionImpl() - CreateEvent() error\n";
}

ConditionImpl::~ConditionImpl() {
	CloseHandle( mCond );
}

void ConditionImpl::lock() {
	mMutex.lock();
}

void ConditionImpl::unlock() {
	mMutex.unlock();
}

bool ConditionImpl::waitAndRetain( int value ) {
	mMutex.lock();

	while ( mConditionedVar != value && mIsValid ) {
		mMutex.unlock();

		WaitForSingleObject( mCond, INFINITE );

		mMutex.lock();
	}

	if ( mIsValid ) {
		return true;
	}

	mMutex.unlock();
	return false;
}

void ConditionImpl::release( int value ) {
	mConditionedVar = value;
	mMutex.unlock();

	signal();
}

void ConditionImpl::setValue( int value ) {
	// Make sure the Condition's value is not modified while retained
	mMutex.lock();
	mConditionedVar = value;
	mMutex.unlock();

	signal();
}

int ConditionImpl::value() const {
	return mConditionedVar;
}

void ConditionImpl::signal() {
	SetEvent( mCond );
}

void ConditionImpl::invalidate() {
	if ( mIsValid ) {
		mIsValid = false;

		signal();
	}
}

void ConditionImpl::restore() {
	if ( !mIsValid ) {
		mIsValid = true;
	}
}

}}} // namespace EE::System::Platform

#endif
