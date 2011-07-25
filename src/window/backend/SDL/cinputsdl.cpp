#include "cinputsdl.hpp"
#include "cjoystickmanagersdl.hpp"
#include "ccursormanagersdl.hpp"

#ifdef EE_BACKEND_SDL_1_2

namespace EE { namespace Window { namespace Backend { namespace SDL {

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
			case SDL_ACTIVEEVENT:
			{
				EEEvent.Type = InputEvent::Active;
				EEEvent.active.gain = SDLEvent.active.gain;
				EEEvent.active.state = SDLEvent.active.state;
				break;
			}
			case SDL_KEYDOWN:
			{
				EEEvent.Type = InputEvent::KeyDown;
				EEEvent.key.state = SDLEvent.key.state;
				EEEvent.key.which = SDLEvent.key.which;
				EEEvent.key.keysym.sym = SDLEvent.key.keysym.sym;
				EEEvent.key.keysym.mod = SDLEvent.key.keysym.mod;
				EEEvent.key.keysym.unicode = SDLEvent.key.keysym.unicode;
				break;
			}
			case SDL_KEYUP:
			{
				EEEvent.Type = InputEvent::KeyUp;
				EEEvent.key.state = SDLEvent.key.state;
				EEEvent.key.which = SDLEvent.key.which;
				EEEvent.key.keysym.sym = SDLEvent.key.keysym.sym;
				EEEvent.key.keysym.mod = SDLEvent.key.keysym.mod;
				EEEvent.key.keysym.unicode = SDLEvent.key.keysym.unicode;
				break;
			}
			case SDL_MOUSEMOTION:
			{
				EEEvent.Type = InputEvent::MouseMotion;
				EEEvent.motion.which = SDLEvent.motion.which;
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
				EEEvent.button.which = SDLEvent.button.which;
				EEEvent.button.state = SDLEvent.button.state;
				EEEvent.button.x = SDLEvent.button.x;
				EEEvent.button.y = SDLEvent.button.y;
				break;
			}
			case SDL_MOUSEBUTTONUP:
			{
				EEEvent.Type = InputEvent::MouseButtonUp;
				EEEvent.button.button = SDLEvent.button.button;
				EEEvent.button.which = SDLEvent.button.which;
				EEEvent.button.state = SDLEvent.button.state;
				EEEvent.button.x = SDLEvent.button.x;
				EEEvent.button.y = SDLEvent.button.y;
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
			case SDL_VIDEORESIZE:
			{
				EEEvent.Type = InputEvent::VideoResize;
				EEEvent.resize.w = SDLEvent.resize.w;
				EEEvent.resize.h = SDLEvent.resize.h;
				break;
			}
			case SDL_VIDEOEXPOSE:
			{
				EEEvent.Type = InputEvent::VideoExpose;
				EEEvent.expose.type = EEEvent.Type;
				break;
			}
			default:
			{
				if ( SDLEvent.type >= SDL_USEREVENT && SDLEvent.type < SDL_NUMEVENTS ) {
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
	return ( SDL_WM_GrabInput( SDL_GRAB_QUERY ) == SDL_GRAB_ON ) ? true : false;
}

void cInputSDL::GrabInput( const bool& Grab ) {
	if ( Grab )
		SDL_WM_GrabInput(SDL_GRAB_ON);
	else
		SDL_WM_GrabInput(SDL_GRAB_OFF);
}

void cInputSDL::InjectMousePos( const Uint16& x, const Uint16& y ) {
	SDL_WarpMouse( x, y );
}

void cInputSDL::Init() {
	eeVector2if mTempMouse;
	SDL_GetMouseState( &mTempMouse.x, &mTempMouse.y );
	mMousePos.x = (eeInt)mTempMouse.x;
	mMousePos.y = (eeInt)mTempMouse.y;

	SDL_EnableUNICODE(1);

	SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);

	mJoystickManager->Open();
}

}}}}

#endif
