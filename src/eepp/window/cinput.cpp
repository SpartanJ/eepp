#include <eepp/window/cinput.hpp>
#include <eepp/window/cview.hpp>

namespace EE { namespace Window {

cInput::cInput( cWindow * window, cJoystickManager * joystickmanager ) :
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

cInput::~cInput() {
	eeSAFE_DELETE( mJoystickManager );
}

void cInput::CleanStates() {
	memset( mKeysUp, 0, EE_KEYS_SPACE );

	mReleaseTrigger 	= 0;
	mLastPressTrigger 	= mPressTrigger;
	mClickTrigger 		= 0;
	mDoubleClickTrigger = 0;

	ResetFingerWasDown();
}

void cInput::SendEvent( InputEvent * Event ) {
	for ( std::map<Uint32, InputCallback>::iterator i = mCallbacks.begin(); i != mCallbacks.end(); i++ ) {
		i->second( Event );
	}
}

void cInput::ProcessEvent( InputEvent * Event ) {
	switch( Event->Type ) {
		case InputEvent::KeyDown:
		{
			if ( Event->key.keysym.sym > EE_KEYS_NUM )
				break;

			if ( Event->key.keysym.mod != eeINDEX_NOT_FOUND )
				mInputMod = Event->key.keysym.mod;

			BitOp::WriteBitKey( &mKeysDown	[ Event->key.keysym.sym / 8 ], Event->key.keysym.sym % 8, 1 );
			break;
		}
		case InputEvent::KeyUp:
		{
			if ( Event->key.keysym.sym > EE_KEYS_NUM )
				break;

			BitOp::WriteBitKey( &mKeysDown	[ Event->key.keysym.sym / 8 ], Event->key.keysym.sym % 8, 0 );
			BitOp::WriteBitKey( &mKeysUp	[ Event->key.keysym.sym / 8 ], Event->key.keysym.sym % 8, 1 );
			break;
		}
		case InputEvent::MouseMotion:
		{
			if ( !mInputGrabed ) {
				mMousePos.x = Event->motion.x;
				mMousePos.y = Event->motion.y;
			} else {
				mMousePos.x += static_cast<Int32>( (eeFloat)Event->motion.xrel * mMouseSpeed );
				mMousePos.y += static_cast<Int32>( (eeFloat)Event->motion.yrel * mMouseSpeed );
			}

			#if EE_PLATFORM == EE_PLATFORM_WIN
			bool bInject = false;
			#endif

			if ( mMousePos.x >= (eeInt)mWindow->GetWidth() ) {
				mMousePos.x = mWindow->GetWidth();
			} else if ( mMousePos.x < 0 ) {
				mMousePos.x = 0;

				#if EE_PLATFORM == EE_PLATFORM_WIN
				bInject = true;
				#endif
			}

			if ( mMousePos.y >= (eeInt)mWindow->GetHeight() ) {
				mMousePos.y = mWindow->GetHeight();
			} else if ( mMousePos.y < 0 ) {
				mMousePos.y = 0;

				#if EE_PLATFORM == EE_PLATFORM_WIN
				bInject = true;
				#endif
			}

			#if EE_PLATFORM == EE_PLATFORM_WIN
			if ( bInject ) {
				InjectMousePos( mMousePos.x, mMousePos.y );
			}
			#endif

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
				mLastButtonLeftClick		= Sys::GetTicks();

				mTClick = mLastButtonLeftClick - mLastButtonLeftClicked;

				if ( mTClick < mDoubleClickInterval && mTClick > 0 ) {
					mDoubleClickTrigger			|= EE_BUTTON_MASK(EE_BUTTON_LEFT);
					mLastButtonLeftClick		= 0;
					mLastButtonLeftClicked		= 0;
				}
			} else if ( Event->button.button == EE_BUTTON_RIGHT ) {
				mLastButtonRightClicked		= mLastButtonRightClick;
				mLastButtonRightClick		= Sys::GetTicks();

				mTClick = mLastButtonRightClick - mLastButtonRightClicked;

				if ( mTClick < mDoubleClickInterval && mTClick > 0 ) {
					mDoubleClickTrigger			|= EE_BUTTON_MASK(EE_BUTTON_RIGHT);
					mLastButtonRightClick		= 0;
					mLastButtonRightClicked		= 0;
				}
			} else if( Event->button.button == EE_BUTTON_MIDDLE ) {
				mLastButtonMiddleClicked	= mLastButtonMiddleClick;
				mLastButtonMiddleClick		= Sys::GetTicks();

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
			cInputFinger * Finger = GetFingerId( Event->finger.fingerId );

			Finger->WriteLast();
			Finger->x			= (Uint16)( Event->finger.x * (eeFloat)mWindow->GetWidth() );
			Finger->y			= (Uint16)( Event->finger.y * (eeFloat)mWindow->GetHeight() );
			Finger->pressure	= Event->finger.pressure;
			Finger->down		= true;
			Finger->xdelta		= Event->finger.dx;
			Finger->ydelta		= Event->finger.dy;

			break;
		}
		case InputEvent::FingerUp:
		{
			cInputFinger * Finger = GetFingerId( Event->finger.fingerId );

			Finger->WriteLast();
			Finger->x			= (Uint16)( Event->finger.x * (eeFloat)mWindow->GetWidth() );
			Finger->y			= (Uint16)( Event->finger.y * (eeFloat)mWindow->GetHeight() );
			Finger->pressure	= Event->finger.pressure;
			Finger->down		= false;
			Finger->was_down	= true;
			Finger->xdelta		= Event->finger.dx;
			Finger->ydelta		= Event->finger.dy;

			break;
		}
		case InputEvent::FingerMotion:
		{
			cInputFinger * Finger = GetFingerId( Event->finger.fingerId );

			Finger->WriteLast();
			Finger->x			= (Uint16)( Event->finger.x * (eeFloat)mWindow->GetWidth() );
			Finger->y			= (Uint16)( Event->finger.y * (eeFloat)mWindow->GetHeight() );
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

cInputFinger * cInput::GetFingerId( const Int64 &fingerId ) {
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

void cInput::ResetFingerWasDown() {
	for ( Uint32 i = 0; i < EE_MAX_FINGERS; i++ ) {
		mFingers[i].was_down = false;
	}
}

bool cInput::IsKeyDown( const EE_KEY& Key ) {
	return 0 != BitOp::ReadBitKey( &mKeysDown[ Key / 8 ], Key % 8 );
}

bool cInput::IsKeyUp( const EE_KEY& Key ) {
	return 0 != BitOp::ReadBitKey( &mKeysUp[ Key / 8 ], Key % 8 );
}

void cInput::InjectKeyDown( const EE_KEY& Key ) {
	BitOp::WriteBitKey( &mKeysDown	[ Key / 8 ], Key % 8, 1 );
}

void cInput::InjectKeyUp( const EE_KEY& Key ) {
	BitOp::WriteBitKey( &mKeysUp	[ Key / 8 ], Key % 8, 1 );
}

void cInput::InjectButtonPress( const Uint32& Button ) {
	if ( Button < 8 )
		if ( !( mPressTrigger & EE_BUTTON_MASK( Button )  ) )
			mPressTrigger |= EE_BUTTON_MASK( Button );
}

void cInput::InjectButtonRelease( const Uint32& Button ) {
	if ( Button < 8 ) {
		if ( mPressTrigger & EE_BUTTON_MASK( Button )  )
			mPressTrigger &= ~EE_BUTTON_MASK( Button );

		if ( !( mReleaseTrigger & EE_BUTTON_MASK( Button )  ) )
			mReleaseTrigger |= EE_BUTTON_MASK( Button );

		if ( !( mClickTrigger & EE_BUTTON_MASK( Button )  ) )
			mClickTrigger |= EE_BUTTON_MASK( Button );
	}
}

eeVector2i cInput::GetMousePos() const {
	return mMousePos;
}

void cInput::SetMousePos( const eeVector2i& Pos ) {
	mMousePos = Pos;
}

eeVector2f cInput::GetMousePosf() {
	return eeVector2f( (eeFloat)mMousePos.x, (eeFloat)mMousePos.y );
}

eeVector2i cInput::GetMousePosFromView( const cView& View ) {
	eeVector2i RealMousePos = GetMousePos();
	eeRecti RView = View.GetView();
	return eeVector2i( RealMousePos.x - RView.Left, RealMousePos.y - RView.Top );
}

Uint16 cInput::MouseX() const {
	return mMousePos.x;
}

Uint16 cInput::MouseY() const {
	return mMousePos.y;
}

Uint32 cInput::PushCallback( const InputCallback& cb ) {
	mNumCallBacks++;
	mCallbacks[ mNumCallBacks ] = cb;
	return mNumCallBacks;
}

void cInput::PopCallback( const Uint32& CallbackId ) {
	mCallbacks[ CallbackId ] = 0;
	mCallbacks.erase( mCallbacks.find(CallbackId) );
}

void cInput::InjectMousePos( const eeVector2i& Pos ) {
	InjectMousePos( Pos.x, Pos.y );
}

bool cInput::ControlPressed() const {
	return ( mInputMod & KEYMOD_CTRL ) != 0;
}

bool cInput::ShiftPressed() const {
	return ( mInputMod & KEYMOD_SHIFT ) != 0;
}

bool cInput::AltPressed() const {
	return ( mInputMod & KEYMOD_ALT ) != 0;
}

bool cInput::MetaPressed() const {
	return ( mInputMod & KEYMOD_META ) != 0;
}

bool cInput::MouseLeftPressed() const {
	return ( mPressTrigger & EE_BUTTON_LMASK ) != 0;
}

bool cInput::MouseRightPressed() const {
	return ( mPressTrigger & EE_BUTTON_RMASK ) != 0;
}

bool cInput::MouseMiddlePressed() const {
	return ( mPressTrigger & EE_BUTTON_MMASK ) != 0;
}

bool cInput::MouseLeftClick() const {
	return ( mClickTrigger & EE_BUTTON_LMASK ) != 0;
}

bool cInput::MouseRightClick() const {
	return ( mClickTrigger & EE_BUTTON_RMASK ) != 0;
}

bool cInput::MouseMiddleClick() const {
	return ( mClickTrigger & EE_BUTTON_MMASK ) != 0;
}

bool cInput::MouseLeftDoubleClick() const {
	return ( mDoubleClickTrigger & EE_BUTTON_LMASK ) != 0;
}

bool cInput::MouseRightDoubleClick() const {
	return ( mDoubleClickTrigger & EE_BUTTON_RMASK ) != 0;
}

bool cInput::MouseMiddleDoubleClick() const {
	return ( mDoubleClickTrigger & EE_BUTTON_MMASK ) != 0;
}

bool cInput::MouseWheelUp() const {
	return ( mReleaseTrigger & EE_BUTTON_WUMASK ) != 0;
}

bool cInput::MouseWheelDown() const {
	return ( mReleaseTrigger & EE_BUTTON_WDMASK ) != 0;
}

void cInput::MouseSpeed( const eeFloat& Speed ) {
	mMouseSpeed = Speed;
}

const eeFloat& cInput::MouseSpeed() const {
	return mMouseSpeed;
}

const Uint32& cInput::LastPressTrigger() const {
	return mLastPressTrigger;
}

const Uint32& cInput::PressTrigger() const {
	return mPressTrigger;
}

const Uint32& cInput::ReleaseTrigger() const {
	return mReleaseTrigger;
}

const Uint32& cInput::ClickTrigger() const {
	return mClickTrigger;
}

const Uint32& cInput::DoubleClickTrigger() const {
	return mDoubleClickTrigger;
}

const Uint32& cInput::DoubleClickInterval() const {
	return mDoubleClickInterval;
}

void cInput::DoubleClickInterval( const Uint32& Interval ) {
	mDoubleClickInterval = Interval;
}

cJoystickManager * cInput::GetJoystickManager() const {
	return mJoystickManager;
}

Uint32 cInput::GetFingerCount() {
	 return EE_MAX_FINGERS;
}

cInputFinger * cInput::GetFingerIndex( const Uint32 &Index ) {
	eeASSERT( Index < EE_MAX_FINGERS );
	return &mFingers[Index];
}

cInputFinger * cInput::GetFinger( const Int64 &fingerId ) {
	for ( Uint32 i = 0; i < EE_MAX_FINGERS; i++ ) {
		if ( mFingers[i].id == fingerId ) {
			return &mFingers[i];
		}
	}

	return NULL;
}

std::list<cInputFinger *> cInput::GetFingersDown() {
	std::list<cInputFinger *> fDown;

	for ( Uint32 i = 0; i < EE_MAX_FINGERS; i++ ) {
		if ( mFingers[i].down ) {
			fDown.push_back( &mFingers[i] );
		}
	}

	return fDown;
}

std::list<cInputFinger *> cInput::GetFingersWasDown() {
	std::list<cInputFinger *> fDown;

	for ( Uint32 i = 0; i < EE_MAX_FINGERS; i++ ) {
		if ( mFingers[i].was_down ) {
			fDown.push_back( &mFingers[i] );
		}
	}

	return fDown;
}

}}
