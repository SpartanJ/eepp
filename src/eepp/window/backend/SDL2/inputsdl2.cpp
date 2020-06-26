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
	InputEvent EEEvent;

	cleanStates();

	while ( SDL_PollEvent( &SDLEvent ) ) {
		switch ( SDLEvent.type ) {
			case SDL_WINDOWEVENT: {
				switch ( SDLEvent.window.event ) {
					case SDL_WINDOWEVENT_RESIZED: {
						EEEvent.Type = InputEvent::VideoResize;
						EEEvent.resize.w = SDLEvent.window.data1 * mDPIScale;
						EEEvent.resize.h = SDLEvent.window.data2 * mDPIScale;
						break;
					}
					case SDL_WINDOWEVENT_HIT_TEST: {
						EEEvent.Type = InputEvent::Window;
						EEEvent.window.gain = 1;
						EEEvent.window.type = InputEvent::WindowHitTest;
						break;
					}
					case SDL_WINDOWEVENT_TAKE_FOCUS: {
						EEEvent.Type = InputEvent::VideoExpose;
						EEEvent.expose.type = EEEvent.Type;
						break;
					}
					case SDL_WINDOWEVENT_CLOSE: {
						EEEvent.Type = InputEvent::Window;
						EEEvent.window.gain = 0;
						EEEvent.window.type = InputEvent::WindowClose;
						break;
					}
					case SDL_WINDOWEVENT_SIZE_CHANGED: {
						EEEvent.Type = InputEvent::Window;
						EEEvent.window.gain = 1;
						EEEvent.window.type = InputEvent::WindowSizeChanged;
						break;
					}
					case SDL_WINDOWEVENT_MAXIMIZED: {
						EEEvent.Type = InputEvent::Window;
						EEEvent.window.gain = 1;
						EEEvent.window.type = InputEvent::WindowMaximized;
						break;
					}
					case SDL_WINDOWEVENT_EXPOSED: {
						EEEvent.Type = InputEvent::VideoExpose;
						EEEvent.expose.type = EEEvent.Type;
						break;
					}
					case SDL_WINDOWEVENT_MOVED: {
						EEEvent.Type = InputEvent::Window;
						EEEvent.window.gain = 1;
						EEEvent.window.type = InputEvent::WindowMoved;
						break;
					}
					case SDL_WINDOWEVENT_SHOWN: {
						EEEvent.Type = InputEvent::Window;
						EEEvent.window.gain = 1;
						EEEvent.window.type = InputEvent::WindowShown;
						break;
					}
					case SDL_WINDOWEVENT_HIDDEN: {
						EEEvent.Type = InputEvent::Window;
						EEEvent.window.gain = 0;
						EEEvent.window.type = InputEvent::WindowHidden;
						break;
					}
					case SDL_WINDOWEVENT_MINIMIZED: {
						EEEvent.Type = InputEvent::Window;
						EEEvent.window.gain = 0;
						EEEvent.window.type = InputEvent::WindowMinimized;
						break;
					}
					case SDL_WINDOWEVENT_RESTORED: {
						EEEvent.Type = InputEvent::Window;
						EEEvent.window.gain = 1;
						EEEvent.window.type = InputEvent::WindowRestored;
						break;
					}
					case SDL_WINDOWEVENT_ENTER: {
						EEEvent.Type = InputEvent::Window;
						EEEvent.window.gain = 1;
						EEEvent.window.type = InputEvent::WindowMouseEnter;
						break;
					}
					case SDL_WINDOWEVENT_LEAVE: {
						EEEvent.Type = InputEvent::Window;
						EEEvent.window.gain = 0;
						EEEvent.window.type = InputEvent::WindowMouseLeave;
						break;
					}
					case SDL_WINDOWEVENT_FOCUS_GAINED: {
						EEEvent.Type = InputEvent::Window;
						EEEvent.window.gain = 1;
						EEEvent.window.type = InputEvent::WindowKeyboardFocusGain;
						break;
					}
					case SDL_WINDOWEVENT_FOCUS_LOST: {
						EEEvent.Type = InputEvent::Window;
						EEEvent.window.gain = 0;
						EEEvent.window.type = InputEvent::WindowKeyboardFocusLost;
						break;
					}
				}

				break;
			}
			case SDL_TEXTINPUT: {
				String txt = String::fromUtf8( SDLEvent.text.text );
				EEEvent.Type = InputEvent::TextInput;
				EEEvent.text.timestamp = SDLEvent.text.timestamp;
				EEEvent.text.text = txt[0];
				break;
			}
			case SDL_KEYDOWN: {
				EEEvent.Type = InputEvent::KeyDown;
				EEEvent.key.state = SDLEvent.key.state;
				EEEvent.key.which = SDLEvent.key.windowID;
				EEEvent.key.keysym.sym = (Keycode)SDLEvent.key.keysym.sym;
				EEEvent.key.keysym.scancode = (Scancode)SDLEvent.key.keysym.scancode;
				EEEvent.key.keysym.mod = SDLEvent.key.keysym.mod;
				EEEvent.key.keysym.unicode = 0;
				break;
			}
			case SDL_KEYUP: {
				EEEvent.Type = InputEvent::KeyUp;
				EEEvent.key.state = SDLEvent.key.state;
				EEEvent.key.which = SDLEvent.key.windowID;
				EEEvent.key.keysym.sym = (Keycode)SDLEvent.key.keysym.sym;
				EEEvent.key.keysym.scancode = (Scancode)SDLEvent.key.keysym.scancode;
				EEEvent.key.keysym.mod = SDLEvent.key.keysym.mod;
				EEEvent.key.keysym.unicode = 0;
				break;
			}
			case SDL_MOUSEMOTION: {
				EEEvent.Type = InputEvent::MouseMotion;
				EEEvent.motion.which = SDLEvent.motion.windowID;
				EEEvent.motion.state = SDLEvent.motion.state;
				EEEvent.motion.x = SDLEvent.motion.x * mDPIScale;
				EEEvent.motion.y = SDLEvent.motion.y * mDPIScale;
				EEEvent.motion.xrel = SDLEvent.motion.xrel * mDPIScale;
				EEEvent.motion.yrel = SDLEvent.motion.yrel * mDPIScale;
				break;
			}
			case SDL_MOUSEBUTTONDOWN: {
				EEEvent.Type = InputEvent::MouseButtonDown;
				EEEvent.button.button = SDLEvent.button.button;
				EEEvent.button.which = SDLEvent.button.windowID;
				EEEvent.button.state = SDLEvent.button.state;
				EEEvent.button.x = SDLEvent.button.x;
				EEEvent.button.y = SDLEvent.button.y;
				break;
			}
			case SDL_MOUSEBUTTONUP: {
				EEEvent.Type = InputEvent::MouseButtonUp;
				EEEvent.button.button = SDLEvent.button.button;
				EEEvent.button.which = SDLEvent.button.windowID;
				EEEvent.button.state = SDLEvent.button.state;
				EEEvent.button.x = SDLEvent.button.x;
				EEEvent.button.y = SDLEvent.button.y;
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

				EEEvent.button.button = button;
				EEEvent.button.x = x;
				EEEvent.button.y = y;
				EEEvent.button.which = SDLEvent.wheel.windowID;

				EEEvent.Type = InputEvent::MouseButtonDown;
				EEEvent.button.state = 1;
				processEvent( &EEEvent );

				EEEvent.Type = InputEvent::MouseButtonUp;
				EEEvent.button.state = 0;
				break;
			}
			case SDL_FINGERMOTION: {
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
			case SDL_FINGERDOWN: {
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
			case SDL_FINGERUP: {
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
			case SDL_JOYAXISMOTION: {
				EEEvent.Type = InputEvent::JoyAxisMotion;
				EEEvent.jaxis.which = SDLEvent.jaxis.which;
				EEEvent.jaxis.axis = SDLEvent.jaxis.axis;
				EEEvent.jaxis.value = SDLEvent.jaxis.value;
				break;
			}
			case SDL_JOYBALLMOTION: {
				EEEvent.Type = InputEvent::JoyBallMotion;
				EEEvent.jball.which = SDLEvent.jball.which;
				EEEvent.jball.ball = SDLEvent.jball.ball;
				EEEvent.jball.xrel = SDLEvent.jball.xrel;
				EEEvent.jball.yrel = SDLEvent.jball.yrel;
				break;
			}
			case SDL_JOYHATMOTION: {
				EEEvent.Type = InputEvent::JoyHatMotion;
				EEEvent.jhat.which = SDLEvent.jhat.which;
				EEEvent.jhat.value = SDLEvent.jhat.value;
				EEEvent.jhat.hat = SDLEvent.jhat.hat;
				break;
			}
			case SDL_JOYBUTTONDOWN: {
				EEEvent.Type = InputEvent::JoyButtonDown;
				EEEvent.jbutton.which = SDLEvent.jbutton.which;
				EEEvent.jbutton.state = SDLEvent.jbutton.state;
				EEEvent.jbutton.button = SDLEvent.jbutton.button;
				break;
			}
			case SDL_JOYBUTTONUP: {
				EEEvent.Type = InputEvent::JoyButtonUp;
				EEEvent.jbutton.which = SDLEvent.jbutton.which;
				EEEvent.jbutton.state = SDLEvent.jbutton.state;
				EEEvent.jbutton.button = SDLEvent.jbutton.button;
				break;
			}
			case SDL_QUIT: {
				EEEvent.Type = InputEvent::Quit;
				EEEvent.quit.type = EEEvent.Type;
				break;
			}
			case SDL_SYSWMEVENT: {
				EEEvent.Type = InputEvent::SysWM;
				EEEvent.syswm.msg = (InputEvent::SysWMmsg*)SDLEvent.syswm.msg;
				break;
			}
			case SDL_DROPFILE: {
				EEEvent.Type = InputEvent::FileDropped;
				EEEvent.file.file = SDLEvent.drop.file;
				break;
			}
			case SDL_DROPTEXT: {
				EEEvent.Type = InputEvent::TextDropped;
				EEEvent.textdrop.text = SDLEvent.drop.file;
				break;
			}
			default: {
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
			processEvent( &EEEvent );
		}

		if ( InputEvent::FileDropped == EEEvent.Type || InputEvent::TextDropped == EEEvent.Type ) {
			SDL_free( SDLEvent.drop.file );
		}
	}

	InputEvent endProcessingEvent;
	endProcessingEvent.Type = InputEvent::EventsSent;
	processEvent( &endProcessingEvent );
}

bool InputSDL::grabInput() {
	return ( SDL_GetWindowGrab( reinterpret_cast<WindowSDL*>( mWindow )->GetSDLWindow() ) ==
			 SDL_TRUE )
			   ? true
			   : false;
}

void InputSDL::grabInput( const bool& Grab ) {
	SDL_SetWindowGrab( reinterpret_cast<WindowSDL*>( mWindow )->GetSDLWindow(),
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
	SDL_GetWindowBordersSize( sdlw, &bordersSize.Top, &bordersSize.Left, &bordersSize.Bottom,
							  &bordersSize.Right );
	mousePos.x = (int)tempMouse.x - tempWinPos.x - bordersSize.Left;
	mousePos.y = (int)tempMouse.y - tempWinPos.y - bordersSize.Top;
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

}}}} // namespace EE::Window::Backend::SDL2

#endif
