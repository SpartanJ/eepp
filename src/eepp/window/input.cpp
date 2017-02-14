#include <eepp/window/input.hpp>
#include <eepp/window/view.hpp>

namespace EE { namespace Window {

Input::Input( EE::Window::Window * window, JoystickManager * joystickmanager ) :
	mWindow( window ),
	mJoystickManager( joystickmanager ),
	mPressTrigger(0), mReleaseTrigger(0), mLastPressTrigger(0), mClickTrigger(0), mDoubleClickTrigger(0), mInputMod(0),
	mDoubleClickInterval(400),
	mLastButtonLeftClicked(0), 		mLastButtonRightClicked(0), 	mLastButtonMiddleClicked(0),
	mLastButtonLeftClick(0), 		mLastButtonRightClick(0), 		mLastButtonMiddleClick(0),
	mTClick(0), mNumCallBacks(0),
	mMouseSpeed(1.0f),
	mInputGrabed( false )
{
	memset( mKeysDown	, 0, EE_KEYS_SPACE );
	memset( mKeysUp		, 0, EE_KEYS_SPACE );
}

Input::~Input() {
	eeSAFE_DELETE( mJoystickManager );
}

void Input::CleanStates() {
	memset( mKeysUp, 0, EE_KEYS_SPACE );

	mReleaseTrigger 	= 0;
	mLastPressTrigger 	= mPressTrigger;
	mClickTrigger 		= 0;
	mDoubleClickTrigger = 0;

	ResetFingerWasDown();
}

void Input::SendEvent( InputEvent * Event ) {
	for ( std::map<Uint32, InputCallback>::iterator i = mCallbacks.begin(); i != mCallbacks.end(); i++ ) {
		i->second( Event );
	}
}

void Input::ProcessEvent( InputEvent * Event ) {
	switch( Event->Type ) {
		case InputEvent::KeyDown:
		{
			if ( Event->key.keysym.sym > EE_KEYS_NUM )
				break;

			if ( Event->key.keysym.mod != eeINDEX_NOT_FOUND )
				mInputMod = Event->key.keysym.mod;

			BitOp::writeBitKey( &mKeysDown	[ Event->key.keysym.sym / 8 ], Event->key.keysym.sym % 8, 1 );
			break;
		}
		case InputEvent::KeyUp:
		{
			if ( Event->key.keysym.sym > EE_KEYS_NUM )
				break;

			BitOp::writeBitKey( &mKeysDown	[ Event->key.keysym.sym / 8 ], Event->key.keysym.sym % 8, 0 );
			BitOp::writeBitKey( &mKeysUp	[ Event->key.keysym.sym / 8 ], Event->key.keysym.sym % 8, 1 );
			break;
		}
		case InputEvent::MouseMotion:
		{
			if ( !mInputGrabed ) {
				mMousePos.x = Event->motion.x;
				mMousePos.y = Event->motion.y;
			} else {
				mMousePos.x += static_cast<Int32>( (Float)Event->motion.xrel * mMouseSpeed );
				mMousePos.y += static_cast<Int32>( (Float)Event->motion.yrel * mMouseSpeed );
			}

			if ( mMousePos.x >= (int)mWindow->GetWidth() ) {
				mMousePos.x = mWindow->GetWidth();
			} else if ( mMousePos.x < 0 ) {
				mMousePos.x = 0;
			}

			if ( mMousePos.y >= (int)mWindow->GetHeight() ) {
				mMousePos.y = mWindow->GetHeight();
			} else if ( mMousePos.y < 0 ) {
				mMousePos.y = 0;
			}

			break;
		}
		case InputEvent::MouseButtonDown:
		{
			mPressTrigger		|= EE_BUTTON_MASK( Event->button.button );
			break;
		}
		case InputEvent::MouseButtonUp:
		{
			mPressTrigger		&= ~EE_BUTTON_MASK( Event->button.button );
			mReleaseTrigger		|= EE_BUTTON_MASK( Event->button.button );
			mClickTrigger		|= EE_BUTTON_MASK( Event->button.button );

			// I know this is ugly, but i'm too lazy to fix it, it works...
			if ( Event->button.button == EE_BUTTON_LEFT ) {
				mLastButtonLeftClicked		= mLastButtonLeftClick;
				mLastButtonLeftClick		= Sys::getTicks();

				mTClick = mLastButtonLeftClick - mLastButtonLeftClicked;

				if ( mTClick < mDoubleClickInterval && mTClick > 0 ) {
					mDoubleClickTrigger			|= EE_BUTTON_MASK(EE_BUTTON_LEFT);
					mLastButtonLeftClick		= 0;
					mLastButtonLeftClicked		= 0;
				}
			} else if ( Event->button.button == EE_BUTTON_RIGHT ) {
				mLastButtonRightClicked		= mLastButtonRightClick;
				mLastButtonRightClick		= Sys::getTicks();

				mTClick = mLastButtonRightClick - mLastButtonRightClicked;

				if ( mTClick < mDoubleClickInterval && mTClick > 0 ) {
					mDoubleClickTrigger			|= EE_BUTTON_MASK(EE_BUTTON_RIGHT);
					mLastButtonRightClick		= 0;
					mLastButtonRightClicked		= 0;
				}
			} else if( Event->button.button == EE_BUTTON_MIDDLE ) {
				mLastButtonMiddleClicked	= mLastButtonMiddleClick;
				mLastButtonMiddleClick		= Sys::getTicks();

				mTClick = mLastButtonMiddleClick - mLastButtonMiddleClicked;

				if ( mTClick < mDoubleClickInterval && mTClick > 0 ) {
					mDoubleClickTrigger			|= EE_BUTTON_MASK(EE_BUTTON_MIDDLE);
					mLastButtonMiddleClick		= 0;
					mLastButtonMiddleClicked	= 0;
				}
			}

			break;
		}
		case InputEvent::FingerDown:
		{
			InputFinger * Finger = GetFingerId( Event->finger.fingerId );

			Finger->WriteLast();
			Finger->x			= (Uint16)( Event->finger.x * (Float)mWindow->GetWidth() );
			Finger->y			= (Uint16)( Event->finger.y * (Float)mWindow->GetHeight() );
			Finger->pressure	= Event->finger.pressure;
			Finger->down		= true;
			Finger->xdelta		= Event->finger.dx;
			Finger->ydelta		= Event->finger.dy;

			break;
		}
		case InputEvent::FingerUp:
		{
			InputFinger * Finger = GetFingerId( Event->finger.fingerId );

			Finger->WriteLast();
			Finger->x			= (Uint16)( Event->finger.x * (Float)mWindow->GetWidth() );
			Finger->y			= (Uint16)( Event->finger.y * (Float)mWindow->GetHeight() );
			Finger->pressure	= Event->finger.pressure;
			Finger->down		= false;
			Finger->was_down	= true;
			Finger->xdelta		= Event->finger.dx;
			Finger->ydelta		= Event->finger.dy;

			break;
		}
		case InputEvent::FingerMotion:
		{
			InputFinger * Finger = GetFingerId( Event->finger.fingerId );

			Finger->WriteLast();
			Finger->x			= (Uint16)( Event->finger.x * (Float)mWindow->GetWidth() );
			Finger->y			= (Uint16)( Event->finger.y * (Float)mWindow->GetHeight() );
			Finger->pressure	= Event->finger.pressure;
			Finger->down		= true;
			Finger->xdelta		= Event->finger.dx;
			Finger->ydelta		= Event->finger.dy;

			break;
		}
		case InputEvent::VideoResize:
		{
			mWindow->Size( Event->resize.w, Event->resize.h, mWindow->Windowed() );
			break;
		}
		case InputEvent::Quit:
		{
			mWindow->Close();
			break;
		}
	}

	SendEvent( Event );
}

InputFinger * Input::GetFingerId( const Int64 &fingerId ) {
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

void Input::ResetFingerWasDown() {
	for ( Uint32 i = 0; i < EE_MAX_FINGERS; i++ ) {
		mFingers[i].was_down = false;
	}
}

bool Input::IsKeyDown( const EE_KEY& Key ) {
	return 0 != BitOp::readBitKey( &mKeysDown[ Key / 8 ], Key % 8 );
}

bool Input::IsKeyUp( const EE_KEY& Key ) {
	return 0 != BitOp::readBitKey( &mKeysUp[ Key / 8 ], Key % 8 );
}

void Input::InjectKeyDown( const EE_KEY& Key ) {
	BitOp::writeBitKey( &mKeysDown	[ Key / 8 ], Key % 8, 1 );
}

void Input::InjectKeyUp( const EE_KEY& Key ) {
	BitOp::writeBitKey( &mKeysUp	[ Key / 8 ], Key % 8, 1 );
}

void Input::InjectButtonPress( const Uint32& Button ) {
	if ( Button < 8 )
		if ( !( mPressTrigger & EE_BUTTON_MASK( Button )  ) )
			mPressTrigger |= EE_BUTTON_MASK( Button );
}

void Input::InjectButtonRelease( const Uint32& Button ) {
	if ( Button < 8 ) {
		if ( mPressTrigger & EE_BUTTON_MASK( Button )  )
			mPressTrigger &= ~EE_BUTTON_MASK( Button );

		if ( !( mReleaseTrigger & EE_BUTTON_MASK( Button )  ) )
			mReleaseTrigger |= EE_BUTTON_MASK( Button );

		if ( !( mClickTrigger & EE_BUTTON_MASK( Button )  ) )
			mClickTrigger |= EE_BUTTON_MASK( Button );
	}
}

Vector2i Input::GetMousePos() const {
	return mMousePos;
}

void Input::SetMousePos( const Vector2i& Pos ) {
	mMousePos = Pos;
}

Vector2f Input::GetMousePosf() {
	return Vector2f( (Float)mMousePos.x, (Float)mMousePos.y );
}

Vector2i Input::GetMousePosFromView( const View& View ) {
	Vector2i RealMousePos = GetMousePos();
	Recti RView = View.GetView();
	return Vector2i( RealMousePos.x - RView.Left, RealMousePos.y - RView.Top );
}

Uint16 Input::MouseX() const {
	return mMousePos.x;
}

Uint16 Input::MouseY() const {
	return mMousePos.y;
}

Uint32 Input::PushCallback( const InputCallback& cb ) {
	mNumCallBacks++;
	mCallbacks[ mNumCallBacks ] = cb;
	return mNumCallBacks;
}

void Input::PopCallback( const Uint32& CallbackId ) {
	mCallbacks[ CallbackId ] = 0;
	mCallbacks.erase( mCallbacks.find(CallbackId) );
}

void Input::InjectMousePos( const Vector2i& Pos ) {
	InjectMousePos( Pos.x, Pos.y );
}

bool Input::ControlPressed() const {
	return ( mInputMod & KEYMOD_CTRL ) != 0;
}

bool Input::ShiftPressed() const {
	return ( mInputMod & KEYMOD_SHIFT ) != 0;
}

bool Input::AltPressed() const {
	return ( mInputMod & KEYMOD_ALT ) != 0;
}

bool Input::MetaPressed() const {
	return ( mInputMod & KEYMOD_META ) != 0;
}

bool Input::MouseLeftPressed() const {
	return ( mPressTrigger & EE_BUTTON_LMASK ) != 0;
}

bool Input::MouseRightPressed() const {
	return ( mPressTrigger & EE_BUTTON_RMASK ) != 0;
}

bool Input::MouseMiddlePressed() const {
	return ( mPressTrigger & EE_BUTTON_MMASK ) != 0;
}

bool Input::MouseLeftClick() const {
	return ( mClickTrigger & EE_BUTTON_LMASK ) != 0;
}

bool Input::MouseRightClick() const {
	return ( mClickTrigger & EE_BUTTON_RMASK ) != 0;
}

bool Input::MouseMiddleClick() const {
	return ( mClickTrigger & EE_BUTTON_MMASK ) != 0;
}

bool Input::MouseLeftDoubleClick() const {
	return ( mDoubleClickTrigger & EE_BUTTON_LMASK ) != 0;
}

bool Input::MouseRightDoubleClick() const {
	return ( mDoubleClickTrigger & EE_BUTTON_RMASK ) != 0;
}

bool Input::MouseMiddleDoubleClick() const {
	return ( mDoubleClickTrigger & EE_BUTTON_MMASK ) != 0;
}

bool Input::MouseWheelUp() const {
	return ( mReleaseTrigger & EE_BUTTON_WUMASK ) != 0;
}

bool Input::MouseWheelDown() const {
	return ( mReleaseTrigger & EE_BUTTON_WDMASK ) != 0;
}

void Input::MouseSpeed( const Float& Speed ) {
	mMouseSpeed = Speed;
}

const Float& Input::MouseSpeed() const {
	return mMouseSpeed;
}

const Uint32& Input::LastPressTrigger() const {
	return mLastPressTrigger;
}

const Uint32& Input::PressTrigger() const {
	return mPressTrigger;
}

const Uint32& Input::ReleaseTrigger() const {
	return mReleaseTrigger;
}

const Uint32& Input::ClickTrigger() const {
	return mClickTrigger;
}

const Uint32& Input::DoubleClickTrigger() const {
	return mDoubleClickTrigger;
}

const Uint32& Input::DoubleClickInterval() const {
	return mDoubleClickInterval;
}

void Input::DoubleClickInterval( const Uint32& Interval ) {
	mDoubleClickInterval = Interval;
}

JoystickManager * Input::GetJoystickManager() const {
	return mJoystickManager;
}

Uint32 Input::GetFingerCount() {
	 return EE_MAX_FINGERS;
}

InputFinger * Input::GetFingerIndex( const Uint32 &Index ) {
	eeASSERT( Index < EE_MAX_FINGERS );
	return &mFingers[Index];
}

InputFinger * Input::GetFinger( const Int64 &fingerId ) {
	for ( Uint32 i = 0; i < EE_MAX_FINGERS; i++ ) {
		if ( mFingers[i].id == fingerId ) {
			return &mFingers[i];
		}
	}

	return NULL;
}

std::list<InputFinger *> Input::GetFingersDown() {
	std::list<InputFinger *> fDown;

	for ( Uint32 i = 0; i < EE_MAX_FINGERS; i++ ) {
		if ( mFingers[i].down ) {
			fDown.push_back( &mFingers[i] );
		}
	}

	return fDown;
}

std::list<InputFinger *> Input::GetFingersWasDown() {
	std::list<InputFinger *> fDown;

	for ( Uint32 i = 0; i < EE_MAX_FINGERS; i++ ) {
		if ( mFingers[i].was_down ) {
			fDown.push_back( &mFingers[i] );
		}
	}

	return fDown;
}

}}
