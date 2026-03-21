#ifndef EE_WINDOWBACKEND_BASE_SDL3_HPP
#define EE_WINDOWBACKEND_BASE_SDL3_HPP

#include <eepp/core.hpp>

#ifdef EE_BACKEND_SDL_ACTIVE

#if defined( EE_SDL_VERSION_3 )
#ifndef EE_BACKEND_SDL3
#define EE_BACKEND_SDL3
#endif

#if ( EE_PLATFORM == EE_PLATFORM_ANDROID || EE_PLATFORM == EE_PLATFORM_IOS ) && defined( main )
#undef main
#endif

#if EE_PLATFORM != EE_PLATFORM_ANDROID && EE_PLATFORM != EE_PLATFORM_IOS &&  \
	EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN && !defined( EE_COMPILER_MSVC ) && \
	!defined( EE_SDL3_FROM_ROOTPATH )
#include <SDL3/SDL.h>
#else
#include <SDL.h>
#endif
#else
#ifndef EE_BACKEND_SDL_1_2
#define EE_BACKEND_SDL_1_2
#endif
#endif

#endif

#endif
