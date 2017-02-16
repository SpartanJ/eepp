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

void JoystickManagerSFML::update() {
}

void JoystickManagerSFML::open() {
	mCount = sf::Joystick::Count;

	for ( unsigned int i = 0; i < mCount; i++ )
		create(i);

	mInit = true;
}

void JoystickManagerSFML::close() {
	mInit = false;
}

void JoystickManagerSFML::create( const Uint32& index ) {
	if ( NULL != mJoysticks[ index ] )
		mJoysticks[ index ]->reOpen();
	else
		mJoysticks[ index ] = eeNew( JoystickSFML, ( index ) );
}

}}}}

#endif
