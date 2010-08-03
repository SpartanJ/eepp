#include "cinput.hpp"
#include "cengine.hpp"
#include "cview.hpp"

namespace EE { namespace Window {

Int32 convertKeyCharacter( EE_Event* event ) {
	SDL_keysym keysym = event->key.keysym;

	Int32 value = 0;

	if ( keysym.unicode < 255 )
		value = (Int32)keysym.unicode;

	if ( isCharacter(value) )
		return value;

	switch (keysym.sym) {
		case KEY_TAB: 			value = KEY_TAB; 		break;
		case KEY_LALT: 			value = KEY_LALT; 		break;
		case KEY_RALT: 			value = KEY_RALT;		break;
		case KEY_LSHIFT:		value = KEY_LSHIFT;		break;
		case KEY_RSHIFT:		value = KEY_RSHIFT;		break;
		case KEY_LCTRL:			value = KEY_RSHIFT;		break;
		case KEY_RCTRL:			value = KEY_LCTRL;		break;
		case KEY_BACKSPACE:		value = KEY_BACKSPACE; 	break;
		case KEY_PAUSE:			value = KEY_PAUSE;		break;
		case KEY_SPACE:
			// Special characters like ~ (tilde) ends up with the keysym.sym KEY_SPACE which
			// without this check would be lost. The check is only valid on key down events in SDL.
			if (event->type == SDL_KEYUP || keysym.unicode == ' ')
				value = KEY_SPACE;
			break;
		case KEY_ESCAPE:		value = KEY_ESCAPE;		break;
		case KEY_DELETE:		value = KEY_DELETE;		break;
		case KEY_INSERT:		value = KEY_INSERT;		break;
		case KEY_HOME:			value = KEY_HOME;		break;
		case KEY_END:			value = KEY_END;		break;
		case KEY_PAGEUP:		value = KEY_PAGEUP;		break;
		case KEY_PRINT:			value = KEY_PRINT;		break;
		case KEY_PAGEDOWN:		value = KEY_PAGEDOWN;	break;
		case KEY_F1:			value = KEY_F1;			break;
		case KEY_F2:			value = KEY_F2;			break;
		case KEY_F3:			value = KEY_F3;			break;
		case KEY_F4:			value = KEY_F4;			break;
		case KEY_F5:			value = KEY_F5;			break;
		case KEY_F6:			value = KEY_F6;			break;
		case KEY_F7:			value = KEY_F7;			break;
		case KEY_F8:			value = KEY_F8;			break;
		case KEY_F9:			value = KEY_F9;			break;
		case KEY_F10:			value = KEY_F10;		break;
		case KEY_F11:			value = KEY_F11;		break;
		case KEY_F12:			value = KEY_F12;		break;
		case KEY_F13:			value = KEY_F13;		break;
		case KEY_F14:			value = KEY_F14;		break;
		case KEY_F15:			value = KEY_F15;		break;
		case KEY_NUMLOCK:		value = KEY_NUMLOCK;	break;
		case KEY_CAPSLOCK:		value = KEY_CAPSLOCK;	break;
		case KEY_SCROLLOCK:		value = KEY_SCROLLOCK;	break;
		case KEY_RMETA:			value = KEY_RMETA;		break;
		case KEY_LMETA:			value = KEY_LMETA;		break;
		case KEY_LSUPER:		value = KEY_LSUPER;		break;
		case KEY_RSUPER:		value = KEY_RSUPER;		break;
		case KEY_MODE:			value = KEY_MODE;		break;
		case KEY_UP:			value = KEY_UP;			break;
		case KEY_DOWN:			value = KEY_DOWN;		break;
		case KEY_LEFT:			value = KEY_LEFT;		break;
		case KEY_RIGHT:			value = KEY_RIGHT;		break;
		case KEY_RETURN:		value = KEY_RETURN;		break;
		case KEY_KP_ENTER:		value = KEY_KP_ENTER;	break;
		default:				break;
	}

	if (! (keysym.mod & KMOD_NUM) ) {
		switch (keysym.sym) {
			case KEY_KP0:		value = KEY_INSERT;		break;
			case KEY_KP1:		value = KEY_END;		break;
			case KEY_KP2:		value = KEY_DOWN;		break;
			case KEY_KP3:		value = KEY_PAGEDOWN;	break;
			case KEY_KP4:		value = KEY_LEFT;		break;
			case KEY_KP5:		value = 0;				break;
			case KEY_KP6:		value = KEY_RIGHT;		break;
			case KEY_KP7:		value = KEY_HOME;		break;
			case KEY_KP8:		value = KEY_UP;			break;
			case KEY_KP9:		value = KEY_PAGEUP;		break;
			default:			break;
		}
	}
	return value;
}

cInput::cInput() :
	mPressTrigger(0), mReleaseTrigger(0), mLastPressTrigger(0), mClickTrigger(0), mDoubleClickTrigger(0), mInputMod(0),
	mDoubleClickInterval(500),
	mLastButtonLeftClicked(0), 		mLastButtonRightClicked(0), 	mLastButtonMiddleClicked(0),
	mLastButtonLeftClick(0), 		mLastButtonRightClick(0), 		mLastButtonMiddleClick(0),
	mTClick(0), mVRCall(NULL), mNumCallBacks(0), mInputGrabed(false),
	#if EE_PLATFORM == EE_PLATFORM_LINUX
	mMouseSpeed(2.0f)
	#else
	mMouseSpeed(1.0f)
	#endif
{
	EE = cEngine::instance();

	eeVector2if mTempMouse;
	SDL_GetMouseState( &mTempMouse.x, &mTempMouse.y );
	mMousePos.x = (eeInt)mTempMouse.x;
	mMousePos.y = (eeInt)mTempMouse.y;

	SDL_EnableUNICODE(1);
	SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);

	memset( mKeysDown	, 0, EE_KEYS_SPACE );
	memset( mKeysUp		, 0, EE_KEYS_SPACE );
}

cInput::~cInput() {
	mCallbacks.clear();
}

void cInput::Update() {
	memset( mKeysUp, 0, EE_KEYS_SPACE );

	mReleaseTrigger 	= 0;
	mLastPressTrigger 	= mPressTrigger;
	mClickTrigger 		= 0;
	mDoubleClickTrigger = 0;
	mInputMod 			= 0;

	while ( SDL_PollEvent(&mEvent) ) {
		switch(mEvent.type) {
			case SDL_KEYDOWN:
				mInputMod = mEvent.key.keysym.mod;

				PushKey( &mKeysDown	[ mEvent.key.keysym.sym / 8 ], mEvent.key.keysym.sym % 8, true );
				break;
			case SDL_KEYUP:
				mInputMod = mEvent.key.keysym.mod;

				PushKey( &mKeysDown	[ mEvent.key.keysym.sym / 8 ], mEvent.key.keysym.sym % 8, false );
				PushKey( &mKeysUp	[ mEvent.key.keysym.sym / 8 ], mEvent.key.keysym.sym % 8, true );
				break;
			case SDL_MOUSEMOTION:
			{
				if ( !mInputGrabed ) {
					mMousePos.x = mEvent.motion.x;
					mMousePos.y = mEvent.motion.y;
				} else {
					mMousePos.x += static_cast<Int32>( (eeFloat)mEvent.motion.xrel * mMouseSpeed );
					mMousePos.y += static_cast<Int32>( (eeFloat)mEvent.motion.yrel * mMouseSpeed );
				}

				bool bInject = false;

				if ( mMousePos.x >= (eeInt)EE->GetWidth() )
					mMousePos.x = EE->GetWidth();
				else if ( mMousePos.x < 0 ) {
					mMousePos.x = 0;
					bInject = true;
				}

				if ( mMousePos.y >= (eeInt)EE->GetHeight() )
					mMousePos.y = EE->GetHeight();
				else if ( mMousePos.y < 0 ) {
					mMousePos.y = 0;
					bInject = true;
				}

				#if EE_PLATFORM == EE_PLATFORM_WIN32
				if ( bInject )
					InjectMousePos( mMousePos.x, mMousePos.y );
				#endif

				break;
			}
			case SDL_MOUSEBUTTONDOWN:
				mPressTrigger |= EE_BUTTON( mEvent.button.button );
				break;
			case SDL_MOUSEBUTTONUP:
				mPressTrigger &= ~EE_BUTTON( mEvent.button.button );
				mReleaseTrigger |= EE_BUTTON( mEvent.button.button );
				mClickTrigger |= EE_BUTTON( mEvent.button.button );

				if ( mEvent.button.button == EE_BUTTON_LEFT ) {
					mLastButtonLeftClicked = mLastButtonLeftClick;
					mLastButtonLeftClick = eeGetTicks();

					mTClick = mLastButtonLeftClick - mLastButtonLeftClicked;
					if (mTClick < mDoubleClickInterval && mTClick > 0) {
						mDoubleClickTrigger |= EE_BUTTON(EE_BUTTON_LEFT);
						mLastButtonLeftClick = 0;
						mLastButtonLeftClicked = 0;
					}
				} else if ( mEvent.button.button == EE_BUTTON_RIGHT ) {
					mLastButtonRightClicked = mLastButtonRightClick;
					mLastButtonRightClick = eeGetTicks();

					mTClick = mLastButtonRightClick - mLastButtonRightClicked;
					if (mTClick < mDoubleClickInterval && mTClick > 0) {
						mDoubleClickTrigger |= EE_BUTTON(EE_BUTTON_RIGHT);
						mLastButtonRightClick = 0;
						mLastButtonRightClicked = 0;
					}
				} else if( mEvent.button.button == EE_BUTTON_MIDDLE ) {
					mLastButtonMiddleClicked = mLastButtonMiddleClick;
					mLastButtonMiddleClick = eeGetTicks();

					mTClick = mLastButtonMiddleClick - mLastButtonMiddleClicked;
					if (mTClick < mDoubleClickInterval && mTClick > 0) {
						mDoubleClickTrigger |= EE_BUTTON(EE_BUTTON_MIDDLE);
						mLastButtonMiddleClick = 0;
						mLastButtonMiddleClicked = 0;
					}
				}

				break;
			case SDL_VIDEORESIZE:
				EE->ChangeRes(mEvent.resize.w, mEvent.resize.h, EE->Windowed() );

				if ( NULL != mVRCall )
					mVRCall();

				break;
			case SDL_QUIT:
				EE->Running(false);
				break;
		}

		for ( std::map<Uint32, InputCallback>::iterator i = mCallbacks.begin(); i != mCallbacks.end(); i++ ) {
			i->second( &mEvent );
		}
	}
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
		if ( !( mPressTrigger & EE_BUTTON( Button )  ) )
			mPressTrigger |= EE_BUTTON( Button );
}

void cInput::InjectButtonRelease( const Uint32& Button ) {
	if ( Button < 8 ) {
		if ( mPressTrigger & EE_BUTTON( Button )  )
			mPressTrigger &= ~EE_BUTTON( Button );

		if ( !( mReleaseTrigger & EE_BUTTON( Button )  ) )
			mReleaseTrigger |= EE_BUTTON( Button );

		if ( !( mClickTrigger & EE_BUTTON( Button )  ) )
			mClickTrigger |= EE_BUTTON( Button );
	}
}

eeVector2i cInput::GetMousePos() const {
	return mMousePos;
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

void cInput::SetVideoResizeCallback( const VideoResizeCallback& vrc ) {
	mVRCall = vrc;
}

bool cInput::GrabInput() {
	return ( SDL_WM_GrabInput( SDL_GRAB_QUERY ) == SDL_GRAB_ON ) ? true : false;
}

void cInput::GrabInput( const bool& Grab ) {
	mInputGrabed = Grab;

	if ( Grab )
		SDL_WM_GrabInput(SDL_GRAB_ON);
	else
		SDL_WM_GrabInput(SDL_GRAB_OFF);
}

void cInput::InjectMousePos( const Uint16& x, const Uint16& y ) {
	SDL_WarpMouse( x, y );
}

void cInput::InjectMousePos( const eeVector2i& Pos ) {
	InjectMousePos( Pos.x, Pos.y );
}

bool cInput::ControlPressed() const {
	return ( mInputMod & KMOD_CTRL ) != 0;
}

bool cInput::ShiftPressed() const {
	return ( mInputMod & KMOD_SHIFT ) != 0;
}

bool cInput::AltPressed() const {
	return ( mInputMod & KMOD_ALT ) != 0;
}

bool cInput::MetaPressed() const {
	return ( mInputMod & KMOD_META ) != 0;
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

}}
