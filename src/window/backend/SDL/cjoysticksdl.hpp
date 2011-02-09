#ifndef EE_WINDOWCJOYSTICKSDL_HPP
#define EE_WINDOWCJOYSTICKSDL_HPP

#include "../../cjoystick.hpp"
#include <SDL/SDL.h>

namespace EE { namespace Window { namespace Backend { namespace SDL {

class EE_API cJoystickSDL : public cJoystick {
	public:
		cJoystickSDL( const Uint32& index );

		~cJoystickSDL();

		void 		Close();

		void 		Open();

		void		Update();

		Uint8		GetHat( const Int32& index );

		Int16		GetAxis( const Int32& axis );

		eeVector2i	GetBallMotion( const Int32& ball );

		bool		Plugged() const;
	protected:
		SDL_Joystick * mJoystick;
};

}}}}

#endif
