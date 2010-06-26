#ifndef EE_SYSTEMCTHREAD_H
#define EE_SYSTEMCTHREAD_H

#include "base.hpp"

namespace EE { namespace System {

/** @brief Thread manager class */
class EE_API cThread {
	public:
		typedef void (*FuncType)(void*);

		cThread(FuncType Function, void* UserData = NULL);
		virtual ~cThread();

		/** Launch the thread */
		void Launch();

		/** Wait the thread until end */
		void Wait();

		/** Terminate the thread */
		void Terminate();
	protected:
		cThread();
		bool mIsActive;
	private:
		/** The virtual function to run in the thread */
		virtual void Run();
		static int ThreadFunc(void* UserData);

		SDL_Thread* mThreadPtr;
		FuncType mFunction;
		void* mUserData;
};

}}

#endif
