#ifndef EE_WINDOWCJOYSTICKAl_HPP
#define EE_WINDOWCJOYSTICKAl_HPP

#include <eepp/window/cbackend.hpp>

#ifdef EE_BACKEND_ALLEGRO_ACTIVE

#include <eepp/window/cjoystick.hpp>
#include <allegro5/allegro.h>

namespace EE { namespace Window { namespace Backend { namespace Al {

class EE_API cJoystickAl : public cJoystick {
	public:
		cJoystickAl( const Uint32& index );

		virtual ~cJoystickAl();

		void 		Close();

		void 		Open();

		void		Update();

		Uint8		GetHat( const Int32& index );

		eeFloat		GetAxis( const Int32& axis );

		eeVector2i	GetBallMotion( const Int32& ball );

		bool		Plugged() const;
	protected:
		ALLEGRO_JOYSTICK * mJoystick;
		ALLEGRO_JOYSTICK_STATE * mJoyState;
		Uint32	mSticks;
};

}}}}

#endif

#endif
