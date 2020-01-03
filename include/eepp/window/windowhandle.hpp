#ifndef EE_WINDOWHANDLE_HPP
#define EE_WINDOWHANDLE_HPP

#include <eepp/config.hpp>

#if EE_PLATFORM == EE_PLATFORM_WIN
struct HWND__;
#elif defined( EE_X11_PLATFORM )
struct _XDisplay;
#endif

namespace EE { namespace Window {

#if EE_PLATFORM == EE_PLATFORM_WIN

	typedef HWND__*			eeWindowHandle;

#elif defined( EE_X11_PLATFORM )

	typedef unsigned long			X11Window;
	typedef unsigned long			X11Cursor;

	typedef struct _XDisplay *		eeWindowHandle;

#elif EE_PLATFORM == EE_PLATFORM_MACOSX
	typedef void *			eeWindowHandle; // NSWindow *
#else
	typedef unsigned int	eeWindowHandle; //! Fallback
#endif

}}

#endif

