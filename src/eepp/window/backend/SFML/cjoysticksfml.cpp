#include <eepp/window/backend/SFML/cjoysticksfml.hpp>

#ifdef EE_BACKEND_SFML_ACTIVE

#ifdef None
#undef None
#endif
#include <SFML/Window.hpp>

namespace EE { namespace Window { namespace Backend { namespace SFML {

JoystickSFML::JoystickSFML( const Uint32& index ) :
	Joystick( index ),
	mHat( HAT_CENTERED )
{
	Open();
}

JoystickSFML::~JoystickSFML() {
}

void JoystickSFML::Open() {
	mName		= "Joystick " + String::ToStr( mIndex );
	mHats		= 0;
	mButtons	= eemin( sf::Joystick::getButtonCount( mIndex ), (unsigned int)32 );
	mAxes		= sf::Joystick::AxisCount;
	mBalls		= 0;
	mHat		= HAT_CENTERED;
	mButtonDown	= mButtonDownLast = mButtonUp = 0;
}

void JoystickSFML::Close() {
}

void JoystickSFML::Update() {
	ClearStates();

	for ( Int32 i = 0; i < mButtons; i++ ) {
		UpdateButton( i, sf::Joystick::isButtonPressed( mIndex, i ) );
	}

	CalcHat();
}

void JoystickSFML::CalcHat() {
	Float hatX = sf::Joystick::getAxisPosition( mIndex, sf::Joystick::PovX );
	Float hatY = sf::Joystick::getAxisPosition( mIndex, sf::Joystick::PovY );

	mHat = HAT_CENTERED;

	if ( hatX < 0 ) mHat |= HAT_LEFT;
	else if ( hatX > 0 ) mHat |= HAT_RIGHT;

	if ( hatY < 0 ) mHat |= HAT_UP;
	else if ( hatY > 0 ) mHat |= HAT_DOWN;
}

Uint8 JoystickSFML::GetHat( const Int32& index ) {
	return mHat;
}

Float JoystickSFML::GetAxis( const Int32& axis ) {
	sf::Joystick::Axis raxis = sf::Joystick::X;

	switch ( axis )
	{
		case AXIS_X:	raxis = sf::Joystick::X; break;
		case AXIS_Y:	raxis = sf::Joystick::Y; break;
		case AXIS_X2:	raxis = sf::Joystick::Z; break;
		case AXIS_Y2:	raxis = sf::Joystick::R; break;
	}

	return sf::Joystick::getAxisPosition( mIndex, raxis ) * 0.01;
}

Vector2i JoystickSFML::GetBallMotion( const Int32& ball ) {
	return Vector2i();
}

bool JoystickSFML::Plugged() const {
	return sf::Joystick::isConnected( mIndex );
}


}}}}

#endif
