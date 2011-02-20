#ifndef EE_SYSTEMCTHREAD_H
#define EE_SYSTEMCTHREAD_H

#include "base.hpp"

#if EE_PLATFORM == EE_PLATFORM_WIN
#ifndef WIN32_LEAN_AND_MEAN
	#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <process.h>
#elif defined( EE_PLATFORM_POSIX )
#include <pthread.h>
#endif

namespace EE { namespace System {

/** @brief Thread manager class */
class EE_API cThread {
	public:
		typedef void (*FuncType)(void*);

		cThread( FuncType Function, void* UserData = NULL );

		virtual ~cThread();

		/** Launch the thread */
		virtual void	Launch();

		/** Wait the thread until end */
		void			Wait();

		/** Terminate the thread */
		void			Terminate();
	protected:
		cThread();

		bool			mIsActive;
	private:
		/** The virtual function to run in the thread */
		virtual void	Run();

		#if EE_PLATFORM == EE_PLATFORM_WIN

		static unsigned int __stdcall EntryPoint(void* userData);

		HANDLE			mThread;

		#elif defined( EE_PLATFORM_POSIX )

		static void* EntryPoint(void* userData);

		pthread_t		mThread;

		#endif

		FuncType		mFunction;

		void *			mUserData;
};

}}

#endif
