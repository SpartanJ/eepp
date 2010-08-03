#ifndef EE_WINDOWCJOYSTICK_HPP
#define EE_WINDOWCJOYSTICK_HPP

#include "base.hpp"

namespace EE { namespace Window {

#define HAT_CENTERED	0x00
#define HAT_UP			0x01
#define HAT_RIGHT		0x02
#define HAT_DOWN		0x04
#define HAT_LEFT		0x08
#define HAT_RIGHTUP		(HAT_RIGHT|HAT_UP)
#define HAT_RIGHTDOWN	(HAT_RIGHT|HAT_DOWN)
#define HAT_LEFTUP		(HAT_LEFT|HAT_UP)
#define HAT_LEFTDOWN	(HAT_LEFT|HAT_DOWN)

#define AXIS_X			(0)
#define AXIS_Y			(1)
#define AXIS_X2			(3)
#define AXIS_Y2			(2)

#define AXIS_MAX		(32768)
#define AXIS_MIN		(-32768)

class cJoystick {
	public:
		cJoystick( const Uint32& index );

		~cJoystick();

		void 			Close();

		void 			Open();

		void			ReOpen();

		bool			Plugged() const;

		void			Update();

		const Int32&	GetNumHats() const 		{ return mHats; 		}

		const Int32&	GetNumButtons() const 		{ return mButtons; 	}

		const Int32&	GetNumAxes() const 		{ return mAxes; 		}

		const Int32&	GetNumBalls() const 		{ return mBalls; 		}

		const Uint32&	GetButtonTrigger() const 	{ return mButtonDown;	}

		const Uint32&	GetButtonUpTrigger() const	{ return mButtonUp; 	}

		Uint8			GetHat( const Int32& index = 0 );

		Int16			GetAxis( const Int32& axis );

		eeVector2i		GetBallMotion( const Int32& ball );

		bool			IsButtonDown( const Int32& index );

		bool			IsButtonUp( const Int32& index );
	protected:
		Uint32 			mIndex;
		SDL_Joystick * 	mJoystick;
		std::string		mName;
		Int32			mHats;
		Int32			mButtons;
		Int32			mAxes;
		Int32			mBalls;
		Uint32			mButtonDown;
		Uint32			mButtonDownLast;
		Uint32			mButtonUp;
};

}}

#endif
