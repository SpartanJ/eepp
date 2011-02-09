#ifndef EE_WINDOWCBACKENDSDL_HPP
#define EE_WINDOWCBACKENDSDL_HPP

#include "../../cbackend.hpp"
#include <SDL/SDL.h>

namespace EE { namespace Window { namespace Backend { namespace SDL {

class EE_API cBackendSDL : public cBackend {
	public:
		inline cBackendSDL() : cBackend()
		{
		}

		inline ~cBackendSDL()
		{
			SDL_Quit();
		}
};

}}}}

#endif
