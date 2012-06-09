#ifndef EE_WINDOWCBACKENDSDL2_HPP
#define EE_WINDOWCBACKENDSDL2_HPP

#include <eepp/window/cbackend.hpp>

#ifdef EE_BACKEND_SDL_ACTIVE

#include <eepp/window/backend/SDL2/cwindowsdl.hpp>

namespace EE { namespace Window { namespace Backend { namespace SDL2 {

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

#endif
