#include "cjoystickal.hpp"

#ifdef EE_BACKEND_ALLEGRO_ACTIVE

namespace EE { namespace Window { namespace Backend { namespace Al {

cJoystickAl::cJoystickAl( const Uint32& index ) :
	cJoystick( index )
{	
}

cJoystickAl::~cJoystickAl() {
}

void cJoystickAl::Open() {
}

void cJoystickAl::Close() {
}

void cJoystickAl::Update() {
}

Uint8 cJoystickAl::GetHat( const Int32& index ) {
	return 0;
}

Int16 cJoystickAl::GetAxis( const Int32& axis ) {
	return 0;
}

eeVector2i cJoystickAl::GetBallMotion( const Int32& ball ) {
	return eeVector2i();
}

bool cJoystickAl::Plugged() const {
	return false;
}


}}}}

#endif
