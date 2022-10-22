#include <eepp/window/joystick.hpp>
#include <eepp/window/joystickmanager.hpp>

namespace EE { namespace Window {

Joystick::Joystick( const Uint32& index ) :
	mIndex( index ),
	mHats( 0 ),
	mButtons( 0 ),
	mAxes( 0 ),
	mBalls( 0 ),
	mButtonDown( 0 ),
	mButtonDownLast( 0 ),
	mButtonUp( 0 ) {
	open();
}

Joystick::~Joystick() {
	close();
}

void Joystick::reOpen() {
	close();

	open();
}

void Joystick::clearStates() {
	mButtonUp = 0;
	mButtonDownLast = mButtonDown;
}

void Joystick::updateButton( const Uint32& index, const bool& down ) {
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

const Int32& Joystick::getNumHats() const {
	return mHats;
}

const Int32& Joystick::getNumButtons() const {
	return mButtons;
}

const Int32& Joystick::getNumAxes() const {
	return mAxes;
}

const Int32& Joystick::getNumBalls() const {
	return mBalls;
}

const Uint32& Joystick::getButtonTrigger() const {
	return mButtonDown;
}

const Uint32& Joystick::getButtonUpTrigger() const {
	return mButtonUp;
}

bool Joystick::isButtonDown( const Int32& index ) {
	if ( index >= 0 && index < mButtons )
		return 0 != ( mButtonDown & ( 1 << index ) );

	return false;
}

bool Joystick::isButtonUp( const Int32& index ) {
	if ( index >= 0 && index < mButtons )
		return 0 != ( mButtonUp & ( 1 << index ) );

	return false;
}

void Joystick::close() {}

void Joystick::open() {}

}} // namespace EE::Window
