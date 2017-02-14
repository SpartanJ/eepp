#include <eepp/system/platform/posix/conditionimpl.hpp>

#if defined( EE_PLATFORM_POSIX )

#include <iostream>

namespace EE { namespace System { namespace Platform {

ConditionImpl::ConditionImpl( int var ) :
	mIsInvalid( true ),
	mConditionnedVar( var ),
	mCond(),
	mMutex()
{
	if ( 0 != pthread_cond_init( &mCond, NULL ) )
		std::cerr << "ConditionImpl::ConditionImpl(): pthread_cond_init() error\n";

	if ( 0 != pthread_mutex_init( &mMutex, NULL ) )
		std::cerr << "ConditionImpl::ConditionImpl(): pthread_mutex_init() error\n";
}

ConditionImpl::~ConditionImpl() {
	if ( 0 != pthread_cond_destroy( &mCond ) )
		std::cerr << "ConditionImpl::~ConditionImpl(): pthread_cond_destroy() error\n";

	if ( 0 != pthread_mutex_destroy( &mMutex ) )
		std::cerr << "ConditionImpl::~ConditionImpl(): pthread_mutex_destroy() error\n";
}

void ConditionImpl::lock() {
	pthread_mutex_lock( &mMutex );
}

void ConditionImpl::unlock() {
	pthread_mutex_unlock( &mMutex );
}

bool ConditionImpl::waitAndRetain( int value ) {
	pthread_mutex_lock(&mMutex);
	
	while ( mConditionnedVar != value && mIsInvalid ) {
		pthread_cond_wait(&mCond, &mMutex);
	}
	
	if ( mIsInvalid ) {
		return true;
	}
	
	pthread_mutex_unlock( &mMutex );
	
	return false;
}

void ConditionImpl::release( int value ) {
	mConditionnedVar = value;
	
	pthread_mutex_unlock( &mMutex );
	
	signal();
}

void ConditionImpl::setValue( int value ) {
	// Make sure the Condition's value is not modified while retained
	pthread_mutex_lock( &mMutex );
	
	mConditionnedVar = value;
	
	pthread_mutex_unlock(&mMutex);
	
	signal();
}

int ConditionImpl::value() const {
	return mConditionnedVar;
}

void ConditionImpl::signal() {
	pthread_cond_signal( &mCond );
}

void ConditionImpl::invalidate() {
	if (mIsInvalid) {
		mIsInvalid = false;
		signal();
	}
}

void ConditionImpl::restore() {
	if ( !mIsInvalid ) {
		mIsInvalid = true;
	}
}

}}}

#endif
