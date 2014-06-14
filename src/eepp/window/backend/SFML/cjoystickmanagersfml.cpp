#include <eepp/window/backend/SFML/cjoystickmanagersfml.hpp>
#include <eepp/window/backend/SFML/cjoysticksfml.hpp>

#ifdef EE_BACKEND_SFML_ACTIVE

#ifdef None
#undef None
#endif
#include <SFML/Window.hpp>

namespace EE { namespace Window { namespace Backend { namespace SFML {

cJoystickManagerSFML::cJoystickManagerSFML() :
	cJoystickManager()
{
}

cJoystickManagerSFML::~cJoystickManagerSFML() {
}

void cJoystickManagerSFML::Update() {
}

void cJoystickManagerSFML::Open() {
	mCount = sf::Joystick::Count;

	for ( unsigned int i = 0; i < mCount; i++ )
		Create(i);

	mInit = true;
}

void cJoystickManagerSFML::Close() {
	mInit = false;
}

void cJoystickManagerSFML::Create( const Uint32& index ) {
	if ( NULL != mJoysticks[ index ] )
		mJoysticks[ index ]->ReOpen();
	else
		mJoysticks[ index ] = eeNew( cJoystickSFML, ( index ) );
}

}}}}

#endif
