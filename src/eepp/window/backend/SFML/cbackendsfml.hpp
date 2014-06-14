#ifndef EE_WINDOWCBACKENDSFML_HPP
#define EE_WINDOWCBACKENDSFML_HPP

#ifdef EE_BACKEND_SFML_ACTIVE

#include <eepp/window/cbackend.hpp>
#include <eepp/window/backend/SFML/cwindowsfml.hpp>

namespace EE { namespace Window { namespace Backend { namespace SFML {

class EE_API WindowBackendSFML : public WindowBackend {
	public:
		inline WindowBackendSFML() : WindowBackend()
		{
		}

		inline ~WindowBackendSFML()
		{
		}
};

}}}}

#endif

#endif
