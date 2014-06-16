#include <eepp/window/backend/null/joystickmanagernull.hpp>
#include <eepp/window/backend/null/joysticknull.hpp>

namespace EE { namespace Window { namespace Backend { namespace Null {

JoystickManagerNull::JoystickManagerNull() :
	JoystickManager()
{
}

JoystickManagerNull::~JoystickManagerNull() {
}

void JoystickManagerNull::Update() {
}

void JoystickManagerNull::Open() {
}

void JoystickManagerNull::Close() {
}

void JoystickManagerNull::Create( const Uint32& index ) {
}

}}}}
