#include <eepp/window/backend/null/joystickmanagernull.hpp>
#include <eepp/window/backend/null/joysticknull.hpp>

namespace EE { namespace Window { namespace Backend { namespace Null {

JoystickManagerNull::JoystickManagerNull() :
	JoystickManager()
{
}

JoystickManagerNull::~JoystickManagerNull() {
}

void JoystickManagerNull::update() {
}

void JoystickManagerNull::open() {
}

void JoystickManagerNull::close() {
}

void JoystickManagerNull::create( const Uint32& index ) {
}

}}}}
