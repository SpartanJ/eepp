#ifndef EE_SYSTEMPLATFORMIMPL_HPP
#define EE_SYSTEMPLATFORMIMPL_HPP

#include <eepp/declares.hpp>

#if defined( EE_PLATFORM_POSIX )
	#include <eepp/system/platform/posix/cthreadimpl.hpp>
	#include <eepp/system/platform/posix/cmuteximpl.hpp>
	#include <eepp/system/platform/posix/cclockimpl.hpp>
	#include <eepp/system/platform/posix/cconditionimpl.hpp>
#elif EE_PLATFORM == EE_PLATFORM_WIN
	#include <eepp/system/platform/win/cthreadimpl.hpp>
	#include <eepp/system/platform/win/cmuteximpl.hpp>
	#include <eepp/system/platform/win/cclockimpl.hpp>
	#include <eepp/system/platform/win/cconditionimpl.hpp>
#else
	#error Threads, mutexes, conditions and timers not implemented for this platform.
#endif

#endif
