#include <eepp/window/backend/null/inputnull.hpp>
#include <eepp/window/backend/null/joystickmanagernull.hpp>

namespace EE { namespace Window { namespace Backend { namespace Null {

InputNull::InputNull( EE::Window::Window * window ) :
	Input( window, eeNew( JoystickManagerNull, () ) )
{
}

InputNull::~InputNull() {
}

void InputNull::update() {
}

bool InputNull::grabInput() {
	return false;
}

void InputNull::grabInput( const bool& Grab ) {
}

void InputNull::injectMousePos( const Uint16& x, const Uint16& y ) {
}

void InputNull::init() {
}

}}}}
