#include "cjoystick.hpp"
#include "cjoystickmanager.hpp"

namespace EE { namespace Window {

cJoystick::cJoystick( const Uint32& index ) :
	mIndex( index ),
	mJoystick( NULL ),
	mHats(0),
	mButtons(0),
	mAxes(0),
	mBalls(0),
	mButtonDown(0),
	mButtonDownLast(0),
	mButtonUp(0)
{
	Open();
}

cJoystick::~cJoystick() {
	Close();
}

void cJoystick::Open() {
	mJoystick 	= SDL_JoystickOpen( mIndex );

	if ( NULL == mJoystick ) {

	} else {
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

void cJoystick::Close() {
	if( SDL_JoystickOpened( mIndex ) )
		SDL_JoystickClose( mJoystick );

	mJoystick 	= NULL;
	mName		= "";
	mHats = mButtons = mAxes = mBalls = 0;
}

void cJoystick::ReOpen() {
	Close();

	Open();
}

bool cJoystick::Plugged() const {
	return NULL != mJoystick;
}

void cJoystick::Update() {
	if ( NULL != mJoystick ) {
		Int32 i;
		Uint8 v;

		mButtonUp		= 0;
		mButtonDownLast = mButtonDown;

		for ( i = 0; i < mButtons; i++ ) {
			v = SDL_JoystickGetButton( mJoystick, i );

			if ( v ) {
				mButtonDown |= ( 1 << i );
			} else {
				if ( ( mButtonDown ) & ( 1 << i ) )
					mButtonDown &= ~( 1 << i );
			}

			if ( !( ( mButtonDown ) & ( 1 << i ) ) && ( ( mButtonDownLast ) & ( 1 << i ) ) )
				mButtonUp |= ( 1 << i );
		}

	}
}

Uint8 cJoystick::GetHat( const Int32& index ) {
	if ( index >= 0 && index < mHats )
		return SDL_JoystickGetHat( mJoystick, index );

	return HAT_CENTERED;
}

bool cJoystick::IsButtonDown( const Int32& index ) {
	if ( index >= 0 && index < mButtons )
		return 0 != ( mButtonDown & ( 1 << index ) );

	return false;
}

bool cJoystick::IsButtonUp( const Int32& index ) {
	if ( index >= 0 && index < mButtons )
		return 0 != ( mButtonUp & ( 1 << index ) );

	return false;
}

Int16 cJoystick::GetAxis( const Int32& axis ) {
	if ( axis >= 0 && axis < mAxes )
		return SDL_JoystickGetAxis( mJoystick, axis );

	return 0;
}

eeVector2i cJoystick::GetBallMotion( const Int32& ball ) {
	eeVector2i v;

	if ( ball >= 0 && ball < mBalls )
		SDL_JoystickGetBall( mJoystick, ball, &v.x, &v.y );

	return v;
}

}}
