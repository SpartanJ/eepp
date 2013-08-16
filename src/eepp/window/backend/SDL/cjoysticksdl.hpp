#ifndef EE_WINDOWCJOYSTICKSDL_HPP
#define EE_WINDOWCJOYSTICKSDL_HPP

#include <eepp/window/cbackend.hpp>
#include <eepp/window/backend/SDL/base.hpp>

#ifdef EE_BACKEND_SDL_1_2

#include <eepp/window/cjoystick.hpp>
#if !defined( EE_COMPILER_MSVC )
#include <SDL/SDL.h>
#else
#include <SDL.h>
#endif

namespace EE { namespace Window { namespace Backend { namespace SDL {

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
