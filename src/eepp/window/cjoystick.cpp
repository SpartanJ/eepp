#include <eepp/window/cjoystick.hpp>
#include <eepp/window/cjoystickmanager.hpp>

namespace EE { namespace Window {

Joystick::Joystick( const Uint32& index ) :
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

Joystick::~Joystick() {
	Close();
}

void Joystick::ReOpen() {
	Close();

	Open();
}

void Joystick::ClearStates() {
	mButtonUp		= 0;
	mButtonDownLast = mButtonDown;
}

void Joystick::UpdateButton( const Uint32& index, const bool& down ) {
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

const Int32& Joystick::GetNumHats() const {
	return mHats;
}

const Int32& Joystick::GetNumButtons() const {
	return mButtons;
}

const Int32& Joystick::GetNumAxes() const {
	return mAxes;
}

const Int32& Joystick::GetNumBalls() const {
	return mBalls;
}

const Uint32& Joystick::GetButtonTrigger() const {
	return mButtonDown;
}

const Uint32& Joystick::GetButtonUpTrigger() const {
	return mButtonUp;
}

bool Joystick::IsButtonDown( const Int32& index ) {
	if ( index >= 0 && index < mButtons )
		return 0 != ( mButtonDown & ( 1 << index ) );

	return false;
}

bool Joystick::IsButtonUp( const Int32& index ) {
	if ( index >= 0 && index < mButtons )
		return 0 != ( mButtonUp & ( 1 << index ) );

	return false;
}

void Joystick::Close() {
}

void Joystick::Open() {
}

}}
