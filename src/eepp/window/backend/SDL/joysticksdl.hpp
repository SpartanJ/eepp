#ifndef EE_WINDOWCJOYSTICKSDL_HPP
#define EE_WINDOWCJOYSTICKSDL_HPP

#include <eepp/window/backend.hpp>
#include <eepp/window/backend/SDL/base.hpp>

#ifdef EE_BACKEND_SDL_1_2

#include <eepp/window/joystick.hpp>
#if !defined( EE_COMPILER_MSVC )
#include <SDL/SDL.h>
#else
#include <SDL.h>
#endif

namespace EE { namespace Window { namespace Backend { namespace SDL {

class EE_API JoystickSDL : public Joystick {
	public:
		JoystickSDL( const Uint32& index );

		virtual ~JoystickSDL();

		void 		close();

		void 		open();

		void		update();

		Uint8		getHat( const Int32& index );

		Float		getAxis( const Int32& axis );

		Vector2i	getBallMotion( const Int32& ball );

		bool		isPlugged() const;
	protected:
		SDL_Joystick * mJoystick;
};

}}}}

#endif

#endif
