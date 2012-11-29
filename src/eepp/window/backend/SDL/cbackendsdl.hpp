#ifndef EE_WINDOWCBACKENDSDL_HPP
#define EE_WINDOWCBACKENDSDL_HPP

#include <eepp/window/cbackend.hpp>
#include <eepp/window/backend/SDL/base.hpp>

#ifdef EE_BACKEND_SDL_1_2

#include <eepp/window/backend/SDL/cwindowsdl.hpp>

namespace EE { namespace Window { namespace Backend { namespace SDL {

class EE_API cBackendSDL : public cBackend {
	public:
		cBackendSDL();

		~cBackendSDL();
};

}}}}

#endif

#endif
