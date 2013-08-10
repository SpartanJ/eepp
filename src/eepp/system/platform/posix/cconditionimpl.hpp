#ifndef EE_SYSTEMCCONDITIONIMPLPOSIX_HPP
#define EE_SYSTEMCCONDITIONIMPLPOSIX_HPP

#include <eepp/declares.hpp>

#if defined( EE_PLATFORM_POSIX )

#include <pthread.h>

namespace EE { namespace System { namespace Platform {

class cConditionImpl {
	public:
		cConditionImpl( int var );
		
		~cConditionImpl();
		
		void Lock();
		
		void Unlock();
		
		bool WaitAndRetain( int value );
		
		void Release( int value );
		
		void SetValue( int value );
		
		int Value() const;
		
		void Signal();
		
		void Invalidate();
		
		void Restore();
	private:
		int mIsInvalid;
		int mConditionnedVar;
		
		pthread_cond_t mCond;
		pthread_mutex_t mMutex;
};
	
}}}

#endif

#endif
