#ifndef EE_SYSTEMPLATFORMIMPL_HPP
#define EE_SYSTEMPLATFORMIMPL_HPP

#include <eepp/config.hpp>

#if defined( EE_PLATFORM_POSIX )
	#include <eepp/system/platform/posix/threadimpl.hpp>
	#include <eepp/system/platform/posix/muteximpl.hpp>
	#include <eepp/system/platform/posix/clockimpl.hpp>
	#include <eepp/system/platform/posix/conditionimpl.hpp>
	#include <eepp/system/platform/posix/threadlocalimpl.hpp>
#elif EE_PLATFORM == EE_PLATFORM_WIN
	#include <eepp/system/platform/win/threadimpl.hpp>
	#include <eepp/system/platform/win/muteximpl.hpp>
	#include <eepp/system/platform/win/clockimpl.hpp>
	#include <eepp/system/platform/win/conditionimpl.hpp>
	#include <eepp/system/platform/win/threadlocalimpl.hpp>
#else
	#error Threads, mutexes, conditions, timers and thread local storage not implemented for this platform.
#endif

#endif
