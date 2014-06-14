#ifndef EE_SYSTEMCTHREADIMPLPOSIX_HPP
#define EE_SYSTEMCTHREADIMPLPOSIX_HPP

#include <eepp/config.hpp>

#if defined( EE_PLATFORM_POSIX )

#include <pthread.h>

namespace EE { namespace System {

class Thread;

namespace Platform {

class ThreadImpl {
	public:
		static UintPtr GetCurrentThreadId();

		ThreadImpl( Thread * owner );
		
		void Wait();
		
		void Terminate();

		UintPtr Id();
	protected:
		static void *	EntryPoint( void* userData );

		pthread_t		mThread;
		bool			mIsActive;
};

}}}

#endif

#endif
