#ifndef EE_WINDOWCJOYSTICKMANAGERAl_HPP
#define EE_WINDOWCJOYSTICKMANAGERAl_HPP

#include "../../cbackend.hpp"

#ifdef EE_BACKEND_ALLEGRO_ACTIVE

#include "../../cjoystickmanager.hpp"
#include <allegro5/allegro.h>

namespace EE { namespace Window { namespace Backend { namespace Al {

class EE_API cJoystickManagerAl : public cJoystickManager {
	public:
		cJoystickManagerAl();
		
		~cJoystickManagerAl();
		
		void Update();
	protected:
		void Close();

		void Open();
		
		void Create( const Uint32& index );
};

}}}}

#endif

#endif
