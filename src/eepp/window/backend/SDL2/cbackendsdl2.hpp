#ifndef EE_WINDOWCBACKENDSDL2_HPP
#define EE_WINDOWCBACKENDSDL2_HPP

#include <eepp/window/cbackend.hpp>
#include <eepp/window/backend/SDL2/base.hpp>

#ifdef EE_BACKEND_SDL2

#include <eepp/window/backend/SDL2/cwindowsdl2.hpp>

namespace EE { namespace Window { namespace Backend { namespace SDL2 {

class EE_API cBackendSDL2 : public cBackend {
	public:
		cBackendSDL2();
		
		~cBackendSDL2();
};

}}}}

#endif

#endif
