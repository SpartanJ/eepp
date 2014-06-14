#include <eepp/window/backend/SDL/cjoysticksdl.hpp>

#ifdef EE_BACKEND_SDL_1_2

namespace EE { namespace Window { namespace Backend { namespace SDL {

JoystickSDL::JoystickSDL( const Uint32& index ) :
	Joystick( index ),
	mJoystick( NULL )
{
	Open();
}

JoystickSDL::~JoystickSDL() {
	Close();
}

void JoystickSDL::Open() {
	mJoystick 	= SDL_JoystickOpen( mIndex );

	if ( NULL != mJoystick ) {
		mName 		= SDL_JoystickName( mIndex );
		mHats		= SDL_JoystickNumHats( mJoystick );
		mButtons	= SDL_JoystickNumButtons( mJoystick );
		mAxes		= SDL_JoystickNumAxes( mJoystick );
		mBalls		= SDL_JoystickNumBalls( mJoystick );

		mButtonDown	= mButtonDownLast = mButtonUp = 0;

		if ( mButtons > 32 )
			mButtons = 32;
	}
}

void JoystickSDL::Close() {
	if( SDL_JoystickOpened( mIndex ) )
		SDL_JoystickClose( mJoystick );

	mJoystick 	= NULL;
	mName		= "";
	mHats = mButtons = mAxes = mBalls = 0;
}

void JoystickSDL::Update() {
	if ( NULL != mJoystick ) {
		ClearStates();

		for ( Int32 i = 0; i < mButtons; i++ ) {
			UpdateButton( i, 0 != SDL_JoystickGetButton( mJoystick, i ) );
		}

	}
}

Uint8 JoystickSDL::GetHat( const Int32& index ) {
	if ( index >= 0 && index < mHats )
		return SDL_JoystickGetHat( mJoystick, index );

	return HAT_CENTERED;
}

Float JoystickSDL::GetAxis( const Int32& axis ) {
	if ( axis >= 0 && axis < mAxes ) {
		return (Float)SDL_JoystickGetAxis( mJoystick, axis ) / 32768.f;
	}

	return 0;
}

Vector2i JoystickSDL::GetBallMotion( const Int32& ball ) {
	Vector2i v;

	if ( ball >= 0 && ball < mBalls )
		SDL_JoystickGetBall( mJoystick, ball, &v.x, &v.y );

	return v;
}


bool JoystickSDL::Plugged() const {
	return NULL != mJoystick;
}


}}}}

#endif
