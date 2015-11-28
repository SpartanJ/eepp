#ifndef EE_NETWORK_PLATFORMIMPL_HPP
#define EE_NETWORK_PLATFORMIMPL_HPP

#include <eepp/config.hpp>

#if defined( EE_PLATFORM_POSIX )
	#include <eepp/network/platform/unix/socketimpl.hpp>
#elif EE_PLATFORM == EE_PLATFORM_WIN
	#include <eepp/network/platform/win/socketimpl.hpp>
#else
	#error Sockets not implemented for this platform
#endif

#endif
