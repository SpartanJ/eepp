#ifndef EE_SYSTEMCCONDITIONIMPLWIN_HPP
#define EE_SYSTEMCCONDITIONIMPLWIN_HPP

#include <eepp/config.hpp>

#if EE_PLATFORM == EE_PLATFORM_WIN

#include <eepp/system/platform/win/muteximpl.hpp>

namespace EE { namespace System { namespace Platform {


class ConditionImpl {
	public:
		ConditionImpl( int var );
		
		~ConditionImpl();
		
		void lock();
		
		void unlock();
		
		bool waitAndRetain( int value );
		
		void release( int value );
		
		void setValue( int value );
		
		int value() const;
		
		void signal();
		
		void invalidate();
		
		void restore();
	private:
		int mIsValid;
		int mConditionnedVar;
		
		HANDLE mCond;
		MutexImpl mMutex;
};
	
}}}

#endif

#endif
