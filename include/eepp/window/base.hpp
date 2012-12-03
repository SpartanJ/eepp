#ifndef EE_WINDOW_BASE
#define EE_WINDOW_BASE

#include <eepp/base.hpp>

#include <eepp/math/rect.hpp>
#include <eepp/math/vector2.hpp>
using namespace EE::Math;

#include <eepp/system/colors.hpp>
#include <eepp/system/bitop.hpp>
#include <eepp/system/ctimeelapsed.hpp>
#include <eepp/system/tsingleton.hpp>
#include <eepp/system/clog.hpp>
using namespace EE::System;

#include <eepp/graphics/renders.hpp>
#include <eepp/graphics/glhelper.hpp>
using namespace EE::Graphics;

#if EE_PLATFORM == EE_PLATFORM_WIN
	#ifndef WIN32_LEAN_AND_MEAN
		#define WIN32_LEAN_AND_MEAN
	#endif
	#include <windows.h>

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
	#include <X11/Xatom.h>
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
		#include <eepp/helper/glew/wglew.h>
		typedef HGLRC		eeWindowContex;

	#elif defined( EE_X11_PLATFORM )

		#include <eepp/helper/glew/glxew.h>
		typedef GLXContext	eeWindowContex;

	#elif EE_PLATFORM == EE_PLATFORM_MACOSX
		#include <AGL/agl.h>

		typedef AGLContext	eeWindowContex;
	#endif
#else
	typedef unsigned int	eeWindowContex;	//! Fallback
#endif

#include <eepp/window/keycodes.hpp>

#endif
