#include <eepp/system/platform/posix/cconditionimpl.hpp>
#include <iostream>

namespace EE { namespace System { namespace Platform {

cConditionImpl::cConditionImpl( int var ) :
	mIsInvalid( true ),
	mConditionnedVar( var ),
	mCond(),
	mMutex()
{
	if ( 0 != pthread_cond_init( &mCond, NULL ) )
		std::cerr << "pthread_cond_init() error\n";
	
	if ( 0 != pthread_mutex_init( &mMutex, NULL ) )
		std::cerr << "pthread_mutex_init() error\n";
}

cConditionImpl::~cConditionImpl() {
	if ( 0 != pthread_mutex_destroy( &mMutex ) )
		std::cerr << "pthread_mutex_destroy() error\n";
	
	if ( 0 != pthread_cond_destroy( &mCond ) )
		std::cerr << "pthread_cond_destroy() error\n";
}

void cConditionImpl::Lock() {
	pthread_mutex_lock( &mMutex );
}

void cConditionImpl::Unlock() {
	pthread_mutex_unlock( &mMutex );
}

bool cConditionImpl::WaitAndRetain( int value ) {
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

void cConditionImpl::Release( int value ) {
	mConditionnedVar = value;
	
	pthread_mutex_unlock( &mMutex );
	
	Signal();
}

void cConditionImpl::SetValue( int value ) {
	// Make sure the Condition's value is not modified while retained
	pthread_mutex_lock( &mMutex );
	
	mConditionnedVar = value;
	
	pthread_mutex_unlock(&mMutex);
	
	Signal();
}

int cConditionImpl::Value() const {
	return mConditionnedVar;
}

void cConditionImpl::Signal() {
	pthread_cond_signal( &mCond );
}

void cConditionImpl::Invalidate() {
	if (mIsInvalid) {
		mIsInvalid = false;
		Signal();
	}
}

void cConditionImpl::Restore() {
	if ( !mIsInvalid ) {
		mIsInvalid = true;
	}
}

}}}
