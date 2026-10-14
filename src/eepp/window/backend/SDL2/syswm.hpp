#ifndef EE_BACKEND_SDL2_SYSWM_HPP
#define EE_BACKEND_SDL2_SYSWM_HPP

#include <eepp/window/backend/SDL2/base.hpp>

#ifdef EE_BACKEND_SDL2

// Xlib's global typedefs collide with eepp's namespace imports. Local aliases preserve the native
// types and SDL_SysWMinfo ABI while keeping those names out of the surrounding C++ translation
// unit.
#ifdef SDL_VIDEO_DRIVER_X11
#define Window SDLNativeWindow
#define Font SDLNativeFont
#define Cursor SDLNativeCursor
#define Time SDLNativeTime
#define Display SDLNativeDisplay
#define Drawable SDLNativeDrawable
#endif

#if !defined( EE_COMPILER_MSVC ) && EE_PLATFORM != EE_PLATFORM_ANDROID && \
	EE_PLATFORM != EE_PLATFORM_IOS && !defined( EE_SDL2_FROM_ROOTPATH )
#include <SDL2/SDL_syswm.h>
#else
#include <SDL_syswm.h>
#endif

// Win32 registry access flags also use the name of an eepp key code.
#ifdef KEY_EXECUTE
#undef KEY_EXECUTE
#endif

#ifdef SDL_VIDEO_DRIVER_X11
#undef Window
#undef Font
#undef Cursor
#undef Time
#undef Display
#undef Drawable
#undef None
#undef Status
#endif

#endif

#endif
