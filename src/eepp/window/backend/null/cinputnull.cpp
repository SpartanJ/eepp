#include <eepp/window/backend/null/cinputnull.hpp>
#include <eepp/window/backend/null/cjoystickmanagernull.hpp>

namespace EE { namespace Window { namespace Backend { namespace Null {

InputNull::InputNull( EE::Window::Window * window ) :
	Input( window, eeNew( JoystickManagerNull, () ) )
{
}

InputNull::~InputNull() {
}

void InputNull::Update() {
}

bool InputNull::GrabInput() {
	return false;
}

void InputNull::GrabInput( const bool& Grab ) {
}

void InputNull::InjectMousePos( const Uint16& x, const Uint16& y ) {
}

void InputNull::Init() {
}

}}}}
