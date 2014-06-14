#ifndef EE_WINDOWCJOYSTICK_HPP
#define EE_WINDOWCJOYSTICK_HPP

#include <eepp/window/base.hpp>
#include <eepp/window/joycodes.hpp>

namespace EE { namespace Window {

/** @brief Represents a physical Joystick, and contains all its states */
class EE_API cJoystick {
	public:
		cJoystick( const Uint32& index );

		virtual ~cJoystick();

		/** Close the joystick */
		virtual void 		Close();

		/** Open the joystick */
		virtual void 		Open();

		/** Update the current joystick states ( users don't need to call this manually ) */
		virtual void		Update() = 0;

		/** @return The hat index state
		* @see EE_HAT_POS to get info about the possible states
		* You just need to compare the state against what position you need, example if ( myJoy->GetHat() == HAT_LEFT ) { ... do whatever you need to do }
		*/
		virtual Uint8		GetHat( const Int32& index = 0 ) = 0;

		/** @return the axis state
		* @see EE_JOYAXIS to know the possible axis ( usually you will use AXIS_X and AXIS_Y
		* Axis values goes from AXIS_MIN to AXIS_MAX
		*/
		virtual Float		GetAxis( const Int32& axis ) = 0;

		/** @return The ball motion position */
		virtual Vector2i	GetBallMotion( const Int32& ball ) = 0;

		/** @return True if the joystick is plugged in */
		virtual bool		Plugged() const = 0;
		
		/** ReOpen the joysick ( this is the same of doing Close and the Open ) */
		virtual void		ReOpen();

		/** @return The number of hats */
		const Int32&		GetNumHats() const;

		/** @return The number of buttons */
		const Int32&		GetNumButtons() const;

		/** @return The number of axes */
		const Int32&		GetNumAxes() const;

		/** @return The number of balls */
		const Int32&		GetNumBalls() const;

		/** @return The buttons states flags */
		const Uint32&		GetButtonTrigger() const;

		/** @return The buttons released states flags */
		const Uint32&		GetButtonUpTrigger() const;

		/** @return If the button index is down ( pressed ) */
		bool				IsButtonDown( const Int32& index );

		/** @return If the button index is up ( released ) */
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
 
