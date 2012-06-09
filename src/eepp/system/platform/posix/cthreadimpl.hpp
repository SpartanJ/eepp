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
		cThreadImpl( cThread * owner );
		
		void Wait();
		
		void Terminate();
	protected:
		static void *	EntryPoint( void* userData );

		pthread_t		mThread;
		bool			mIsActive;
};

}}}

#endif

#endif
