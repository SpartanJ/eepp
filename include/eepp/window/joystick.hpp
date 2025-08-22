#ifndef EE_WINDOWCJOYSTICK_HPP
#define EE_WINDOWCJOYSTICK_HPP

#include <eepp/window/base.hpp>
#include <eepp/window/joycodes.hpp>

namespace EE { namespace Window {

/** @brief Represents a physical Joystick, and contains all its states */
class EE_API Joystick {
  public:
	Joystick( const Uint32& index );

	virtual ~Joystick();

	/** Close the joystick */
	virtual void close();

	/** Open the joystick */
	virtual void open();

	/** Update the current joystick states ( users don't need to call this manually ) */
	virtual void update() = 0;

	/** @return The hat index state
	 * @see EE_HAT_POS to get info about the possible states
	 * You just need to compare the state against what position you need, example if (
	 * myJoy->GetHat() == HAT_LEFT ) { ... do whatever you need to do }
	 */
	virtual Uint8 getHat( const Int32& index = 0 ) = 0;

	/** @return the axis state
	 * @see EE_JOYAXIS to know the possible axis ( usually you will use AXIS_X and AXIS_Y
	 * Axis values goes from AXIS_MIN to AXIS_MAX
	 */
	virtual Float getAxis( const Int32& axis ) = 0;

	/** @return The ball motion position */
	virtual Vector2i getBallMotion( const Int32& ball ) = 0;

	/** @return True if the joystick is plugged in */
	virtual bool isPlugged() const = 0;

	/** ReOpen the joystick ( this is the same of doing Close and the Open ) */
	virtual void reOpen();

	/** @return The number of hats */
	const Int32& getNumHats() const;

	/** @return The number of buttons */
	const Int32& getNumButtons() const;

	/** @return The number of axes */
	const Int32& getNumAxes() const;

	/** @return The number of balls */
	const Int32& getNumBalls() const;

	/** @return The buttons states flags */
	const Uint32& getButtonTrigger() const;

	/** @return The buttons released states flags */
	const Uint32& getButtonUpTrigger() const;

	/** @return If the button index is down ( pressed ) */
	bool isButtonDown( const Int32& index );

	/** @return If the button index is up ( released ) */
	bool isButtonUp( const Int32& index );

  protected:
	friend class JoystickManager;
	Uint32 mIndex;
	std::string mName;
	Int32 mHats;
	Int32 mButtons;
	Int32 mAxes;
	Int32 mBalls;
	Uint32 mButtonDown;
	Uint32 mButtonDownLast;
	Uint32 mButtonUp;

	void updateButton( const Uint32& index, const bool& down );

	virtual void clearStates();
};

}} // namespace EE::Window

#endif
