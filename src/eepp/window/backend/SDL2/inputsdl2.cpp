#include <eepp/window/backend/SDL2/base.hpp>

#ifdef EE_BACKEND_SDL2

#include <eepp/window/backend/SDL2/cursormanagersdl2.hpp>
#include <eepp/window/backend/SDL2/inputsdl2.hpp>
#include <eepp/window/backend/SDL2/joystickmanagersdl2.hpp>
#include <eepp/window/backend/SDL2/windowsdl2.hpp>

namespace EE { namespace Window { namespace Backend { namespace SDL2 {

InputSDL::InputSDL( EE::Window::Window* window ) :
	Input( window, eeNew( JoystickManagerSDL, () ) ), mDPIScale( 1.f ) {
#if defined( EE_X11_PLATFORM )
	mMouseSpeed = 1.75f;
#endif
}

InputSDL::~InputSDL() {}

void InputSDL::update() {
	SDL_Event SDLEvent;
	cleanStates();
	if ( !mQueuedEvents.empty() ) {
		for ( auto prevEvent : mQueuedEvents )
			sendEvent( prevEvent );
		mQueuedEvents.clear();
	}
	while ( SDL_PollEvent( &SDLEvent ) )
		sendEvent( SDLEvent );
	InputEvent endProcessingEvent;
	endProcessingEvent.Type = InputEvent::EventsSent;
	processEvent( &endProcessingEvent );
}

void InputSDL::waitEvent( const Time& timeout ) {
	SDL_Event SDLEvent;
	if ( timeout == Time::Zero ) {
		if ( SDL_WaitEvent( &SDLEvent ) )
			mQueuedEvents.emplace_back( SDLEvent );
	} else if ( SDL_WaitEventTimeout( &SDLEvent, (int)timeout.asMilliseconds() ) )
		mQueuedEvents.emplace_back( SDLEvent );
}

bool InputSDL::grabInput() {
	return ( SDL_GetWindowGrab( static_cast<WindowSDL*>( mWindow )->GetSDLWindow() ) == SDL_TRUE )
			   ? true
			   : false;
}

void InputSDL::grabInput( const bool& Grab ) {
	SDL_SetWindowGrab( static_cast<WindowSDL*>( mWindow )->GetSDLWindow(),
					   Grab ? SDL_TRUE : SDL_FALSE );
}

void InputSDL::injectMousePos( const Uint16& x, const Uint16& y ) {
	SDL_WarpMouseInWindow( reinterpret_cast<WindowSDL*>( mWindow )->GetSDLWindow(), x, y );
}

Vector2i InputSDL::queryMousePos() {
	Vector2i mousePos;
	Vector2i tempMouse;
	Vector2i tempWinPos;
	Rect bordersSize;
	SDL_Window* sdlw = reinterpret_cast<WindowSDL*>( mWindow )->GetSDLWindow();
	SDL_GetGlobalMouseState( &tempMouse.x, &tempMouse.y );
	SDL_GetWindowPosition( sdlw, &tempWinPos.x, &tempWinPos.y );
	// Since an unknown version the window position includes the margin from the border size.
	// So we don't need to compute that manually.
	/*SDL_GetWindowBordersSize( sdlw, &bordersSize.Top, &bordersSize.Left, &bordersSize.Bottom,
							  &bordersSize.Right );*/
	mousePos.x = (int)tempMouse.x - tempWinPos.x /* - bordersSize.Left*/;
	mousePos.y = (int)tempMouse.y - tempWinPos.y /* - bordersSize.Top*/;
	return mousePos;
}

void InputSDL::captureMouse( const bool& capture ) {
	SDL_CaptureMouse( capture ? SDL_TRUE : SDL_FALSE );
}

bool InputSDL::isMouseCaptured() const {
	return SDL_GetWindowFlags( reinterpret_cast<WindowSDL*>( mWindow )->GetSDLWindow() ) &
		   SDL_WINDOW_MOUSE_CAPTURE;
}

std::string InputSDL::getKeyName( const Keycode& keyCode ) const {
	return std::string( SDL_GetKeyName( keyCode ) );
}

Keycode InputSDL::getKeyFromName( const std::string& keycode ) const {
	return (Keycode)SDL_GetKeyFromName( keycode.c_str() );
}

std::string InputSDL::getScancodeName( const Scancode& scancode ) const {
	return SDL_GetScancodeName( (SDL_Scancode)scancode );
}

Scancode InputSDL::getScancodeFromName( const std::string& scancode ) const {
	return (Scancode)SDL_GetScancodeFromName( scancode.c_str() );
}

Keycode InputSDL::getKeyFromScancode( const Scancode& scancode ) const {
	return (Keycode)SDL_GetKeyFromScancode( (SDL_Scancode)scancode );
}

Scancode InputSDL::getScancodeFromKey( const Keycode& scancode ) const {
	return (Scancode)SDL_GetScancodeFromKey( (SDL_Keycode)scancode );
}

void InputSDL::init() {
	mDPIScale = mWindow->getScale();
	mMousePos = queryMousePos();
#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN
	mJoystickManager->open();
#endif
}

void InputSDL::sendEvent( const SDL_Event& SDLEvent ) {
	InputEvent event;
	switch ( SDLEvent.type ) {
		case SDL_WINDOWEVENT: {
			switch ( SDLEvent.window.event ) {
				case SDL_WINDOWEVENT_RESIZED: {
					event.Type = InputEvent::VideoResize;
					event.resize.w = SDLEvent.window.data1 * mDPIScale;
					event.resize.h = SDLEvent.window.data2 * mDPIScale;
					break;
				}
				case SDL_WINDOWEVENT_HIT_TEST: {
					event.Type = InputEvent::Window;
					event.window.gain = 1;
					event.window.type = InputEvent::WindowHitTest;
					break;
				}
				case SDL_WINDOWEVENT_TAKE_FOCUS: {
					event.Type = InputEvent::VideoExpose;
					event.expose.type = event.Type;
					break;
				}
				case SDL_WINDOWEVENT_CLOSE: {
					event.Type = InputEvent::Window;
					event.window.gain = 0;
					event.window.type = InputEvent::WindowClose;
					break;
				}
				case SDL_WINDOWEVENT_SIZE_CHANGED: {
					event.Type = InputEvent::Window;
					event.window.gain = 1;
					event.window.type = InputEvent::WindowSizeChanged;
					break;
				}
				case SDL_WINDOWEVENT_MAXIMIZED: {
					event.Type = InputEvent::Window;
					event.window.gain = 1;
					event.window.type = InputEvent::WindowMaximized;
					break;
				}
				case SDL_WINDOWEVENT_EXPOSED: {
					event.Type = InputEvent::VideoExpose;
					event.expose.type = event.Type;
					break;
				}
				case SDL_WINDOWEVENT_MOVED: {
					event.Type = InputEvent::Window;
					event.window.gain = 1;
					event.window.type = InputEvent::WindowMoved;
					break;
				}
				case SDL_WINDOWEVENT_SHOWN: {
					event.Type = InputEvent::Window;
					event.window.gain = 1;
					event.window.type = InputEvent::WindowShown;
					break;
				}
				case SDL_WINDOWEVENT_HIDDEN: {
					event.Type = InputEvent::Window;
					event.window.gain = 0;
					event.window.type = InputEvent::WindowHidden;
					break;
				}
				case SDL_WINDOWEVENT_MINIMIZED: {
					event.Type = InputEvent::Window;
					event.window.gain = 0;
					event.window.type = InputEvent::WindowMinimized;
					break;
				}
				case SDL_WINDOWEVENT_RESTORED: {
					event.Type = InputEvent::Window;
					event.window.gain = 1;
					event.window.type = InputEvent::WindowRestored;
					break;
				}
				case SDL_WINDOWEVENT_ENTER: {
					event.Type = InputEvent::Window;
					event.window.gain = 1;
					event.window.type = InputEvent::WindowMouseEnter;
					break;
				}
				case SDL_WINDOWEVENT_LEAVE: {
					event.Type = InputEvent::Window;
					event.window.gain = 0;
					event.window.type = InputEvent::WindowMouseLeave;
					break;
				}
				case SDL_WINDOWEVENT_FOCUS_GAINED: {
					event.Type = InputEvent::Window;
					event.window.gain = 1;
					event.window.type = InputEvent::WindowKeyboardFocusGain;
					break;
				}
				case SDL_WINDOWEVENT_FOCUS_LOST: {
					event.Type = InputEvent::Window;
					event.window.gain = 0;
					event.window.type = InputEvent::WindowKeyboardFocusLost;
					break;
				}
			}

			break;
		}
		case SDL_TEXTINPUT: {
			String txt = String::fromUtf8( SDLEvent.text.text );
			event.Type = InputEvent::TextInput;
			event.text.timestamp = SDLEvent.text.timestamp;
			event.text.text = txt[0];
			break;
		}
		case SDL_KEYDOWN: {
			event.Type = InputEvent::KeyDown;
			event.key.state = SDLEvent.key.state;
			event.key.which = SDLEvent.key.windowID;
			event.key.keysym.sym = (Keycode)SDLEvent.key.keysym.sym;
			event.key.keysym.scancode = (Scancode)SDLEvent.key.keysym.scancode;
			event.key.keysym.mod = SDLEvent.key.keysym.mod;
			event.key.keysym.unicode = 0;
			break;
		}
		case SDL_KEYUP: {
			event.Type = InputEvent::KeyUp;
			event.key.state = SDLEvent.key.state;
			event.key.which = SDLEvent.key.windowID;
			event.key.keysym.sym = (Keycode)SDLEvent.key.keysym.sym;
			event.key.keysym.scancode = (Scancode)SDLEvent.key.keysym.scancode;
			event.key.keysym.mod = SDLEvent.key.keysym.mod;
			event.key.keysym.unicode = 0;
			break;
		}
		case SDL_MOUSEMOTION: {
			event.Type = InputEvent::MouseMotion;
			event.motion.which = SDLEvent.motion.windowID;
			event.motion.state = SDLEvent.motion.state;
			event.motion.x = SDLEvent.motion.x * mDPIScale;
			event.motion.y = SDLEvent.motion.y * mDPIScale;
			event.motion.xrel = SDLEvent.motion.xrel * mDPIScale;
			event.motion.yrel = SDLEvent.motion.yrel * mDPIScale;
			break;
		}
		case SDL_MOUSEBUTTONDOWN: {
			event.Type = InputEvent::MouseButtonDown;
			event.button.button = SDLEvent.button.button;
			event.button.which = SDLEvent.button.windowID;
			event.button.state = SDLEvent.button.state;
			event.button.x = SDLEvent.button.x;
			event.button.y = SDLEvent.button.y;
			break;
		}
		case SDL_MOUSEBUTTONUP: {
			event.Type = InputEvent::MouseButtonUp;
			event.button.button = SDLEvent.button.button;
			event.button.which = SDLEvent.button.windowID;
			event.button.state = SDLEvent.button.state;
			event.button.x = SDLEvent.button.x;
			event.button.y = SDLEvent.button.y;
			break;
		}
		case SDL_MOUSEWHEEL: {
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

			event.button.button = button;
			event.button.x = x;
			event.button.y = y;
			event.button.which = SDLEvent.wheel.windowID;

			event.Type = InputEvent::MouseButtonDown;
			event.button.state = 1;
			processEvent( &event );

			event.Type = InputEvent::MouseButtonUp;
			event.button.state = 0;
			break;
		}
		case SDL_FINGERMOTION: {
			event.Type = InputEvent::FingerMotion;
			event.finger.timestamp = SDLEvent.tfinger.timestamp;
			event.finger.touchId = SDLEvent.tfinger.touchId;
			event.finger.fingerId = SDLEvent.tfinger.fingerId;
			event.finger.x = SDLEvent.tfinger.x;
			event.finger.y = SDLEvent.tfinger.y;
			event.finger.dx = SDLEvent.tfinger.dx;
			event.finger.dy = SDLEvent.tfinger.dy;
			event.finger.pressure = SDLEvent.tfinger.pressure;
			break;
		}
		case SDL_FINGERDOWN: {
			event.Type = InputEvent::FingerDown;
			event.finger.timestamp = SDLEvent.tfinger.timestamp;
			event.finger.touchId = SDLEvent.tfinger.touchId;
			event.finger.fingerId = SDLEvent.tfinger.fingerId;
			event.finger.x = SDLEvent.tfinger.x;
			event.finger.y = SDLEvent.tfinger.y;
			event.finger.dx = SDLEvent.tfinger.dx;
			event.finger.dy = SDLEvent.tfinger.dy;
			event.finger.pressure = SDLEvent.tfinger.pressure;
			break;
		}
		case SDL_FINGERUP: {
			event.Type = InputEvent::FingerUp;
			event.finger.timestamp = SDLEvent.tfinger.timestamp;
			event.finger.touchId = SDLEvent.tfinger.touchId;
			event.finger.fingerId = SDLEvent.tfinger.fingerId;
			event.finger.x = SDLEvent.tfinger.x;
			event.finger.y = SDLEvent.tfinger.y;
			event.finger.dx = SDLEvent.tfinger.dx;
			event.finger.dy = SDLEvent.tfinger.dy;
			event.finger.pressure = SDLEvent.tfinger.pressure;
			break;
		}
		case SDL_JOYAXISMOTION: {
			event.Type = InputEvent::JoyAxisMotion;
			event.jaxis.which = SDLEvent.jaxis.which;
			event.jaxis.axis = SDLEvent.jaxis.axis;
			event.jaxis.value = SDLEvent.jaxis.value;
			break;
		}
		case SDL_JOYBALLMOTION: {
			event.Type = InputEvent::JoyBallMotion;
			event.jball.which = SDLEvent.jball.which;
			event.jball.ball = SDLEvent.jball.ball;
			event.jball.xrel = SDLEvent.jball.xrel;
			event.jball.yrel = SDLEvent.jball.yrel;
			break;
		}
		case SDL_JOYHATMOTION: {
			event.Type = InputEvent::JoyHatMotion;
			event.jhat.which = SDLEvent.jhat.which;
			event.jhat.value = SDLEvent.jhat.value;
			event.jhat.hat = SDLEvent.jhat.hat;
			break;
		}
		case SDL_JOYBUTTONDOWN: {
			event.Type = InputEvent::JoyButtonDown;
			event.jbutton.which = SDLEvent.jbutton.which;
			event.jbutton.state = SDLEvent.jbutton.state;
			event.jbutton.button = SDLEvent.jbutton.button;
			break;
		}
		case SDL_JOYBUTTONUP: {
			event.Type = InputEvent::JoyButtonUp;
			event.jbutton.which = SDLEvent.jbutton.which;
			event.jbutton.state = SDLEvent.jbutton.state;
			event.jbutton.button = SDLEvent.jbutton.button;
			break;
		}
		case SDL_QUIT: {
			event.Type = InputEvent::Quit;
			event.quit.type = event.Type;
			break;
		}
		case SDL_SYSWMEVENT: {
			event.Type = InputEvent::SysWM;
			event.syswm.msg = (InputEvent::SysWMmsg*)SDLEvent.syswm.msg;
			break;
		}
		case SDL_DROPFILE: {
			event.Type = InputEvent::FileDropped;
			event.file.file = SDLEvent.drop.file;
			break;
		}
		case SDL_DROPTEXT: {
			event.Type = InputEvent::TextDropped;
			event.textdrop.text = SDLEvent.drop.file;
			break;
		}
		default: {
			if ( SDLEvent.type >= SDL_USEREVENT && SDLEvent.type < SDL_LASTEVENT ) {
				event.Type = InputEvent::EventUser + SDLEvent.type - SDL_USEREVENT;
				event.user.type = event.Type;
				event.user.code = SDLEvent.user.code;
				event.user.data1 = SDLEvent.user.data1;
				event.user.data2 = SDLEvent.user.data2;
			} else {
				event.Type = InputEvent::NoEvent;
			}
		}
	}
	if ( InputEvent::NoEvent != event.Type )
		processEvent( &event );
	if ( InputEvent::FileDropped == event.Type || InputEvent::TextDropped == event.Type )
		SDL_free( SDLEvent.drop.file );
}

}}}} // namespace EE::Window::Backend::SDL2

#endif
