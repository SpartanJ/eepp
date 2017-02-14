#ifndef EE_SYSTEMCMUTEXIMPLWIN_HPP
#define EE_SYSTEMCMUTEXIMPLWIN_HPP

#include <eepp/config.hpp>

#if EE_PLATFORM == EE_PLATFORM_WIN

#ifndef WIN32_LEAN_AND_MEAN
	#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
	#define NOMINMAX
#endif
#include <windows.h>

namespace EE { namespace System { namespace Platform {

class MutexImpl {
	public:
		MutexImpl();

		~MutexImpl();

		void lock();

		void unlock();

		int tryLock();
	private:
		CRITICAL_SECTION mMutex;
};

}}}

#endif

#endif
