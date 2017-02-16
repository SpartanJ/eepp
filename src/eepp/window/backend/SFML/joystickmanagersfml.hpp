#ifndef EE_WINDOWCJOYSTICKMANAGERSFML_HPP
#define EE_WINDOWCJOYSTICKMANAGERSFML_HPP

#ifdef EE_BACKEND_SFML_ACTIVE

#include <eepp/window/joystickmanager.hpp>

namespace EE { namespace Window { namespace Backend { namespace SFML {

class EE_API JoystickManagerSFML : public JoystickManager {
	public:
		JoystickManagerSFML();
		
		virtual ~JoystickManagerSFML();
		
		void update();

		void close();

		void open();
	protected:
		void create( const Uint32& index );
};

}}}}

#endif

#endif
