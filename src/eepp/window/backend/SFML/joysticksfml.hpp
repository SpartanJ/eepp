#ifndef EE_WINDOWCJOYSTICKSFML_HPP
#define EE_WINDOWCJOYSTICKSFML_HPP

#ifdef EE_BACKEND_SFML_ACTIVE

#include <eepp/window/joystick.hpp>

namespace EE { namespace Window { namespace Backend { namespace SFML {

class EE_API JoystickSFML : public Joystick {
	public:
		JoystickSFML( const Uint32& index );

		virtual ~JoystickSFML();

		void 		close();

		void 		open();

		void		update();

		Uint8		getHat( const Int32& index );

		Float		getAxis( const Int32& axis );

		Vector2i	getBallMotion( const Int32& ball );

		bool		isPlugged() const;
	protected:
		void		calcHat();

		Uint8		mHat;
};

}}}}

#endif

#endif
