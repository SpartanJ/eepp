#include <eepp/window/backend/SFML/cinputsfml.hpp>
#include <eepp/window/backend/SFML/cjoystickmanagersfml.hpp>

#ifdef EE_BACKEND_SFML_ACTIVE

#include <eepp/window/backend/SFML/cwindowsfml.hpp>
#include <SFML/Window.hpp>

namespace EE { namespace Window { namespace Backend { namespace SFML {

static Uint32	KeyCodesTable[ sf::Keyboard::KeyCount ];
static bool		KeyCodesTableInit = false;

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
				Vector2i mp( win->GetInput()->GetMousePos() );
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
					EEEvent.key.keysym.sym = KeyCodesTable[ event.key.code ];
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
				EEEvent.key.keysym.sym = KeyCodesTable[ event.key.code ];
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
				EEEvent.key.keysym.mod = eeINDEX_NOT_FOUND;
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
	if ( KeyCodesTableInit )
		return;

	Uint32 i;

	for ( i = sf::Keyboard::A; i <= sf::Keyboard::Z; i++ )
		KeyCodesTable[ i ] = KEY_A + i;

	KeyCodesTable[ sf::Keyboard::Num0 ] = KEY_0;
	KeyCodesTable[ sf::Keyboard::Num1 ] = KEY_1;
	KeyCodesTable[ sf::Keyboard::Num2 ] = KEY_2;
	KeyCodesTable[ sf::Keyboard::Num3 ] = KEY_3;
	KeyCodesTable[ sf::Keyboard::Num4 ] = KEY_4;
	KeyCodesTable[ sf::Keyboard::Num5 ] = KEY_5;
	KeyCodesTable[ sf::Keyboard::Num6 ] = KEY_6;
	KeyCodesTable[ sf::Keyboard::Num7 ] = KEY_7;
	KeyCodesTable[ sf::Keyboard::Num8 ] = KEY_8;
	KeyCodesTable[ sf::Keyboard::Num9 ] = KEY_9;
	KeyCodesTable[ sf::Keyboard::Escape ] = KEY_ESCAPE;
	KeyCodesTable[ sf::Keyboard::LControl ] = KEY_LCTRL;
	KeyCodesTable[ sf::Keyboard::LShift ] = KEY_LSHIFT;
	KeyCodesTable[ sf::Keyboard::LAlt ] = KEY_LALT;
	KeyCodesTable[ sf::Keyboard::LSystem ] = KEY_LMETA;
	KeyCodesTable[ sf::Keyboard::RControl ] = KEY_RCTRL;
	KeyCodesTable[ sf::Keyboard::RShift ] = KEY_RSHIFT;
	KeyCodesTable[ sf::Keyboard::RAlt ] = KEY_RALT;
	KeyCodesTable[ sf::Keyboard::RSystem ] = KEY_RMETA;
	KeyCodesTable[ sf::Keyboard::Menu ] = KEY_MENU;
	KeyCodesTable[ sf::Keyboard::LBracket ] = KEY_LEFTBRACKET;
	KeyCodesTable[ sf::Keyboard::RBracket ] = KEY_RIGHTBRACKET;
	KeyCodesTable[ sf::Keyboard::SemiColon ] = KEY_SEMICOLON;
	KeyCodesTable[ sf::Keyboard::Comma ] = KEY_COMMA;
	KeyCodesTable[ sf::Keyboard::Period ] = KEY_PERIOD;
	KeyCodesTable[ sf::Keyboard::Quote ] = KEY_QUOTE;
	KeyCodesTable[ sf::Keyboard::Slash ] = KEY_SLASH;
	KeyCodesTable[ sf::Keyboard::BackSlash ] = KEY_BACKSLASH;
	KeyCodesTable[ sf::Keyboard::Tilde ] = KEY_BACKQUOTE;
	KeyCodesTable[ sf::Keyboard::Equal ] = KEY_EQUALS;
	KeyCodesTable[ sf::Keyboard::Dash ] = KEY_MINUS;
	KeyCodesTable[ sf::Keyboard::Space ] = KEY_SPACE;
	KeyCodesTable[ sf::Keyboard::Return ] = KEY_RETURN;
	KeyCodesTable[ sf::Keyboard::BackSpace ] = KEY_BACKSPACE;
	KeyCodesTable[ sf::Keyboard::Tab ] = KEY_TAB;
	KeyCodesTable[ sf::Keyboard::PageUp ] = KEY_PAGEUP;
	KeyCodesTable[ sf::Keyboard::PageDown ] = KEY_PAGEDOWN;
	KeyCodesTable[ sf::Keyboard::End ] = KEY_END;
	KeyCodesTable[ sf::Keyboard::Home ] = KEY_HOME;
	KeyCodesTable[ sf::Keyboard::Insert ] = KEY_INSERT;
	KeyCodesTable[ sf::Keyboard::Delete ] = KEY_DELETE;
	KeyCodesTable[ sf::Keyboard::Add ] = KEY_KP_PLUS;
	KeyCodesTable[ sf::Keyboard::Subtract ] = KEY_KP_MINUS;
	KeyCodesTable[ sf::Keyboard::Multiply ] = KEY_KP_MULTIPLY;
	KeyCodesTable[ sf::Keyboard::Divide ] = KEY_KP_DIVIDE;
	KeyCodesTable[ sf::Keyboard::Left ] = KEY_LEFT;
	KeyCodesTable[ sf::Keyboard::Right ] = KEY_RIGHT;
	KeyCodesTable[ sf::Keyboard::Up ] = KEY_UP;
	KeyCodesTable[ sf::Keyboard::Down ] = KEY_DOWN;
	KeyCodesTable[ sf::Keyboard::Numpad0 ] = KEY_KP0;
	KeyCodesTable[ sf::Keyboard::Numpad1 ] = KEY_KP1;
	KeyCodesTable[ sf::Keyboard::Numpad2 ] = KEY_KP2;
	KeyCodesTable[ sf::Keyboard::Numpad3 ] = KEY_KP3;
	KeyCodesTable[ sf::Keyboard::Numpad4 ] = KEY_KP4;
	KeyCodesTable[ sf::Keyboard::Numpad5 ] = KEY_KP5;
	KeyCodesTable[ sf::Keyboard::Numpad6 ] = KEY_KP6;
	KeyCodesTable[ sf::Keyboard::Numpad7 ] = KEY_KP7;
	KeyCodesTable[ sf::Keyboard::Numpad8 ] = KEY_KP8;
	KeyCodesTable[ sf::Keyboard::Numpad9 ] = KEY_KP9;
	KeyCodesTable[ sf::Keyboard::F1 ] = KEY_F1;
	KeyCodesTable[ sf::Keyboard::F2 ] = KEY_F2;
	KeyCodesTable[ sf::Keyboard::F3 ] = KEY_F3;
	KeyCodesTable[ sf::Keyboard::F4 ] = KEY_F4;
	KeyCodesTable[ sf::Keyboard::F5 ] = KEY_F5;
	KeyCodesTable[ sf::Keyboard::F6 ] = KEY_F6;
	KeyCodesTable[ sf::Keyboard::F7 ] = KEY_F7;
	KeyCodesTable[ sf::Keyboard::F8 ] = KEY_F8;
	KeyCodesTable[ sf::Keyboard::F9 ] = KEY_F9;
	KeyCodesTable[ sf::Keyboard::F10 ] = KEY_F10;
	KeyCodesTable[ sf::Keyboard::F11 ] = KEY_F11;
	KeyCodesTable[ sf::Keyboard::F12 ] = KEY_F12;
	KeyCodesTable[ sf::Keyboard::F13 ] = KEY_F13;
	KeyCodesTable[ sf::Keyboard::F14 ] = KEY_F14;
	KeyCodesTable[ sf::Keyboard::F15 ] = KEY_F15;
	KeyCodesTable[ sf::Keyboard::Pause ] = KEY_PAUSE;

	KeyCodesTableInit = true;
}

}}}}

#endif
