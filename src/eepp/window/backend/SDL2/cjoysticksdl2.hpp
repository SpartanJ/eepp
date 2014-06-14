#ifndef EE_WINDOWCJOYSTICKSDL2_HPP
#define EE_WINDOWCJOYSTICKSDL2_HPP

#include <eepp/window/cbackend.hpp>
#include <eepp/window/backend/SDL2/base.hpp>

#ifdef EE_BACKEND_SDL2

#include <eepp/window/cjoystick.hpp>

namespace EE { namespace Window { namespace Backend { namespace SDL2 {

class EE_API JoystickSDL : public Joystick {
	public:
		JoystickSDL( const Uint32& index );

		virtual ~JoystickSDL();

		void 		Close();

		void 		Open();

		void		Update();

		Uint8		GetHat( const Int32& index );

		Float		GetAxis( const Int32& axis );

		Vector2i	GetBallMotion( const Int32& ball );

		bool		Plugged() const;
	protected:
		SDL_Joystick * mJoystick;
};

}}}}

#endif

#endif
