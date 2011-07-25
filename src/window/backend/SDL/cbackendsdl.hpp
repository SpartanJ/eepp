#ifndef EE_WINDOWCBACKENDSDL_HPP
#define EE_WINDOWCBACKENDSDL_HPP

#include "../../cbackend.hpp"
#include "base.hpp"

#ifdef EE_BACKEND_SDL_1_2

#include "cwindowsdl.hpp"

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

#endif
