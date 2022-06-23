#include "eterminaldisplay.hpp"
#include "system/processfactory.hpp"
#include <eepp/graphics/primitives.hpp>
#include <eepp/graphics/text.hpp>
#include <eepp/system/color.hpp>
#include <eepp/window.hpp>
#include <eepp/window/clipboard.hpp>

TerminalKeyMap::TerminalKeyMap( const TerminalKey keys[], size_t keysLen,
								const TerminalScancode platformKeys[], size_t platformKeysLen,
								const TerminalShortcut shortcuts[], size_t shortcutsLen ) {
	for ( size_t i = 0; i < keysLen; i++ ) {
		auto& e = m_keyMap[keys[i].keysym];
		e.push_back( { keys[i].mask, keys[i].string, keys[i].appkey, keys[i].appcursor } );
	}

	for ( size_t i = 0; i < platformKeysLen; i++ ) {
		auto& e = m_platformKeyMap[platformKeys[i].scancode];
		e.push_back( { platformKeys[i].mask, platformKeys[i].string, platformKeys[i].appkey,
					   platformKeys[i].appcursor } );
	}

	for ( size_t i = 0; i < shortcutsLen; i++ ) {
		auto& e = m_shortcuts[shortcuts[i].keysym];
		e.push_back( { shortcuts[i].mask, shortcuts[i].action, shortcuts[i].appkey,
					   shortcuts[i].appcursor } );
	}
}

static TerminalShortcut shortcuts[] = {
	{ KEY_INSERT, KEYMOD_SHIFT, TerminalShortcutAction::PASTE, 0, 0 } };

static TerminalKey keys[] = {
	/* keysym           mask            string      appkey appcursor */
	{ KEY_KP_ENTER, KEYMOD_CTRL_SHIFT_ALT_META, "\033OM", +2, 0 },
	{ KEY_KP_ENTER, KEYMOD_CTRL_SHIFT_ALT_META, "\r", -1, 0 },

	{ KEY_UP, KEYMOD_SHIFT, "\033[1;2A", 0, 0 },
	{ KEY_UP, KEYMOD_LALT, "\033[1;3A", 0, 0 },
	{ KEY_UP, KEYMOD_SHIFT | KEYMOD_LALT, "\033[1;4A", 0, 0 },
	{ KEY_UP, KEYMOD_CTRL, "\033[1;5A", 0, 0 },
	{ KEY_UP, KEYMOD_SHIFT | KEYMOD_CTRL, "\033[1;6A", 0, 0 },
	{ KEY_UP, KEYMOD_CTRL | KEYMOD_LALT, "\033[1;7A", 0, 0 },
	{ KEY_UP, KEYMOD_SHIFT | KEYMOD_CTRL | KEYMOD_LALT, "\033[1;8A", 0, 0 },
	{ KEY_UP, KEYMOD_CTRL_SHIFT_ALT_META, "\033[A", 0, -1 },
	{ KEY_UP, KEYMOD_CTRL_SHIFT_ALT_META, "\033OA", 0, +1 },
	{ KEY_DOWN, KEYMOD_SHIFT, "\033[1;2B", 0, 0 },
	{ KEY_DOWN, KEYMOD_LALT, "\033[1;3B", 0, 0 },
	{ KEY_DOWN, KEYMOD_SHIFT | KEYMOD_LALT, "\033[1;4B", 0, 0 },
	{ KEY_DOWN, KEYMOD_CTRL, "\033[1;5B", 0, 0 },
	{ KEY_DOWN, KEYMOD_SHIFT | KEYMOD_CTRL, "\033[1;6B", 0, 0 },
	{ KEY_DOWN, KEYMOD_CTRL | KEYMOD_LALT, "\033[1;7B", 0, 0 },
	{ KEY_DOWN, KEYMOD_SHIFT | KEYMOD_CTRL | KEYMOD_LALT, "\033[1;8B", 0, 0 },
	{ KEY_DOWN, KEYMOD_CTRL_SHIFT_ALT_META, "\033[B", 0, -1 },
	{ KEY_DOWN, KEYMOD_CTRL_SHIFT_ALT_META, "\033OB", 0, +1 },
	{ KEY_LEFT, KEYMOD_SHIFT, "\033[1;2D", 0, 0 },
	{ KEY_LEFT, KEYMOD_LALT, "\033[1;3D", 0, 0 },
	{ KEY_LEFT, KEYMOD_SHIFT | KEYMOD_LALT, "\033[1;4D", 0, 0 },
	{ KEY_LEFT, KEYMOD_CTRL, "\033[1;5D", 0, 0 },
	{ KEY_LEFT, KEYMOD_SHIFT | KEYMOD_CTRL, "\033[1;6D", 0, 0 },
	{ KEY_LEFT, KEYMOD_CTRL | KEYMOD_LALT, "\033[1;7D", 0, 0 },
	{ KEY_LEFT, KEYMOD_SHIFT | KEYMOD_CTRL | KEYMOD_LALT, "\033[1;8D", 0, 0 },
	{ KEY_LEFT, KEYMOD_CTRL_SHIFT_ALT_META, "\033[D", 0, -1 },
	{ KEY_LEFT, KEYMOD_CTRL_SHIFT_ALT_META, "\033OD", 0, +1 },
	{ KEY_RIGHT, KEYMOD_SHIFT, "\033[1;2C", 0, 0 },
	{ KEY_RIGHT, KEYMOD_LALT, "\033[1;3C", 0, 0 },
	{ KEY_RIGHT, KEYMOD_SHIFT | KEYMOD_LALT, "\033[1;4C", 0, 0 },
	{ KEY_RIGHT, KEYMOD_CTRL, "\033[1;5C", 0, 0 },
	{ KEY_RIGHT, KEYMOD_SHIFT | KEYMOD_CTRL, "\033[1;6C", 0, 0 },
	{ KEY_RIGHT, KEYMOD_CTRL | KEYMOD_LALT, "\033[1;7C", 0, 0 },
	{ KEY_RIGHT, KEYMOD_SHIFT | KEYMOD_CTRL | KEYMOD_LALT, "\033[1;8C", 0, 0 },
	{ KEY_RIGHT, KEYMOD_CTRL_SHIFT_ALT_META, "\033[C", 0, -1 },
	{ KEY_RIGHT, KEYMOD_CTRL_SHIFT_ALT_META, "\033OC", 0, +1 },
	{ KEY_TAB, KEYMOD_SHIFT, "\033[Z", 0, 0 },
	{ KEY_TAB, KEYMOD_CTRL_SHIFT_ALT_META, "\t", 0, 0 },
	{ KEY_RETURN, KEYMOD_LALT, "\033\r", 0, 0 },
	{ KEY_RETURN, KEYMOD_CTRL_SHIFT_ALT_META, "\r", 0, 0 },
	{ KEY_INSERT, KEYMOD_SHIFT, "\033[4l", -1, 0 },
	{ KEY_INSERT, KEYMOD_SHIFT, "\033[2;2~", +1, 0 },
	{ KEY_INSERT, KEYMOD_CTRL, "\033[L", -1, 0 },
	{ KEY_INSERT, KEYMOD_CTRL, "\033[2;5~", +1, 0 },
	{ KEY_INSERT, KEYMOD_CTRL_SHIFT_ALT_META, "\033[4h", -1, 0 },
	{ KEY_INSERT, KEYMOD_CTRL_SHIFT_ALT_META, "\033[2~", +1, 0 },
	{ KEY_DELETE, KEYMOD_CTRL, "\033[M", -1, 0 },
	{ KEY_DELETE, KEYMOD_CTRL, "\033[3;5~", +1, 0 },
	{ KEY_DELETE, KEYMOD_SHIFT, "\033[2K", -1, 0 },
	{ KEY_DELETE, KEYMOD_SHIFT, "\033[3;2~", +1, 0 },
	{ KEY_DELETE, KEYMOD_CTRL_SHIFT_ALT_META, "\033[P", -1, 0 },
	{ KEY_DELETE, KEYMOD_CTRL_SHIFT_ALT_META, "\033[3~", +1, 0 },
	{ KEY_BACKSPACE, KEYMOD_NONE, "\177", 0, 0 },
	{ KEY_BACKSPACE, KEYMOD_LALT, "\033\177", 0, 0 },
	{ KEY_HOME, KEYMOD_SHIFT, "\033[2J", 0, -1 },
	{ KEY_HOME, KEYMOD_SHIFT, "\033[1;2H", 0, +1 },
	{ KEY_HOME, KEYMOD_CTRL_SHIFT_ALT_META, "\033[H", 0, -1 },
	{ KEY_HOME, KEYMOD_CTRL_SHIFT_ALT_META, "\033[1~", 0, +1 },
	{ KEY_END, KEYMOD_CTRL, "\033[J", -1, 0 },
	{ KEY_END, KEYMOD_CTRL, "\033[1;5F", +1, 0 },
	{ KEY_END, KEYMOD_SHIFT, "\033[K", -1, 0 },
	{ KEY_END, KEYMOD_SHIFT, "\033[1;2F", +1, 0 },
	{ KEY_END, KEYMOD_CTRL_SHIFT_ALT_META, "\033[4~", 0, 0 },
	{ KEY_ESCAPE, KEYMOD_CTRL_SHIFT_ALT_META, "\033", 0, 0 },

	{ KEY_PAGEUP, KEYMOD_NONE, "\033[5~", 0, 0 },
	{ KEY_PAGEDOWN, KEYMOD_NONE, "\033[6~", 0, 0 },
};

static TerminalScancode platformKeys[] = {
	{ SCANCODE_PRIOR, KEYMOD_CTRL, "\033[5;5~", 0, 0 },

	{ SCANCODE_PRIOR, KEYMOD_CTRL, "\033[5;5~", 0, 0 },
	{ SCANCODE_PRIOR, KEYMOD_SHIFT, "\033[5;2~", 0, 0 },
	{ SCANCODE_PRIOR, KEYMOD_CTRL_SHIFT_ALT_META, "\033[5~", 0, 0 },

	{ SCANCODE_KP_BACKSPACE, KEYMOD_CTRL, "\033[M", -1, 0 },
	{ SCANCODE_KP_BACKSPACE, KEYMOD_CTRL, "\033[3;5~", +1, 0 },
	{ SCANCODE_KP_BACKSPACE, KEYMOD_SHIFT, "\033[2K", -1, 0 },
	{ SCANCODE_KP_BACKSPACE, KEYMOD_SHIFT, "\033[3;2~", +1, 0 },
	{ SCANCODE_KP_BACKSPACE, KEYMOD_CTRL_SHIFT_ALT_META, "\033[P", -1, 0 },
	{ SCANCODE_KP_BACKSPACE, KEYMOD_CTRL_SHIFT_ALT_META, "\033[3~", +1, 0 },

	{ SCANCODE_KP_MINUS, KEYMOD_CTRL_SHIFT_ALT_META, "\033Om", +2, 0 },
	{ SCANCODE_KP_DECIMAL, KEYMOD_CTRL_SHIFT_ALT_META, "\033On", +2, 0 },
	{ SCANCODE_KP_DIVIDE, KEYMOD_CTRL_SHIFT_ALT_META, "\033Oo", +2, 0 },
	{ SCANCODE_KP_MULTIPLY, KEYMOD_CTRL_SHIFT_ALT_META, "\033Oj", +2, 0 },
	{ SCANCODE_KP_PLUS, KEYMOD_CTRL_SHIFT_ALT_META, "\033Ok", +2, 0 },

	{ SCANCODE_F1, KEYMOD_NONE, "\033OP", 0, 0 },
	{ SCANCODE_F1, /* F13 */ KEYMOD_SHIFT, "\033[1;2P", 0, 0 },
	{ SCANCODE_F1, /* F25 */ KEYMOD_CTRL, "\033[1;5P", 0, 0 },
	{ SCANCODE_F1, /* F37 */ KEYMOD_META, "\033[1;6P", 0, 0 },
	{ SCANCODE_F1, /* F49 */ KEYMOD_LALT, "\033[1;3P", 0, 0 },
	{ SCANCODE_F1, /* F61 */ KEYMOD_RALT, "\033[1;4P", 0, 0 },
	{ SCANCODE_F2, KEYMOD_NONE, "\033OQ", 0, 0 },
	{ SCANCODE_F2, /* F14 */ KEYMOD_SHIFT, "\033[1;2Q", 0, 0 },
	{ SCANCODE_F2, /* F26 */ KEYMOD_CTRL, "\033[1;5Q", 0, 0 },
	{ SCANCODE_F2, /* F38 */ KEYMOD_META, "\033[1;6Q", 0, 0 },
	{ SCANCODE_F2, /* F50 */ KEYMOD_LALT, "\033[1;3Q", 0, 0 },
	{ SCANCODE_F2, /* F62 */ KEYMOD_RALT, "\033[1;4Q", 0, 0 },
	{ SCANCODE_F3, KEYMOD_NONE, "\033OR", 0, 0 },
	{ SCANCODE_F3, /* F15 */ KEYMOD_SHIFT, "\033[1;2R", 0, 0 },
	{ SCANCODE_F3, /* F27 */ KEYMOD_CTRL, "\033[1;5R", 0, 0 },
	{ SCANCODE_F3, /* F39 */ KEYMOD_META, "\033[1;6R", 0, 0 },
	{ SCANCODE_F3, /* F51 */ KEYMOD_LALT, "\033[1;3R", 0, 0 },
	{ SCANCODE_F3, /* F63 */ KEYMOD_RALT, "\033[1;4R", 0, 0 },
	{ SCANCODE_F4, KEYMOD_NONE, "\033OS", 0, 0 },
	{ SCANCODE_F4, /* F16 */ KEYMOD_SHIFT, "\033[1;2S", 0, 0 },
	{ SCANCODE_F4, /* F28 */ KEYMOD_CTRL, "\033[1;5S", 0, 0 },
	{ SCANCODE_F4, /* F40 */ KEYMOD_META, "\033[1;6S", 0, 0 },
	{ SCANCODE_F4, /* F52 */ KEYMOD_LALT, "\033[1;3S", 0, 0 },
	{ SCANCODE_F5, KEYMOD_NONE, "\033[15~", 0, 0 },
	{ SCANCODE_F5, /* F17 */ KEYMOD_SHIFT, "\033[15;2~", 0, 0 },
	{ SCANCODE_F5, /* F29 */ KEYMOD_CTRL, "\033[15;5~", 0, 0 },
	{ SCANCODE_F5, /* F41 */ KEYMOD_META, "\033[15;6~", 0, 0 },
	{ SCANCODE_F5, /* F53 */ KEYMOD_LALT, "\033[15;3~", 0, 0 },
	{ SCANCODE_F6, KEYMOD_NONE, "\033[17~", 0, 0 },
	{ SCANCODE_F6, /* F18 */ KEYMOD_SHIFT, "\033[17;2~", 0, 0 },
	{ SCANCODE_F6, /* F30 */ KEYMOD_CTRL, "\033[17;5~", 0, 0 },
	{ SCANCODE_F6, /* F42 */ KEYMOD_META, "\033[17;6~", 0, 0 },
	{ SCANCODE_F6, /* F54 */ KEYMOD_LALT, "\033[17;3~", 0, 0 },
	{ SCANCODE_F7, KEYMOD_NONE, "\033[18~", 0, 0 },
	{ SCANCODE_F7, /* F19 */ KEYMOD_SHIFT, "\033[18;2~", 0, 0 },
	{ SCANCODE_F7, /* F31 */ KEYMOD_CTRL, "\033[18;5~", 0, 0 },
	{ SCANCODE_F7, /* F43 */ KEYMOD_META, "\033[18;6~", 0, 0 },
	{ SCANCODE_F7, /* F55 */ KEYMOD_LALT, "\033[18;3~", 0, 0 },
	{ SCANCODE_F8, KEYMOD_NONE, "\033[19~", 0, 0 },
	{ SCANCODE_F8, /* F20 */ KEYMOD_SHIFT, "\033[19;2~", 0, 0 },
	{ SCANCODE_F8, /* F32 */ KEYMOD_CTRL, "\033[19;5~", 0, 0 },
	{ SCANCODE_F8, /* F44 */ KEYMOD_META, "\033[19;6~", 0, 0 },
	{ SCANCODE_F8, /* F56 */ KEYMOD_LALT, "\033[19;3~", 0, 0 },
	{ SCANCODE_F9, KEYMOD_NONE, "\033[20~", 0, 0 },
	{ SCANCODE_F9, /* F21 */ KEYMOD_SHIFT, "\033[20;2~", 0, 0 },
	{ SCANCODE_F9, /* F33 */ KEYMOD_CTRL, "\033[20;5~", 0, 0 },
	{ SCANCODE_F9, /* F45 */ KEYMOD_META, "\033[20;6~", 0, 0 },
	{ SCANCODE_F9, /* F57 */ KEYMOD_LALT, "\033[20;3~", 0, 0 },
	{ SCANCODE_F10, KEYMOD_NONE, "\033[21~", 0, 0 },
	{ SCANCODE_F10, /* F22 */ KEYMOD_SHIFT, "\033[21;2~", 0, 0 },
	{ SCANCODE_F10, /* F34 */ KEYMOD_CTRL, "\033[21;5~", 0, 0 },
	{ SCANCODE_F10, /* F46 */ KEYMOD_META, "\033[21;6~", 0, 0 },
	{ SCANCODE_F10, /* F58 */ KEYMOD_LALT, "\033[21;3~", 0, 0 },
	{ SCANCODE_F11, KEYMOD_NONE, "\033[23~", 0, 0 },
	{ SCANCODE_F11, /* F23 */ KEYMOD_SHIFT, "\033[23;2~", 0, 0 },
	{ SCANCODE_F11, /* F35 */ KEYMOD_CTRL, "\033[23;5~", 0, 0 },
	{ SCANCODE_F11, /* F47 */ KEYMOD_META, "\033[23;6~", 0, 0 },
	{ SCANCODE_F11, /* F59 */ KEYMOD_LALT, "\033[23;3~", 0, 0 },
	{ SCANCODE_F12, KEYMOD_NONE, "\033[24~", 0, 0 },
	{ SCANCODE_F12, /* F24 */ KEYMOD_SHIFT, "\033[24;2~", 0, 0 },
	{ SCANCODE_F12, /* F36 */ KEYMOD_CTRL, "\033[24;5~", 0, 0 },
	{ SCANCODE_F12, /* F48 */ KEYMOD_META, "\033[24;6~", 0, 0 },
	{ SCANCODE_F12, /* F60 */ KEYMOD_LALT, "\033[24;3~", 0, 0 },
	{ SCANCODE_F13, KEYMOD_NONE, "\033[1;2P", 0, 0 },
	{ SCANCODE_F14, KEYMOD_NONE, "\033[1;2Q", 0, 0 },
	{ SCANCODE_F15, KEYMOD_NONE, "\033[1;2R", 0, 0 },
	{ SCANCODE_F16, KEYMOD_NONE, "\033[1;2S", 0, 0 },
	{ SCANCODE_F17, KEYMOD_NONE, "\033[15;2~", 0, 0 },
	{ SCANCODE_F18, KEYMOD_NONE, "\033[17;2~", 0, 0 },
	{ SCANCODE_F19, KEYMOD_NONE, "\033[18;2~", 0, 0 },
	{ SCANCODE_F20, KEYMOD_NONE, "\033[19;2~", 0, 0 },
	{ SCANCODE_F21, KEYMOD_NONE, "\033[20;2~", 0, 0 },
	{ SCANCODE_F22, KEYMOD_NONE, "\033[21;2~", 0, 0 },
	{ SCANCODE_F23, KEYMOD_NONE, "\033[23;2~", 0, 0 },
	{ SCANCODE_F24, KEYMOD_NONE, "\033[24;2~", 0, 0 },

	{ SCANCODE_KP_0, KEYMOD_CTRL_SHIFT_ALT_META, "\033Op", +2, 0 },
	{ SCANCODE_KP_1, KEYMOD_CTRL_SHIFT_ALT_META, "\033Oq", +2, 0 },
	{ SCANCODE_KP_2, KEYMOD_CTRL_SHIFT_ALT_META, "\033Or", +2, 0 },
	{ SCANCODE_KP_3, KEYMOD_CTRL_SHIFT_ALT_META, "\033Os", +2, 0 },
	{ SCANCODE_KP_4, KEYMOD_CTRL_SHIFT_ALT_META, "\033Ot", +2, 0 },
	{ SCANCODE_KP_5, KEYMOD_CTRL_SHIFT_ALT_META, "\033Ou", +2, 0 },
	{ SCANCODE_KP_6, KEYMOD_CTRL_SHIFT_ALT_META, "\033Ov", +2, 0 },
	{ SCANCODE_KP_7, KEYMOD_CTRL_SHIFT_ALT_META, "\033Ow", +2, 0 },
	{ SCANCODE_KP_8, KEYMOD_CTRL_SHIFT_ALT_META, "\033Ox", +2, 0 },
	{ SCANCODE_KP_9, KEYMOD_CTRL_SHIFT_ALT_META, "\033Oy", +2, 0 } };

TerminalKeyMap terminalKeyMap{ keys,		 eeARRAY_SIZE( keys ),
							   platformKeys, eeARRAY_SIZE( platformKeys ),
							   shortcuts,	 eeARRAY_SIZE( shortcuts ) };

/* Terminal colors (16 first used in escape sequence) */
// This is the customizable colorscheme
const char* colornames[256] = { "#1e2127", "#e06c75", "#98c379", "#d19a66", "#61afef", "#c678dd",
								"#56b6c2", "#abb2bf", "#5c6370", "#e06c75", "#98c379", "#d19a66",
								"#61afef", "#c678dd", "#56b6c2", "#ffffff", "#1e2127", "#abb2bf" };

// This is the default Xterm palette
static const Color colormapped[256] = {
	Color( 0, 0, 0 ),		Color( 128, 0, 0 ),		Color( 0, 128, 0 ),
	Color( 128, 128, 0 ),	Color( 0, 0, 128 ),		Color( 128, 0, 128 ),
	Color( 0, 128, 128 ),	Color( 192, 192, 192 ), Color( 128, 128, 128 ),
	Color( 255, 0, 0 ),		Color( 0, 255, 0 ),		Color( 255, 255, 0 ),
	Color( 0, 0, 255 ),		Color( 255, 0, 255 ),	Color( 0, 255, 255 ),
	Color( 255, 255, 255 ), Color( 0, 0, 0 ),		Color( 0, 0, 95 ),
	Color( 0, 0, 135 ),		Color( 0, 0, 175 ),		Color( 0, 0, 215 ),
	Color( 0, 0, 255 ),		Color( 0, 95, 0 ),		Color( 0, 95, 95 ),
	Color( 0, 95, 135 ),	Color( 0, 95, 175 ),	Color( 0, 95, 215 ),
	Color( 0, 95, 255 ),	Color( 0, 135, 0 ),		Color( 0, 135, 95 ),
	Color( 0, 135, 135 ),	Color( 0, 135, 175 ),	Color( 0, 135, 215 ),
	Color( 0, 135, 255 ),	Color( 0, 175, 0 ),		Color( 0, 175, 95 ),
	Color( 0, 175, 135 ),	Color( 0, 175, 175 ),	Color( 0, 175, 215 ),
	Color( 0, 175, 255 ),	Color( 0, 215, 0 ),		Color( 0, 215, 95 ),
	Color( 0, 215, 135 ),	Color( 0, 215, 175 ),	Color( 0, 215, 215 ),
	Color( 0, 215, 255 ),	Color( 0, 255, 0 ),		Color( 0, 255, 95 ),
	Color( 0, 255, 135 ),	Color( 0, 255, 175 ),	Color( 0, 255, 215 ),
	Color( 0, 255, 255 ),	Color( 95, 0, 0 ),		Color( 95, 0, 95 ),
	Color( 95, 0, 135 ),	Color( 95, 0, 175 ),	Color( 95, 0, 215 ),
	Color( 95, 0, 255 ),	Color( 95, 95, 0 ),		Color( 95, 95, 95 ),
	Color( 95, 95, 135 ),	Color( 95, 95, 175 ),	Color( 95, 95, 215 ),
	Color( 95, 95, 255 ),	Color( 95, 135, 0 ),	Color( 95, 135, 95 ),
	Color( 95, 135, 135 ),	Color( 95, 135, 175 ),	Color( 95, 135, 215 ),
	Color( 95, 135, 255 ),	Color( 95, 175, 0 ),	Color( 95, 175, 95 ),
	Color( 95, 175, 135 ),	Color( 95, 175, 175 ),	Color( 95, 175, 215 ),
	Color( 95, 175, 255 ),	Color( 95, 215, 0 ),	Color( 95, 215, 95 ),
	Color( 95, 215, 135 ),	Color( 95, 215, 175 ),	Color( 95, 215, 215 ),
	Color( 95, 215, 255 ),	Color( 95, 255, 0 ),	Color( 95, 255, 95 ),
	Color( 95, 255, 135 ),	Color( 95, 255, 175 ),	Color( 95, 255, 215 ),
	Color( 95, 255, 255 ),	Color( 135, 0, 0 ),		Color( 135, 0, 95 ),
	Color( 135, 0, 135 ),	Color( 135, 0, 175 ),	Color( 135, 0, 215 ),
	Color( 135, 0, 255 ),	Color( 135, 95, 0 ),	Color( 135, 95, 95 ),
	Color( 135, 95, 135 ),	Color( 135, 95, 175 ),	Color( 135, 95, 215 ),
	Color( 135, 95, 255 ),	Color( 135, 135, 0 ),	Color( 135, 135, 95 ),
	Color( 135, 135, 135 ), Color( 135, 135, 175 ), Color( 135, 135, 215 ),
	Color( 135, 135, 255 ), Color( 135, 175, 0 ),	Color( 135, 175, 95 ),
	Color( 135, 175, 135 ), Color( 135, 175, 175 ), Color( 135, 175, 215 ),
	Color( 135, 175, 255 ), Color( 135, 215, 0 ),	Color( 135, 215, 95 ),
	Color( 135, 215, 135 ), Color( 135, 215, 175 ), Color( 135, 215, 215 ),
	Color( 135, 215, 255 ), Color( 135, 255, 0 ),	Color( 135, 255, 95 ),
	Color( 135, 255, 135 ), Color( 135, 255, 175 ), Color( 135, 255, 215 ),
	Color( 135, 255, 255 ), Color( 175, 0, 0 ),		Color( 175, 0, 95 ),
	Color( 175, 0, 135 ),	Color( 175, 0, 175 ),	Color( 175, 0, 215 ),
	Color( 175, 0, 255 ),	Color( 175, 95, 0 ),	Color( 175, 95, 95 ),
	Color( 175, 95, 135 ),	Color( 175, 95, 175 ),	Color( 175, 95, 215 ),
	Color( 175, 95, 255 ),	Color( 175, 135, 0 ),	Color( 175, 135, 95 ),
	Color( 175, 135, 135 ), Color( 175, 135, 175 ), Color( 175, 135, 215 ),
	Color( 175, 135, 255 ), Color( 175, 175, 0 ),	Color( 175, 175, 95 ),
	Color( 175, 175, 135 ), Color( 175, 175, 175 ), Color( 175, 175, 215 ),
	Color( 175, 175, 255 ), Color( 175, 215, 0 ),	Color( 175, 215, 95 ),
	Color( 175, 215, 135 ), Color( 175, 215, 175 ), Color( 175, 215, 215 ),
	Color( 175, 215, 255 ), Color( 175, 255, 0 ),	Color( 175, 255, 95 ),
	Color( 175, 255, 135 ), Color( 175, 255, 175 ), Color( 175, 255, 215 ),
	Color( 175, 255, 255 ), Color( 215, 0, 0 ),		Color( 215, 0, 95 ),
	Color( 215, 0, 135 ),	Color( 215, 0, 175 ),	Color( 215, 0, 215 ),
	Color( 215, 0, 255 ),	Color( 215, 95, 0 ),	Color( 215, 95, 95 ),
	Color( 215, 95, 135 ),	Color( 215, 95, 175 ),	Color( 215, 95, 215 ),
	Color( 215, 95, 255 ),	Color( 215, 135, 0 ),	Color( 215, 135, 95 ),
	Color( 215, 135, 135 ), Color( 215, 135, 175 ), Color( 215, 135, 215 ),
	Color( 215, 135, 255 ), Color( 215, 175, 0 ),	Color( 215, 175, 95 ),
	Color( 215, 175, 135 ), Color( 215, 175, 175 ), Color( 215, 175, 215 ),
	Color( 215, 175, 255 ), Color( 215, 215, 0 ),	Color( 215, 215, 95 ),
	Color( 215, 215, 135 ), Color( 215, 215, 175 ), Color( 215, 215, 215 ),
	Color( 215, 215, 255 ), Color( 215, 255, 0 ),	Color( 215, 255, 95 ),
	Color( 215, 255, 135 ), Color( 215, 255, 175 ), Color( 215, 255, 215 ),
	Color( 215, 255, 255 ), Color( 255, 0, 0 ),		Color( 255, 0, 95 ),
	Color( 255, 0, 135 ),	Color( 255, 0, 175 ),	Color( 255, 0, 215 ),
	Color( 255, 0, 255 ),	Color( 255, 95, 0 ),	Color( 255, 95, 95 ),
	Color( 255, 95, 135 ),	Color( 255, 95, 175 ),	Color( 255, 95, 215 ),
	Color( 255, 95, 255 ),	Color( 255, 135, 0 ),	Color( 255, 135, 95 ),
	Color( 255, 135, 135 ), Color( 255, 135, 175 ), Color( 255, 135, 215 ),
	Color( 255, 135, 255 ), Color( 255, 175, 0 ),	Color( 255, 175, 95 ),
	Color( 255, 175, 135 ), Color( 255, 175, 175 ), Color( 255, 175, 215 ),
	Color( 255, 175, 255 ), Color( 255, 215, 0 ),	Color( 255, 215, 95 ),
	Color( 255, 215, 135 ), Color( 255, 215, 175 ), Color( 255, 215, 215 ),
	Color( 255, 215, 255 ), Color( 255, 255, 0 ),	Color( 255, 255, 95 ),
	Color( 255, 255, 135 ), Color( 255, 255, 175 ), Color( 255, 255, 215 ),
	Color( 255, 255, 255 ), Color( 8, 8, 8 ),		Color( 18, 18, 18 ),
	Color( 28, 28, 28 ),	Color( 38, 38, 38 ),	Color( 48, 48, 48 ),
	Color( 58, 58, 58 ),	Color( 68, 68, 68 ),	Color( 78, 78, 78 ),
	Color( 88, 88, 88 ),	Color( 98, 98, 98 ),	Color( 108, 108, 108 ),
	Color( 118, 118, 118 ), Color( 128, 128, 128 ), Color( 138, 138, 138 ),
	Color( 148, 148, 148 ), Color( 158, 158, 158 ), Color( 168, 168, 168 ),
	Color( 178, 178, 178 ), Color( 188, 188, 188 ), Color( 198, 198, 198 ),
	Color( 208, 208, 208 ), Color( 218, 218, 218 ), Color( 228, 228, 228 ),
	Color( 238, 238, 238 ) };

std::shared_ptr<ETerminalDisplay> ETerminalDisplay::create(
	EE::Window::Window* window, Font* font, const Float& fontSize, const Sizef& pixelsSize,
	std::shared_ptr<TerminalEmulator>&& terminalEmulator, TerminalConfig* config ) {
	std::shared_ptr<ETerminalDisplay> terminal = std::shared_ptr<ETerminalDisplay>(
		new ETerminalDisplay( window, font, fontSize, pixelsSize, config ) );
	terminal->mTerminal = std::move( terminalEmulator );
	return terminal;
}

static EE::System::IProcessFactory* g_processFactory = new EE::System::ProcessFactory();

static Sizei gridSizeFromTermDimensions( Font* font, const Float& fontSize,
										 const Sizef& pixelsSize ) {
	auto fontHeight = (Float)font->getFontHeight( fontSize );
	auto spaceCharAdvanceX = font->getGlyph( 'A', fontSize, false ).advance;
	auto clipColumns =
		(int)std::floor( std::max( 1.0f, pixelsSize.getWidth() / spaceCharAdvanceX ) );
	auto clipRows = (int)std::floor( std::max( 1.0f, pixelsSize.getHeight() / fontHeight ) );
	return { clipColumns, clipRows };
}

std::shared_ptr<ETerminalDisplay>
ETerminalDisplay::create( EE::Window::Window* window, Font* font, const Float& fontSize,
						  const Sizef& pixelsSize, const std::string& program,
						  const std::vector<std::string>& args, const std::string& workingDir,
						  uint32_t options, EE::System::IProcessFactory* processFactory ) {
	using namespace EE::System;

	TerminalConfig config{};
	config.options = options;

	if ( processFactory == nullptr ) {
		processFactory = g_processFactory;
	}

	Sizei termSize( gridSizeFromTermDimensions( font, fontSize, pixelsSize ) );
	std::unique_ptr<IPseudoTerminal> pseudoTerminal = nullptr;
	std::vector<std::string> argsV( args.begin(), args.end() );
	auto process = processFactory->createWithPseudoTerminal(
		program, argsV, workingDir, termSize.getWidth(), termSize.getHeight(), pseudoTerminal );

	if ( !pseudoTerminal ) {
		fprintf( stderr, "Failed to create pseudo terminal\n" );
		return nullptr;
	}

	if ( !process ) {
		fprintf( stderr, "Failed to spawn process\n" );
		return nullptr;
	}

	std::shared_ptr<ETerminalDisplay> terminal = std::shared_ptr<ETerminalDisplay>(
		new ETerminalDisplay( window, font, fontSize, pixelsSize, &config ) );

	terminal->mTerminal =
		TerminalEmulator::create( std::move( pseudoTerminal ), std::move( process ), terminal );
	return terminal;
}

ETerminalDisplay::ETerminalDisplay( EE::Window::Window* window, Font* font, const Float& fontSize,
									const Sizef& pixelsSize, TerminalConfig* config ) :
	TerminalDisplay(),
	mWindow( window ),
	mFont( font ),
	mFontSize( fontSize ),
	mSize( pixelsSize ) {

	if ( config != nullptr ) {
		if ( config->options & OPTION_COLOR_EMOJI )
			mUseColorEmoji = true;
		if ( config->options & OPTION_NO_BOXDRAWING )
			mUseBoxDrawing = false;
		if ( config->options & OPTION_PASTE_CRLF )
			mPasteNewlineFix = true;
	}

	TerminalGlyph defaultGlyph;
	defaultGlyph.mode = ATTR_INVISIBLE;
	auto defaultColor = std::make_pair<Uint32, std::string>( 0U, "" );
	mCursorGlyph = defaultGlyph;
	mColors.resize( eeARRAY_SIZE( colornames ), defaultColor );
	mBuffer.resize( mColumns * mRows, defaultGlyph );
	( (int&)mMode ) |= MODE_FOCUSED;

	if ( config != nullptr ) {
		if ( config->options & OPTION_COLOR_EMOJI )
			mUseColorEmoji = true;
		if ( config->options & OPTION_NO_BOXDRAWING )
			mUseBoxDrawing = false;
		if ( config->options & OPTION_PASTE_CRLF )
			mPasteNewlineFix = true;
	}
}

void ETerminalDisplay::resetColors() {
	for ( Uint32 i = 0; i < eeARRAY_SIZE( colornames ); i++ ) {
		resetColor( i, colornames[i] );
	}
}

int ETerminalDisplay::resetColor( int index, const char* name ) {
	if ( !name ) {
		if ( index >= 0 && index < (int)mColors.size() ) {
			Color col = 0x000000FF;

			if ( index < 256 )
				col = colormapped[index];

			mColors[index].first = col;
			mColors[index].second = "";
			return 0;
		}
	}

	if ( index >= 0 && index < (int)mColors.size() ) {
		mColors[index].first = Color::fromString( name );
		mColors[index].second = name;
	}

	return 1;
}

bool ETerminalDisplay::isBlinkingCursor() {
	return mCursorMode == EE::Terminal::BlinkingBlock ||
		   mCursorMode == EE::Terminal::BlinkingBlockDefault ||
		   mCursorMode == EE::Terminal::BlinkUnderline || mCursorMode == EE::Terminal::BlinkBar;
}

void ETerminalDisplay::update() {
	if ( mFocus && isBlinkingCursor() && mClock.getElapsedTime().asSeconds() > 0.7 ) {
		mMode ^= MODE_BLINK;
		mClock.restart();
		invalidate();
	}
	if ( mTerminal )
		mTerminal->update();
}

void ETerminalDisplay::action( TerminalShortcutAction action ) {
	if ( action == TerminalShortcutAction::PASTE ) {
		auto clipboard = getClipboard();
		auto clipboardLen = strlen( clipboard );
		if ( clipboardLen > 0 ) {
			mTerminal->write( clipboard, clipboardLen );
		}
	}
}

bool ETerminalDisplay::hasTerminated() const {
	return mTerminal->hasExited();
}

void ETerminalDisplay::setTitle( const char* ) {}

void ETerminalDisplay::setIconTitle( const char* ) {}

void ETerminalDisplay::setClipboard( const char* text ) {
	if ( text == nullptr )
		return;
	mClipboard = text;
	mWindow->getClipboard()->setText( mClipboard );
}

const char* ETerminalDisplay::getClipboard() const {
	mClipboard = mWindow->getClipboard()->getText();
	return mClipboard.c_str();
}

bool ETerminalDisplay::drawBegin( int columns, int rows ) {
	if ( columns != mColumns || rows != mRows ) {
		TerminalGlyph defaultGlyph{};
		mBuffer.resize( columns * rows, defaultGlyph );
		mColumns = columns;
		mRows = rows;
		invalidate();
	}

	return ( ( mMode & MODE_VISIBLE ) != 0 );
}

void ETerminalDisplay::drawLine( Line line, int x1, int y, int x2 ) {
	memcpy( &mBuffer[y * mColumns + x1], line, ( x2 - x1 ) * sizeof( TerminalGlyph ) );
	for ( int i = x1; i < x2; i++ ) {
		if ( mTerminal->selected( i, y ) ) {
			mBuffer[y * mColumns + i].mode |= ATTR_REVERSE;
		}
	}
	invalidate();
}

void ETerminalDisplay::drawCursor( int cx, int cy, TerminalGlyph g, int, int, TerminalGlyph ) {
	if ( mCursor != Vector2i( cx, cy ) || mCursorGlyph != g ) {
		mCursor.x = cx;
		mCursor.y = cy;
		mCursorGlyph = g;
		invalidate();
	}
}

void ETerminalDisplay::drawEnd() {}

void ETerminalDisplay::draw() {
	draw( mPosition );
}

void ETerminalDisplay::onMouseMotion( const Vector2i& pos, const Uint32& flags ) {
	if ( ( flags & EE_BUTTON_LMASK ) &&
		 ( mTerminal->getSelectionMode() == TerminalSelectionMode::SEL_EMPTY ||
		   mTerminal->getSelectionMode() == TerminalSelectionMode::SEL_READY ) ) {
		auto gridPos{ positionToGrid( pos ) };
		mTerminal->selextend(
			gridPos.x, gridPos.y,
			mWindow->getInput()->getModState() == KEYMOD_SHIFT ? SEL_RECTANGULAR : SEL_REGULAR, 0 );
		invalidate();
	}
	mTerminal->mousereport( TerminalMouseEventType::MouseMotion, positionToGrid( pos ), flags,
							mWindow->getInput()->getModState() );
}

void ETerminalDisplay::onMouseDown( const Vector2i& pos, const Uint32& flags ) {
	if ( ( flags & EE_BUTTON_LMASK ) &&
		 ( mTerminal->getSelectionMode() == TerminalSelectionMode::SEL_IDLE ) ) {
		auto gridPos{ positionToGrid( pos ) };
		mTerminal->selstart( gridPos.x, gridPos.y, 0 );
		invalidate();
	}
	mTerminal->mousereport( TerminalMouseEventType::MouseButtonDown, positionToGrid( pos ), flags,
							mWindow->getInput()->getModState() );
}

void ETerminalDisplay::onMouseUp( const Vector2i& pos, const Uint32& flags ) {
	if ( ( flags & EE_BUTTON_LMASK ) ) {
		auto selection = mTerminal->getsel();
		if ( selection )
			setClipboard( selection );
		mTerminal->selclear();
		invalidate();
	}
	mTerminal->mousereport( TerminalMouseEventType::MouseButtonRelease, positionToGrid( pos ),
							flags, mWindow->getInput()->getModState() );
}

static inline Color termColor( unsigned int terminalColor,
							   const std::vector<std::pair<Color, std::string>>& colors ) {
	if ( ( terminalColor & ( 1 << 24 ) ) == 0 ) {
		return colors[terminalColor & 0xFF].first;
	}
	return Color( ( terminalColor >> 16 ) & 0xFF, ( terminalColor >> 8 ) & 0xFF,
				  terminalColor & 0xFF, ( ~( ( terminalColor >> 25 ) & 0xFF ) ) & 0xFF );
}

#define BETWEEN( x, a, b ) ( ( a ) <= ( x ) && ( x ) <= ( b ) )
#define IS_SET( flag ) ( ( mMode & ( flag ) ) != 0 )

void ETerminalDisplay::draw( Vector2f pos ) {
	mDrawing = true;

	auto fontSize = (Float)mFont->getFontHeight( mFontSize );
	auto spaceCharAdvanceX = mFont->getGlyph( 'A', mFontSize, false ).advance;

	auto width = mColumns * spaceCharAdvanceX;
	auto height = mRows * fontSize;

	if ( width < mSize.getWidth() ) {
		pos.x = std::floor( pos.x + ( ( mSize.getWidth() - width ) / 2.0f ) );
	}
	if ( height < mSize.getHeight() ) {
		pos.y = std::floor( pos.y + ( ( mSize.getHeight() - height ) / 2.0f ) );
	}

	float x = 0.0f;
	float y = std::floor( pos.y );
	const float lineHeight = fontSize;
	auto defaultFg = termColor( mEmulator->getDefaultForeground(), mColors );
	auto defaultBg = termColor( mEmulator->getDefaultBackground(), mColors );
	auto borderPx = eeceil( PixelDensity::dpToPx( 1.f ) );
	auto cursorThickness = eeceil( PixelDensity::dpToPx( 1.f ) );

	Primitives p;
	p.setForceDraw( false );
	p.setColor( defaultBg );
	p.drawRectangle( Rectf( mPosition.asFloat(), mSize.asFloat() ) );

	for ( int j = 0; j < mRows; j++ ) {
		x = std::floor( pos.x );

		if ( pos.y + lineHeight * j > mSize.getWidth() )
			break;

		for ( int i = 0; i < mColumns; i++ ) {
			auto& glyph = mBuffer[j * mColumns + i];
			auto fg = termColor( glyph.fg, mColors );
			auto bg = termColor( glyph.bg, mColors );

			if ( IS_SET( MODE_REVERSE ) ) {
				fg = fg == defaultFg ? defaultBg : fg.invert();
				bg = bg == defaultBg ? defaultFg : bg.invert();
			}

			if ( ( glyph.mode & ATTR_BOLD_FAINT ) == ATTR_FAINT )
				fg = fg.div( 2 );

			if ( glyph.mode & ATTR_REVERSE )
				bg = fg;

			bool isWide = glyph.mode & ATTR_WIDE;

			auto advanceX = spaceCharAdvanceX * ( isWide ? 2.0f : 1.0f );

			if ( glyph.mode & ATTR_WDUMMY ) {
				continue;
			}

			p.setColor( bg );
			p.drawRectangle( Rectf( { x, y }, { advanceX, lineHeight } ) );

			x += advanceX;
		}

		y += lineHeight;
	}

	y = std::floor( pos.y );

	for ( int j = 0; j < mRows; j++ ) {
		x = std::floor( pos.x );

		if ( pos.y + lineHeight * j > mSize.getWidth() )
			break;

		for ( int i = 0; i < mColumns; i++ ) {
			auto& glyph = mBuffer[j * mColumns + i];
			auto fg = termColor( glyph.fg, mColors );
			auto bg = termColor( glyph.bg, mColors );
			Color temp{ Color::Transparent };

			if ( ( glyph.mode & ATTR_BOLD_FAINT ) == ATTR_BOLD && BETWEEN( glyph.fg, 0, 7 ) )
				fg = termColor( glyph.fg + 8, mColors );

			if ( IS_SET( MODE_REVERSE ) ) {
				fg = fg == defaultFg ? defaultBg : fg.invert();
				bg = bg == defaultBg ? defaultFg : bg.invert();
			}

			if ( ( glyph.mode & ATTR_BOLD_FAINT ) == ATTR_FAINT )
				fg = fg.div( 2 );

			if ( glyph.mode & ATTR_REVERSE ) {
				temp = fg;
				fg = bg;
				bg = temp;
			}

			if ( glyph.mode & ATTR_BLINK && ( mMode & MODE_BLINK ) )
				fg = bg;

			if ( glyph.mode & ATTR_INVISIBLE )
				fg = bg;

			bool isWide = glyph.mode & ATTR_WIDE;

			auto advanceX = spaceCharAdvanceX * ( isWide ? 2.0f : 1.0f );

			if ( glyph.mode & ATTR_WDUMMY )
				continue;

			auto* gd = mFont->getGlyphDrawable( glyph.u, mFontSize, glyph.mode & ATTR_BOLD );
			gd->setColor( fg );
			gd->setDrawMode( glyph.mode & ATTR_ITALIC ? GlyphDrawable::DrawMode::TextItalic
													  : GlyphDrawable::DrawMode::Text );
			gd->draw( { x, y } );

			if ( glyph.mode & ATTR_UNDERLINE ) {
				p.setColor( fg );
				p.drawRectangle( Rectf( { x, y + borderPx + lineHeight - cursorThickness },
										{ advanceX, cursorThickness } ) );
			}

			if ( glyph.mode & ATTR_STRUCK ) {
				p.setColor( fg );
				p.drawRectangle( Rectf( { x, y + borderPx + eefloor( lineHeight / 2.f ) },
										{ advanceX, cursorThickness } ) );
			}

			if ( glyph.mode & ATTR_BOXDRAW && mUseBoxDrawing ) {
				// auto bd = TerminalEmulator::boxdrawindex( &glyph );
				// drawbox( x, y, advanceX, line_height, fg, bg, bd );
			}

			x += advanceX;
		}

		y += lineHeight;
	}

	if ( !IS_SET( MODE_HIDE ) ) {
		Color drawcol;
		if ( IS_SET( MODE_REVERSE ) ) {
			if ( mEmulator->isSelected( mCursor.x, mCursor.y ) ) {
				drawcol = termColor( mEmulator->getDefaultCursorColor(), mColors );
			} else {
				drawcol = termColor( mEmulator->getDefaultReverseCursorColor(), mColors );
			}
		} else {
			drawcol = termColor( mEmulator->isSelected( mCursor.x, mCursor.y )
									 ? mEmulator->getDefaultReverseCursorColor()
									 : mEmulator->getDefaultCursorColor(),
								 mColors );
		}

		Vector2f a{}, b{}, c{}, d{};

		p.setColor( drawcol );
		/* draw the new one */
		if ( IS_SET( MODE_FOCUSED ) ) {
			switch ( mCursorMode ) {
				case 7:						 /* st extension */
					mCursorGlyph.u = 0x2603; /* snowman (U+2603) */
											 /* FALLTHROUGH */
				case 0:						 /* Blinking Block */
				case 1: {					 /* Blinking Block (Default) */
					if ( !( mMode & MODE_BLINK ) )
						break;
				}
				case 2: { /* Steady Block */
					auto* gd = mFont->getGlyphDrawable( mCursorGlyph.u, mFontSize );
					gd->setColor( termColor( mCursorGlyph.fg, mColors ) );
					gd->setDrawMode( GlyphDrawable::DrawMode::Text );
					gd->draw( { pos.x + borderPx + mCursor.x * spaceCharAdvanceX,
								pos.y + borderPx + mCursor.y * lineHeight } );
					break;
				}
				case 3: { /* Blinking Underline */
					if ( !( mMode & MODE_BLINK ) )
						break;
				}
				case 4: /* Steady Underline */
					p.drawRectangle( Rectf(
						{ pos.x + borderPx + mCursor.x * spaceCharAdvanceX,
						  pos.y + borderPx + ( mCursor.y + 1 ) * lineHeight - cursorThickness },
						{ spaceCharAdvanceX, cursorThickness } ) );
					break;
				case 5: { /* Blinking bar */
					if ( !( mMode & MODE_BLINK ) )
						break;
				}
				case 6: /* Steady bar */
				case TerminalCursorMode::MAX_CURSOR:
					p.drawRectangle( Rectf( { pos.x + borderPx + mCursor.x * spaceCharAdvanceX,
											  pos.y + mCursor.y * lineHeight },
											{ spaceCharAdvanceX, lineHeight } ) );
					break;
			}
		} else {
			p.setFillMode( PrimitiveFillMode::DRAW_LINE );
			p.drawRectangle( Rectf( { pos.x + borderPx + mCursor.x * spaceCharAdvanceX,
									  pos.y + borderPx + mCursor.y * lineHeight },
									{ spaceCharAdvanceX, lineHeight } ) );
		}
	}

	mDrawing = false;
	mDirty = false;
}

Vector2i ETerminalDisplay::positionToGrid( const Vector2i& pos ) {
	Vector2f relPos = { pos.x - mPosition.x, pos.y - mPosition.y };
	int mouseX = 0;
	int mouseY = 0;

	auto fontSize = (Float)mFont->getFontHeight( mFontSize );
	auto spaceCharAdvanceX = mFont->getGlyph( 'A', mFontSize, false ).advance;

	auto clipColumns = (int)std::floor( std::max( 1.0f, mSize.getWidth() / spaceCharAdvanceX ) );
	auto clipRows = (int)std::floor( std::max( 1.0f, mSize.getHeight() / fontSize ) );

	if ( pos.x <= 0.0f || pos.y <= 0.0f ) {
		mouseX = 0;
		mouseY = 0;
	} else if ( relPos.x >= 0.0f && relPos.y >= 0.0f ) {
		mouseX = eeclamp( (int)std::floor( relPos.x / spaceCharAdvanceX ), 0, clipColumns );
		mouseY = eeclamp( (int)std::floor( relPos.y / fontSize ), 0, clipRows );
	}

	return { mouseX, mouseY };
}

void ETerminalDisplay::onSizeChange() {
	Sizei gridSize( gridSizeFromTermDimensions( mFont, mFontSize, mSize ) );
	if ( gridSize.getWidth() != mTerminal->getNumColumns() ||
		 gridSize.getHeight() != mTerminal->getNumRows() ) {
		mTerminal->resize( gridSize.getWidth(), gridSize.getHeight() );
	}
}

void ETerminalDisplay::onTextInput( const Uint32& chr ) {
	if ( !mTerminal )
		return;
	String input;
	input.push_back( chr );
	std::string utf8Input( input.toUtf8() );
	mTerminal->write( utf8Input.c_str(), utf8Input.size() );
}

static Uint32 sanitizeMod( const Uint32& mod ) {
	Uint32 smod = 0;
	if ( mod & KEYMOD_CTRL )
		smod |= KEYMOD_CTRL;
	if ( mod & KEYMOD_SHIFT )
		smod |= KEYMOD_SHIFT;
	if ( mod & KEYMOD_META )
		smod |= KEYMOD_META;
	if ( mod & KEYMOD_LALT )
		smod |= KEYMOD_LALT;
	if ( mod & KEYMOD_RALT )
		smod |= KEYMOD_RALT;
	return smod;
}

void ETerminalDisplay::onKeyDown( const Keycode& keyCode, const Uint32& /*chr*/, const Uint32& mod,
								  const Scancode& scancode ) {
	Uint32 smod = sanitizeMod( mod );

	if ( mod & KEYMOD_CTRL ) {
		// I really dont like this, as it depends on the undelying backend implementation (SDL in
		// this case)
		if ( ( scancode >= SCANCODE_A && scancode <= SCANCODE_0 ) ||
			 SCANCODE_LEFTBRACKET == scancode || SCANCODE_RIGHTBRACKET == scancode ) {
			char tmp = 0;
			for ( size_t i = 0; i < eeARRAY_SIZE( asciiScancodeTable ); ++i ) {
				if ( asciiScancodeTable[i] == scancode ) {
					tmp = i + 1;
					break;
				}
			}

			mTerminal->write( &tmp, 1 );
			return;
		}
	}

	auto scIt = terminalKeyMap.Shortcuts().find( keyCode );
	if ( scIt != terminalKeyMap.Shortcuts().end() ) {
		for ( auto& k : scIt->second ) {
			if ( ( k.mask == KEYMOD_CTRL_SHIFT_ALT_META || k.mask == smod ) &&
				 ( k.appkey == 0 || k.appkey < 0 ) && ( k.appcursor == 0 || k.appcursor > 0 ) ) {
				action( k.action );
				return;
			}
		}
	}

	auto kvIt = terminalKeyMap.KeyMap().find( keyCode );
	if ( kvIt != terminalKeyMap.KeyMap().end() ) {
		for ( auto& k : kvIt->second ) {
			if ( ( k.mask == KEYMOD_CTRL_SHIFT_ALT_META || k.mask == smod ) &&
				 ( k.appkey == 0 || k.appkey < 0 ) && ( k.appcursor == 0 || k.appcursor > 0 ) ) {
				if ( k.string.size() > 0 ) {
					mTerminal->write( k.string.c_str(), k.string.size() );
					return;
				}
				break;
			}
		}
	}

	auto pkmIt = terminalKeyMap.PlatformKeyMap().find( scancode );
	if ( pkmIt != terminalKeyMap.PlatformKeyMap().end() ) {
		for ( auto& k : pkmIt->second ) {
			if ( ( k.mask == KEYMOD_CTRL_SHIFT_ALT_META || k.mask == smod ) &&
				 ( k.appkey == 0 || k.appkey < 0 ) && ( k.appcursor == 0 || k.appcursor > 0 ) ) {
				if ( k.string.size() > 0 ) {
					mTerminal->write( k.string.c_str(), k.string.size() );
					return;
				}
				break;
			}
		}
	}
}

Float ETerminalDisplay::getFontSize() const {
	return mFontSize;
}

void ETerminalDisplay::setFontSize( const Float& fontSize ) {
	if ( mFontSize != fontSize ) {
		mFontSize = fontSize;
		onSizeChange();
	}
}

const Vector2f& ETerminalDisplay::getPosition() const {
	return mPosition;
}

void ETerminalDisplay::setPosition( const Vector2f& position ) {
	mPosition = position;
	invalidate();
}

const Sizef& ETerminalDisplay::getSize() const {
	return mSize;
}

void ETerminalDisplay::setSize( const Sizef& size ) {
	if ( mSize != size ) {
		mSize = size;
		onSizeChange();
	}
}

void ETerminalDisplay::invalidate() {
	mDirty = true;
}

void ETerminalDisplay::setFocus( bool focus ) {
	if ( focus == mFocus )
		return;
	mFocus = focus;
	bool modeFocus = mMode & MODE_FOCUSED;
	if ( mFocus != modeFocus ) {
		if ( mFocus ) {
			mMode |= MODE_FOCUSED | MODE_FOCUS;
		} else {
			mMode ^= MODE_FOCUS | MODE_FOCUSED;
		}
	} else {
		mMode ^= MODE_FOCUS;
	}
	invalidate();
}
