#ifndef EE_WINDOWCBACKEND_HPP
#define EE_WINDOWCBACKEND_HPP

#include "../base.hpp"

#ifndef EE_BACKEND_ALLEGRO_ACTIVE
#define EE_BACKEND_ALLEGRO_ACTIVE
#endif

#ifndef EE_BACKEND_ALLEGRO_ACTIVE
#define EE_BACKEND_SDL_ACTIVE
#endif

namespace EE { namespace Window { namespace Backend {

class EE_API cBackend {
	public:
		inline cBackend() {};
		
		inline virtual ~cBackend() {};
};

}}}

#endif
