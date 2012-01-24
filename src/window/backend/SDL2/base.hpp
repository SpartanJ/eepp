#ifndef EE_WINDOWBACKEND_BASE_SDL2_HPP
#define EE_WINDOWBACKEND_BASE_SDL2_HPP

#include "../../base.hpp"

#ifdef EE_BACKEND_SDL_ACTIVE

	#if defined( EE_SDL_VERSION_1_3 ) || defined( EE_SDL_VERSION_2 )
		#ifndef EE_BACKEND_SDL2
			#define EE_BACKEND_SDL2
		#endif
	#else
		#ifndef EE_BACKEND_SDL_1_2
			#define EE_BACKEND_SDL_1_2
		#endif
	#endif

	#if defined( EE_SDL_VERSION_1_3 )
		#include <SDL/SDL.h>
	#elif defined( EE_SDL_VERSION_2 )
		#include <SDL2/SDL.h>
	#endif

#endif

#endif
