#include "cjoystickal.hpp"

#ifdef EE_BACKEND_ALLEGRO_ACTIVE

namespace EE { namespace Window { namespace Backend { namespace Al {

cJoystickAl::cJoystickAl( const Uint32& index ) :
	cJoystick( index ),
	mJoystick( NULL )
{
	Open();
}

cJoystickAl::~cJoystickAl() {
	Close();
}

void cJoystickAl::Open() {
	mJoystick 	= al_get_joystick( mIndex );

	if ( NULL != mJoystick ) {
		mJoyState	= eeNew( ALLEGRO_JOYSTICK_STATE, () );
		mName 		= al_get_joystick_name( mJoystick );
		mHats		= 1;
		mButtons	= al_get_joystick_num_buttons( mJoystick );
		mAxes		= al_get_joystick_num_axes( mJoystick, 0 );
		mBalls		= 0;
		mSticks		= al_get_joystick_num_sticks( mJoystick );

		mButtonDown	= mButtonDownLast = mButtonUp = 0;

		if ( mButtons > 32 )
			mButtons = 32;
	}
}

void cJoystickAl::Close() {
	al_release_joystick( mJoystick );
	eeSAFE_DELETE( mJoyState );
	mJoystick 	= NULL;
	mName		= "";
	mSticks = mHats = mButtons = mAxes = mBalls = 0;
}

void cJoystickAl::Update() {
	if ( NULL != mJoystick ) {
		ClearStates();

		al_get_joystick_state( mJoystick, mJoyState );

		for ( Int32 i = 0; i < mButtons; i++ ) {
			UpdateButton( i, 0 != mJoyState->button[i] );
		}

	}
}

Uint8 cJoystickAl::GetHat( const Int32& index ) {
	eeASSERT( NULL != mJoyState );

	/// I need to simulate the hat, only tested with my own joystick, so this probably is not exactly a generic solution.
	if ( mSticks >= 4 && index >= 0 && index < mHats ) {
		Uint8 Flags = 0;
		eeFloat HatX = mJoyState->stick[2].axis[1];
		eeFloat HatY = mJoyState->stick[3].axis[0];

		if ( HatX != 0 ) {
			if ( HatX < 0 )
				Flags |= HAT_LEFT;
			else
				Flags |= HAT_RIGHT;
		}

		if ( HatY != 0 ) {
			if ( HatY < 0 )
				Flags |= HAT_UP;
			else
				Flags |= HAT_DOWN;
		}

		return Flags;
	}

	return HAT_CENTERED;
}

eeFloat cJoystickAl::GetAxis( const Int32& axis ) {
	eeASSERT( NULL != mJoyState );
	return mJoyState->stick[0].axis[axis];
}

eeVector2i cJoystickAl::GetBallMotion( const Int32& ball ) {
	return eeVector2i();
}

bool cJoystickAl::Plugged() const {
	return NULL != mJoystick;
}


}}}}

#endif
