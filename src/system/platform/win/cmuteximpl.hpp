#ifndef EE_SYSTEMCMUTEXIMPLWIN_HPP
#define EE_SYSTEMCMUTEXIMPLWIN_HPP

#include "../../../declares.hpp"

#if EE_PLATFORM == EE_PLATFORM_WIN

#ifndef WIN32_LEAN_AND_MEAN
	#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace EE { namespace System { namespace Platform {

class cMutexImpl {
	public:
		cMutexImpl();

		~cMutexImpl();

		void Lock();

		void Unlock();
	private:
		CRITICAL_SECTION mMutex;
};

}}}

#endif

#endif
 
