#include "cinputnull.hpp"
#include "cjoystickmanagernull.hpp"

namespace EE { namespace Window { namespace Backend { namespace Null {

cInputNull::cInputNull( cWindow * window ) :
	cInput( window, eeNew( cJoystickManagerNull, () ) )
{
}

cInputNull::~cInputNull() {
}

void cInputNull::Update() {
}

bool cInputNull::GrabInput() {
	return false;
}

void cInputNull::GrabInput( const bool& Grab ) {
}

void cInputNull::InjectMousePos( const Uint16& x, const Uint16& y ) {
}

void cInputNull::Init() {
}

}}}}
