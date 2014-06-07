#ifndef EE_SYSTEMCCONDITIONIMPLWIN_HPP
#define EE_SYSTEMCCONDITIONIMPLWIN_HPP

#include <eepp/config.hpp>

#if EE_PLATFORM == EE_PLATFORM_WIN

#include <eepp/system/platform/win/cmuteximpl.hpp>

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
		int mIsValid;
		int mConditionnedVar;
		
		HANDLE mCond;
		cMutexImpl mMutex;
};
	
}}}

#endif

#endif
