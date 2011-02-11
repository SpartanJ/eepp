#ifndef EE_WINDOWCJOYSTICK_HPP
#define EE_WINDOWCJOYSTICK_HPP

#include "base.hpp"
#include "joycodes.hpp"

namespace EE { namespace Window {

class EE_API cJoystick {
	public:
		cJoystick( const Uint32& index );

		virtual ~cJoystick();

		virtual void 		Close();

		virtual void 		Open();

		virtual void		Update() = 0;

		virtual Uint8		GetHat( const Int32& index = 0 ) = 0;

		virtual eeFloat		GetAxis( const Int32& axis ) = 0;

		virtual eeVector2i	GetBallMotion( const Int32& ball ) = 0;

		virtual bool		Plugged() const = 0;
		
		virtual void		ReOpen();

		const Int32&		GetNumHats() const;

		const Int32&		GetNumButtons() const;

		const Int32&		GetNumAxes() const;

		const Int32&		GetNumBalls() const;

		const Uint32&		GetButtonTrigger() const;

		const Uint32&		GetButtonUpTrigger() const;

		bool				IsButtonDown( const Int32& index );

		bool				IsButtonUp( const Int32& index );
	protected:
		friend class cJoystickManager;
		Uint32 			mIndex;
		std::string		mName;
		Int32			mHats;
		Int32			mButtons;
		Int32			mAxes;
		Int32			mBalls;
		Uint32			mButtonDown;
		Uint32			mButtonDownLast;
		Uint32			mButtonUp;
		
		void UpdateButton( const Uint32& index, const bool& down );
		
		virtual void ClearStates();
};

}}

#endif
 
