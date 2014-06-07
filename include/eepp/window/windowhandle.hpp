#ifndef EE_WINDOWHANDLE_HPP
#define EE_WINDOWHANDLE_HPP

#include <eepp/config.hpp>

#if EE_PLATFORM == EE_PLATFORM_WIN

	struct HWND__;
	typedef HWND__*			eeWindowHandle;

#elif defined( EE_X11_PLATFORM )

	typedef unsigned long			X11Window;
	typedef unsigned long			X11Cursor;
	
	struct _XDisplay;
	typedef struct _XDisplay *		eeWindowHandle;
	
#elif EE_PLATFORM == EE_PLATFORM_MACOSX
	typedef void *			eeWindowHandle; // NSWindow *
#else
	typedef unsigned int	eeWindowHandle; //! Fallback
#endif


#endif

