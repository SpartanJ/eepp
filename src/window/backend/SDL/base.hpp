#ifndef EE_WINDOWBACKEND_BASE_SDL12_HPP
#define EE_WINDOWBACKEND_BASE_SDL12_HPP

#include "../../base.hpp"

#ifdef EE_BACKEND_SDL_ACTIVE

#include <SDL/SDL.h>

#if SDL_VERSION_ATLEAST(1,3,0)
	#ifndef EE_BACKEND_SDL_1_3
	#define EE_BACKEND_SDL_1_3
	#endif
#else
	#ifndef EE_BACKEND_SDL_1_2
	#define EE_BACKEND_SDL_1_2
	#endif
#endif

#endif

#endif
