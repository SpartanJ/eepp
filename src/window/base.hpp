#ifndef EE_WINDOW_BASE
#define EE_WINDOW_BASE

#include "../base.hpp"
#include <SDL/SDL_syswm.h>

#if EE_PLATFORM == EE_PLATFORM_WIN
inline BOOL WIN_ShowWindow( HWND hWnd, int nCmdShow ) {
	return ShowWindow( hWnd, nCmdShow );
}

#include "../helper/glew/wglew.h"

typedef HGLRC			eeWindowContex;
typedef HWND			eeWindowHandler;
typedef UINT			eeScrapType;

#elif EE_PLATFORM == EE_PLATFORM_LINUX

#include "../helper/glew/glxew.h"
#include <X11/Xlib.h>

typedef Window			X11Window;

typedef GLXContext		eeWindowContex;
typedef Display	*		eeWindowHandler;
typedef Atom			eeScrapType;

#elif EE_PLATFORM == EE_PLATFORM_MACOSX
//#include <AGL/agl.h>

//typedef AGLContext	eeWindowContex;
//typedef NSWindow *	eeWindowHandler;
typedef Uint32			eeScrapType;
#endif

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
using namespace EE::Graphics;

#endif
