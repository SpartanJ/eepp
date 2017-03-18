#include <eepp/window/inputfinger.hpp>

namespace EE { namespace Window {

InputFinger::InputFinger() :
	id(-1),
	x(0),
	y(0),
	pressure(0),
	xdelta(0),
	ydelta(0),
	last_x(0),
	last_y(0),
	lastPressure(0),
	down(false),
	wasDown(false)
{	
}

bool InputFinger::isDown() {
	return down;
}

bool InputFinger::getWasDown() {
	return wasDown;
}

Vector2i InputFinger::getPos() {
	return Vector2i( x, y );
}

void InputFinger::writeLast() {
	last_x = x;
	last_y = y;
	lastPressure = pressure;
}

}} 
