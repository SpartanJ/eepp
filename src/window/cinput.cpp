#include "cinput.hpp"
#include "cview.hpp"

namespace EE { namespace Window {

cInput::cInput( cWindow * window, cJoystickManager * joystickmanager ) :
	mWindow( window ),
	mJoystickManager( joystickmanager ),
	mPressTrigger(0), mReleaseTrigger(0), mLastPressTrigger(0), mClickTrigger(0), mDoubleClickTrigger(0), mInputMod(0),
	mDoubleClickInterval(500),
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
			mInputMod = Event->key.keysym.mod;

			PushKey( &mKeysDown	[ Event->key.keysym.sym / 8 ], Event->key.keysym.sym % 8, true );
			break;
		}
		case InputEvent::KeyUp:
		{
			PushKey( &mKeysDown	[ Event->key.keysym.sym / 8 ], Event->key.keysym.sym % 8, false );
			PushKey( &mKeysUp	[ Event->key.keysym.sym / 8 ], Event->key.keysym.sym % 8, true );
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

			bool bInject = false;

			if ( mMousePos.x >= (eeInt)mWindow->GetWidth() ) {
				mMousePos.x = mWindow->GetWidth();
			} else if ( mMousePos.x < 0 ) {
				mMousePos.x = 0;
				bInject = true;
			}

			if ( mMousePos.y >= (eeInt)mWindow->GetHeight() ) {
				mMousePos.y = mWindow->GetHeight();
			} else if ( mMousePos.y < 0 ) {
				mMousePos.y = 0;
				bInject = true;
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

			if ( Event->button.button == EE_BUTTON_LEFT ) {
				mLastButtonLeftClicked		= mLastButtonLeftClick;
				mLastButtonLeftClick		= eeGetTicks();

				mTClick = mLastButtonLeftClick - mLastButtonLeftClicked;
				
				if ( mTClick < mDoubleClickInterval && mTClick > 0 ) {
					mDoubleClickTrigger			|= EE_BUTTON_MASK(EE_BUTTON_LEFT);
					mLastButtonLeftClick		= 0;
					mLastButtonLeftClicked		= 0;
				}
			} else if ( Event->button.button == EE_BUTTON_RIGHT ) {
				mLastButtonRightClicked		= mLastButtonRightClick;
				mLastButtonRightClick		= eeGetTicks();

				mTClick = mLastButtonRightClick - mLastButtonRightClicked;
				
				if ( mTClick < mDoubleClickInterval && mTClick > 0 ) {
					mDoubleClickTrigger			|= EE_BUTTON_MASK(EE_BUTTON_RIGHT);
					mLastButtonRightClick		= 0;
					mLastButtonRightClicked		= 0;
				}
			} else if( Event->button.button == EE_BUTTON_MIDDLE ) {
				mLastButtonMiddleClicked	= mLastButtonMiddleClick;
				mLastButtonMiddleClick		= eeGetTicks();

				mTClick = mLastButtonMiddleClick - mLastButtonMiddleClicked;
				
				if ( mTClick < mDoubleClickInterval && mTClick > 0 ) {
					mDoubleClickTrigger			|= EE_BUTTON_MASK(EE_BUTTON_MIDDLE);
					mLastButtonMiddleClick		= 0;
					mLastButtonMiddleClicked	= 0;
				}
			}

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

bool cInput::GetKey( Uint8 * Key, Uint8 Pos ) {
	if ( ( * Key ) & ( 1 << Pos ) )
		return true;

	return false;
}

void cInput::PushKey( Uint8 * Key, Uint8 Pos, bool BitWrite ) {
	if ( BitWrite )
		( * Key ) |= ( 1 << Pos );
	else {
		if ( ( * Key ) & ( 1 << Pos ) )
			( * Key ) &= ~( 1 << Pos );
	}
}

bool cInput::IsKeyDown( const EE_KEY& Key ) {
	return GetKey( &mKeysDown[ Key / 8 ], Key % 8 );
}

bool cInput::IsKeyUp( const EE_KEY& Key ) {
	return GetKey( &mKeysUp[ Key / 8 ], Key % 8 );
}

void cInput::InjectKeyDown( const EE_KEY& Key ) {
	PushKey( &mKeysDown	[ Key / 8 ], Key % 8, true );
}

void cInput::InjectKeyUp( const EE_KEY& Key ) {
	PushKey( &mKeysUp	[ Key / 8 ], Key % 8, true );
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

}}
