#include "inputhelper.hpp"

namespace EE { namespace Window {

Uint32 eeConvertKeyCharacter( const Uint32& KeyCode, const Uint16& Unicode, const Uint32& Modifiers ) {
	Uint32 value = 0;

	if ( Unicode < 255 ) {
		value = Unicode;
	}

	if ( isCharacter( value ) ) {
		return value;
	}

	switch ( KeyCode ) {
		case KEY_TAB: 			value = KEY_TAB; 		break;
		case KEY_LALT: 			value = KEY_LALT; 		break;
		case KEY_RALT: 			value = KEY_RALT;		break;
		case KEY_LSHIFT:		value = KEY_LSHIFT;		break;
		case KEY_RSHIFT:		value = KEY_RSHIFT;		break;
		case KEY_LCTRL:			value = KEY_RSHIFT;		break;
		case KEY_RCTRL:			value = KEY_LCTRL;		break;
		case KEY_BACKSPACE:		value = KEY_BACKSPACE; 	break;
		case KEY_PAUSE:			value = KEY_PAUSE;		break;
		case KEY_SPACE:			value = KEY_SPACE;		break;
		case KEY_ESCAPE:		value = KEY_ESCAPE;		break;
		case KEY_DELETE:		value = KEY_DELETE;		break;
		case KEY_INSERT:		value = KEY_INSERT;		break;
		case KEY_HOME:			value = KEY_HOME;		break;
		case KEY_END:			value = KEY_END;		break;
		case KEY_PAGEUP:		value = KEY_PAGEUP;		break;
		case KEY_PRINT:			value = KEY_PRINT;		break;
		case KEY_PAGEDOWN:		value = KEY_PAGEDOWN;	break;
		case KEY_F1:			value = KEY_F1;			break;
		case KEY_F2:			value = KEY_F2;			break;
		case KEY_F3:			value = KEY_F3;			break;
		case KEY_F4:			value = KEY_F4;			break;
		case KEY_F5:			value = KEY_F5;			break;
		case KEY_F6:			value = KEY_F6;			break;
		case KEY_F7:			value = KEY_F7;			break;
		case KEY_F8:			value = KEY_F8;			break;
		case KEY_F9:			value = KEY_F9;			break;
		case KEY_F10:			value = KEY_F10;		break;
		case KEY_F11:			value = KEY_F11;		break;
		case KEY_F12:			value = KEY_F12;		break;
		case KEY_F13:			value = KEY_F13;		break;
		case KEY_F14:			value = KEY_F14;		break;
		case KEY_F15:			value = KEY_F15;		break;
		case KEY_NUMLOCK:		value = KEY_NUMLOCK;	break;
		case KEY_CAPSLOCK:		value = KEY_CAPSLOCK;	break;
		case KEY_SCROLLOCK:		value = KEY_SCROLLOCK;	break;
		case KEY_RMETA:			value = KEY_RMETA;		break;
		case KEY_LMETA:			value = KEY_LMETA;		break;
		case KEY_LSUPER:		value = KEY_LSUPER;		break;
		case KEY_RSUPER:		value = KEY_RSUPER;		break;
		case KEY_MODE:			value = KEY_MODE;		break;
		case KEY_UP:			value = KEY_UP;			break;
		case KEY_DOWN:			value = KEY_DOWN;		break;
		case KEY_LEFT:			value = KEY_LEFT;		break;
		case KEY_RIGHT:			value = KEY_RIGHT;		break;
		case KEY_RETURN:		value = KEY_RETURN;		break;
		case KEY_KP_ENTER:		value = KEY_KP_ENTER;	break;
		default:				break;
	}

	if ( !( Modifiers & KEYMOD_NUM ) ) {
		switch ( KeyCode ) {
			case KEY_KP0:		value = KEY_INSERT;		break;
			case KEY_KP1:		value = KEY_END;		break;
			case KEY_KP2:		value = KEY_DOWN;		break;
			case KEY_KP3:		value = KEY_PAGEDOWN;	break;
			case KEY_KP4:		value = KEY_LEFT;		break;
			case KEY_KP5:		value = 0;				break;
			case KEY_KP6:		value = KEY_RIGHT;		break;
			case KEY_KP7:		value = KEY_HOME;		break;
			case KEY_KP8:		value = KEY_UP;			break;
			case KEY_KP9:		value = KEY_PAGEUP;		break;
			default:			break;
		}
	}

	return value;
}

}}
