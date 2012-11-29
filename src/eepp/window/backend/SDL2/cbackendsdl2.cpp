#include <eepp/window/backend/SDL2/cbackendsdl2.hpp>

#ifdef EE_BACKEND_SDL2

#include <SDL2/SDL.h>

namespace EE { namespace Window { namespace Backend { namespace SDL2 {

cBackendSDL2::cBackendSDL2() : 
	cBackend()
{
}

cBackendSDL2::~cBackendSDL2()
{
	SDL_Quit();
}

}}}}

#endif
