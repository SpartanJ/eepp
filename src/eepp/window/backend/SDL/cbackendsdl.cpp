#include <eepp/window/backend/SDL/cbackendsdl.hpp>

#ifdef EE_BACKEND_SDL_1_2

#include <SDL/SDL.h>

namespace EE { namespace Window { namespace Backend { namespace SDL {

WindowBackendSDL::WindowBackendSDL() : 
	WindowBackend()
{
}

WindowBackendSDL::~WindowBackendSDL()
{
	SDL_Quit();
}

}}}}

#endif
