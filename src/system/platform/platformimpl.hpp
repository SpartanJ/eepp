#ifndef EE_SYSTEMPLATFORMIMPL_HPP
#define EE_SYSTEMPLATFORMIMPL_HPP

#include "../../declares.hpp"

#if defined( EE_PLATFORM_POSIX )
	#include "posix/cthreadimpl.hpp"
	#include "posix/cmuteximpl.hpp"
	#include "posix/ctimerimpl.hpp"
#elif EE_PLATFORM == EE_PLATFORM_WIN
	#include "win/cthreadimpl.hpp"
	#include "win/cmuteximpl.hpp"
	#include "win/ctimerimpl.hpp"
#else
	#error Threads, mutexes and timers not implemented for this platform.
#endif

#endif
