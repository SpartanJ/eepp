#include <eepp/window/backend/null/cjoysticknull.hpp>

namespace EE { namespace Window { namespace Backend { namespace Null {

cJoystickNull::cJoystickNull( const Uint32& index ) :
	cJoystick( index )
{	
}

cJoystickNull::~cJoystickNull() {
}

void cJoystickNull::Open() {
}

void cJoystickNull::Close() {
}

void cJoystickNull::Update() {
}

Uint8 cJoystickNull::GetHat( const Int32& index ) {
	return 0;
}

Float cJoystickNull::GetAxis( const Int32& axis ) {
	return 0;
}

Vector2i cJoystickNull::GetBallMotion( const Int32& ball ) {
	return Vector2i();
}

bool cJoystickNull::Plugged() const {
	return false;
}


}}}}
