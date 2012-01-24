#ifndef EE_WINDOWBACKEND_BASE_SDL12_HPP
#define EE_WINDOWBACKEND_BASE_SDL12_HPP

#include "../../base.hpp"

#ifdef EE_BACKEND_SDL_ACTIVE

	#if defined( EE_SDL_VERSION_1_2 )
		#ifndef EE_BACKEND_SDL_1_2
			#define EE_BACKEND_SDL_1_2
		#endif

		#include <SDL/SDL.h>
	#endif

#endif

#endif
