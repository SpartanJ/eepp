#ifndef EE_WINDOWHANDLE_HPP
#define EE_WINDOWHANDLE_HPP

#include <eepp/declares.hpp>

#if EE_PLATFORM == EE_PLATFORM_WIN

	struct HWND__;
	typedef HWND__*			eeWindowHandle;

#elif defined( EE_X11_PLATFORM )
	#include <X11/Xlib.h>
	#include <X11/Xcursor/Xcursor.h>
	#include <X11/cursorfont.h>
	#undef Window
	#undef Display
	#undef Cursor
	
	typedef unsigned long			X11Window;
	typedef unsigned long			X11Cursor;
	
	typedef Display	*		eeWindowHandle;
	

#elif EE_PLATFORM == EE_PLATFORM_MACOSX
	typedef void *			eeWindowHandle; // NSWindow *
#else
	typedef unsigned int	eeWindowHandle; //! Fallback
#endif


#endif

