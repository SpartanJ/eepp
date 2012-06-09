#ifndef EE_WINDOWCJOYSTICKMANAGERAl_HPP
#define EE_WINDOWCJOYSTICKMANAGERAl_HPP

#include <eepp/window/cbackend.hpp>

#ifdef EE_BACKEND_ALLEGRO_ACTIVE

#include <eepp/window/cjoystickmanager.hpp>
#include <allegro5/allegro.h>

namespace EE { namespace Window { namespace Backend { namespace Al {

class EE_API cJoystickManagerAl : public cJoystickManager {
	public:
		cJoystickManagerAl();
		
		virtual ~cJoystickManagerAl();
		
		void Update();

		void Close();

		void Open();
	protected:
		void Create( const Uint32& index );
};

}}}}

#endif

#endif
