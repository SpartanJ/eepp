#ifndef EE_SYSTEMCTHREADIMPLPOSIX_HPP
#define EE_SYSTEMCTHREADIMPLPOSIX_HPP

#include <eepp/declares.hpp>

#if defined( EE_PLATFORM_POSIX )

#include <pthread.h>

namespace EE { namespace System {

class cThread;

namespace Platform {

class cThreadImpl {
	public:
		static Uint32 GetCurrentThreadId();

		cThreadImpl( cThread * owner );
		
		void Wait();
		
		void Terminate();

		Uint32 Id();
	protected:
		static void *	EntryPoint( void* userData );

		pthread_t		mThread;
		bool			mIsActive;
};

}}}

#endif

#endif
