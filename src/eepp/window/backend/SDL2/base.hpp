#ifndef EE_WINDOWBACKEND_BASE_SDL2_HPP
#define EE_WINDOWBACKEND_BASE_SDL2_HPP

#include <eepp/window/base.hpp>

#ifdef EE_BACKEND_SDL_ACTIVE

	#if defined( EE_SDL_VERSION_2 )
		#ifndef EE_BACKEND_SDL2
			#define EE_BACKEND_SDL2
		#endif

		#include <SDL2/SDL.h>
	#else
		#ifndef EE_BACKEND_SDL_1_2
			#define EE_BACKEND_SDL_1_2
		#endif
	#endif

#endif

#endif
