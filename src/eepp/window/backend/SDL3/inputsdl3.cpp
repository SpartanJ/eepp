#include <eepp/window/backend/SDL3/inputsdl3.hpp>
#include <eepp/window/backend/SDL3/joystickmanagersdl3.hpp>
#include <eepp/window/backend/SDL3/windowsdl3.hpp>
#include <eepp/window/engine.hpp>
#include <limits>

#ifdef EE_BACKEND_SDL3

namespace EE { namespace Window { namespace Backend { namespace SDL3 {

InputSDL::InputSDL( Window* window ) :
	Input( window, eeNew( JoystickManagerSDL, () ) ), mDPIScale( 1.f ) {
#if defined( EE_X11_PLATFORM )
	mMouseSpeed = 1.75f;
#endif
}

InputSDL::~InputSDL() {}

void InputSDL::update() {
	SDL_Event SDLEvent;
	cleanStates();

	++mEventsSentId;
	if ( mEventsSentId == std::numeric_limits<Uint64>::max() )
		mEventsSentId = 0;

	if ( !mQueuedEvents.empty() ) {
		for ( const auto& prevEvent : mQueuedEvents )
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
	} else if ( SDL_WaitEventTimeout( &SDLEvent, (int)timeout.asMilliseconds() ) ) {
		if ( SDLEvent.type != SDL_EVENT_FIRST )
			mQueuedEvents.emplace_back( SDLEvent );
	}
}

bool InputSDL::grabInput() {
	SDL_Window* win = static_cast<WindowSDL*>( mWindow )->getSDLWindow();
	return SDL_GetWindowMouseGrab( win );
}

void InputSDL::grabInput( const bool& Grab ) {
	SDL_Window* win = static_cast<WindowSDL*>( mWindow )->getSDLWindow();
	SDL_SetWindowMouseGrab( win, Grab );
}

void InputSDL::injectMousePos( const Uint16& x, const Uint16& y ) {
	SDL_Window* win = static_cast<WindowSDL*>( mWindow )->getSDLWindow();
	SDL_WarpMouseInWindow( win, static_cast<float>( x ), static_cast<float>( y ) );
}

Vector2i InputSDL::queryMousePos() {
	Vector2i mousePos;
	float tempMouseX, tempMouseY;
	int tempWinPosX, tempWinPosY;
	SDL_Window* win = static_cast<WindowSDL*>( mWindow )->getSDLWindow();
	SDL_GetGlobalMouseState( &tempMouseX, &tempMouseY );
	SDL_GetWindowPosition( win, &tempWinPosX, &tempWinPosY );
	mousePos.x = static_cast<Int32>( tempMouseX ) - tempWinPosX;
	mousePos.y = static_cast<Int32>( tempMouseY ) - tempWinPosY;
	return mousePos;
}

void InputSDL::captureMouse( const bool& capture ) {
	SDL_CaptureMouse( capture );
}

bool InputSDL::isMouseCaptured() const {
	SDL_Window* win = static_cast<WindowSDL*>( mWindow )->getSDLWindow();
	return SDL_GetWindowFlags( win ) & SDL_WINDOW_MOUSE_CAPTURE;
}

std::string InputSDL::getKeyName( const Keycode& keycode ) const {
	return std::string( SDL_GetKeyName( static_cast<SDL_Keycode>( keycode ) ) );
}

Keycode InputSDL::getKeyFromName( const std::string& keycode ) const {
	return static_cast<Keycode>( SDL_GetKeyFromName( keycode.c_str() ) );
}

std::string InputSDL::getScancodeName( const Scancode& scancode ) const {
	return SDL_GetScancodeName( static_cast<SDL_Scancode>( scancode ) );
}

Scancode InputSDL::getScancodeFromName( const std::string& scancode ) const {
	return static_cast<Scancode>( SDL_GetScancodeFromName( scancode.c_str() ) );
}

Keycode InputSDL::getKeyFromScancode( const Scancode& scancode ) const {
	return static_cast<Keycode>(
		SDL_GetKeyFromScancode( static_cast<SDL_Scancode>( scancode ), SDL_KMOD_NONE, 1 ) );
}

Scancode InputSDL::getScancodeFromKey( const Keycode& keycode ) const {
	return static_cast<Scancode>(
		SDL_GetScancodeFromKey( static_cast<SDL_Keycode>( keycode ), nullptr ) );
}

void InputSDL::init() {
	mDPIScale = mWindow->getScale();
	mMousePos = queryMousePos();
}

void InputSDL::sendEvent( const SDL_Event& SDLEvent ) {
	InputEvent event;
	event.Type = InputEvent::NoEvent;
	event.WinID = 0;

	switch ( SDLEvent.type ) {
		case SDL_EVENT_WINDOW_SHOWN: {
			event.Type = InputEvent::Window;
			event.window.gain = 1;
			event.WinID = SDLEvent.window.windowID;
			event.window.type = InputEvent::WindowShown;
			break;
		}
		case SDL_EVENT_WINDOW_HIDDEN: {
			event.Type = InputEvent::Window;
			event.window.gain = 0;
			event.WinID = SDLEvent.window.windowID;
			event.window.type = InputEvent::WindowHidden;
			break;
		}
		case SDL_EVENT_WINDOW_EXPOSED: {
			event.Type = InputEvent::VideoExpose;
			event.WinID = SDLEvent.window.windowID;
			event.expose.type = event.Type;
			break;
		}
		case SDL_EVENT_WINDOW_MOVED: {
			event.Type = InputEvent::Window;
			event.window.gain = 1;
			event.WinID = SDLEvent.window.windowID;
			event.window.type = InputEvent::WindowMoved;
			break;
		}
		case SDL_EVENT_WINDOW_RESIZED: {
			event.Type = InputEvent::VideoResize;
			event.WinID = SDLEvent.window.windowID;
			mDPIScale = mWindow->getScale();
			event.resize.w = static_cast<int>( SDLEvent.window.data1 * mDPIScale );
			event.resize.h = static_cast<int>( SDLEvent.window.data2 * mDPIScale );
			break;
		}
		case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED: {
			event.Type = InputEvent::Window;
			event.window.gain = 1;
			event.WinID = SDLEvent.window.windowID;
			event.window.type = InputEvent::WindowSizeChanged;
			break;
		}
		case SDL_EVENT_WINDOW_MINIMIZED: {
			event.Type = InputEvent::Window;
			event.window.gain = 0;
			event.WinID = SDLEvent.window.windowID;
			event.window.type = InputEvent::WindowMinimized;
			break;
		}
		case SDL_EVENT_WINDOW_MAXIMIZED: {
			event.Type = InputEvent::Window;
			event.window.gain = 1;
			event.WinID = SDLEvent.window.windowID;
			event.window.type = InputEvent::WindowMaximized;
			break;
		}
		case SDL_EVENT_WINDOW_RESTORED: {
			event.Type = InputEvent::Window;
			event.window.gain = 1;
			event.WinID = SDLEvent.window.windowID;
			event.window.type = InputEvent::WindowRestored;
			break;
		}
		case SDL_EVENT_WINDOW_MOUSE_ENTER: {
			event.Type = InputEvent::Window;
			event.window.gain = 1;
			event.WinID = SDLEvent.window.windowID;
			event.window.type = InputEvent::WindowMouseEnter;
			break;
		}
		case SDL_EVENT_WINDOW_MOUSE_LEAVE: {
			event.Type = InputEvent::Window;
			event.window.gain = 0;
			event.WinID = SDLEvent.window.windowID;
			event.window.type = InputEvent::WindowMouseLeave;
			break;
		}
		case SDL_EVENT_WINDOW_FOCUS_GAINED: {
			event.Type = InputEvent::Window;
			event.window.gain = 1;
			event.WinID = SDLEvent.window.windowID;
			event.window.type = InputEvent::WindowKeyboardFocusGain;
			break;
		}
		case SDL_EVENT_WINDOW_FOCUS_LOST: {
			event.Type = InputEvent::Window;
			event.window.gain = 0;
			event.WinID = SDLEvent.window.windowID;
			event.window.type = InputEvent::WindowKeyboardFocusLost;
			break;
		}
		case SDL_EVENT_WINDOW_CLOSE_REQUESTED: {
			event.Type = InputEvent::Window;
			event.window.gain = 0;
			event.WinID = SDLEvent.window.windowID;
			event.window.type = InputEvent::WindowClose;
			break;
		}
		case SDL_EVENT_WINDOW_HIT_TEST: {
			event.Type = InputEvent::Window;
			event.window.gain = 1;
			event.WinID = SDLEvent.window.windowID;
			event.window.type = InputEvent::WindowHitTest;
			break;
		}
		case SDL_EVENT_TEXT_INPUT: {
			String txt = String::fromUtf8( std::string_view{ SDLEvent.text.text } );
			if ( txt.empty() )
				break;
			event.Type = InputEvent::TextInput;
			// Convert from nanoseconds (SDL3) to milliseconds (EE framework)
			event.text.timestamp = static_cast<Uint32>( SDLEvent.text.timestamp / 1000000ULL );
			event.WinID = SDLEvent.text.windowID;
			for ( const auto& character : txt ) {
				event.text.text = character;
				processEvent( &event );
			}
			event.Type = InputEvent::NoEvent; // Already processed all characters
			break;
		}
		case SDL_EVENT_TEXT_EDITING: {
			event.Type = InputEvent::TextEditing;
			event.textediting.text = SDLEvent.edit.text;
			event.textediting.start = SDLEvent.edit.start;
			event.textediting.length = SDLEvent.edit.length;
			event.WinID = SDLEvent.edit.windowID;
			break;
		}
		case SDL_EVENT_KEY_DOWN: {
			event.Type = InputEvent::KeyDown;
			event.key.state = SDLEvent.key.down ? 1 : 0;
			event.key.which = SDLEvent.key.windowID;
			event.key.keysym.sym = static_cast<Keycode>( SDLEvent.key.key );
			event.key.keysym.scancode = static_cast<Scancode>( SDLEvent.key.scancode );
			event.key.keysym.mod = SDLEvent.key.mod;
			event.key.keysym.unicode = 0;
			event.WinID = SDLEvent.key.windowID;
			break;
		}
		case SDL_EVENT_KEY_UP: {
			event.Type = InputEvent::KeyUp;
			event.key.state = SDLEvent.key.down ? 1 : 0;
			event.key.which = SDLEvent.key.windowID;
			event.key.keysym.sym = static_cast<Keycode>( SDLEvent.key.key );
			event.key.keysym.scancode = static_cast<Scancode>( SDLEvent.key.scancode );
			event.key.keysym.mod = SDLEvent.key.mod;
			event.key.keysym.unicode = 0;
			event.WinID = SDLEvent.key.windowID;
			break;
		}
		case SDL_EVENT_MOUSE_MOTION: {
			event.Type = InputEvent::MouseMotion;
			event.motion.which = SDLEvent.motion.windowID;
			event.motion.state = SDLEvent.motion.state;
			event.motion.x = static_cast<Int16>( SDLEvent.motion.x * mDPIScale );
			event.motion.y = static_cast<Int16>( SDLEvent.motion.y * mDPIScale );
			event.motion.xrel = static_cast<Int16>( SDLEvent.motion.xrel * mDPIScale );
			event.motion.yrel = static_cast<Int16>( SDLEvent.motion.yrel * mDPIScale );
			event.WinID = SDLEvent.motion.windowID;
			break;
		}
		case SDL_EVENT_MOUSE_BUTTON_DOWN: {
			event.Type = InputEvent::MouseButtonDown;
			event.button.button = SDLEvent.button.button;
			event.button.which = SDLEvent.button.windowID;
			event.button.state = SDLEvent.button.down ? 1 : 0;
			event.button.x = static_cast<Int16>( SDLEvent.button.x * mDPIScale );
			event.button.y = static_cast<Int16>( SDLEvent.button.y * mDPIScale );
			event.WinID = SDLEvent.button.windowID;
			break;
		}
		case SDL_EVENT_MOUSE_BUTTON_UP: {
			event.Type = InputEvent::MouseButtonUp;
			event.button.button = SDLEvent.button.button;
			event.button.which = SDLEvent.button.windowID;
			event.button.state = SDLEvent.button.down ? 1 : 0;
			event.button.x = static_cast<Int16>( SDLEvent.button.x * mDPIScale );
			event.button.y = static_cast<Int16>( SDLEvent.button.y * mDPIScale );
			event.WinID = SDLEvent.button.windowID;
			break;
		}
		case SDL_EVENT_MOUSE_WHEEL: {
			Uint8 button;
			float x = SDLEvent.wheel.x;
			float y = SDLEvent.wheel.y;

			if ( y == 0 && x == 0 )
				break;

			if ( y > 0 ) {
				button = EE_BUTTON_WHEELUP;
			} else if ( y < 0 ) {
				button = EE_BUTTON_WHEELDOWN;
			} else if ( x > 0 ) {
				button = EE_BUTTON_WHEELRIGHT;
			} else if ( x < 0 ) {
				button = EE_BUTTON_WHEELLEFT;
			} else {
				return;
			}

			// Get mouse position from the event (mouse_x, mouse_y are in window coordinates)
			event.button.button = button;
			event.button.x = static_cast<Int16>( SDLEvent.wheel.mouse_x * mDPIScale );
			event.button.y = static_cast<Int16>( SDLEvent.wheel.mouse_y * mDPIScale );
			event.button.which = SDLEvent.wheel.windowID;
			event.WinID = SDLEvent.wheel.windowID;

			event.Type = InputEvent::MouseButtonDown;
			event.button.state = 1;
			processEvent( &event );

			event.Type = InputEvent::MouseButtonUp;
			event.button.state = 0;
			processEvent( &event );

			event.Type = InputEvent::MouseWheel;
			event.wheel.which = SDLEvent.wheel.windowID;
			event.wheel.direction = SDLEvent.wheel.direction == SDL_MOUSEWHEEL_NORMAL
										? InputEvent::WheelEvent::Normal
										: InputEvent::WheelEvent::Flipped;
			event.wheel.x = SDLEvent.wheel.x;
			event.wheel.y = SDLEvent.wheel.y;
			processEvent( &event );
			break;
		}
		case SDL_EVENT_JOYSTICK_AXIS_MOTION: {
			event.Type = InputEvent::JoyAxisMotion;
			event.jaxis.which = SDLEvent.jaxis.which;
			event.jaxis.axis = SDLEvent.jaxis.axis;
			event.jaxis.value = SDLEvent.jaxis.value;
			break;
		}
		case SDL_EVENT_JOYSTICK_BALL_MOTION: {
			event.Type = InputEvent::JoyBallMotion;
			event.jball.which = SDLEvent.jball.which;
			event.jball.ball = SDLEvent.jball.ball;
			event.jball.xrel = SDLEvent.jball.xrel;
			event.jball.yrel = SDLEvent.jball.yrel;
			break;
		}
		case SDL_EVENT_JOYSTICK_HAT_MOTION: {
			event.Type = InputEvent::JoyHatMotion;
			event.jhat.which = SDLEvent.jhat.which;
			event.jhat.value = SDLEvent.jhat.value;
			event.jhat.hat = SDLEvent.jhat.hat;
			break;
		}
		case SDL_EVENT_JOYSTICK_BUTTON_DOWN: {
			event.Type = InputEvent::JoyButtonDown;
			event.jbutton.which = SDLEvent.jbutton.which;
			event.jbutton.state = SDLEvent.jbutton.down ? 1 : 0;
			event.jbutton.button = SDLEvent.jbutton.button;
			break;
		}
		case SDL_EVENT_JOYSTICK_BUTTON_UP: {
			event.Type = InputEvent::JoyButtonUp;
			event.jbutton.which = SDLEvent.jbutton.which;
			event.jbutton.state = SDLEvent.jbutton.down ? 1 : 0;
			event.jbutton.button = SDLEvent.jbutton.button;
			break;
		}
		case SDL_EVENT_JOYSTICK_ADDED: {
			static_cast<JoystickManagerSDL*>( mJoystickManager )->addJoystick( SDLEvent.jdevice.which );
			break;
		}
		case SDL_EVENT_JOYSTICK_REMOVED: {
			static_cast<JoystickManagerSDL*>( mJoystickManager )->removeJoystick( SDLEvent.jdevice.which );
			break;
		}
		case SDL_EVENT_QUIT: {
			event.Type = InputEvent::Quit;
			event.quit.type = event.Type;
			break;
		}
		case SDL_EVENT_DROP_FILE: {
			event.Type = InputEvent::FileDropped;
			event.file.file = SDLEvent.drop.data;
			event.WinID = SDLEvent.drop.windowID;
			break;
		}
		case SDL_EVENT_DROP_TEXT: {
			event.Type = InputEvent::TextDropped;
			event.textdrop.text = SDLEvent.drop.data;
			event.WinID = SDLEvent.drop.windowID;
			break;
		}
		default: {
			if ( SDLEvent.type >= SDL_EVENT_USER && SDLEvent.type < SDL_EVENT_LAST ) {
				event.Type = InputEvent::EventUser + SDLEvent.type - SDL_EVENT_USER;
				event.user.type = event.Type;
				event.user.code = SDLEvent.user.code;
				event.user.data1 = SDLEvent.user.data1;
				event.user.data2 = SDLEvent.user.data2;
				event.WinID = SDLEvent.user.windowID;
			} else {
				event.Type = InputEvent::NoEvent;
			}
		}
	}

	// Convert SDL3 joystick ID (which is a handle) to index for framework compatibility
	if ( event.Type == InputEvent::JoyAxisMotion || event.Type == InputEvent::JoyBallMotion ||
		 event.Type == InputEvent::JoyHatMotion || event.Type == InputEvent::JoyButtonDown ||
		 event.Type == InputEvent::JoyButtonUp ) {
		SDL_JoystickID joyId = 0;
		switch ( event.Type ) {
			case InputEvent::JoyAxisMotion:
				joyId = SDLEvent.jaxis.which;
				break;
			case InputEvent::JoyBallMotion:
				joyId = SDLEvent.jball.which;
				break;
			case InputEvent::JoyHatMotion:
				joyId = SDLEvent.jhat.which;
				break;
			case InputEvent::JoyButtonDown:
			case InputEvent::JoyButtonUp:
				joyId = SDLEvent.jbutton.which;
				break;
			default:
				joyId = 0;
				break;
		}
		auto* mgr = static_cast<JoystickManagerSDL*>( mJoystickManager );
		Uint32 idx = mgr->getIndexFromID( joyId );
		if ( UINT32_MAX == idx ) {
			// Unknown joystick ID, drop event
			return;
		}
		// Overwrite the 'which' field with the correct index
		switch ( event.Type ) {
			case InputEvent::JoyAxisMotion:
				event.jaxis.which = idx;
				break;
			case InputEvent::JoyBallMotion:
				event.jball.which = idx;
				break;
			case InputEvent::JoyHatMotion:
				event.jhat.which = idx;
				break;
			case InputEvent::JoyButtonDown:
			case InputEvent::JoyButtonUp:
				event.jbutton.which = idx;
				break;
			default:
				break;
		}
	}

	EE::Window::Window* win = nullptr;

	if ( InputEvent::NoEvent != event.Type ) {
		if ( event.WinID == mWindow->getWindowID() || event.WinID == 0 ) {
			processEvent( &event );
		} else if ( ( win = EE::Window::Engine::instance()->getWindowID( event.WinID ) ) ) {
			win->getInput()->processEvent( &event );
		} else {
			processEvent( &event );
		}
	}

	// In SDL3, drop event data is managed by SDL, do not free
}

}}}} // namespace EE::Window::Backend::SDL3

#endif
