#include <eepp/window/engine.hpp>
#include <eepp/window/input.hpp>

namespace EE { namespace Window {

Input::Input( EE::Window::Window* window, JoystickManager* joystickmanager ) :
	mWindow( window ),
	mJoystickManager( joystickmanager ),
	mPressTrigger( 0 ),
	mReleaseTrigger( 0 ),
	mLastPressTrigger( 0 ),
	mClickTrigger( 0 ),
	mDoubleClickTrigger( 0 ),
	mInputMod( 0 ),
	mDoubleClickInterval( Milliseconds( 400 ) ),
	mLastButtonLeftClicked( 0 ),
	mLastButtonRightClicked( 0 ),
	mLastButtonMiddleClicked( 0 ),
	mLastButtonLeftClick( 0 ),
	mLastButtonRightClick( 0 ),
	mLastButtonMiddleClick( 0 ),
	mTClick( 0 ),
	mNumCallBacks( 0 ),
	mMouseSpeed( 1.0f ),
	mInputGrabed( false ) {
	memset( mScancodeDown, 0, EE_KEYS_SPACE );
	memset( mScancodeUp, 0, EE_KEYS_SPACE );
}

Input::~Input() {
	eeSAFE_DELETE( mJoystickManager );
}

void Input::cleanStates() {
	memset( mScancodeUp, 0, EE_KEYS_SPACE );

	mReleaseTrigger = 0;
	mLastPressTrigger = mPressTrigger;
	mClickTrigger = 0;
	mDoubleClickTrigger = 0;

	resetFingerWasDown();
}

void Input::sendEvent( InputEvent* Event ) {
	for ( std::map<Uint32, InputCallback>::iterator i = mCallbacks.begin(); i != mCallbacks.end();
		  ++i ) {
		i->second( Event );
	}
}

void Input::processEvent( InputEvent* Event ) {
	mLastEvent.restart();

	switch ( Event->Type ) {
		case InputEvent::Window: {
			if ( Event->window.type == InputEvent::WindowClose )
				mWindow->onCloseRequest();
			break;
		}
		case InputEvent::KeyDown: {
			if ( Event->key.keysym.scancode > EE_KEYS_NUM )
				break;

			if ( Event->key.keysym.mod != eeINDEX_NOT_FOUND )
				mInputMod = Event->key.keysym.mod;

			BitOp::writeBitKey( &mScancodeDown[Event->key.keysym.scancode / 8],
								Event->key.keysym.scancode % 8, 1 );

			mLastKeyboardEvent.restart();
			break;
		}
		case InputEvent::KeyUp: {
			if ( Event->key.keysym.mod != eeINDEX_NOT_FOUND )
				mInputMod = Event->key.keysym.mod;

			if ( Event->key.keysym.scancode > EE_KEYS_NUM )
				break;

			BitOp::writeBitKey( &mScancodeDown[Event->key.keysym.scancode / 8],
								Event->key.keysym.scancode % 8, 0 );
			BitOp::writeBitKey( &mScancodeUp[Event->key.keysym.scancode / 8],
								Event->key.keysym.scancode % 8, 1 );

			mLastKeyboardEvent.restart();
			break;
		}
		case InputEvent::MouseMotion: {
			if ( !mInputGrabed ) {
				mMousePos.x = Event->motion.x;
				mMousePos.y = Event->motion.y;
			} else {
				mMousePos.x += static_cast<Int32>( (Float)Event->motion.xrel * mMouseSpeed );
				mMousePos.y += static_cast<Int32>( (Float)Event->motion.yrel * mMouseSpeed );
			}

			if ( mMousePos.x >= (int)mWindow->getWidth() ) {
				mMousePos.x = mWindow->getWidth();
			} else if ( mMousePos.x < 0 ) {
				mMousePos.x = 0;
			}

			if ( mMousePos.y >= (int)mWindow->getHeight() ) {
				mMousePos.y = mWindow->getHeight();
			} else if ( mMousePos.y < 0 ) {
				mMousePos.y = 0;
			}

			mLastMouseEvent.restart();
			break;
		}
		case InputEvent::MouseButtonDown: {
			mPressTrigger |= EE_BUTTON_MASK( Event->button.button );
			mLastMouseEvent.restart();
			break;
		}
		case InputEvent::MouseButtonUp: {
			mPressTrigger &= ~EE_BUTTON_MASK( Event->button.button );
			mReleaseTrigger |= EE_BUTTON_MASK( Event->button.button );
			mClickTrigger |= EE_BUTTON_MASK( Event->button.button );

			// I know this is ugly, but i'm too lazy to fix it, it works...
			if ( Event->button.button == EE_BUTTON_LEFT ) {
				mLastButtonLeftClicked = mLastButtonLeftClick;
				mLastButtonLeftClick = Sys::getTicks();

				mTClick = mLastButtonLeftClick - mLastButtonLeftClicked;

				if ( mTClick < mDoubleClickInterval.asMilliseconds() && mTClick > 0 ) {
					mDoubleClickTrigger |= EE_BUTTON_MASK( EE_BUTTON_LEFT );
					mLastButtonLeftClick = 0;
					mLastButtonLeftClicked = 0;
				}
			} else if ( Event->button.button == EE_BUTTON_RIGHT ) {
				mLastButtonRightClicked = mLastButtonRightClick;
				mLastButtonRightClick = Sys::getTicks();

				mTClick = mLastButtonRightClick - mLastButtonRightClicked;

				if ( mTClick < mDoubleClickInterval.asMilliseconds() && mTClick > 0 ) {
					mDoubleClickTrigger |= EE_BUTTON_MASK( EE_BUTTON_RIGHT );
					mLastButtonRightClick = 0;
					mLastButtonRightClicked = 0;
				}
			} else if ( Event->button.button == EE_BUTTON_MIDDLE ) {
				mLastButtonMiddleClicked = mLastButtonMiddleClick;
				mLastButtonMiddleClick = Sys::getTicks();

				mTClick = mLastButtonMiddleClick - mLastButtonMiddleClicked;

				if ( mTClick < mDoubleClickInterval.asMilliseconds() && mTClick > 0 ) {
					mDoubleClickTrigger |= EE_BUTTON_MASK( EE_BUTTON_MIDDLE );
					mLastButtonMiddleClick = 0;
					mLastButtonMiddleClicked = 0;
				}
			}

			mLastMouseEvent.restart();
			break;
		}
		case InputEvent::FingerDown: {
			InputFinger* Finger = getFingerId( Event->finger.fingerId );

			Finger->writeLast();
			Finger->x = (Uint16)( Event->finger.x * (Float)mWindow->getWidth() );
			Finger->y = (Uint16)( Event->finger.y * (Float)mWindow->getHeight() );
			Finger->pressure = Event->finger.pressure;
			Finger->down = true;
			Finger->xdelta = Event->finger.dx;
			Finger->ydelta = Event->finger.dy;

			if ( 0 == Event->finger.fingerId ) {
				mPressTrigger |= EE_BUTTON_LMASK;
			}

			mLastMouseEvent.restart();
			break;
		}
		case InputEvent::FingerUp: {
			InputFinger* Finger = getFingerId( Event->finger.fingerId );

			Finger->writeLast();
			Finger->x = (Uint16)( Event->finger.x * (Float)mWindow->getWidth() );
			Finger->y = (Uint16)( Event->finger.y * (Float)mWindow->getHeight() );
			Finger->pressure = Event->finger.pressure;
			Finger->down = false;
			Finger->wasDown = true;
			Finger->xdelta = Event->finger.dx;
			Finger->ydelta = Event->finger.dy;

			if ( 0 == Event->finger.fingerId ) {
				mPressTrigger &= ~EE_BUTTON_LMASK;
			}

			mLastMouseEvent.restart();
			break;
		}
		case InputEvent::FingerMotion: {
			InputFinger* Finger = getFingerId( Event->finger.fingerId );

			Finger->writeLast();
			Finger->x = (Uint16)( Event->finger.x * (Float)mWindow->getWidth() );
			Finger->y = (Uint16)( Event->finger.y * (Float)mWindow->getHeight() );
			Finger->pressure = Event->finger.pressure;
			Finger->down = true;
			Finger->xdelta = Event->finger.dx;
			Finger->ydelta = Event->finger.dy;

			if ( 0 == Event->finger.fingerId ) {
				mPressTrigger |= EE_BUTTON_LMASK;
			}

			mLastMouseEvent.restart();
			break;
		}
		case InputEvent::VideoResize: {
			mWindow->onWindowResize( Event->resize.w, Event->resize.h );
			break;
		}
		case InputEvent::Quit: {
			mWindow->onQuit();
			break;
		}
	}

	sendEvent( Event );
}

InputFinger* Input::getFingerId( const Int64& fingerId ) {
	Uint32 i;

	for ( i = 0; i < EE_MAX_FINGERS; i++ ) {
		if ( mFingers[i].id == fingerId ) {
			return &mFingers[i];
		}
	}

	for ( i = 0; i < EE_MAX_FINGERS; i++ ) {
		if ( -1 == mFingers[i].id ) {
			mFingers[i].id = fingerId;

			return &mFingers[i];
		}
	}

	//! Find first unused
	for ( i = 0; i < EE_MAX_FINGERS; i++ ) {
		if ( !mFingers[i].down ) {
			mFingers[i].id = fingerId;
			return &mFingers[i];
		}
	}

	return NULL;
}

void Input::resetFingerWasDown() {
	for ( Uint32 i = 0; i < EE_MAX_FINGERS; i++ ) {
		mFingers[i].wasDown = false;
	}
}

bool Input::isKeyDown( const Keycode& Key ) {
	return isScancodeDown( getScancodeFromKey( Key ) );
}

bool Input::isKeyUp( const Keycode& Key ) {
	return isScancodeUp( getScancodeFromKey( Key ) );
}

bool Input::isScancodeUp( const Scancode& scancode ) {
	return 0 != BitOp::readBitKey( &mScancodeUp[scancode / 8], scancode % 8 );
}

bool Input::isScancodeDown( const Scancode& scancode ) {
	return 0 != BitOp::readBitKey( &mScancodeDown[scancode / 8], scancode % 8 );
}

void Input::injectScancodeDown( const Scancode& scancode ) {
	BitOp::writeBitKey( &mScancodeDown[scancode / 8], scancode % 8, 1 );
}

void Input::injectScancodeUp( const Scancode& scancode ) {
	BitOp::writeBitKey( &mScancodeUp[scancode / 8], scancode % 8, 1 );
}

void Input::injectButtonPress( const Uint32& Button ) {
	if ( Button < 8 )
		if ( !( mPressTrigger & EE_BUTTON_MASK( Button ) ) )
			mPressTrigger |= EE_BUTTON_MASK( Button );
}

void Input::injectButtonRelease( const Uint32& Button ) {
	if ( Button < 8 ) {
		if ( mPressTrigger & EE_BUTTON_MASK( Button ) )
			mPressTrigger &= ~EE_BUTTON_MASK( Button );

		if ( !( mReleaseTrigger & EE_BUTTON_MASK( Button ) ) )
			mReleaseTrigger |= EE_BUTTON_MASK( Button );

		if ( !( mClickTrigger & EE_BUTTON_MASK( Button ) ) )
			mClickTrigger |= EE_BUTTON_MASK( Button );
	}
}

Vector2i Input::getMousePos() const {
	return mMousePos;
}

void Input::setMousePos( const Vector2i& Pos ) {
	mMousePos = Pos;
}

Vector2f Input::getMousePosFromView( const View& View ) {
	return mWindow->mapPixelToCoords( getMousePos(), View );
}

Uint32 Input::pushCallback( const InputCallback& cb ) {
	mNumCallBacks++;
	mCallbacks[mNumCallBacks] = cb;
	return mNumCallBacks;
}

void Input::popCallback( const Uint32& CallbackId ) {
	mCallbacks[CallbackId] = 0;
	mCallbacks.erase( mCallbacks.find( CallbackId ) );
}

void Input::injectMousePos( const Vector2i& Pos ) {
	injectMousePos( Pos.x, Pos.y );
}

bool Input::isLeftControlPressed() const {
	return ( mInputMod & KEYMOD_LCTRL ) != 0;
}

bool Input::isRightControlPressed() const {
	return ( mInputMod & KEYMOD_RCTRL ) != 0;
}

bool Input::isControlPressed() const {
	return ( mInputMod & KEYMOD_CTRL ) != 0;
}

bool Input::isKeyModPressed() const {
	return ( mInputMod & KeyMod::getDefaultModifier() ) != 0;
}

bool Input::isLeftShiftPressed() const {
	return ( mInputMod & KEYMOD_LSHIFT ) != 0;
}

bool Input::isRightShiftPressed() const {
	return ( mInputMod & KEYMOD_RSHIFT ) != 0;
}

bool Input::isShiftPressed() const {
	return ( mInputMod & KEYMOD_SHIFT ) != 0;
}

bool Input::isAltPressed() const {
	return ( mInputMod & KEYMOD_ALT ) != 0;
}

bool Input::isLeftAltPressed() const {
	return ( mInputMod & KEYMOD_LALT ) != 0;
}

bool Input::isAltGrPressed() const {
	return ( mInputMod & KEYMOD_RALT ) != 0;
}

bool Input::isMetaPressed() const {
	return ( mInputMod & KEYMOD_META ) != 0;
}

bool Input::isMouseLeftPressed() const {
	return ( mPressTrigger & EE_BUTTON_LMASK ) != 0;
}

bool Input::isMouseRightPressed() const {
	return ( mPressTrigger & EE_BUTTON_RMASK ) != 0;
}

bool Input::isMouseMiddlePressed() const {
	return ( mPressTrigger & EE_BUTTON_MMASK ) != 0;
}

bool Input::mouseLeftClicked() const {
	return ( mClickTrigger & EE_BUTTON_LMASK ) != 0;
}

bool Input::mouseRightClicked() const {
	return ( mClickTrigger & EE_BUTTON_RMASK ) != 0;
}

bool Input::mouseMiddleClicked() const {
	return ( mClickTrigger & EE_BUTTON_MMASK ) != 0;
}

bool Input::mouseLeftDoubleClicked() const {
	return ( mDoubleClickTrigger & EE_BUTTON_LMASK ) != 0;
}

bool Input::mouseRightDoubleClicked() const {
	return ( mDoubleClickTrigger & EE_BUTTON_RMASK ) != 0;
}

bool Input::mouseMiddleDoubleClicked() const {
	return ( mDoubleClickTrigger & EE_BUTTON_MMASK ) != 0;
}

bool Input::mouseWheelScrolledUp() const {
	return ( mReleaseTrigger & EE_BUTTON_WUMASK ) != 0;
}

bool Input::mouseWheelScrolledDown() const {
	return ( mReleaseTrigger & EE_BUTTON_WDMASK ) != 0;
}

void Input::setMouseSpeed( const Float& Speed ) {
	mMouseSpeed = Speed;
}

const Float& Input::getMouseSpeed() const {
	return mMouseSpeed;
}

const Uint32& Input::getLastPressTrigger() const {
	return mLastPressTrigger;
}

const Uint32& Input::getPressTrigger() const {
	return mPressTrigger;
}

const Uint32& Input::getReleaseTrigger() const {
	return mReleaseTrigger;
}

const Uint32& Input::getClickTrigger() const {
	return mClickTrigger;
}

const Uint32& Input::getDoubleClickTrigger() const {
	return mDoubleClickTrigger;
}

const Time& Input::getDoubleClickInterval() const {
	return mDoubleClickInterval;
}

void Input::setDoubleClickInterval( const Time& Interval ) {
	mDoubleClickInterval = Interval;
}

JoystickManager* Input::getJoystickManager() const {
	return mJoystickManager;
}

Uint32 Input::getFingerCount() {
	return EE_MAX_FINGERS;
}

InputFinger* Input::getFingerIndex( const Uint32& Index ) {
	eeASSERT( Index < EE_MAX_FINGERS );
	return &mFingers[Index];
}

InputFinger* Input::getFinger( const Int64& fingerId ) {
	for ( Uint32 i = 0; i < EE_MAX_FINGERS; i++ ) {
		if ( mFingers[i].id == fingerId ) {
			return &mFingers[i];
		}
	}

	return NULL;
}

std::vector<InputFinger*> Input::getFingersDown() {
	std::vector<InputFinger*> fDown;

	for ( Uint32 i = 0; i < EE_MAX_FINGERS; i++ ) {
		if ( mFingers[i].down ) {
			fDown.push_back( &mFingers[i] );
		}
	}

	return fDown;
}

std::vector<InputFinger*> Input::getFingersWasDown() {
	std::vector<InputFinger*> fDown;

	for ( Uint32 i = 0; i < EE_MAX_FINGERS; i++ ) {
		if ( mFingers[i].wasDown ) {
			fDown.push_back( &mFingers[i] );
		}
	}

	return fDown;
}

const Uint32& Input::getModState() const {
	return mInputMod;
}

static Uint32 sanitizeMod( const Uint32& mod ) {
	Uint32 smod = 0;
	if ( mod & KEYMOD_CTRL )
		smod |= KEYMOD_CTRL;
	if ( mod & KEYMOD_SHIFT )
		smod |= KEYMOD_SHIFT;
	if ( mod & KEYMOD_META )
		smod |= KEYMOD_META;
	if ( mod & KEYMOD_LALT )
		smod |= KEYMOD_LALT;
	if ( mod & KEYMOD_RALT )
		smod |= KEYMOD_RALT;
	return smod;
}

Uint32 Input::getSanitizedModState() const {
	return sanitizeMod( mInputMod );
}

bool Input::isModState( const Uint32& state ) const {
	return getSanitizedModState() == state;
}

Time Input::getElapsedSinceLastMouseEvent() const {
	return mLastMouseEvent.getElapsedTime();
}

Time Input::getElapsedSinceLastKeyboardOrMouseEvent() const {
	return eemin( getElapsedSinceLastMouseEvent(), getElapsedSinceLastKeyboardEvent() );
}

Time Input::getElapsedSinceLastKeyboardEvent() const {
	return mLastKeyboardEvent.getElapsedTime();
}

Time Input::getElapsedSinceLastEvent() const {
	return mLastEvent.getElapsedTime();
}

const Uint64& Input::getEventsSentId() const {
	return mEventsSentId;
}

}} // namespace EE::Window
