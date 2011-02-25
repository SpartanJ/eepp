#ifndef EE_WINDOW_BASE
#define EE_WINDOW_BASE

#include "../base.hpp"

#include "../utils/colors.hpp"
#include "../utils/rect.hpp"
#include "../utils/vector2.hpp"
#include "../utils/string.hpp"
#include "../utils/utils.hpp"
using namespace EE::Utils;

#include "../system/ctimeelapsed.hpp"
#include "../system/tsingleton.hpp"
#include "../system/clog.hpp"
using namespace EE::System;

#include "../graphics/renders.hpp"
#include "../graphics/glhelper.hpp"
using namespace EE::Graphics;

#if EE_PLATFORM == EE_PLATFORM_WIN
	inline BOOL WIN_ShowWindow( HWND hWnd, int nCmdShow ) {
		return ShowWindow( hWnd, nCmdShow );
	}

	typedef UINT			eeScrapType;
	typedef HWND			eeWindowHandler;

	#ifdef CreateWindow
	#undef CreateWindow
	#endif
#elif defined( EE_X11_PLATFORM )
	#include <X11/Xlib.h>
	#include <X11/Xcursor/Xcursor.h>
	#include <X11/cursorfont.h>
	typedef Atom			eeScrapType;
	typedef Window			X11Window;
	typedef Display	*		eeWindowHandler;
	typedef Cursor			X11Cursor;
	#undef Window
	#undef Display
	#undef Cursor
#elif EE_PLATFORM == EE_PLATFORM_MACOSX
	typedef unsigned int	eeScrapType;
	typedef void *			eeWindowHandler; // NSWindow *
#else
	typedef unsigned int	eeWindowHandler; //! Fallback
	typedef unsigned int	eeScrapType;
#endif

#ifdef EE_GLEW_AVAILABLE
	#if EE_PLATFORM == EE_PLATFORM_WIN
		#include "../helper/glew/wglew.h"
		typedef HGLRC		eeWindowContex;

	#elif defined( EE_X11_PLATFORM )

		#include "../helper/glew/glxew.h"
		typedef GLXContext	eeWindowContex;

	#elif EE_PLATFORM == EE_PLATFORM_MACOSX
		#include <AGL/agl.h>

		typedef AGLContext	eeWindowContex;
	#endif
#else
	typedef unsigned int	eeWindowContex;	//! Fallback
#endif

#include "keycodes.hpp"

#endif
