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
		static UintPtr getCurrentThreadId();

		ThreadImpl( Thread * owner );
		
		void wait();
		
		void terminate();

		UintPtr id();
	protected:
		static void *	entryPoint( void* userData );

		pthread_t		mThread;
		bool			mIsActive;
};

}}}

#endif

#endif
