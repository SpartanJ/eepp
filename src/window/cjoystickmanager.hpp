#ifndef EE_WINDOWCJOYSTICKMANAGER_HPP
#define EE_WINDOWCJOYSTICKMANAGER_HPP

#include "base.hpp"
#include "cjoystick.hpp"

namespace EE { namespace Window {

class EE_API cJoystickManager {
	public:
		cJoystickManager();

		virtual ~cJoystickManager();

		virtual Uint32 Count();

		virtual void 	Update() = 0;

		cJoystick * 	GetJoystick( const Uint32& index );

		virtual void	Rescan();

		virtual void 	Close();

		virtual void 	Open();
	protected:
		friend class cJoystick;
		
		bool			mInit;

		cJoystick * 	mJoysticks[ MAX_JOYSTICKS ];

		Uint32			mCount;

		virtual void 	Create( const Uint32& index ) = 0;
};

}}

#endif
