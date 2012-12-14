#include <eepp/window/backend/allegro5/cinputal.hpp>
#include <eepp/window/backend/allegro5/cjoystickmanageral.hpp>
#include <eepp/window/backend/allegro5/cwindowal.hpp>

#ifdef EE_BACKEND_ALLEGRO_ACTIVE

namespace EE { namespace Window { namespace Backend { namespace Al {

static Uint32	KeyCodesTable[ ALLEGRO_KEY_MAX ];
static bool		KeyCodesTableInit = false;

cInputAl::cInputAl( cWindow * window ) :
	cInput( window, eeNew( cJoystickManagerAl, () ) ),
	mGrab( false ),
	mZ( 0 )
{
	memset( KeyCodesTable, 0, KEY_LAST );
}

cInputAl::~cInputAl() {
	al_destroy_event_queue( mQueue );
}

void cInputAl::Update() {
	ALLEGRO_EVENT	ALEvent;
	InputEvent		EEEvent;

	CleanStates();

	while ( al_get_next_event( mQueue, &ALEvent ) ) {
		switch ( ALEvent.type ) {
			case ALLEGRO_EVENT_KEY_CHAR:
			{
				EEEvent.Type = InputEvent::KeyDown;
				EEEvent.key.keysym.mod = SetMod( ALEvent.keyboard.modifiers );
				EEEvent.key.keysym.unicode = 0;

				if ( KEY_BACKSPACE == ALEvent.keyboard.unichar ) {
					EEEvent.key.keysym.sym = ALLEGRO_KEY_BACKSPACE;
					ProcessEvent( &EEEvent );
				} else if ( KEY_TAB == ALEvent.keyboard.unichar ) {
					EEEvent.key.keysym.sym = ALLEGRO_KEY_TAB;
					ProcessEvent( &EEEvent );
				} else if ( KEY_RETURN == ALEvent.keyboard.unichar ) {
					EEEvent.key.keysym.sym = ALLEGRO_KEY_ENTER;
					ProcessEvent( &EEEvent );
				} else if ( KEY_SPACE == ALEvent.keyboard.unichar ) {
					EEEvent.key.keysym.sym = ALLEGRO_KEY_SPACE;
					ProcessEvent( &EEEvent );
				}

				if ( ALEvent.keyboard.unichar > 0 && KEY_TAB != ALEvent.keyboard.unichar ) { // otherwise generates the TextInput event
					EEEvent.Type = InputEvent::TextInput;
					EEEvent.text.timestamp = ALEvent.any.timestamp;
					EEEvent.text.text = (Uint32)ALEvent.keyboard.unichar;
					ProcessEvent( &EEEvent );
					EEEvent.Type = InputEvent::KeyDown;
				}

				EEEvent.key.keysym.sym = KeyCodesTable[ ALEvent.keyboard.keycode ];
				EEEvent.key.keysym.unicode = ALEvent.keyboard.unichar;

				break;
			}
			case ALLEGRO_EVENT_KEY_DOWN:
			{
				if ( ALLEGRO_KEY_SPACE != ALEvent.keyboard.keycode &&
					 ALLEGRO_KEY_ENTER != ALEvent.keyboard.keycode &&
					 ALLEGRO_KEY_TAB != ALEvent.keyboard.keycode &&
					 ALLEGRO_KEY_BACKSPACE != ALEvent.keyboard.keycode
				) {
					EEEvent.Type = InputEvent::KeyDown;
					EEEvent.key.keysym.sym = KeyCodesTable[ ALEvent.keyboard.keycode ];
					EEEvent.key.keysym.mod = SetMod( ALEvent.keyboard.modifiers );
					EEEvent.key.keysym.unicode = ALEvent.keyboard.unichar;
				} else {
					EEEvent.Type = InputEvent::NoEvent;
				}
				break;
			}
			case ALLEGRO_EVENT_KEY_UP:
			{
				EEEvent.Type = InputEvent::KeyUp;
				EEEvent.key.keysym.sym = KeyCodesTable[ ALEvent.keyboard.keycode ];
				EEEvent.key.keysym.mod = SetMod( ALEvent.keyboard.modifiers );
				EEEvent.key.keysym.unicode = ALEvent.keyboard.unichar;
				break;
			}
			case ALLEGRO_EVENT_MOUSE_BUTTON_DOWN:
			{
				EEEvent.Type = InputEvent::MouseButtonDown;
				EEEvent.button.button = ALEvent.mouse.button;

				/// Middle button is the third button on Allegro, so they need to be switched
				if ( 2 == ALEvent.mouse.button ) EEEvent.button.button = 3;
				else if ( 3 == ALEvent.mouse.button ) EEEvent.button.button = 2;

				EEEvent.button.x = ALEvent.mouse.x;
				EEEvent.button.y = ALEvent.mouse.y;
				break;
			}
			case ALLEGRO_EVENT_MOUSE_BUTTON_UP:
			{
				EEEvent.Type = InputEvent::MouseButtonUp;
				EEEvent.button.button = ALEvent.mouse.button;

				if ( 2 == ALEvent.mouse.button ) EEEvent.button.button = 3;
				else if ( 3 == ALEvent.mouse.button ) EEEvent.button.button = 2;

				EEEvent.button.x = ALEvent.mouse.x;
				EEEvent.button.y = ALEvent.mouse.y;
				break;
			}
			case ALLEGRO_EVENT_MOUSE_AXES:
			{
				/// Mouse wheel works a little different, so i simulate the expected behavior
				if ( ALEvent.mouse.z != mZ ) {
					EEEvent.Type = InputEvent::MouseButtonUp;

					if ( mZ > ALEvent.mouse.z ) {
						EEEvent.button.button = EE_BUTTON_WHEELDOWN;
					} else {
						EEEvent.button.button = EE_BUTTON_WHEELUP;
					}

					EEEvent.button.x = ALEvent.mouse.x;
					EEEvent.button.y = ALEvent.mouse.y;
					mZ = ALEvent.mouse.z;

					ProcessEvent( &EEEvent );
				}

				EEEvent.Type = InputEvent::MouseMotion;
				EEEvent.motion.x = ALEvent.mouse.x;
				EEEvent.motion.y = ALEvent.mouse.y;
				EEEvent.motion.xrel = ALEvent.mouse.dx;
				EEEvent.motion.yrel = ALEvent.mouse.dy;
				break;
			}
			case ALLEGRO_EVENT_DISPLAY_EXPOSE:
			{
				EEEvent.Type = InputEvent::VideoExpose;
				break;
			}
			case ALLEGRO_EVENT_DISPLAY_CLOSE:
			{
				EEEvent.Type = InputEvent::Quit;
				break;
			}
			case ALLEGRO_EVENT_DISPLAY_RESIZE:
			{
				EEEvent.Type = InputEvent::VideoResize;

				EEEvent.resize.w = ALEvent.display.width;
				EEEvent.resize.h = ALEvent.display.height;

				al_acknowledge_resize( GetDisplay() );
				break;
			}
			case ALLEGRO_EVENT_DISPLAY_SWITCH_IN:
			{
				EEEvent.Type = InputEvent::Active;
				GetWindowAl()->mActive = true;
				break;
			}
			case ALLEGRO_EVENT_DISPLAY_SWITCH_OUT:
			{
				GetWindowAl()->mActive = false;
				break;
			}
			case ALLEGRO_EVENT_JOYSTICK_AXIS:
			{
				EEEvent.Type = InputEvent::JoyAxisMotion;
				EEEvent.jaxis.which = ALEvent.joystick.stick;
				EEEvent.jaxis.axis = ALEvent.joystick.axis;
				EEEvent.jaxis.value = ALEvent.joystick.pos;

				break;
			}
			case ALLEGRO_EVENT_JOYSTICK_BUTTON_DOWN:
			{
				EEEvent.Type = InputEvent::JoyButtonDown;
				EEEvent.jbutton.which = ALEvent.joystick.stick;
				EEEvent.jbutton.button = ALEvent.joystick.button;
				break;
			}
			case ALLEGRO_EVENT_JOYSTICK_BUTTON_UP:
			{
				EEEvent.Type = InputEvent::JoyButtonUp;
				EEEvent.jbutton.which = ALEvent.joystick.stick;
				EEEvent.jbutton.button = ALEvent.joystick.button;
				break;
			}
			case ALLEGRO_EVENT_JOYSTICK_CONFIGURATION:
			{
				mJoystickManager->Rescan();
				break;
			}
			default:
			{
				if ( ALLEGRO_EVENT_TYPE_IS_USER( ALEvent.type ) ) {
					EEEvent.Type = InputEvent::EventUser + ALEvent.type - 512;
					EEEvent.user.type = EEEvent.Type;
					EEEvent.user.code = 0;
					EEEvent.user.data1 = (void*)ALEvent.user.data1;
					EEEvent.user.data2 = (void*)ALEvent.user.data2;
				} else {
					EEEvent.Type = InputEvent::NoEvent;
				}
			}
		}

		if ( InputEvent::NoEvent != EEEvent.Type) {
			ProcessEvent( &EEEvent );
		}
	}
}

bool cInputAl::GrabInput() {
	return mInputGrabed;
}

void cInputAl::GrabInput( const bool& Grab ) {
	if ( Grab ) {
		al_grab_mouse( GetDisplay() );
	} else {
		al_ungrab_mouse();
	}

	mInputGrabed = Grab;
}

void cInputAl::InjectMousePos( const Uint16& x, const Uint16& y ) {
	if ( x <= GetWindowAl()->GetWidth() && y <= GetWindowAl()->GetHeight() ) {
		al_set_mouse_xy( GetDisplay(), x, y );
		mMousePos.x = x;
		mMousePos.y = y;
	}
}

void cInputAl::Init() {
	al_install_keyboard();
	al_install_mouse();
	al_install_joystick();

	mQueue = al_create_event_queue();

	al_register_event_source( mQueue, al_get_keyboard_event_source() );
	al_register_event_source( mQueue, al_get_mouse_event_source() );
	al_register_event_source( mQueue, al_get_display_event_source( GetDisplay() ) );
	al_register_event_source( mQueue, al_get_joystick_event_source() );

	ALLEGRO_MOUSE_STATE state;
	al_get_mouse_state(&state);
	mMousePos.x = (eeInt)state.x;
	mMousePos.y = (eeInt)state.y;
	mZ = state.z;

	InitializeTables();

	mJoystickManager->Open();
}

cWindowAl * cInputAl::GetWindowAl() const {
	return reinterpret_cast<cWindowAl*>( mWindow );
}

ALLEGRO_DISPLAY * cInputAl::GetDisplay() {
	return GetWindowAl()->GetDisplay();
}

void cInputAl::InitializeTables() {
	if ( KeyCodesTableInit )
		return;

	Uint32 i;

	for ( i = ALLEGRO_KEY_A; i <= ALLEGRO_KEY_Z; i++ )
		KeyCodesTable[ i ] = KEY_A - 1 + i;

	KeyCodesTable[ ALLEGRO_KEY_0 ] = KEY_0;
	KeyCodesTable[ ALLEGRO_KEY_1 ] = KEY_1;
	KeyCodesTable[ ALLEGRO_KEY_2 ] = KEY_2;
	KeyCodesTable[ ALLEGRO_KEY_3 ] = KEY_3;
	KeyCodesTable[ ALLEGRO_KEY_4 ] = KEY_4;
	KeyCodesTable[ ALLEGRO_KEY_5 ] = KEY_5;
	KeyCodesTable[ ALLEGRO_KEY_6 ] = KEY_6;
	KeyCodesTable[ ALLEGRO_KEY_7 ] = KEY_7;
	KeyCodesTable[ ALLEGRO_KEY_8 ] = KEY_8;
	KeyCodesTable[ ALLEGRO_KEY_9 ] = KEY_9;
	KeyCodesTable[ ALLEGRO_KEY_PAD_0 ] = KEY_KP0;
	KeyCodesTable[ ALLEGRO_KEY_PAD_1 ] = KEY_KP1;
	KeyCodesTable[ ALLEGRO_KEY_PAD_2 ] = KEY_KP2;
	KeyCodesTable[ ALLEGRO_KEY_PAD_3 ] = KEY_KP3;
	KeyCodesTable[ ALLEGRO_KEY_PAD_4 ] = KEY_KP4;
	KeyCodesTable[ ALLEGRO_KEY_PAD_5 ] = KEY_KP5;
	KeyCodesTable[ ALLEGRO_KEY_PAD_6 ] = KEY_KP6;
	KeyCodesTable[ ALLEGRO_KEY_PAD_7 ] = KEY_KP7;
	KeyCodesTable[ ALLEGRO_KEY_PAD_8 ] = KEY_KP8;
	KeyCodesTable[ ALLEGRO_KEY_PAD_9 ] = KEY_KP9;
	KeyCodesTable[ ALLEGRO_KEY_F1 ] = KEY_F1;
	KeyCodesTable[ ALLEGRO_KEY_F2 ] = KEY_F2;
	KeyCodesTable[ ALLEGRO_KEY_F3 ] = KEY_F3;
	KeyCodesTable[ ALLEGRO_KEY_F4 ] = KEY_F4;
	KeyCodesTable[ ALLEGRO_KEY_F5 ] = KEY_F5;
	KeyCodesTable[ ALLEGRO_KEY_F6 ] = KEY_F6;
	KeyCodesTable[ ALLEGRO_KEY_F7 ] = KEY_F7;
	KeyCodesTable[ ALLEGRO_KEY_F8 ] = KEY_F8;
	KeyCodesTable[ ALLEGRO_KEY_F9 ] = KEY_F9;
	KeyCodesTable[ ALLEGRO_KEY_F10 ] = KEY_F10;
	KeyCodesTable[ ALLEGRO_KEY_F11 ] = KEY_F11;
	KeyCodesTable[ ALLEGRO_KEY_F12 ] = KEY_F12;
	KeyCodesTable[ ALLEGRO_KEY_ESCAPE ] = KEY_ESCAPE;
	//KeyCodesTable[ ALLEGRO_KEY_TILDE ] = KEY_?;
	KeyCodesTable[ ALLEGRO_KEY_MINUS ] = KEY_MINUS;
	KeyCodesTable[ ALLEGRO_KEY_EQUALS ] = KEY_EQUALS;
	KeyCodesTable[ ALLEGRO_KEY_BACKSPACE ] = KEY_BACKSPACE;
	KeyCodesTable[ ALLEGRO_KEY_TAB ] = KEY_TAB;
	//KeyCodesTable[ ALLEGRO_KEY_OPENBRACE ] = KEY_?;
	//KeyCodesTable[ ALLEGRO_KEY_CLOSEBRACE ] = KEY_?;
	KeyCodesTable[ ALLEGRO_KEY_ENTER ] = KEY_RETURN;
	KeyCodesTable[ ALLEGRO_KEY_SEMICOLON ] = KEY_SEMICOLON;
	KeyCodesTable[ ALLEGRO_KEY_QUOTE ] = KEY_QUOTE;
	KeyCodesTable[ ALLEGRO_KEY_BACKSLASH ] = KEY_BACKSLASH;
	//KeyCodesTable[ ALLEGRO_KEY_BACKSLASH2 ] = KEY_?;
	KeyCodesTable[ ALLEGRO_KEY_COMMA ] = KEY_COMMA;
	//KeyCodesTable[ ALLEGRO_KEY_FULLSTOP ] = KEY_?;
	KeyCodesTable[ ALLEGRO_KEY_SLASH ] = KEY_SLASH;
	KeyCodesTable[ ALLEGRO_KEY_SPACE ] = KEY_SPACE;
	KeyCodesTable[ ALLEGRO_KEY_INSERT] = KEY_INSERT;
	KeyCodesTable[ ALLEGRO_KEY_DELETE ] = KEY_DELETE;
	KeyCodesTable[ ALLEGRO_KEY_HOME ] = KEY_HOME;
	KeyCodesTable[ ALLEGRO_KEY_END ] = KEY_END;
	KeyCodesTable[ ALLEGRO_KEY_PGUP ] = KEY_PAGEUP;
	KeyCodesTable[ ALLEGRO_KEY_PGDN ] = KEY_PAGEDOWN;
	KeyCodesTable[ ALLEGRO_KEY_LEFT ] = KEY_LEFT;
	KeyCodesTable[ ALLEGRO_KEY_RIGHT ] = KEY_RIGHT;
	KeyCodesTable[ ALLEGRO_KEY_UP ] = KEY_UP;
	KeyCodesTable[ ALLEGRO_KEY_DOWN ] = KEY_DOWN;
	KeyCodesTable[ ALLEGRO_KEY_PAD_SLASH ] = KEY_KP_DIVIDE;
	KeyCodesTable[ ALLEGRO_KEY_PAD_ASTERISK ] = KEY_KP_MULTIPLY;
	KeyCodesTable[ ALLEGRO_KEY_PAD_MINUS ] = KEY_KP_MINUS;
	KeyCodesTable[ ALLEGRO_KEY_PAD_PLUS ] = KEY_KP_PLUS;
	//KeyCodesTable[ ALLEGRO_KEY_PAD_DELETE ] = KEY_KP_?;
	KeyCodesTable[ ALLEGRO_KEY_PAD_ENTER ] = KEY_KP_ENTER;
	KeyCodesTable[ ALLEGRO_KEY_PRINTSCREEN ] = KEY_PRINT;
	KeyCodesTable[ ALLEGRO_KEY_PAUSE ] = KEY_PAUSE;
	//KeyCodesTable[ ALLEGRO_KEY_ABNT_C1 ] = KEY_KP_?;
	//KeyCodesTable[ ALLEGRO_KEY_YEN ] = KEY_KP_?;
	//KeyCodesTable[ ALLEGRO_KEY_KANA ] = KEY_KP_?;
	//KeyCodesTable[ ALLEGRO_KEY_CONVERT ] = KEY_KP_?;
	//KeyCodesTable[ ALLEGRO_KEY_NOCONVERT ] = KEY_KP_?;
	//KeyCodesTable[ ALLEGRO_KEY_AT ] = KEY_KP_?;
	//KeyCodesTable[ ALLEGRO_KEY_CIRCUMFLEX ] = KEY_KP_?;
	//KeyCodesTable[ ALLEGRO_KEY_COLON2 ] = KEY_KP_?;
	//KeyCodesTable[ ALLEGRO_KEY_KANJI ] = KEY_KP_?;
	KeyCodesTable[ ALLEGRO_KEY_PAD_EQUALS ] = KEY_KP_EQUALS;
	KeyCodesTable[ ALLEGRO_KEY_BACKQUOTE ] = KEY_BACKQUOTE;
	//KeyCodesTable[ ALLEGRO_KEY_SEMICOLON2 ] = KEY_?;
	//KeyCodesTable[ ALLEGRO_KEY_COMMAND ] = KEY_?;
	KeyCodesTable[ ALLEGRO_KEY_UNKNOWN ] = KEY_UNKNOWN;
	KeyCodesTable[ ALLEGRO_KEY_LSHIFT ] = KEY_LSHIFT;
	KeyCodesTable[ ALLEGRO_KEY_RSHIFT ] = KEY_RSHIFT;
	KeyCodesTable[ ALLEGRO_KEY_LCTRL ] = KEY_LCTRL;
	KeyCodesTable[ ALLEGRO_KEY_RCTRL ] = KEY_RCTRL;
	KeyCodesTable[ ALLEGRO_KEY_ALT ] = KEY_LALT;
	KeyCodesTable[ ALLEGRO_KEY_ALTGR ] = KEY_MODE; //KEY_RALT;
	KeyCodesTable[ ALLEGRO_KEY_LWIN ] = KEY_LSUPER;
	KeyCodesTable[ ALLEGRO_KEY_RWIN ] = KEY_RSUPER;
	//KeyCodesTable[ ALLEGRO_KEY_MENU ] = KEY_?;
	KeyCodesTable[ ALLEGRO_KEY_SCROLLLOCK ] = KEY_SCROLLOCK;
	KeyCodesTable[ ALLEGRO_KEY_NUMLOCK ] = KEY_NUMLOCK;
	KeyCodesTable[ ALLEGRO_KEY_CAPSLOCK ] = KEY_CAPSLOCK;

	KeyCodesTableInit = true;
}

Uint32 cInputAl::SetMod( Uint32 Mod ) {
	if ( !Mod )
		return 0;

	Uint32 Ret = 0;

	if ( Mod & ALLEGRO_KEYMOD_SHIFT )		Ret |= KEYMOD_LSHIFT;
	if ( Mod & ALLEGRO_KEYMOD_CTRL )		Ret |= KEYMOD_LCTRL;
	if ( Mod & ALLEGRO_KEYMOD_ALT )			Ret |= KEYMOD_LALT;
	if ( Mod & ALLEGRO_KEYMOD_ALTGR )		Ret |= KEYMOD_RALT;
	if ( Mod & ALLEGRO_KEYMOD_NUMLOCK )		Ret |= KEYMOD_NUM;
	if ( Mod & ALLEGRO_KEYMOD_CAPSLOCK )	Ret |= KEYMOD_CAPS;

	return Ret;
}

}}}}

#endif
