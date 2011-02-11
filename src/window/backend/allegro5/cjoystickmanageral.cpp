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
	if ( mInit ) {
		for ( eeUint i = 0; i < mCount; i++ )
			if ( NULL != mJoysticks[i] )
				mJoysticks[i]->Update();
	}
}

void cJoystickManagerAl::Open() {
	al_install_joystick();

	mCount = (eeUint)al_get_num_joysticks();

	for ( eeUint i = 0; i < mCount; i++ )
		Create(i);

	mInit = true;
}

void cJoystickManagerAl::Close() {
	al_uninstall_joystick();
}

void cJoystickManagerAl::Create( const Uint32& index ) {
	if ( NULL != mJoysticks[ index ] )
		mJoysticks[ index ]->ReOpen();
	else
		mJoysticks[ index ] = eeNew( cJoystickAl, ( index ) );
}

}}}}

#endif
