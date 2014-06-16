#include <eepp/window/backend/null/joysticknull.hpp>

namespace EE { namespace Window { namespace Backend { namespace Null {

JoystickNull::JoystickNull( const Uint32& index ) :
	Joystick( index )
{	
}

JoystickNull::~JoystickNull() {
}

void JoystickNull::Open() {
}

void JoystickNull::Close() {
}

void JoystickNull::Update() {
}

Uint8 JoystickNull::GetHat( const Int32& index ) {
	return 0;
}

Float JoystickNull::GetAxis( const Int32& axis ) {
	return 0;
}

Vector2i JoystickNull::GetBallMotion( const Int32& ball ) {
	return Vector2i();
}

bool JoystickNull::Plugged() const {
	return false;
}


}}}}
