#ifndef EE_NETWORKSOCKETHANDLE_HPP
#define EE_NETWORKSOCKETHANDLE_HPP

#include <eepp/declares.hpp>

#if EE_PLATFORM == EE_PLATFORM_WIN
	#include <basetsd.h>
#endif

namespace EE { namespace Network {

/** Define the low-level socket handle type, specific to each platform */

#if EE_PLATFORM == EE_PLATFORM_WIN
	typedef UINT_PTR SocketHandle;
#else
	typedef int SocketHandle;
#endif

}}

#endif // EE_NETWORKSOCKETHANDLE_HPP
