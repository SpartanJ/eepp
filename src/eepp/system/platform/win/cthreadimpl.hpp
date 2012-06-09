#ifndef EE_SYSTEMCTHREADIMPLWIN_HPP
#define EE_SYSTEMCTHREADIMPLWIN_HPP

#include <eepp/declares.hpp>

#if EE_PLATFORM == EE_PLATFORM_WIN

#ifndef WIN32_LEAN_AND_MEAN
	#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <process.h>

namespace EE { namespace System {
class cThread;

namespace Platform {

class cThreadImpl {
	public:
		cThreadImpl( cThread * owner );
		
		void Wait();
		
		void Terminate();
	protected:
		static unsigned int __stdcall EntryPoint(void* userData);

		HANDLE			mThread;
		unsigned int	mThreadId;
};

}}}

#endif

#endif
