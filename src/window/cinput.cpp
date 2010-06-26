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
		case SDLK_TAB: 			value = SDLK_TAB; 		break;
		case SDLK_LALT: 		value = SDLK_LALT; 		break;
		case SDLK_RALT: 		value = SDLK_RALT;		break;
		case SDLK_LSHIFT:		value = SDLK_LSHIFT;	break;
		case SDLK_RSHIFT:		value = SDLK_RSHIFT;	break;
		case SDLK_LCTRL:		value = SDLK_RSHIFT;	break;
		case SDLK_RCTRL:		value = SDLK_LCTRL;		break;
		case SDLK_BACKSPACE:	value = SDLK_BACKSPACE; break;
		case SDLK_PAUSE:		value = SDLK_PAUSE;		break;
		case SDLK_SPACE:
			// Special characters like ~ (tilde) ends up with the keysym.sym SDLK_SPACE which
			// without this check would be lost. The check is only valid on key down events in SDL.
			if (event->type == SDL_KEYUP || keysym.unicode == ' ')
				value = SDLK_SPACE;
			break;
		case SDLK_ESCAPE:		value = SDLK_ESCAPE;	break;
		case SDLK_DELETE:		value = SDLK_DELETE;	break;
		case SDLK_INSERT:		value = SDLK_INSERT;	break;
		case SDLK_HOME:			value = SDLK_HOME;		break;
		case SDLK_END:			value = SDLK_END;		break;
		case SDLK_PAGEUP:		value = SDLK_PAGEUP;	break;
		case SDLK_PRINT:		value = SDLK_PRINT;		break;
		case SDLK_PAGEDOWN:		value = SDLK_PAGEDOWN;	break;
		case SDLK_F1:			value = SDLK_F1;		break;
		case SDLK_F2:			value = SDLK_F2;		break;
		case SDLK_F3:			value = SDLK_F3;		break;
		case SDLK_F4:			value = SDLK_F4;		break;
		case SDLK_F5:			value = SDLK_F5;		break;
		case SDLK_F6:			value = SDLK_F6;		break;
		case SDLK_F7:			value = SDLK_F7;		break;
		case SDLK_F8:			value = SDLK_F8;		break;
		case SDLK_F9:			value = SDLK_F9;		break;
		case SDLK_F10:			value = SDLK_F10;		break;
		case SDLK_F11:			value = SDLK_F11;		break;
		case SDLK_F12:			value = SDLK_F12;		break;
		case SDLK_F13:			value = SDLK_F13;		break;
		case SDLK_F14:			value = SDLK_F14;		break;
		case SDLK_F15:			value = SDLK_F15;		break;
		case SDLK_NUMLOCK:		value = SDLK_NUMLOCK;	break;
		case SDLK_CAPSLOCK:		value = SDLK_CAPSLOCK;	break;
		case SDLK_SCROLLOCK:	value = SDLK_SCROLLOCK;	break;
		case SDLK_RMETA:		value = SDLK_RMETA;		break;
		case SDLK_LMETA:		value = SDLK_LMETA;		break;
		case SDLK_LSUPER:		value = SDLK_LSUPER;	break;
		case SDLK_RSUPER:		value = SDLK_RSUPER;	break;
		case SDLK_MODE:			value = SDLK_MODE;		break;
		case SDLK_UP:			value = SDLK_UP;		break;
		case SDLK_DOWN:			value = SDLK_DOWN;		break;
		case SDLK_LEFT:			value = SDLK_LEFT;		break;
		case SDLK_RIGHT:		value = SDLK_RIGHT;		break;
		case SDLK_RETURN:		value = SDLK_RETURN;	break;
		case SDLK_KP_ENTER:		value = SDLK_KP_ENTER;	break;
		default:				break;
	}

	if (! (keysym.mod & KMOD_NUM) ) {
		switch (keysym.sym) {
			case SDLK_KP0:		value = SDLK_INSERT;	break;
			case SDLK_KP1:		value = SDLK_END;		break;
			case SDLK_KP2:		value = SDLK_DOWN;		break;
			case SDLK_KP3:		value = SDLK_PAGEDOWN;	break;
			case SDLK_KP4:		value = SDLK_LEFT;		break;
			case SDLK_KP5:		value = 0;				break;
			case SDLK_KP6:		value = SDLK_RIGHT;		break;
			case SDLK_KP7:		value = SDLK_HOME;		break;
			case SDLK_KP8:		value = SDLK_UP;		break;
			case SDLK_KP9:		value = SDLK_PAGEUP;	break;
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

	FE_Init();
	
	eeVector2if mTempMouse;
	SDL_GetMouseState( &mTempMouse.x, &mTempMouse.y );
	mMousePos.x = (eeInt)mTempMouse.x;
	mMousePos.y = (eeInt)mTempMouse.y;
	
	SDL_EnableUNICODE(1);
	SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
}

cInput::~cInput() {
	mCallbacks.clear();
	FE_Quit();
}

void cInput::Update() {
	for( std::map<Uint16, EE_KEY_STATE>::iterator itr = mKeystates.begin(); itr != mKeystates.end(); itr++) {
		if ( itr->second == EE_KEYUP )
			itr->second = EE_KEYOFF;
	}
	
	mReleaseTrigger 	= 0;
	mLastPressTrigger 	= mPressTrigger;
	mClickTrigger 		= 0;
	mDoubleClickTrigger = 0;
	mInputMod 			= 0;
	
	while ( FE_PollEvent(&mEvent) ) {
		switch(mEvent.type) {
			case SDL_KEYDOWN:
				mKeystates[mEvent.key.keysym.sym] = EE_KEYDOWN;
				mInputMod = mEvent.key.keysym.mod;
				break;
			case SDL_KEYUP:
				mKeystates[mEvent.key.keysym.sym] = EE_KEYUP;
				mInputMod = mEvent.key.keysym.mod;
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

bool cInput::IsKeyDown(const EE_KEY& Key) {
	return mKeystates[Key] == EE_KEYDOWN;
}

bool cInput::IsKeyUp(const EE_KEY& Key) {
	return mKeystates[Key] == EE_KEYUP;
}

void cInput::InjectKeyDown(const EE_KEY& Key) {
	mKeystates[Key] = EE_KEYDOWN;
}

void cInput::InjectKeyUp(const EE_KEY& Key) {
	mKeystates[Key] = EE_KEYUP;
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
