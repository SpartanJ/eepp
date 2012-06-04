#include "cinputfinger.hpp"

namespace EE { namespace Window {

cInputFinger::cInputFinger() :
	id(-1),
	x(0),
	y(0),
	pressure(0),
	xdelta(0),
	ydelta(0),
	last_x(0),
	last_y(0),
	last_pressure(0),
	down(false),
	was_down(false)
{	
}

bool cInputFinger::IsDown() {
	return down;
}

bool cInputFinger::WasDown() {
	return was_down;
}

eeVector2i cInputFinger::Pos() {
	return eeVector2i( x, y );
}

void cInputFinger::WriteLast() {
	last_x = x;
	last_y = y;
	last_pressure = pressure;
}

}} 
