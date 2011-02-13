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
#elif EE_PLATFORM == EE_PLATFORM_LINUX
	#include <X11/Xlib.h>
	typedef Atom			eeScrapType;
	typedef Window			X11Window;
	typedef Display	*		eeWindowHandler;
	#undef Window
	#undef Display
#elif EE_PLATFORM == EE_PLATFORM_MACOSX
	typedef Uint32			eeScrapType;
	typedef void *			eeWindowHandler; // NSWindow *
#else
	typedef Uint32			eeWindowHandler; //! Fallback
	typedef Uint32			eeScrapType;
#endif

#ifdef EE_GLEW_AVAILABLE
	#if EE_PLATFORM == EE_PLATFORM_WIN
		#include "../helper/glew/wglew.h"
		typedef HGLRC		eeWindowContex;

	#elif EE_PLATFORM == EE_PLATFORM_LINUX

		#include "../helper/glew/glxew.h"
		typedef GLXContext	eeWindowContex;

	#elif EE_PLATFORM == EE_PLATFORM_MACOSX
		#include <AGL/agl.h>

		typedef AGLContext	eeWindowContex;
	#endif
#else
	typedef Uint32			eeWindowContex;	//! Fallback
#endif

#include "keycodes.hpp"

#endif
