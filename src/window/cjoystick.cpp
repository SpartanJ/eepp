#include "cjoystick.hpp"
#include "cjoystickmanager.hpp"

namespace EE { namespace Window {

cJoystick::cJoystick( const Uint32& index ) :
	mIndex( index ),
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

void cJoystick::ReOpen() {
	Close();

	Open();
}

void cJoystick::ClearStates() {
	mButtonUp		= 0;
	mButtonDownLast = mButtonDown;
}

void cJoystick::UpdateButton( const Uint32& index, const bool& down ) {
	if ( down ) {
		mButtonDown |= ( 1 << index );
	} else {
		if ( ( mButtonDown ) & ( 1 << index ) )
			mButtonDown &= ~( 1 << index );
	}

	if ( !( ( mButtonDown ) & ( 1 << index ) ) && ( ( mButtonDownLast ) & ( 1 << index ) ) ) {
		mButtonUp |= ( 1 << index );
	}
}

const Int32& cJoystick::GetNumHats() const {
	return mHats;
}

const Int32& cJoystick::GetNumButtons() const {
	return mButtons;
}

const Int32& cJoystick::GetNumAxes() const {
	return mAxes;
}

const Int32& cJoystick::GetNumBalls() const {
	return mBalls;
}

const Uint32& cJoystick::GetButtonTrigger() const {
	return mButtonDown;
}

const Uint32& cJoystick::GetButtonUpTrigger() const {
	return mButtonUp;
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

void cJoystick::Close() {
}

void cJoystick::Open() {
}

}}
