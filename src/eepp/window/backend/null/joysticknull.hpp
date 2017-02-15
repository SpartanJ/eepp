#ifndef EE_WINDOWCJOYSTICKNULL_HPP
#define EE_WINDOWCJOYSTICKNULL_HPP

#include <eepp/window/joystick.hpp>

namespace EE { namespace Window { namespace Backend { namespace Null {

class EE_API JoystickNull : public Joystick {
	public:
		JoystickNull( const Uint32& index );

		virtual ~JoystickNull();

		void 		close();

		void 		open();

		void		update();

		Uint8		getHat( const Int32& index );

		Float		getAxis( const Int32& axis );

		Vector2i	getBallMotion( const Int32& ball );

		bool		isPlugged() const;
	protected:
};

}}}}

#endif
