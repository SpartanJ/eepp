#include <eepp/window/backend/SFML/cinputsfml.hpp>
#include <eepp/window/backend/SFML/cjoystickmanagersfml.hpp>

#ifdef EE_BACKEND_SFML_ACTIVE

#include <eepp/window/backend/SFML/cwindowsfml.hpp>
#include <SFML/Window.hpp>

namespace EE { namespace Window { namespace Backend { namespace SFML {

cInputSFML::cInputSFML( cWindow * window ) :
	cInput( window, eeNew( cJoystickManagerSFML, () ) ),
	mWinActive( true )
{
}

cInputSFML::~cInputSFML() {
}

void cInputSFML::Update() {
	sf::Event	event;
	InputEvent 	EEEvent;

	cWindowSFML * win = reinterpret_cast<cWindowSFML*>( mWindow );
	sf::Window * window = win->GetSFMLWindow();

	CleanStates();

	while ( window->pollEvent(event) )
	{
		switch ( event.type )
		{
			case sf::Event::MouseEntered:
			{
				EEEvent.Type = InputEvent::Active;
				EEEvent.active.gain = 1;
				EEEvent.active.state = EE_APPMOUSEFOCUS;
				break;
			}
			case sf::Event::MouseLeft:
			{
				EEEvent.Type = InputEvent::Active;
				EEEvent.active.gain = 0;
				EEEvent.active.state = EE_APPMOUSEFOCUS;
				break;
			}
			case sf::Event::MouseButtonPressed:
			{
				EEEvent.Type = InputEvent::MouseButtonDown;
				EEEvent.button.button = GetButton( event.mouseButton.button );
				EEEvent.button.x = event.mouseButton.x;
				EEEvent.button.y = event.mouseButton.y;
				break;
			}
			case sf::Event::MouseButtonReleased:
			{
				EEEvent.Type = InputEvent::MouseButtonUp;
				EEEvent.button.button = GetButton( event.mouseButton.button );
				EEEvent.button.x = event.mouseButton.x;
				EEEvent.button.y = event.mouseButton.y;
				break;
			}
			case sf::Event::MouseMoved:
			{
				EEEvent.Type = InputEvent::MouseMotion;
				eeVector2i mp( win->GetInput()->GetMousePos() );
				EEEvent.motion.xrel = mp.x - event.mouseMove.x;
				EEEvent.motion.yrel = mp.y - event.mouseMove.y;
				EEEvent.motion.x = event.mouseMove.x;
				EEEvent.motion.y = event.mouseMove.y;
				break;
			}
			case sf::Event::MouseWheelMoved:
			{
				int d = event.mouseWheel.delta;

				Uint8 button;

				if ( d > 0 )
					button = EE_BUTTON_WHEELUP;
				else
					button = EE_BUTTON_WHEELDOWN;

				EEEvent.button.button = button;
				EEEvent.button.x = event.mouseWheel.x;
				EEEvent.button.y = event.mouseWheel.y;

				EEEvent.Type = InputEvent::MouseButtonDown;
				EEEvent.button.state = 1;
				ProcessEvent( &EEEvent );

				EEEvent.Type = InputEvent::MouseButtonUp;
				EEEvent.button.state = 0;
				break;
			}
			case sf::Event::KeyPressed:
			{
				if ( sf::Keyboard::Space != event.key.code &&
					 sf::Keyboard::Return != event.key.code &&
					 sf::Keyboard::Tab != event.key.code &&
					 sf::Keyboard::BackSpace != event.key.code
				) {
					EEEvent.Type = InputEvent::KeyDown;
					EEEvent.key.keysym.sym = mKeyCodesTable[ event.key.code ];
					EEEvent.key.keysym.mod = SetMod( event.key );
					EEEvent.key.keysym.unicode = 0;
				} else {
					EEEvent.Type = InputEvent::NoEvent;
				}

				break;
			}
			case sf::Event::KeyReleased:
			{
				EEEvent.Type = InputEvent::KeyUp;
				EEEvent.key.keysym.sym = mKeyCodesTable[ event.key.code ];
				EEEvent.key.keysym.mod = SetMod( event.key );
				EEEvent.key.keysym.unicode = 0;
				break;
			}
			case sf::Event::TextEntered:
			{
				if ( KEY_TAB != event.text.unicode ) {
					EEEvent.Type = InputEvent::TextInput;
					EEEvent.text.timestamp = Sys::GetTicks();
					EEEvent.text.text = event.text.unicode;

					ProcessEvent( &EEEvent );
				}

				EEEvent.Type = InputEvent::KeyDown;
				EEEvent.key.keysym.sym = 0;
				EEEvent.key.keysym.mod = 0xFFFFFFFF;
				EEEvent.key.keysym.unicode = event.text.unicode;

				break;
			}
			case sf::Event::GainedFocus:
			{
				EEEvent.Type = InputEvent::Active;
				EEEvent.active.gain = 1;
				EEEvent.active.state = EE_APPINPUTFOCUS;
				mWinActive = true;
				break;
			}
			case sf::Event::LostFocus:
			{
				EEEvent.Type = InputEvent::Active;
				EEEvent.active.gain = 0;
				EEEvent.active.state = EE_APPINPUTFOCUS;
				mWinActive = false;
				break;
			}
			case sf::Event::Resized:
			{
				EEEvent.Type = InputEvent::NoEvent;

				win->VideoResize( event.size.width, event.size.height );

				break;
			}
			case sf::Event::Closed:
			{
				EEEvent.Type = InputEvent::Quit;
				EEEvent.quit.type = EEEvent.Type;

				window->close();

				break;
			}
			case sf::Event::JoystickButtonPressed:
			case sf::Event::JoystickButtonReleased:
			case sf::Event::JoystickConnected:
			case sf::Event::JoystickDisconnected:
			case sf::Event::JoystickMoved:
			default:
			{
				EEEvent.Type = InputEvent::NoEvent;
				break;
			}
		}

		if ( InputEvent::NoEvent != EEEvent.Type ) {
			ProcessEvent( &EEEvent );
		}
   }
}

bool cInputSFML::GrabInput() {
	return false;
}

void cInputSFML::GrabInput( const bool& Grab ) {
}

void cInputSFML::InjectMousePos( const Uint16& x, const Uint16& y ) {
	cWindowSFML * win = reinterpret_cast<cWindowSFML*>( mWindow );
	sf::Window * window = win->GetSFMLWindow();
	sf::Mouse::setPosition( sf::Vector2i( x, y ), *window );
}

void cInputSFML::Init() {
	InitializeTables();

	mJoystickManager->Open();
}

Uint32 cInputSFML::GetButton( const Uint32& sfmlBut ) {
	switch ( sfmlBut ) {
		case sf::Mouse::Left:		return EE_BUTTON_LEFT;
		case sf::Mouse::Right:		return EE_BUTTON_RIGHT;
		case sf::Mouse::Middle:		return EE_BUTTON_MIDDLE;
		case sf::Mouse::XButton1:	return EE_BUTTON_X1;
		case sf::Mouse::XButton2:	return EE_BUTTON_X2;
	}

	return EE_BUTTON_LEFT;
}

Uint32 cInputSFML::SetMod( sf::Event::KeyEvent& key ) {
	Uint32 Ret = 0;

	if ( key.shift )		Ret |= KEYMOD_SHIFT;
	if ( key.control )		Ret |= KEYMOD_CTRL;
	if ( key.alt )			Ret |= KEYMOD_ALT;
	if ( key.system )		Ret |= KEYMOD_META;

	return Ret;
}

void cInputSFML::InitializeTables() {
	Uint32 i;

	for ( i = sf::Keyboard::A; i <= sf::Keyboard::Z; i++ )
		mKeyCodesTable[ i ] = KEY_A + i;

	mKeyCodesTable[ sf::Keyboard::Num0 ] = KEY_0;
	mKeyCodesTable[ sf::Keyboard::Num1 ] = KEY_1;
	mKeyCodesTable[ sf::Keyboard::Num2 ] = KEY_2;
	mKeyCodesTable[ sf::Keyboard::Num3 ] = KEY_3;
	mKeyCodesTable[ sf::Keyboard::Num4 ] = KEY_4;
	mKeyCodesTable[ sf::Keyboard::Num5 ] = KEY_5;
	mKeyCodesTable[ sf::Keyboard::Num6 ] = KEY_6;
	mKeyCodesTable[ sf::Keyboard::Num7 ] = KEY_7;
	mKeyCodesTable[ sf::Keyboard::Num8 ] = KEY_8;
	mKeyCodesTable[ sf::Keyboard::Num9 ] = KEY_9;
	mKeyCodesTable[ sf::Keyboard::Escape ] = KEY_ESCAPE;
	mKeyCodesTable[ sf::Keyboard::LControl ] = KEY_LCTRL;
	mKeyCodesTable[ sf::Keyboard::LShift ] = KEY_LSHIFT;
	mKeyCodesTable[ sf::Keyboard::LAlt ] = KEY_LALT;
	mKeyCodesTable[ sf::Keyboard::LSystem ] = KEY_LMETA;
	mKeyCodesTable[ sf::Keyboard::RControl ] = KEY_RCTRL;
	mKeyCodesTable[ sf::Keyboard::RShift ] = KEY_RSHIFT;
	mKeyCodesTable[ sf::Keyboard::RAlt ] = KEY_RALT;
	mKeyCodesTable[ sf::Keyboard::RSystem ] = KEY_RMETA;
	mKeyCodesTable[ sf::Keyboard::Menu ] = KEY_MENU;
	mKeyCodesTable[ sf::Keyboard::LBracket ] = KEY_LEFTBRACKET;
	mKeyCodesTable[ sf::Keyboard::RBracket ] = KEY_RIGHTBRACKET;
	mKeyCodesTable[ sf::Keyboard::SemiColon ] = KEY_SEMICOLON;
	mKeyCodesTable[ sf::Keyboard::Comma ] = KEY_COMMA;
	mKeyCodesTable[ sf::Keyboard::Period ] = KEY_PERIOD;
	mKeyCodesTable[ sf::Keyboard::Quote ] = KEY_QUOTE;
	mKeyCodesTable[ sf::Keyboard::Slash ] = KEY_SLASH;
	mKeyCodesTable[ sf::Keyboard::BackSlash ] = KEY_BACKSLASH;
	mKeyCodesTable[ sf::Keyboard::Tilde ] = KEY_BACKQUOTE;
	mKeyCodesTable[ sf::Keyboard::Equal ] = KEY_EQUALS;
	mKeyCodesTable[ sf::Keyboard::Dash ] = KEY_MINUS;
	mKeyCodesTable[ sf::Keyboard::Space ] = KEY_SPACE;
	mKeyCodesTable[ sf::Keyboard::Return ] = KEY_RETURN;
	mKeyCodesTable[ sf::Keyboard::BackSpace ] = KEY_BACKSPACE;
	mKeyCodesTable[ sf::Keyboard::Tab ] = KEY_TAB;
	mKeyCodesTable[ sf::Keyboard::PageUp ] = KEY_PAGEUP;
	mKeyCodesTable[ sf::Keyboard::PageDown ] = KEY_PAGEDOWN;
	mKeyCodesTable[ sf::Keyboard::End ] = KEY_END;
	mKeyCodesTable[ sf::Keyboard::Home ] = KEY_HOME;
	mKeyCodesTable[ sf::Keyboard::Insert ] = KEY_INSERT;
	mKeyCodesTable[ sf::Keyboard::Delete ] = KEY_DELETE;
	mKeyCodesTable[ sf::Keyboard::Add ] = KEY_KP_PLUS;
	mKeyCodesTable[ sf::Keyboard::Subtract ] = KEY_KP_MINUS;
	mKeyCodesTable[ sf::Keyboard::Multiply ] = KEY_KP_MULTIPLY;
	mKeyCodesTable[ sf::Keyboard::Divide ] = KEY_KP_DIVIDE;
	mKeyCodesTable[ sf::Keyboard::Left ] = KEY_LEFT;
	mKeyCodesTable[ sf::Keyboard::Right ] = KEY_RIGHT;
	mKeyCodesTable[ sf::Keyboard::Up ] = KEY_UP;
	mKeyCodesTable[ sf::Keyboard::Down ] = KEY_DOWN;
	mKeyCodesTable[ sf::Keyboard::Numpad0 ] = KEY_KP0;
	mKeyCodesTable[ sf::Keyboard::Numpad1 ] = KEY_KP1;
	mKeyCodesTable[ sf::Keyboard::Numpad2 ] = KEY_KP2;
	mKeyCodesTable[ sf::Keyboard::Numpad3 ] = KEY_KP3;
	mKeyCodesTable[ sf::Keyboard::Numpad4 ] = KEY_KP4;
	mKeyCodesTable[ sf::Keyboard::Numpad5 ] = KEY_KP5;
	mKeyCodesTable[ sf::Keyboard::Numpad6 ] = KEY_KP6;
	mKeyCodesTable[ sf::Keyboard::Numpad7 ] = KEY_KP7;
	mKeyCodesTable[ sf::Keyboard::Numpad8 ] = KEY_KP8;
	mKeyCodesTable[ sf::Keyboard::Numpad9 ] = KEY_KP9;
	mKeyCodesTable[ sf::Keyboard::F1 ] = KEY_F1;
	mKeyCodesTable[ sf::Keyboard::F2 ] = KEY_F2;
	mKeyCodesTable[ sf::Keyboard::F3 ] = KEY_F3;
	mKeyCodesTable[ sf::Keyboard::F4 ] = KEY_F4;
	mKeyCodesTable[ sf::Keyboard::F5 ] = KEY_F5;
	mKeyCodesTable[ sf::Keyboard::F6 ] = KEY_F6;
	mKeyCodesTable[ sf::Keyboard::F7 ] = KEY_F7;
	mKeyCodesTable[ sf::Keyboard::F8 ] = KEY_F8;
	mKeyCodesTable[ sf::Keyboard::F9 ] = KEY_F9;
	mKeyCodesTable[ sf::Keyboard::F10 ] = KEY_F10;
	mKeyCodesTable[ sf::Keyboard::F11 ] = KEY_F11;
	mKeyCodesTable[ sf::Keyboard::F12 ] = KEY_F12;
	mKeyCodesTable[ sf::Keyboard::F13 ] = KEY_F13;
	mKeyCodesTable[ sf::Keyboard::F14 ] = KEY_F14;
	mKeyCodesTable[ sf::Keyboard::F15 ] = KEY_F15;
	mKeyCodesTable[ sf::Keyboard::Pause ] = KEY_PAUSE;
}

}}}}

#endif
