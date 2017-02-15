#ifndef EE_WINDOWCJOYSTICKMANAGERNULL_HPP
#define EE_WINDOWCJOYSTICKMANAGERNULL_HPP

#include <eepp/window/joystickmanager.hpp>

namespace EE { namespace Window { namespace Backend { namespace Null {

class EE_API JoystickManagerNull : public JoystickManager {
	public:
		JoystickManagerNull();
		
		virtual ~JoystickManagerNull();
		
		void update();

		void close();

		void open();
	protected:
		void create( const Uint32& index );
};

}}}}

#endif
