#include <eepp/window/backend/SDL2/base.hpp>

#ifdef EE_BACKEND_SDL2

#include <eepp/window/backend/SDL2/cinputsdl2.hpp>
#include <eepp/window/backend/SDL2/cjoystickmanagersdl2.hpp>
#include <eepp/window/backend/SDL2/ccursormanagersdl2.hpp>
#include <eepp/window/backend/SDL2/cwindowsdl2.hpp>

namespace EE { namespace Window { namespace Backend { namespace SDL2 {

static Uint32	KeyCodesTable[ SDL_NUM_SCANCODES ];
static bool		KeyCodesTableInit = false;

cInputSDL::cInputSDL( cWindow * window ) :
	cInput( window, eeNew( cJoystickManagerSDL, () ) )
{
	#if defined( EE_X11_PLATFORM )
	mMouseSpeed = 1.75f;
	#endif
}

cInputSDL::~cInputSDL() {
}

void cInputSDL::Update() {
	SDL_Event 	SDLEvent;
	InputEvent 	EEEvent;

	CleanStates();

	while ( SDL_PollEvent( &SDLEvent ) ) {
		switch( SDLEvent.type ) {
			case SDL_WINDOWEVENT:
			{
				switch ( SDLEvent.window.event ) {
					case SDL_WINDOWEVENT_RESIZED:
					{
						EEEvent.Type = InputEvent::VideoResize;
						EEEvent.resize.w = SDLEvent.window.data1;
						EEEvent.resize.h = SDLEvent.window.data2;
						break;
					}
					case SDL_WINDOWEVENT_EXPOSED:
					{
						EEEvent.Type = InputEvent::VideoExpose;
						EEEvent.expose.type = EEEvent.Type;
						break;
					}
					case SDL_WINDOWEVENT_MINIMIZED:
					{
						EEEvent.Type = InputEvent::Active;
						EEEvent.active.gain = 0;
						EEEvent.active.state = EE_APPACTIVE;
						break;
					}
					case SDL_WINDOWEVENT_RESTORED:
					{
						EEEvent.Type = InputEvent::Active;
						EEEvent.active.gain = 1;
						EEEvent.active.state = EE_APPACTIVE;
						break;
					}
					case SDL_WINDOWEVENT_ENTER:
					{
						EEEvent.Type = InputEvent::Active;
						EEEvent.active.gain = 1;
						EEEvent.active.state = EE_APPMOUSEFOCUS;
						break;
					}
					case SDL_WINDOWEVENT_LEAVE:
					{
						EEEvent.Type = InputEvent::Active;
						EEEvent.active.gain = 0;
						EEEvent.active.state = EE_APPMOUSEFOCUS;
						break;
					}
					case SDL_WINDOWEVENT_FOCUS_GAINED:
					{
						EEEvent.Type = InputEvent::Active;
						EEEvent.active.gain = 1;
						EEEvent.active.state = EE_APPINPUTFOCUS;
						break;
					}
					case SDL_WINDOWEVENT_FOCUS_LOST:
					{
						EEEvent.Type = InputEvent::Active;
						EEEvent.active.gain = 0;
						EEEvent.active.state = EE_APPINPUTFOCUS;
						break;
					}
				}

				break;
			}
			case SDL_TEXTINPUT:
			{
				String txt = String::FromUtf8( SDLEvent.text.text );

				EEEvent.Type = InputEvent::TextInput;
				EEEvent.text.timestamp = SDLEvent.text.timestamp;
				EEEvent.text.text = txt[0];

				ProcessEvent( &EEEvent );

				EEEvent.Type = InputEvent::KeyDown;
				EEEvent.key.state = SDLEvent.key.state;
				EEEvent.key.which = SDLEvent.key.windowID;
				EEEvent.key.keysym.sym = 0;
				EEEvent.key.keysym.mod = eeINDEX_NOT_FOUND;
				EEEvent.key.keysym.unicode = txt[0];
				break;
			}
			case SDL_KEYDOWN:
			{
				EEEvent.Type = InputEvent::KeyDown;
				EEEvent.key.state = SDLEvent.key.state;
				EEEvent.key.which = SDLEvent.key.windowID;
				EEEvent.key.keysym.sym = KeyCodesTable[ SDLEvent.key.keysym.scancode ];
				EEEvent.key.keysym.mod = SDLEvent.key.keysym.mod;
				EEEvent.key.keysym.unicode = 0;
				break;
			}
			case SDL_KEYUP:
			{
				EEEvent.Type = InputEvent::KeyUp;
				EEEvent.key.state = SDLEvent.key.state;
				EEEvent.key.which = SDLEvent.key.windowID;
				EEEvent.key.keysym.sym = KeyCodesTable[ SDLEvent.key.keysym.scancode ];

				if ( SDLEvent.key.keysym.scancode == SDL_SCANCODE_1 ) {
					EEEvent.key.state = SDLEvent.key.state;
				}

				EEEvent.key.keysym.mod = SDLEvent.key.keysym.mod;
				EEEvent.key.keysym.unicode = 0;
				break;
			}
			case SDL_MOUSEMOTION:
			{
				EEEvent.Type = InputEvent::MouseMotion;
				EEEvent.motion.which = SDLEvent.motion.windowID;
				EEEvent.motion.state = SDLEvent.motion.state;
				EEEvent.motion.x = SDLEvent.motion.x;
				EEEvent.motion.y = SDLEvent.motion.y;
				EEEvent.motion.xrel = SDLEvent.motion.xrel;
				EEEvent.motion.yrel = SDLEvent.motion.yrel;
				break;
			}
			case SDL_MOUSEBUTTONDOWN:
			{
				EEEvent.Type = InputEvent::MouseButtonDown;
				EEEvent.button.button = SDLEvent.button.button;
				EEEvent.button.which = SDLEvent.button.windowID;
				EEEvent.button.state = SDLEvent.button.state;
				EEEvent.button.x = SDLEvent.button.x;
				EEEvent.button.y = SDLEvent.button.y;
				break;
			}
			case SDL_MOUSEBUTTONUP:
			{
				EEEvent.Type = InputEvent::MouseButtonUp;
				EEEvent.button.button = SDLEvent.button.button;
				EEEvent.button.which = SDLEvent.button.windowID;
				EEEvent.button.state = SDLEvent.button.state;
				EEEvent.button.x = SDLEvent.button.x;
				EEEvent.button.y = SDLEvent.button.y;
				break;
			}
			case SDL_MOUSEWHEEL:
			{
				Uint8 button;
				int x, y;

				if ( SDLEvent.wheel.y == 0 ) {
					break;
				}

				SDL_GetMouseState( &x, &y );

				if ( SDLEvent.wheel.y > 0 ) {
					button = EE_BUTTON_WHEELUP;
				} else {
					button = EE_BUTTON_WHEELDOWN;
				}

				EEEvent.button.button = button;
				EEEvent.button.x = x;
				EEEvent.button.y = y;
				EEEvent.button.which = SDLEvent.wheel.windowID;

				EEEvent.Type = InputEvent::MouseButtonDown;
				EEEvent.button.state = 1;
				ProcessEvent( &EEEvent );

				EEEvent.Type = InputEvent::MouseButtonUp;
				EEEvent.button.state = 0;
				break;
			}
			case SDL_FINGERMOTION:
			{
				EEEvent.Type = InputEvent::FingerMotion;
				EEEvent.finger.timestamp = SDLEvent.tfinger.timestamp;
				EEEvent.finger.touchId = SDLEvent.tfinger.touchId;
				EEEvent.finger.fingerId = SDLEvent.tfinger.fingerId;
				EEEvent.finger.x = SDLEvent.tfinger.x;
				EEEvent.finger.y = SDLEvent.tfinger.y;
				EEEvent.finger.dx = SDLEvent.tfinger.dx;
				EEEvent.finger.dy = SDLEvent.tfinger.dy;
				EEEvent.finger.pressure = SDLEvent.tfinger.pressure;
				break;
			}
			case SDL_FINGERDOWN:
			{
				EEEvent.Type = InputEvent::FingerDown;
				EEEvent.finger.timestamp = SDLEvent.tfinger.timestamp;
				EEEvent.finger.touchId = SDLEvent.tfinger.touchId;
				EEEvent.finger.fingerId = SDLEvent.tfinger.fingerId;
				EEEvent.finger.x = SDLEvent.tfinger.x;
				EEEvent.finger.y = SDLEvent.tfinger.y;
				EEEvent.finger.dx = SDLEvent.tfinger.dx;
				EEEvent.finger.dy = SDLEvent.tfinger.dy;
				EEEvent.finger.pressure = SDLEvent.tfinger.pressure;
				break;
			}
			case SDL_FINGERUP:
			{
				EEEvent.Type = InputEvent::FingerUp;
				EEEvent.finger.timestamp = SDLEvent.tfinger.timestamp;
				EEEvent.finger.touchId = SDLEvent.tfinger.touchId;
				EEEvent.finger.fingerId = SDLEvent.tfinger.fingerId;
				EEEvent.finger.x = SDLEvent.tfinger.x;
				EEEvent.finger.y = SDLEvent.tfinger.y;
				EEEvent.finger.dx = SDLEvent.tfinger.dx;
				EEEvent.finger.dy = SDLEvent.tfinger.dy;
				EEEvent.finger.pressure = SDLEvent.tfinger.pressure;
				break;
			}
			case SDL_JOYAXISMOTION:
			{
				EEEvent.Type = InputEvent::JoyAxisMotion;
				EEEvent.jaxis.which = SDLEvent.jaxis.which;
				EEEvent.jaxis.axis = SDLEvent.jaxis.axis;
				EEEvent.jaxis.value = SDLEvent.jaxis.value;
				break;
			}
			case SDL_JOYBALLMOTION:
			{
				EEEvent.Type = InputEvent::JoyBallMotion;
				EEEvent.jball.which = SDLEvent.jball.which;
				EEEvent.jball.ball = SDLEvent.jball.ball;
				EEEvent.jball.xrel = SDLEvent.jball.xrel;
				EEEvent.jball.yrel = SDLEvent.jball.yrel;
				break;
			}
			case SDL_JOYHATMOTION:
			{
				EEEvent.Type = InputEvent::JoyHatMotion;
				EEEvent.jhat.which = SDLEvent.jhat.which;
				EEEvent.jhat.value = SDLEvent.jhat.value;
				EEEvent.jhat.hat = SDLEvent.jhat.hat;
				break;
			}
			case SDL_JOYBUTTONDOWN:
			{
				EEEvent.Type = InputEvent::JoyButtonDown;
				EEEvent.jbutton.which = SDLEvent.jbutton.which;
				EEEvent.jbutton.state = SDLEvent.jbutton.state;
				EEEvent.jbutton.button = SDLEvent.jbutton.button;
				break;
			}
			case SDL_JOYBUTTONUP:
			{
				EEEvent.Type = InputEvent::JoyButtonUp;
				EEEvent.jbutton.which = SDLEvent.jbutton.which;
				EEEvent.jbutton.state = SDLEvent.jbutton.state;
				EEEvent.jbutton.button = SDLEvent.jbutton.button;
				break;
			}
			case SDL_QUIT:
			{
				EEEvent.Type = InputEvent::Quit;
				EEEvent.quit.type = EEEvent.Type;
				break;
			}
			case SDL_SYSWMEVENT:
			{
				EEEvent.Type = InputEvent::SysWM;
				EEEvent.syswm.msg = (InputEvent::SysWMmsg*)SDLEvent.syswm.msg;
				break;
			}
			default:
			{
				if ( SDLEvent.type >= SDL_USEREVENT && SDLEvent.type < SDL_LASTEVENT ) {
					EEEvent.Type = InputEvent::EventUser + SDLEvent.type - SDL_USEREVENT;
					EEEvent.user.type = EEEvent.Type;
					EEEvent.user.code = SDLEvent.user.code;
					EEEvent.user.data1 = SDLEvent.user.data1;
					EEEvent.user.data2 = SDLEvent.user.data2;
				} else {
					EEEvent.Type = InputEvent::NoEvent;
				}
			}
		}

		if ( InputEvent::NoEvent != EEEvent.Type ) {
			ProcessEvent( &EEEvent );
		}
	}
}

bool cInputSDL::GrabInput() {
	return ( SDL_GetWindowGrab( reinterpret_cast<cWindowSDL*> ( mWindow )->GetSDLWindow() ) == SDL_TRUE ) ? true : false;
}

void cInputSDL::GrabInput( const bool& Grab ) {
	SDL_SetWindowGrab( reinterpret_cast<cWindowSDL*> ( mWindow )->GetSDLWindow(), Grab ? SDL_TRUE : SDL_FALSE );
}

void cInputSDL::InjectMousePos( const Uint16& x, const Uint16& y ) {
	SDL_WarpMouseInWindow( reinterpret_cast<cWindowSDL*>( mWindow )->GetSDLWindow(), x, y );
}

void cInputSDL::Init() {
	eeVector2if mTempMouse;

	SDL_GetMouseState( &mTempMouse.x, &mTempMouse.y );

	mMousePos.x = (eeInt)mTempMouse.x;
	mMousePos.y = (eeInt)mTempMouse.y;

	InitializeTables();

	mJoystickManager->Open();
}

void cInputSDL::InitializeTables() {
	if ( KeyCodesTableInit )
		return;

	Uint32 i;

	memset( &KeyCodesTable[0], 0, SDL_NUM_SCANCODES );

	for ( i = SDL_SCANCODE_A; i <= SDL_SCANCODE_Z; i++ )
		KeyCodesTable[ i ] = KEY_A + i - SDL_SCANCODE_A;

	KeyCodesTable[ SDL_SCANCODE_0 ] = KEY_0;
	KeyCodesTable[ SDL_SCANCODE_1 ] = KEY_1;
	KeyCodesTable[ SDL_SCANCODE_2 ] = KEY_2;
	KeyCodesTable[ SDL_SCANCODE_3 ] = KEY_3;
	KeyCodesTable[ SDL_SCANCODE_4 ] = KEY_4;
	KeyCodesTable[ SDL_SCANCODE_5 ] = KEY_5;
	KeyCodesTable[ SDL_SCANCODE_6 ] = KEY_6;
	KeyCodesTable[ SDL_SCANCODE_7 ] = KEY_7;
	KeyCodesTable[ SDL_SCANCODE_8 ] = KEY_8;
	KeyCodesTable[ SDL_SCANCODE_9 ] = KEY_9;
	KeyCodesTable[ SDL_SCANCODE_KP_0 ] = KEY_KP0;
	KeyCodesTable[ SDL_SCANCODE_KP_1 ] = KEY_KP1;
	KeyCodesTable[ SDL_SCANCODE_KP_2 ] = KEY_KP2;
	KeyCodesTable[ SDL_SCANCODE_KP_3 ] = KEY_KP3;
	KeyCodesTable[ SDL_SCANCODE_KP_4 ] = KEY_KP4;
	KeyCodesTable[ SDL_SCANCODE_KP_5 ] = KEY_KP5;
	KeyCodesTable[ SDL_SCANCODE_KP_6 ] = KEY_KP6;
	KeyCodesTable[ SDL_SCANCODE_KP_7 ] = KEY_KP7;
	KeyCodesTable[ SDL_SCANCODE_KP_8 ] = KEY_KP8;
	KeyCodesTable[ SDL_SCANCODE_KP_9 ] = KEY_KP9;
	KeyCodesTable[ SDL_SCANCODE_F1 ] = KEY_F1;
	KeyCodesTable[ SDL_SCANCODE_F2 ] = KEY_F2;
	KeyCodesTable[ SDL_SCANCODE_F3 ] = KEY_F3;
	KeyCodesTable[ SDL_SCANCODE_F4 ] = KEY_F4;
	KeyCodesTable[ SDL_SCANCODE_F5 ] = KEY_F5;
	KeyCodesTable[ SDL_SCANCODE_F6 ] = KEY_F6;
	KeyCodesTable[ SDL_SCANCODE_F7 ] = KEY_F7;
	KeyCodesTable[ SDL_SCANCODE_F8 ] = KEY_F8;
	KeyCodesTable[ SDL_SCANCODE_F9 ] = KEY_F9;
	KeyCodesTable[ SDL_SCANCODE_F10 ] = KEY_F10;
	KeyCodesTable[ SDL_SCANCODE_F11 ] = KEY_F11;
	KeyCodesTable[ SDL_SCANCODE_F12 ] = KEY_F12;
	KeyCodesTable[ SDL_SCANCODE_ESCAPE ] = KEY_ESCAPE;
	KeyCodesTable[ SDL_SCANCODE_MINUS ] = KEY_MINUS;
	KeyCodesTable[ SDL_SCANCODE_EQUALS ] = KEY_EQUALS;
	KeyCodesTable[ SDL_SCANCODE_BACKSPACE ] = KEY_BACKSPACE;
	KeyCodesTable[ SDL_SCANCODE_TAB ] = KEY_TAB;
	KeyCodesTable[ SDL_SCANCODE_RETURN ] = KEY_RETURN;
	KeyCodesTable[ SDL_SCANCODE_SEMICOLON ] = KEY_SEMICOLON;
	KeyCodesTable[ SDL_SCANCODE_BACKSLASH ] = KEY_BACKSLASH;
	KeyCodesTable[ SDL_SCANCODE_COMMA ] = KEY_COMMA;
	KeyCodesTable[ SDL_SCANCODE_SLASH ] = KEY_SLASH;
	KeyCodesTable[ SDL_SCANCODE_KP_SPACE ] = KEY_SPACE;
	KeyCodesTable[ SDL_SCANCODE_INSERT] = KEY_INSERT;
	KeyCodesTable[ SDL_SCANCODE_DELETE ] = KEY_DELETE;
	KeyCodesTable[ SDL_SCANCODE_HOME ] = KEY_HOME;
	KeyCodesTable[ SDL_SCANCODE_END ] = KEY_END;
	KeyCodesTable[ SDL_SCANCODE_PAGEUP ] = KEY_PAGEUP;
	KeyCodesTable[ SDL_SCANCODE_PAGEDOWN ] = KEY_PAGEDOWN;
	KeyCodesTable[ SDL_SCANCODE_LEFT ] = KEY_LEFT;
	KeyCodesTable[ SDL_SCANCODE_RIGHT ] = KEY_RIGHT;
	KeyCodesTable[ SDL_SCANCODE_UP ] = KEY_UP;
	KeyCodesTable[ SDL_SCANCODE_DOWN ] = KEY_DOWN;
	KeyCodesTable[ SDL_SCANCODE_KP_DIVIDE ] = KEY_KP_DIVIDE;
	KeyCodesTable[ SDL_SCANCODE_KP_MULTIPLY ] = KEY_KP_MULTIPLY;
	KeyCodesTable[ SDL_SCANCODE_KP_MINUS ] = KEY_KP_MINUS;
	KeyCodesTable[ SDL_SCANCODE_KP_PLUS ] = KEY_KP_PLUS;
	KeyCodesTable[ SDL_SCANCODE_KP_ENTER ] = KEY_KP_ENTER;
	KeyCodesTable[ SDL_SCANCODE_PRINTSCREEN ] = KEY_PRINT;
	KeyCodesTable[ SDL_SCANCODE_PAUSE ] = KEY_PAUSE;
	KeyCodesTable[ SDL_SCANCODE_KP_EQUALS ] = KEY_KP_EQUALS;
	KeyCodesTable[ SDL_SCANCODE_LSHIFT ] = KEY_LSHIFT;
	KeyCodesTable[ SDL_SCANCODE_RSHIFT ] = KEY_RSHIFT;
	KeyCodesTable[ SDL_SCANCODE_LCTRL ] = KEY_LCTRL;
	KeyCodesTable[ SDL_SCANCODE_RCTRL ] = KEY_RCTRL;
	KeyCodesTable[ SDL_SCANCODE_LALT ] = KEY_LALT;
	KeyCodesTable[ SDL_SCANCODE_RALT ] = KEY_RALT;
	KeyCodesTable[ SDL_SCANCODE_MODE ] = KEY_MODE;
	KeyCodesTable[ SDL_SCANCODE_LGUI ] = KEY_LSUPER;
	KeyCodesTable[ SDL_SCANCODE_RGUI ] = KEY_RSUPER;
	KeyCodesTable[ SDL_SCANCODE_SCROLLLOCK ] = KEY_SCROLLOCK;
	KeyCodesTable[ SDL_SCANCODE_NUMLOCKCLEAR ] = KEY_NUMLOCK;
	KeyCodesTable[ SDL_SCANCODE_CAPSLOCK ] = KEY_CAPSLOCK;

	KeyCodesTableInit = true;
}

}}}}

#endif
