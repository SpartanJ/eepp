#include "cjoystickmanagernull.hpp"
#include "cjoysticknull.hpp"

namespace EE { namespace Window { namespace Backend { namespace Null {

cJoystickManagerNull::cJoystickManagerNull() :
	cJoystickManager()
{
}

cJoystickManagerNull::~cJoystickManagerNull() {
}

void cJoystickManagerNull::Update() {
}

void cJoystickManagerNull::Open() {
}

void cJoystickManagerNull::Close() {
}

void cJoystickManagerNull::Create( const Uint32& index ) {
}

}}}}
