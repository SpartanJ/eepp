#include <eepp/window/backend/SFML/joystickmanagersfml.hpp>
#include <eepp/window/backend/SFML/joysticksfml.hpp>

#ifdef EE_BACKEND_SFML_ACTIVE

#ifdef None
#undef None
#endif
#include <SFML/Window.hpp>

namespace EE { namespace Window { namespace Backend { namespace SFML {

JoystickManagerSFML::JoystickManagerSFML() :
	JoystickManager()
{
}

JoystickManagerSFML::~JoystickManagerSFML() {
}

void JoystickManagerSFML::Update() {
}

void JoystickManagerSFML::Open() {
	mCount = sf::Joystick::Count;

	for ( unsigned int i = 0; i < mCount; i++ )
		Create(i);

	mInit = true;
}

void JoystickManagerSFML::Close() {
	mInit = false;
}

void JoystickManagerSFML::Create( const Uint32& index ) {
	if ( NULL != mJoysticks[ index ] )
		mJoysticks[ index ]->ReOpen();
	else
		mJoysticks[ index ] = eeNew( JoystickSFML, ( index ) );
}

}}}}

#endif
