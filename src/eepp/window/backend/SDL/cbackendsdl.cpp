#include <eepp/window/backend/SDL/cbackendsdl.hpp>

#ifdef EE_BACKEND_SDL_1_2

#include <SDL/SDL.h>

namespace EE { namespace Window { namespace Backend { namespace SDL {

cBackendSDL::cBackendSDL() : 
	cBackend()
{
}

cBackendSDL::~cBackendSDL()
{
	SDL_Quit();
}

}}}}

#endif
