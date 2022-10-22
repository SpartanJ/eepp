#ifndef EE_WINDOWJOYCODES_HPP
#define EE_WINDOWJOYCODES_HPP

namespace EE { namespace Window {

/** @enum JoystickHatPosition Indicates the code for every hat position possible */
enum JoystickHatPosition {
	HAT_CENTERED = 0x00,
	HAT_UP = 0x01,
	HAT_RIGHT = 0x02,
	HAT_DOWN = 0x04,
	HAT_LEFT = 0x08,
	HAT_RIGHTUP = ( HAT_RIGHT | HAT_UP ),
	HAT_RIGHTDOWN = ( HAT_RIGHT | HAT_DOWN ),
	HAT_LEFTUP = ( HAT_LEFT | HAT_UP ),
	HAT_LEFTDOWN = ( HAT_LEFT | HAT_DOWN )
};

/** @enum JoystickAxis enumarates the joysticks axis */
enum JoystickAxis { AXIS_X = 0, AXIS_Y = 1, AXIS_X2 = 3, AXIS_Y2 = 2 };

#define AXIS_MAX ( 1 )
#define AXIS_MIN ( -1 )

#define MAX_JOYSTICKS ( 16 )

}} // namespace EE::Window

#endif
