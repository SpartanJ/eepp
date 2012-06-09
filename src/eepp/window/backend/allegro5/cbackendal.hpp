#ifndef EE_WINDOWCBACKENDAl_HPP
#define EE_WINDOWCBACKENDAl_HPP

#include <eepp/window/cbackend.hpp>

#ifdef EE_BACKEND_ALLEGRO_ACTIVE

#include <eepp/window/backend/allegro5/cwindowal.hpp>
#include <allegro5/allegro.h>

namespace EE { namespace Window { namespace Backend { namespace Al {

class EE_API cBackendAl : public cBackend {
	public:
		inline cBackendAl() : cBackend()
		{
			al_init();
		}

		inline ~cBackendAl()
		{	
		}
};

}}}}

#endif

#endif
