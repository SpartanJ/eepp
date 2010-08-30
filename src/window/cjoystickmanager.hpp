#ifndef EE_WINDOWCJOYSTICKMANAGER_HPP
#define EE_WINDOWCJOYSTICKMANAGER_HPP

#include "base.hpp"
#include "cjoystick.hpp"

namespace EE { namespace Window {

#define MAX_JOYSTICKS (16)

class EE_API cJoystickManager : public tSingleton<cJoystickManager> {
	friend class tSingleton<cJoystickManager>;
	friend class cJoystick;
	public:
		cJoystickManager();

		~cJoystickManager();

		Uint32 			Count();

		void 			Update();

		cJoystick * 	GetJoystick( const Uint32& index );

		void			Rescan();
	protected:
		bool			mInit;

		cJoystick * 	mJoysticks[ MAX_JOYSTICKS ];

		Uint32			mCount;
	private:
		void 			Close();

		void 			Open();

		void 			Create( const Uint32& index );
};

}}

#endif

