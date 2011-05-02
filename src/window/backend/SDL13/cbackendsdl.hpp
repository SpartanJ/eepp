#ifndef EE_WINDOWCBACKENDSDL13_HPP
#define EE_WINDOWCBACKENDSDL13_HPP

#include "../../cbackend.hpp"

#ifdef EE_BACKEND_SDL_ACTIVE

#include "cwindowsdl.hpp"

namespace EE { namespace Window { namespace Backend { namespace SDL13 {

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
