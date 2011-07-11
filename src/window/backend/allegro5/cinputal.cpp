#include "cinputal.hpp"
#include "cjoystickmanageral.hpp"
#include "cwindowal.hpp"

#ifdef EE_BACKEND_ALLEGRO_ACTIVE

namespace EE { namespace Window { namespace Backend { namespace Al {

cInputAl::cInputAl( cWindow * window ) :
	cInput( window, eeNew( cJoystickManagerAl, () ) ),
	mGrab( false ),
	mZ( 0 )
{
	memset( mKeyCodesTable, 0, KEY_LAST );
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
				/// Since EEPP doesn't have a separated event for the chars, it's necessary to filtrate some chars
				EEEvent.Type = InputEvent::KeyDown;
				EEEvent.key.keysym.mod = SetMod( ALEvent.keyboard.modifiers );
				EEEvent.key.keysym.unicode = 0;

				if ( 8 == ALEvent.keyboard.unichar ) {
					EEEvent.key.keysym.sym = ALLEGRO_KEY_BACKSPACE;
					ProcessEvent( &EEEvent );
				} else if ( 9 == ALEvent.keyboard.unichar ) {
					EEEvent.key.keysym.sym = ALLEGRO_KEY_TAB;
					ProcessEvent( &EEEvent );
				} else if ( 13 == ALEvent.keyboard.unichar ) {
					EEEvent.key.keysym.sym = ALLEGRO_KEY_ENTER;
					ProcessEvent( &EEEvent );
				} else if ( 32 == ALEvent.keyboard.unichar ) {
					EEEvent.key.keysym.sym = ALLEGRO_KEY_SPACE;
					ProcessEvent( &EEEvent );
				}

				EEEvent.key.keysym.sym = mKeyCodesTable[ ALEvent.keyboard.keycode ];
				EEEvent.key.keysym.unicode = ALEvent.keyboard.unichar;

				break;
			}
			case ALLEGRO_EVENT_KEY_DOWN:
			{
				if ( ALLEGRO_KEY_SPACE != ALEvent.keyboard.keycode && ALLEGRO_KEY_ENTER != ALEvent.keyboard.keycode && ALLEGRO_KEY_TAB != ALEvent.keyboard.keycode && ALLEGRO_KEY_BACKSPACE != ALEvent.keyboard.keycode ) {
					EEEvent.Type = InputEvent::KeyDown;
					EEEvent.key.keysym.sym = mKeyCodesTable[ ALEvent.keyboard.keycode ];
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
				EEEvent.key.keysym.sym = mKeyCodesTable[ ALEvent.keyboard.keycode ];
				EEEvent.key.keysym.mod = 0;
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
	Uint32 i;

	for ( i = ALLEGRO_KEY_A; i <= ALLEGRO_KEY_Z; i++ )
		mKeyCodesTable[ i ] = KEY_A - 1 + i;

	mKeyCodesTable[ ALLEGRO_KEY_0 ] = KEY_0;
	mKeyCodesTable[ ALLEGRO_KEY_1 ] = KEY_1;
	mKeyCodesTable[ ALLEGRO_KEY_2 ] = KEY_2;
	mKeyCodesTable[ ALLEGRO_KEY_3 ] = KEY_3;
	mKeyCodesTable[ ALLEGRO_KEY_4 ] = KEY_4;
	mKeyCodesTable[ ALLEGRO_KEY_5 ] = KEY_5;
	mKeyCodesTable[ ALLEGRO_KEY_6 ] = KEY_6;
	mKeyCodesTable[ ALLEGRO_KEY_7 ] = KEY_7;
	mKeyCodesTable[ ALLEGRO_KEY_8 ] = KEY_8;
	mKeyCodesTable[ ALLEGRO_KEY_9 ] = KEY_9;
	mKeyCodesTable[ ALLEGRO_KEY_PAD_0 ] = KEY_KP0;
	mKeyCodesTable[ ALLEGRO_KEY_PAD_1 ] = KEY_KP1;
	mKeyCodesTable[ ALLEGRO_KEY_PAD_2 ] = KEY_KP2;
	mKeyCodesTable[ ALLEGRO_KEY_PAD_3 ] = KEY_KP3;
	mKeyCodesTable[ ALLEGRO_KEY_PAD_4 ] = KEY_KP4;
	mKeyCodesTable[ ALLEGRO_KEY_PAD_5 ] = KEY_KP5;
	mKeyCodesTable[ ALLEGRO_KEY_PAD_6 ] = KEY_KP6;
	mKeyCodesTable[ ALLEGRO_KEY_PAD_7 ] = KEY_KP7;
	mKeyCodesTable[ ALLEGRO_KEY_PAD_8 ] = KEY_KP8;
	mKeyCodesTable[ ALLEGRO_KEY_PAD_9 ] = KEY_KP9;
	mKeyCodesTable[ ALLEGRO_KEY_F1 ] = KEY_F1;
	mKeyCodesTable[ ALLEGRO_KEY_F2 ] = KEY_F2;
	mKeyCodesTable[ ALLEGRO_KEY_F3 ] = KEY_F3;
	mKeyCodesTable[ ALLEGRO_KEY_F4 ] = KEY_F4;
	mKeyCodesTable[ ALLEGRO_KEY_F5 ] = KEY_F5;
	mKeyCodesTable[ ALLEGRO_KEY_F6 ] = KEY_F6;
	mKeyCodesTable[ ALLEGRO_KEY_F7 ] = KEY_F7;
	mKeyCodesTable[ ALLEGRO_KEY_F8 ] = KEY_F8;
	mKeyCodesTable[ ALLEGRO_KEY_F9 ] = KEY_F9;
	mKeyCodesTable[ ALLEGRO_KEY_F10 ] = KEY_F10;
	mKeyCodesTable[ ALLEGRO_KEY_F11 ] = KEY_F11;
	mKeyCodesTable[ ALLEGRO_KEY_F12 ] = KEY_F12;
	mKeyCodesTable[ ALLEGRO_KEY_ESCAPE ] = KEY_ESCAPE;
	//mKeyCodesTable[ ALLEGRO_KEY_TILDE ] = KEY_?;
	mKeyCodesTable[ ALLEGRO_KEY_MINUS ] = KEY_MINUS;
	mKeyCodesTable[ ALLEGRO_KEY_EQUALS ] = KEY_EQUALS;
	mKeyCodesTable[ ALLEGRO_KEY_BACKSPACE ] = KEY_BACKSPACE;
	mKeyCodesTable[ ALLEGRO_KEY_TAB ] = KEY_TAB;
	//mKeyCodesTable[ ALLEGRO_KEY_OPENBRACE ] = KEY_?;
	//mKeyCodesTable[ ALLEGRO_KEY_CLOSEBRACE ] = KEY_?;
	mKeyCodesTable[ ALLEGRO_KEY_ENTER ] = KEY_RETURN;
	mKeyCodesTable[ ALLEGRO_KEY_SEMICOLON ] = KEY_SEMICOLON;
	mKeyCodesTable[ ALLEGRO_KEY_QUOTE ] = KEY_QUOTE;
	mKeyCodesTable[ ALLEGRO_KEY_BACKSLASH ] = KEY_BACKSLASH;
	//mKeyCodesTable[ ALLEGRO_KEY_BACKSLASH2 ] = KEY_?;
	mKeyCodesTable[ ALLEGRO_KEY_COMMA ] = KEY_COMMA;
	//mKeyCodesTable[ ALLEGRO_KEY_FULLSTOP ] = KEY_?;
	mKeyCodesTable[ ALLEGRO_KEY_SLASH ] = KEY_SLASH;
	mKeyCodesTable[ ALLEGRO_KEY_SPACE ] = KEY_SPACE;
	mKeyCodesTable[ ALLEGRO_KEY_INSERT] = KEY_INSERT;
	mKeyCodesTable[ ALLEGRO_KEY_DELETE ] = KEY_DELETE;
	mKeyCodesTable[ ALLEGRO_KEY_HOME ] = KEY_HOME;
	mKeyCodesTable[ ALLEGRO_KEY_END ] = KEY_END;
	mKeyCodesTable[ ALLEGRO_KEY_PGUP ] = KEY_PAGEUP;
	mKeyCodesTable[ ALLEGRO_KEY_PGDN ] = KEY_PAGEDOWN;
	mKeyCodesTable[ ALLEGRO_KEY_LEFT ] = KEY_LEFT;
	mKeyCodesTable[ ALLEGRO_KEY_RIGHT ] = KEY_RIGHT;
	mKeyCodesTable[ ALLEGRO_KEY_UP ] = KEY_UP;
	mKeyCodesTable[ ALLEGRO_KEY_DOWN ] = KEY_DOWN;
	mKeyCodesTable[ ALLEGRO_KEY_PAD_SLASH ] = KEY_KP_DIVIDE;
	mKeyCodesTable[ ALLEGRO_KEY_PAD_ASTERISK ] = KEY_KP_MULTIPLY;
	mKeyCodesTable[ ALLEGRO_KEY_PAD_MINUS ] = KEY_KP_MINUS;
	mKeyCodesTable[ ALLEGRO_KEY_PAD_PLUS ] = KEY_KP_PLUS;
	//mKeyCodesTable[ ALLEGRO_KEY_PAD_DELETE ] = KEY_KP_?;
	mKeyCodesTable[ ALLEGRO_KEY_PAD_ENTER ] = KEY_KP_ENTER;
	mKeyCodesTable[ ALLEGRO_KEY_PRINTSCREEN ] = KEY_PRINT;
	mKeyCodesTable[ ALLEGRO_KEY_PAUSE ] = KEY_PAUSE;
	//mKeyCodesTable[ ALLEGRO_KEY_ABNT_C1 ] = KEY_KP_?;
	//mKeyCodesTable[ ALLEGRO_KEY_YEN ] = KEY_KP_?;
	//mKeyCodesTable[ ALLEGRO_KEY_KANA ] = KEY_KP_?;
	//mKeyCodesTable[ ALLEGRO_KEY_CONVERT ] = KEY_KP_?;
	//mKeyCodesTable[ ALLEGRO_KEY_NOCONVERT ] = KEY_KP_?;
	//mKeyCodesTable[ ALLEGRO_KEY_AT ] = KEY_KP_?;
	//mKeyCodesTable[ ALLEGRO_KEY_CIRCUMFLEX ] = KEY_KP_?;
	//mKeyCodesTable[ ALLEGRO_KEY_COLON2 ] = KEY_KP_?;
	//mKeyCodesTable[ ALLEGRO_KEY_KANJI ] = KEY_KP_?;
	mKeyCodesTable[ ALLEGRO_KEY_PAD_EQUALS ] = KEY_KP_EQUALS;
	mKeyCodesTable[ ALLEGRO_KEY_BACKQUOTE ] = KEY_BACKQUOTE;
	//mKeyCodesTable[ ALLEGRO_KEY_SEMICOLON2 ] = KEY_?;
	//mKeyCodesTable[ ALLEGRO_KEY_COMMAND ] = KEY_?;
	mKeyCodesTable[ ALLEGRO_KEY_UNKNOWN ] = KEY_UNKNOWN;
	mKeyCodesTable[ ALLEGRO_KEY_LSHIFT ] = KEY_LSHIFT;
	mKeyCodesTable[ ALLEGRO_KEY_RSHIFT ] = KEY_RSHIFT;
	mKeyCodesTable[ ALLEGRO_KEY_LCTRL ] = KEY_LCTRL;
	mKeyCodesTable[ ALLEGRO_KEY_RCTRL ] = KEY_RCTRL;
	mKeyCodesTable[ ALLEGRO_KEY_ALT ] = KEY_LALT;
	mKeyCodesTable[ ALLEGRO_KEY_ALTGR ] = KEY_MODE; //KEY_RALT;
	mKeyCodesTable[ ALLEGRO_KEY_LWIN ] = KEY_LSUPER;
	mKeyCodesTable[ ALLEGRO_KEY_RWIN ] = KEY_RSUPER;
	//mKeyCodesTable[ ALLEGRO_KEY_MENU ] = KEY_?;
	mKeyCodesTable[ ALLEGRO_KEY_SCROLLLOCK ] = KEY_SCROLLOCK;
	mKeyCodesTable[ ALLEGRO_KEY_NUMLOCK ] = KEY_NUMLOCK;
	mKeyCodesTable[ ALLEGRO_KEY_CAPSLOCK ] = KEY_CAPSLOCK;
}

Uint32 cInputAl::SetMod(  Uint32 Mod ) {
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
