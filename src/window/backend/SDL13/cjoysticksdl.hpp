#ifndef EE_WINDOWCJOYSTICKSDL13_HPP
#define EE_WINDOWCJOYSTICKSDL13_HPP

#include "../../cbackend.hpp"
#include "base.hpp"

#ifdef EE_BACKEND_SDL_1_3

#include "../../cjoystick.hpp"

namespace EE { namespace Window { namespace Backend { namespace SDL13 {

class EE_API cJoystickSDL : public cJoystick {
	public:
		cJoystickSDL( const Uint32& index );

		virtual ~cJoystickSDL();

		void 		Close();

		void 		Open();

		void		Update();

		Uint8		GetHat( const Int32& index );

		eeFloat		GetAxis( const Int32& axis );

		eeVector2i	GetBallMotion( const Int32& ball );

		bool		Plugged() const;
	protected:
		SDL_Joystick * mJoystick;
};

}}}}

#endif

#endif
