#include <eepp/window/backend/null/joysticknull.hpp>

namespace EE { namespace Window { namespace Backend { namespace Null {

JoystickNull::JoystickNull( const Uint32& index ) :
	Joystick( index )
{	
}

JoystickNull::~JoystickNull() {
}

void JoystickNull::open() {
}

void JoystickNull::close() {
}

void JoystickNull::update() {
}

Uint8 JoystickNull::getHat( const Int32& index ) {
	return 0;
}

Float JoystickNull::getAxis( const Int32& axis ) {
	return 0;
}

Vector2i JoystickNull::getBallMotion( const Int32& ball ) {
	return Vector2i();
}

bool JoystickNull::isPlugged() const {
	return false;
}


}}}}
