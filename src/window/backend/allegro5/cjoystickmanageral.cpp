#include "cjoystickmanageral.hpp"
#include "cjoystickal.hpp"

#ifdef EE_BACKEND_ALLEGRO_ACTIVE

namespace EE { namespace Window { namespace Backend { namespace Al {

cJoystickManagerAl::cJoystickManagerAl() :
	cJoystickManager()
{
}

cJoystickManagerAl::~cJoystickManagerAl() {
}

void cJoystickManagerAl::Update() {
}

void cJoystickManagerAl::Open() {
	al_install_joystick();
}

void cJoystickManagerAl::Close() {
	al_uninstall_joystick();
}

void cJoystickManagerAl::Create( const Uint32& index ) {
}

}}}}

#endif
