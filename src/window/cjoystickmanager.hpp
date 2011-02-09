#ifndef EE_WINDOWCJOYSTICKMANAGER_HPP
#define EE_WINDOWCJOYSTICKMANAGER_HPP

#include "base.hpp"
#include "cjoystick.hpp"

namespace EE { namespace Window {

class EE_API cJoystickManager {
	public:
		cJoystickManager();

		~cJoystickManager();

		virtual Uint32 Count();

		virtual void 	Update() = 0;

		cJoystick * 	GetJoystick( const Uint32& index );

		virtual void	Rescan();
	protected:
		friend class cJoystick;
		
		bool			mInit;

		cJoystick * 	mJoysticks[ MAX_JOYSTICKS ];

		Uint32			mCount;

		virtual void 	Close();

		virtual void 	Open();

		virtual void 	Create( const Uint32& index ) = 0;
};

}}

#endif
