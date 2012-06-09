#ifndef EE_WINDOWCJOYSTICKNULL_HPP
#define EE_WINDOWCJOYSTICKNULL_HPP

#include <eepp/window/cjoystick.hpp>

namespace EE { namespace Window { namespace Backend { namespace Null {

class EE_API cJoystickNull : public cJoystick {
	public:
		cJoystickNull( const Uint32& index );

		virtual ~cJoystickNull();

		void 		Close();

		void 		Open();

		void		Update();

		Uint8		GetHat( const Int32& index );

		eeFloat		GetAxis( const Int32& axis );

		eeVector2i	GetBallMotion( const Int32& ball );

		bool		Plugged() const;
	protected:
};

}}}}

#endif
