#ifndef EE_SYSTEMPLATFORMIMPL_HPP
#define EE_SYSTEMPLATFORMIMPL_HPP

#include <eepp/config.hpp>

#if defined( EE_PLATFORM_POSIX )
	#include <eepp/system/platform/posix/cthreadimpl.hpp>
	#include <eepp/system/platform/posix/cmuteximpl.hpp>
	#include <eepp/system/platform/posix/cclockimpl.hpp>
	#include <eepp/system/platform/posix/cconditionimpl.hpp>
	#include <eepp/system/platform/posix/cthreadlocalimpl.hpp>
#elif EE_PLATFORM == EE_PLATFORM_WIN
	#include <eepp/system/platform/win/cthreadimpl.hpp>
	#include <eepp/system/platform/win/cmuteximpl.hpp>
	#include <eepp/system/platform/win/cclockimpl.hpp>
	#include <eepp/system/platform/win/cconditionimpl.hpp>
	#include <eepp/system/platform/win/cthreadlocalimpl.hpp>
#else
	#error Threads, mutexes, conditions, timers and thread local storage not implemented for this platform.
#endif

#endif
