#include <eepp/graphics/fontmanager.hpp>
#include <eepp/graphics/fonttruetype.hpp>
#include <eepp/graphics/renderer/renderer.hpp>
#include <eepp/graphics/text.hpp>
#include <eepp/graphics/vertexbuffer.hpp>
#include <eepp/math/math.hpp>
#include <eepp/system/color.hpp>
#include <eepp/window.hpp>
#include <eepp/window/clipboard.hpp>
#include <eterm/system/processfactory.hpp>
#include <eterm/terminal/boxdrawdata.hpp>
#include <eterm/terminal/terminaldisplay.hpp>
#include <limits.h>

namespace eterm { namespace Terminal {

#define BETWEEN( x, a, b ) ( ( a ) <= ( x ) && ( x ) <= ( b ) )
#define IS_SET( flag ) ( ( mMode & ( flag ) ) != 0 )
#define DIV( n, d ) ( ( ( n ) + ( d ) / 2.0f ) / ( d ) )
#define DIVI( n, d ) ( ( ( n ) + ( d ) / 2 ) / ( d ) )

static const Scancode asciiScancodeTable[] = {
	SCANCODE_A, SCANCODE_B, SCANCODE_C,			  SCANCODE_D,	  SCANCODE_E,			SCANCODE_F,
	SCANCODE_G, SCANCODE_H, SCANCODE_I,			  SCANCODE_J,	  SCANCODE_K,			SCANCODE_L,
	SCANCODE_M, SCANCODE_N, SCANCODE_O,			  SCANCODE_P,	  SCANCODE_Q,			SCANCODE_R,
	SCANCODE_S, SCANCODE_T, SCANCODE_U,			  SCANCODE_V,	  SCANCODE_W,			SCANCODE_X,
	SCANCODE_Y, SCANCODE_Z, SCANCODE_LEFTBRACKET, SCANCODE_SLASH, SCANCODE_RIGHTBRACKET };

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

TerminalKeyMap::TerminalKeyMap( const TerminalKey keys[], size_t keysLen,
								const TerminalScancode platformKeys[], size_t platformKeysLen,
								const TerminalShortcut shortcuts[], size_t shortcutsLen,
								const TerminalMouseShortcut mouseShortcuts[],
								size_t mouseShortcutsLen ) {
	mKeyMap.reserve( keysLen );
	for ( size_t i = 0; i < keysLen; i++ ) {
		auto& e = mKeyMap[keys[i].keysym];
		e.push_back( { keys[i].mask, keys[i].string, keys[i].appkey, keys[i].appcursor } );
	}

	mPlatformKeyMap.reserve( platformKeysLen );
	for ( size_t i = 0; i < platformKeysLen; i++ ) {
		auto& e = mPlatformKeyMap[platformKeys[i].scancode];
		e.push_back( { platformKeys[i].mask, platformKeys[i].string, platformKeys[i].appkey,
					   platformKeys[i].appcursor } );
	}

	mShortcuts.reserve( shortcutsLen );
	for ( size_t i = 0; i < shortcutsLen; i++ ) {
		auto& e = mShortcuts[shortcuts[i].keysym];
		e.push_back( { shortcuts[i].mask, shortcuts[i].action, shortcuts[i].appkey,
					   shortcuts[i].appcursor, shortcuts[i].altscrn } );
	}

	mMouseShortcuts.reserve( mouseShortcutsLen );
	for ( size_t i = 0; i < mouseShortcutsLen; i++ ) {
		auto& e = mMouseShortcuts[mouseShortcuts[i].button];
		e.push_back( { mouseShortcuts[i].mask, mouseShortcuts[i].action, mouseShortcuts[i].appkey,
					   mouseShortcuts[i].appcursor, mouseShortcuts[i].altscrn } );
	}
}

static TerminalShortcut shortcuts[] = {
	{ KEY_INSERT, KEYMOD_SHIFT, TerminalShortcutAction::PASTE, 0, 0 },
	{ KEY_V, KEYMOD_SHIFT | KEYMOD_CTRL, TerminalShortcutAction::PASTE, 0, 0 },
	{ KEY_C, KEYMOD_SHIFT | KEYMOD_CTRL, TerminalShortcutAction::COPY, 0, 0 },
	{ KEY_PAGEUP, KEYMOD_SHIFT, TerminalShortcutAction::SCROLLUP_SCREEN, 0, 0, -1 },
	{ KEY_PAGEDOWN, KEYMOD_SHIFT, TerminalShortcutAction::SCROLLDOWN_SCREEN, 0, 0, -1 },
	{ KEY_UP, KEYMOD_SHIFT, TerminalShortcutAction::SCROLLUP_ROW, 0, 0, -1 },
	{ KEY_DOWN, KEYMOD_SHIFT, TerminalShortcutAction::SCROLLDOWN_ROW, 0, 0, -1 },
	{ KEY_HOME, KEYMOD_SHIFT, TerminalShortcutAction::SCROLLUP_HISTORY, 0, 0, -1 },
	{ KEY_END, KEYMOD_SHIFT, TerminalShortcutAction::SCROLLDOWN_HISTORY, 0, 0, -1 },
};

static TerminalMouseShortcut mouseShortcuts[] = {
	{ EE_BUTTON_WUMASK, KEYMOD_SHIFT, TerminalShortcutAction::SCROLLUP_SCREEN, 0, 0, -1 },
	{ EE_BUTTON_WDMASK, KEYMOD_SHIFT, TerminalShortcutAction::SCROLLDOWN_SCREEN, 0, 0, -1 },
	{ EE_BUTTON_WUMASK, 0, TerminalShortcutAction::SCROLLUP_ROW, 0, 0, -1 },
	{ EE_BUTTON_WDMASK, 0, TerminalShortcutAction::SCROLLDOWN_ROW, 0, 0, -1 },
	{ EE_BUTTON_WUMASK, KEYMOD_CTRL, TerminalShortcutAction::FONTSIZE_GROW, 0, 0, 0 },
	{ EE_BUTTON_WDMASK, KEYMOD_CTRL, TerminalShortcutAction::FONTSIZE_SHRINK, 0, 0, 0 } };

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
	{ KEY_DELETE, KEYMOD_CTRL_SHIFT_ALT_META, "\033[3~", -1, 0 },
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
	{ SCANCODE_RETURN, KEYMOD_CTRL_SHIFT_ALT_META, "\r", 0, 0 },
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

TerminalKeyMap terminalKeyMap{
	keys,	   eeARRAY_SIZE( keys ),	  platformKeys,	  eeARRAY_SIZE( platformKeys ),
	shortcuts, eeARRAY_SIZE( shortcuts ), mouseShortcuts, eeARRAY_SIZE( mouseShortcuts ) };

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

std::shared_ptr<TerminalDisplay> TerminalDisplay::create(
	EE::Window::Window* window, Font* font, const Float& fontSize, const Sizef& pixelsSize,
	std::shared_ptr<TerminalEmulator>&& terminalEmulator, const bool& useFrameBuffer ) {
	std::shared_ptr<TerminalDisplay> terminal = std::shared_ptr<TerminalDisplay>(
		new TerminalDisplay( window, font, fontSize, pixelsSize, useFrameBuffer ) );
	terminal->mTerminal = std::move( terminalEmulator );
	return terminal;
}

static Sizei gridSizeFromTermDimensions( Font* font, const Float& fontSize,
										 const Sizef& pixelsSize ) {
	auto fontHeight = (Float)font->getFontHeight( fontSize );
	auto spaceCharAdvanceX = font->getGlyph( 'A', fontSize, false, false ).advance;
	auto clipColumns =
		(int)std::floor( std::max( 1.0f, pixelsSize.getWidth() / spaceCharAdvanceX ) );
	auto clipRows = (int)std::floor( std::max( 1.0f, pixelsSize.getHeight() / fontHeight ) );
	return { clipColumns, clipRows };
}

std::shared_ptr<TerminalDisplay> TerminalDisplay::create(
	EE::Window::Window* window, Font* font, const Float& fontSize, const Sizef& pixelsSize,
	std::string program, std::vector<std::string> args, const std::string& workingDir,
	const size_t& historySize, IProcessFactory* processFactory, bool useFrameBuffer, bool keepAlive,
	const std::unordered_map<std::string, std::string>& env ) {
	if ( program.empty() ) {
		const char* shellenv = getenv( "SHELL" );
#ifdef _WIN32
		program = shellenv != nullptr ? shellenv : "powershell.exe";
#else
#if EE_PLATFORM == EE_PLATFORM_ANDROID
		program = shellenv != nullptr ? shellenv : "/bin/sh";
#else
		program = shellenv != nullptr ? shellenv : "/bin/bash";
#endif

#endif
	}

	if ( program.find_first_of( ' ' ) != std::string::npos ) {
		auto programSplit = String::split( program, ' ' );
		if ( !programSplit.empty() ) {
			program = programSplit[0];
			for ( size_t i = 1; i < programSplit.size(); ++i )
				args.push_back( programSplit[i] );
		}
	}

	bool freeProcessFactory = processFactory == nullptr;
	if ( processFactory == nullptr )
		processFactory = eeNew( ProcessFactory, () );

	Sizei termSize( gridSizeFromTermDimensions( font, fontSize, pixelsSize ) );
	std::unique_ptr<IPseudoTerminal> pseudoTerminal = nullptr;
	auto process = processFactory->createWithPseudoTerminal(
		program, args, workingDir, termSize.getWidth(), termSize.getHeight(), pseudoTerminal, env );

	if ( !pseudoTerminal ) {
		fprintf( stderr, "Failed to create pseudo terminal\n" );
		Log::error( "Failed to create pseudo terminal" );
		if ( freeProcessFactory )
			eeSAFE_DELETE( processFactory );
		return nullptr;
	}

	if ( !process ) {
		fprintf( stderr, "Failed to spawn process\n" );
		Log::error( "Failed to spawn process" );
		if ( freeProcessFactory )
			eeSAFE_DELETE( processFactory );
		return nullptr;
	}

	std::shared_ptr<TerminalDisplay> terminal = std::shared_ptr<TerminalDisplay>(
		new TerminalDisplay( window, font, fontSize, pixelsSize, useFrameBuffer ) );

	terminal->mTerminal = TerminalEmulator::create( std::move( pseudoTerminal ),
													std::move( process ), terminal, historySize );
	terminal->mProgram = program;
	terminal->mArgs = args;
	terminal->mEnv = env;
	terminal->mWorkingDir = workingDir;
	terminal->mKeepAlive = keepAlive;

	if ( freeProcessFactory )
		eeSAFE_DELETE( processFactory );

	return terminal;
}

TerminalDisplay::~TerminalDisplay() {
	eeSAFE_DELETE( mVBBackground );
	eeSAFE_DELETE( mVBForeground );
	for ( VertexBuffer* vb : mVBStyles )
		eeSAFE_DELETE( vb );
	eeSAFE_DELETE( mFrameBuffer );
}

TerminalDisplay::TerminalDisplay( EE::Window::Window* window, Font* font, const Float& fontSize,
								  const Sizef& pixelsSize, const bool& useFrameBuffer ) :
	ITerminalDisplay(),
	mWindow( window ),
	mFont( font ),
	mFontSize( fontSize ),
	mSize( pixelsSize ),
	mUseFrameBuffer( useFrameBuffer ),
	mColorScheme( TerminalColorScheme::getDefault() ) {
	TerminalGlyph defaultGlyph;
	defaultGlyph.mode = ATTR_INVISIBLE;
	mCursorGlyph = defaultGlyph;
	mColors.resize( eeARRAY_SIZE( colormapped ), Color::Transparent );
	mBuffer.resize( mColumns * mRows, defaultGlyph );
	( (int&)mMode ) |= MODE_FOCUSED;

	Sizei gridSize( gridSizeFromTermDimensions( mFont, mFontSize, mSize - mPadding * 2.f ) );
	mDirtyLines.resize( gridSize.getHeight(), 1 );

	mQuadVertex = GLi->quadVertex();

	if ( mUseFrameBuffer )
		createFrameBuffer();
	else
		initVBOs();
}

void TerminalDisplay::resetColors() {
	for ( Uint32 i = 0; i < eeARRAY_SIZE( colormapped ); i++ )
		resetColor( i, i < mColorScheme.getPaletteSize()
						   ? mColorScheme.getPaletteIndex( i ).toHexString().c_str()
						   : nullptr );
}

int TerminalDisplay::resetColor( const Uint32& index, const char* name ) {
	if ( !name && index < mColors.size() ) {
		Color col = 0x000000FF;

		if ( index < 256 )
			col = colormapped[index];

		mColors[index] = col;
		return 0;
	}

	if ( index < mColors.size() ) {
		if ( name && String::startsWith( name, "rgb:" ) ) {
			auto split = String::split( std::string( name ), ':' );
			if ( split.size() == 2 ) {
				auto splitRgb = String::split( split[1], '/' );
				if ( splitRgb.size() == 3 ) {
					char* pr = NULL;
					char* pg = NULL;
					char* pb = NULL;
					long r = 0, g = 0, b = 0;
					r = std::strtol( splitRgb[0].c_str(), &pr, 16 );
					g = std::strtol( splitRgb[1].c_str(), &pg, 16 );
					b = std::strtol( splitRgb[2].c_str(), &pb, 16 );
					if ( pr && pg && pb ) {
						mColors[index] = Color( r, g, b );
						return 0;
					}
				}
			}
		} else {
			mColors[index] = Color::fromString( name );
			return 0;
		}
	}

	return 1;
}

bool TerminalDisplay::isBlinkingCursor() {
	return mCursorMode == Terminal::BlinkingBlock ||
		   mCursorMode == Terminal::BlinkingBlockDefault ||
		   mCursorMode == Terminal::BlinkUnderline || mCursorMode == Terminal::BlinkBar;
}

const Rectf& TerminalDisplay::getPadding() const {
	return mPadding;
}

void TerminalDisplay::setPadding( const Rectf& padding ) {
	if ( mPadding != padding.ceil() ) {
		mPadding = padding.ceil();
		onSizeChange();
	}
}

const std::shared_ptr<TerminalEmulator>& TerminalDisplay::getTerminal() const {
	return mTerminal;
}

void TerminalDisplay::attach( TerminalEmulator* terminal ) {
	ITerminalDisplay::attach( terminal );
	onSizeChange();
}

int TerminalDisplay::scrollSize() const {
	return mEmulator ? mEmulator->scrollSize() : 0;
}

int TerminalDisplay::rowCount() const {
	return mEmulator ? mEmulator->rowCount() : 0;
}

void TerminalDisplay::sendEvent( const Event& event ) {
	for ( auto it : mCallbacks )
		it.second( event );
}

Uint32 TerminalDisplay::pushEventCallback( const EventFunc& func ) {
	mCallbacks[++mNumCallBacks] = func;
	return mNumCallBacks;
}

void TerminalDisplay::popEventCallback( const Uint32& id ) {
	auto it = mCallbacks.find( id );
	if ( it != mCallbacks.end() )
		mCallbacks.erase( it );
}

Float TerminalDisplay::getLineHeight() const {
	return mFont->getFontHeight( mFontSize );
}

const TerminalColorScheme& TerminalDisplay::getColorScheme() const {
	return mColorScheme;
}

void TerminalDisplay::setColorScheme( const TerminalColorScheme& colorScheme ) {
	mColorScheme = colorScheme;
	resetColors();
	invalidateLines();
}

bool TerminalDisplay::isAppCapturingMouse() const {
	return mTerminal &&
		   ( mMode & ( MODE_MOUSEX10 | MODE_MOUSEBTN | MODE_MOUSEMOTION | MODE_MOUSEMANY ) );
}

bool TerminalDisplay::isAltScr() const {
	return mEmulator && mEmulator->tisaltscr();
}

const Uint32& TerminalDisplay::getClickStep() const {
	return mClickStep;
}

void TerminalDisplay::setClickStep( const Uint32& clickStep ) {
	mClickStep = clickStep;
}

bool TerminalDisplay::getKeepAlive() const {
	return mKeepAlive;
}

void TerminalDisplay::setKeepAlive( bool keepAlive ) {
	mKeepAlive = keepAlive;
}

bool TerminalDisplay::update() {
	bool ret = true;
	if ( mFocus && isBlinkingCursor() && mClock.getElapsedTime().asSeconds() > 0.7 ) {
		mMode ^= MODE_BLINK;
		mClock.restart();
		invalidateCursor();
	}
	if ( mTerminal ) {
		int histi = mTerminal->getHistorySize();
		ret = mTerminal->update();
		if ( histi != mTerminal->getHistorySize() )
			sendEvent( { EventType::HISTORY_LENGTH_CHANGE } );
	}
	return ret;
}

void TerminalDisplay::executeFile( const std::string& cmd ) {
	if ( mTerminal ) {
		std::string rcmd( cmd + "\r" );
#if EE_PLATFORM != EE_PLATFORM_WIN
		char clearLine = 0x15;
		mTerminal->ttywrite( &clearLine, 1, 1 );
#endif
		mTerminal->ttywrite( rcmd.c_str(), rcmd.size(), 1 );
	}
}

void TerminalDisplay::executeBinary( const std::string& binaryPath, const std::string& args ) {
	if ( mTerminal ) {
		std::string rcmd( "\"" + binaryPath + "\"" + " " + args + "\r" );
#if EE_PLATFORM != EE_PLATFORM_WIN
		char clearLine = 0x15;
		mTerminal->ttywrite( &clearLine, 1, 1 );
#endif
		mTerminal->ttywrite( rcmd.c_str(), rcmd.size(), 1 );
	}
}

void TerminalDisplay::action( TerminalShortcutAction action ) {
	switch ( action ) {
		case TerminalShortcutAction::PASTE: {
			getClipboard();
			if ( !mClipboard.empty() )
				mTerminal->ttywrite( mClipboardUtf8.c_str(), mClipboardUtf8.size(), 1 );
			break;
		}
		case TerminalShortcutAction::COPY: {
			auto selection = mTerminal->getSelection();
			if ( !selection.empty() )
				setClipboard( selection.c_str() );
			break;
		}
		case TerminalShortcutAction::SCROLLUP_SCREEN: {
			TerminalArg arg( (int)-mClickStep );
			mTerminal->kscrollup( &arg );
			sendEvent( { EventType::SCROLL_HISTORY } );
			break;
		}
		case TerminalShortcutAction::SCROLLDOWN_SCREEN: {
			TerminalArg arg( (int)-mClickStep );
			mTerminal->kscrolldown( &arg );
			sendEvent( { EventType::SCROLL_HISTORY } );
			break;
		}
		case TerminalShortcutAction::SCROLLUP_ROW: {
			TerminalArg arg( (int)mClickStep );
			mTerminal->kscrollup( &arg );
			sendEvent( { EventType::SCROLL_HISTORY } );
			break;
		}
		case TerminalShortcutAction::SCROLLDOWN_ROW: {
			TerminalArg arg( (int)mClickStep );
			mTerminal->kscrolldown( &arg );
			sendEvent( { EventType::SCROLL_HISTORY } );
			break;
		}
		case TerminalShortcutAction::SCROLLUP_HISTORY: {
			TerminalArg arg( (int)INT_MAX );
			mTerminal->kscrollup( &arg );
			sendEvent( { EventType::SCROLL_HISTORY } );
			break;
		}
		case TerminalShortcutAction::SCROLLDOWN_HISTORY: {
			TerminalArg arg( (int)INT_MAX );
			mTerminal->kscrolldown( &arg );
			sendEvent( { EventType::SCROLL_HISTORY } );
			break;
		}
		case TerminalShortcutAction::FONTSIZE_GROW: {
			setFontSize( getFontSize() + 1 );
			break;
		}
		case TerminalShortcutAction::FONTSIZE_SHRINK: {
			setFontSize( getFontSize() - 1 );
			break;
		}
	}
}

bool TerminalDisplay::hasTerminated() const {
	return mTerminal->hasExited();
}

void TerminalDisplay::setTitle( const char* title ) {
	if ( title )
		sendEvent( { EventType::TITLE, std::string( title ) } );
}

void TerminalDisplay::setIconTitle( const char* title ) {
	if ( title )
		sendEvent( { EventType::ICON_TITLE, std::string( title ) } );
}

void TerminalDisplay::setClipboard( const char* text ) {
	if ( text == nullptr )
		return;
	mClipboard = text;
	mClipboardUtf8 = mClipboard.toUtf8();
	mWindow->getClipboard()->setText( mClipboard );
}

const char* TerminalDisplay::getClipboard() const {
	mClipboard = mWindow->getClipboard()->getText();
#ifdef _WIN32
	if ( mPasteNewlineFix ) {
		size_t pos;
		while ( ( pos = mClipboard.find( '\r' ) ) != std::string::npos )
			mClipboard.erase( pos, 1 );
	}
#endif
	mClipboardUtf8 = mClipboard.toUtf8();
	return mClipboardUtf8.c_str();
}

bool TerminalDisplay::drawBegin( Uint32 columns, Uint32 rows ) {
	if ( columns != mColumns || rows != mRows ) {
		TerminalGlyph defaultGlyph{};
		mBuffer.resize( columns * rows, defaultGlyph );
		mColumns = columns;
		mRows = rows;

		if ( !mUseFrameBuffer )
			initVBOs();

		invalidateLines();
		invalidateCursor();
	}

	return ( ( mMode & MODE_VISIBLE ) != 0 );
}

void TerminalDisplay::drawLine( Line line, int x1, int y, int x2 ) {
	memcpy( &mBuffer[y * mColumns + x1], line, ( x2 - x1 ) * sizeof( TerminalGlyph ) );
	for ( int i = x1; i < x2; i++ ) {
		if ( mTerminal->selected( i, y ) ) {
			mBuffer[y * mColumns + i].mode |= ATTR_REVERSE;
		}
	}
	invalidateLine( y );
}

void TerminalDisplay::drawCursor( int cx, int cy, TerminalGlyph g, int, int, TerminalGlyph ) {
	if ( mCursor != Vector2i( cx, cy ) || mCursorGlyph != g ) {
		mCursor.x = cx;
		mCursor.y = cy;
		if ( isBlinkingCursor() ) {
			mMode |= MODE_BLINK;
			mClock.restart();
		}
		mCursorGlyph = g;
		invalidateCursor();
	}
}

void TerminalDisplay::drawEnd() {}

void TerminalDisplay::draw() {
	draw( nullptr != mFrameBuffer ? Vector2f( mPadding.Left, mPadding.Top )
								  : mPosition.floor() + Vector2f( mPadding.Left, mPadding.Top ) );
}

void TerminalDisplay::onMouseDoubleClick( const Vector2i& pos, const Uint32& flags ) {
	if ( flags & EE_BUTTON_LMASK )
		mLastDoubleClick.restart();

	if ( !isAppCapturingMouse() && ( flags & EE_BUTTON_LMASK ) &&
		 ( mTerminal->getSelectionMode() == TerminalSelectionMode::SEL_EMPTY ||
		   mTerminal->getSelectionMode() == TerminalSelectionMode::SEL_IDLE ) ) {
		auto gridPos{ positionToGrid( pos ) };
		mTerminal->selstart( gridPos.x, gridPos.y, SNAP_WORD );
		invalidateLines();
	}
}

void TerminalDisplay::onMouseMove( const Vector2i& pos, const Uint32& flags ) {
	bool shiftPressed = ( mWindow->getInput()->getModState() & KEYMOD_SHIFT ) != 0;
	bool isCapturingMouse = isAppCapturingMouse() && !shiftPressed;

	if ( !isCapturingMouse && ( flags & EE_BUTTON_LMASK ) &&
		 ( mTerminal->getSelectionMode() == TerminalSelectionMode::SEL_EMPTY ||
		   mTerminal->getSelectionMode() == TerminalSelectionMode::SEL_READY ) ) {
		auto gridPos{ positionToGrid( pos ) };
		mTerminal->selextend(
			gridPos.x, gridPos.y,
			mWindow->getInput()->getModState() & KEYMOD_SHIFT ? SEL_RECTANGULAR : SEL_REGULAR, 0 );
		invalidateLines();
	}
	mTerminal->mousereport( TerminalMouseEventType::MouseMotion, positionToGrid( pos ), flags,
							mWindow->getInput()->getModState() );
}

void TerminalDisplay::onMouseDown( const Vector2i& pos, const Uint32& flags ) {
	bool shiftPressed = ( mWindow->getInput()->getModState() & KEYMOD_SHIFT ) != 0;
	bool isCapturingMouse = isAppCapturingMouse() && !shiftPressed;

	if ( ( flags & EE_BUTTON_LMASK ) && mDraggingSel )
		return;

	auto gridPos{ positionToGrid( pos ) };

	if ( !isCapturingMouse && ( flags & EE_BUTTON_LMASK ) &&
		 mLastDoubleClick.getElapsedTime() < Milliseconds( 300.f ) ) {
		mTerminal->selstart( gridPos.x, gridPos.y, SNAP_LINE );
	} else if ( !isCapturingMouse && ( flags & EE_BUTTON_LMASK ) ) {
		if ( !mDraggingSel ) {
			mTerminal->selstart( gridPos.x, gridPos.y, 0 );
			mDraggingSel = true;
			invalidateLines();
		}
	} else if ( flags & EE_BUTTON_MMASK ) {
		if ( !mAlreadyClickedMButton ) {
			mAlreadyClickedMButton = true;
			auto selection = mTerminal->getSelection();
			if ( !selection.empty() && selection != "\n" ) {
				for ( auto& chr : selection )
					onTextInput( chr );
			} else {
				getClipboard();
				if ( !mClipboard.empty() ) {
					for ( auto& chr : mClipboard )
						onTextInput( chr );
				}
			}
		}
	}
	if ( ( flags & EE_BUTTON_LMASK ) ) {
		if ( !mAlreadyClickedLButton ) {
			mAlreadyClickedLButton = true;
		} else {
			return;
		}
	}
	mTerminal->mousereport( TerminalMouseEventType::MouseButtonDown, positionToGrid( pos ), flags,
							mWindow->getInput()->getModState() );
}

void TerminalDisplay::onMouseUp( const Vector2i& pos, const Uint32& flags ) {
	if ( ( flags & EE_BUTTON_LMASK ) && mDraggingSel ) {
		mDraggingSel = false;
	}

	Uint32 smod = sanitizeMod( mWindow->getInput()->getModState() );

	if ( flags & EE_BUTTON_LMASK )
		mAlreadyClickedLButton = false;

	if ( flags & EE_BUTTON_MMASK )
		mAlreadyClickedMButton = false;

	auto scIt = terminalKeyMap.MouseShortcuts().find( flags );
	if ( scIt != terminalKeyMap.MouseShortcuts().end() ) {
		for ( auto& k : scIt->second ) {
			Uint32 kmask = sanitizeMod( k.mask );

			if ( kmask != smod )
				continue;

			if ( IS_SET( MODE_APPKEYPAD ) ? k.appkey < 0 : k.appkey > 0 )
				continue;

			if ( IS_SET( MODE_NUMLOCK ) && k.appkey == 2 )
				continue;

			if ( IS_SET( MODE_APPCURSOR ) ? k.appcursor < 0 : k.appcursor > 0 )
				continue;

			if ( !k.altscrn || ( k.altscrn == ( mEmulator->tisaltscr() ? 1 : -1 ) ) ) {
				action( k.action );
				return;
			}
		}
	}

	mTerminal->mousereport( TerminalMouseEventType::MouseButtonRelease, positionToGrid( pos ),
							flags, mWindow->getInput()->getModState() );
}

static inline Color termColor( unsigned int terminalColor, const std::vector<Color>& colors ) {
	if ( ( terminalColor & ( 1 << 24 ) ) == 0 ) {
		return colors[terminalColor & 0xFF];
	}
	return Color( ( terminalColor >> 16 ) & 0xFF, ( terminalColor >> 8 ) & 0xFF,
				  terminalColor & 0xFF, ( ~( ( terminalColor >> 25 ) & 0xFF ) ) & 0xFF );
}

void TerminalDisplay::drawrect( const Color& col, const float& x, const float& y, const float& w,
								const float& h ) {
	if ( mVBStyles.empty() ) {
		mPrimitives.setColor( col );
		mPrimitives.drawRectangle(
			{ { eefloor( x ), eefloor( y ) }, { eeceil( w ), eeceil( h ) } } );
	} else {
		mVBStyles[mCurGridPos.y]->addQuad( { eefloor( x ), eefloor( y ) },
										   { eeceil( w ), eeceil( h ) }, col );
	}
}

void TerminalDisplay::drawpoint( const Color& col, const float& x, const float& y, const float& w,
								 const float& h ) {
	if ( mVBStyles.empty() ) {
		mPrimitives.setColor( col );
		mPrimitives.drawPoint( { eefloor( x ), eefloor( y ) }, eemin( w / 2.f, h / 2.f ) );
	} else {
		Float s = eemin( w / 2.f, h / 2.f );
		mVBStyles[mCurGridPos.y]->addQuad( { eefloor( x ), eefloor( y ) },
										   { eefloor( s ), eefloor( s ) }, col );
	}
}

void TerminalDisplay::drawboxlines( float x, float y, float w, float h, Color fg, ushort bd ) {
	/* s: stem thickness. width/8 roughly matches underscore thickness. */
	/* We draw bold as 1.5 * normal-stem and at least 1px thicker.      */
	/* doubles draw at least 3px, even when w or h < 3. bold needs 6px. */
	float mwh = eemin( w, h );
	float base_s = eemin( 1.0f, DIV( mwh, 8.0f ) );
	int bold = ( bd & BDB ) && mwh >= 6.0f; /* possibly ignore boldness */
	float s = bold ? eemax( base_s + 1.0f, DIV( 3.0f * base_s, 2.0f ) ) : base_s;
	float w2 = DIV( w - s, 2.0f ), h2 = DIV( h - s, 2.0f );
	/* the s-by-s square (x + w2, y + h2, s, s) is the center texel.    */
	/* The base length (per direction till edge) includes this square.  */

	int light = bd & ( LL | LU | LR | LD );
	int double_ = bd & ( DL | DU | DR | DD );

	if ( light ) {
		/* d: additional (negative) length to not-draw the center   */
		/* texel - at arcs and avoid drawing inside (some) doubles  */
		int arc = bd & BDA;
		int multi_light = light & ( light - 1 );
		int multi_double = double_ & ( double_ - 1 );
		/* light crosses double only at DH+LV, DV+LH (ref. shapes)  */
		float d = arc || ( multi_double && !multi_light ) ? -s : 0.0f;

		if ( bd & LL )
			drawrect( fg, x, y + h2, w2 + s + d, s );
		if ( bd & LU )
			drawrect( fg, x + w2, y, s, h2 + s + d );
		if ( bd & LR )
			drawrect( fg, x + w2 - d, y + h2, w - w2 + d, s );
		if ( bd & LD )
			drawrect( fg, x + w2, y + h2 - d, s, h - h2 + d );
	}

	/* double lines - also align with light to form heavy when combined */
	if ( double_ ) {
		/*
		 * going clockwise, for each double-ray: p is additional length
		 * to the single-ray nearer to the previous direction, and n to
		 * the next. p and n adjust from the base length to lengths
		 * which consider other doubles - shorter to avoid intersections
		 * (p, n), or longer to draw the far-corner texel (n).
		 */
		int dl = bd & DL, du = bd & DU, dr = bd & DR, dd = bd & DD;
		if ( dl ) {
			float p = dd ? -s : 0.0f, n = du ? -s : dd ? s : 0.0f;
			drawrect( fg, x, y + h2 + s, w2 + s + p, s );
			drawrect( fg, x, y + h2 - s, w2 + s + n, s );
		}
		if ( du ) {
			float p = dl ? -s : 0.0f, n = dr ? -s : dl ? s : 0.0f;
			drawrect( fg, x + w2 - s, y, s, h2 + s + p );
			drawrect( fg, x + w2 + s, y, s, h2 + s + n );
		}
		if ( dr ) {
			float p = du ? -s : 0.0f, n = dd ? -s : du ? s : 0.0f;
			drawrect( fg, x + w2 - p, y + h2 - s, w - w2 + p, s );
			drawrect( fg, x + w2 - n, y + h2 + s, w - w2 + n, s );
		}
		if ( dd ) {
			float p = dr ? -s : 0.0f, n = dl ? -s : dr ? s : 0.0f;
			drawrect( fg, x + w2 + s, y + h2 - p, s, h - h2 + p );
			drawrect( fg, x + w2 - s, y + h2 - n, s, h - h2 + n );
		}
	}
}

void TerminalDisplay::drawbox( float x, float y, float w, float h, Color fg, Color bg, ushort bd ) {
	ushort cat = bd & ~( BDB | 0xff ); /* mask out bold and data */
	if ( bd & ( BDL | BDA ) ) {
		/* lines (light/double/heavy/arcs) */
		drawboxlines( x, y, w, h, fg, bd );
	} else if ( cat == BBD ) {
		/* lower (8-X)/8 block */
		float d = DIV( ( bd & 0xFF ) * h, 8.0f );
		drawrect( fg, x, y + d, w, h - d );
	} else if ( cat == BBU ) {
		/* upper X/8 block */
		drawrect( fg, x, y, w, DIV( ( bd & 0xFF ) * h, 8 ) );
	} else if ( cat == BBL ) {
		/* left X/8 block */
		drawrect( fg, x, y, DIV( ( bd & 0xFF ) * w, 8 ), h );
	} else if ( cat == BBR ) {
		/* right (8-X)/8 block */
		float d = DIV( ( bd & 0xFF ) * w, 8.0f );
		drawrect( fg, x + d, y, w - d, h );
	} else if ( cat == BBQ ) {
		/* Quadrants */
		float w2 = DIV( w, 2.0f ), h2 = DIV( h, 2.0f );
		if ( bd & TL )
			drawrect( fg, x, y, w2, h2 );
		if ( bd & TR )
			drawrect( fg, x + w2, y, w - w2, h2 );
		if ( bd & BL )
			drawrect( fg, x, y + h2, w2, h - h2 );
		if ( bd & BR )
			drawrect( fg, x + w2, y + h2, w - w2, h - h2 );
	} else if ( bd & BBS ) {
		/* Shades - data is 1/2/3 for 25%/50%/75% alpha, respectively */
		int d = ( bd & 0xFF );

		Uint8 red = DIVI( ( ( fg.getValue() >> 24 ) & 0xFF ) * d +
							  ( ( bg.getValue() >> 24 ) & 0xFF ) * ( 4 - d ),
						  4 );
		Uint8 green = DIVI( ( ( fg.getValue() >> 16 ) & 0xFF ) * d +
								( ( bg.getValue() >> 16 ) & 0xFF ) * ( 4 - d ),
							4 );
		Uint8 blue = DIVI( ( ( fg.getValue() >> 8 ) & 0xFF ) * d +
							   ( ( bg.getValue() >> 8 ) & 0xFF ) * ( 4 - d ),
						   4 );
		Color drawcol = Color( red, green, blue, 0xFF );

		drawrect( drawcol, x, y, w, h );
	} else if ( cat == BRL ) {
		/* braille, each data bit corresponds to one dot at 2x4 grid */
		float w1 = DIV( w, 2.0f );
		float h1 = DIV( h, 4.0f ), h2 = DIV( h, 2.0f ), h3 = DIV( 3.0f * h, 4.0f );

		if ( bd & 1 )
			drawpoint( fg, x, y, w1, h1 );
		if ( bd & 2 )
			drawpoint( fg, x, y + h1, w1, h2 - h1 );
		if ( bd & 4 )
			drawpoint( fg, x, y + h2, w1, h3 - h2 );
		if ( bd & 8 )
			drawpoint( fg, x + w1, y, w - w1, h1 );
		if ( bd & 16 )
			drawpoint( fg, x + w1, y + h1, w - w1, h2 - h1 );
		if ( bd & 32 )
			drawpoint( fg, x + w1, y + h2, w - w1, h3 - h2 );
		if ( bd & 64 )
			drawpoint( fg, x, y + h3, w1, h - h3 );
		if ( bd & 128 )
			drawpoint( fg, x + w1, y + h3, w - w1, h - h3 );
	}
}

void TerminalDisplay::drawGrid( const Vector2f& pos ) {
	if ( mFrameBuffer ) {
		mFrameBuffer->setPosition( mPosition.floor() + Vector2f( mPadding.Left, mPadding.Top ) );
		mFrameBuffer->bind();

		if ( mFullDirty )
			drawBg( true );
	}

	auto fontSize = mFont->getFontHeight( mFontSize );
	auto spaceCharAdvanceX = mFont->getGlyph( 'A', mFontSize, false, false ).advance;

	float x = 0.0f;
	float y = pos.y;
	const float lineHeight = fontSize;
	auto defaultFg = mColorScheme.getForeground();
	auto defaultBg = mColorScheme.getBackground();
	auto cursorThickness = Math::roundDown( PixelDensity::dpToPx( 1.f ) );
	Rectf xBounds = mFont->getGlyph( L'x', mFontSize, false, false ).bounds;
	Float strikeThroughOffset = lineHeight + xBounds.Top + cursorThickness;

	mPrimitives.setForceDraw( false );

	bool dirtyBG = false;
	if ( mVBBackground )
		mVBBackground->bind();

	for ( Uint32 j = 0; j < mRows; j++ ) {
		x = std::floor( pos.x );

		if ( pos.y + lineHeight * j > pos.y + mSize.getHeight() )
			break;

		if ( !mDirtyLines[j] ) {
			y += lineHeight;
			continue;
		}

		for ( Uint32 i = 0; i < mColumns; i++ ) {
			mCurGridPos = { i, j };
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
				if ( mVBBackground )
					mVBBackground->setQuad( mCurGridPos, { x, y }, { advanceX, lineHeight },
											Color::Transparent );
				continue;
			}

			if ( mVBBackground ) {
				mVBBackground->setQuad( mCurGridPos, { x, y }, { advanceX, lineHeight }, bg );
				dirtyBG = true;
			} else {
				mPrimitives.setColor( bg );
				mPrimitives.drawRectangle( Rectf( { x, y }, { advanceX, lineHeight } ) );
			}

			x += advanceX;
		}

		y += lineHeight;

		if ( j == (Uint32)mCursor.y )
			invalidateCursor();
	}

	if ( mVBBackground ) {
		if ( dirtyBG )
			mVBBackground->update( VERTEX_FLAGS_PRIMITIVE, false );
		mVBBackground->draw();
		mVBBackground->unbind();
	}

	y = std::floor( pos.y );

	bool dirtyFG = false;

	for ( Uint32 j = 0; j < mRows; j++ ) {
		x = std::floor( pos.x );

		if ( pos.y + lineHeight * j > pos.y + mSize.getHeight() )
			break;

		if ( ( mFrameBuffer || mVBForeground ) && !mDirtyLines[j] ) {
			y += lineHeight;
			continue;
		}

		mDirtyLines[j] = false;

		if ( !mVBStyles.empty() )
			mVBStyles[j]->clearData();

		for ( Uint32 i = 0; i < mColumns; i++ ) {
			mCurGridPos = { i, j };
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

			if ( glyph.mode & ATTR_WDUMMY ) {
				if ( mVBForeground )
					mVBForeground->setQuadColor( mCurGridPos, Color::Transparent );
				continue;
			}

			if ( glyph.u == 32 ) {
				x += advanceX;
				if ( mVBForeground )
					mVBForeground->setQuadColor( mCurGridPos, Color::Transparent );
				continue;
			}

			if ( glyph.mode & ATTR_BOXDRAW ) {
				auto bd = TerminalEmulator::boxdrawindex( &glyph );
				drawbox( x, y, advanceX, lineHeight, fg, bg, bd );
				if ( mVBForeground )
					mVBForeground->setQuadColor( mCurGridPos, Color::Transparent );
			} else {
				auto* gd = mFont->getGlyphDrawable( glyph.u, mFontSize, glyph.mode & ATTR_BOLD,
													glyph.mode & ATTR_ITALIC, 0 );

				if ( ( glyph.mode & ATTR_EMOJI ) && FontManager::instance()->getColorEmojiFont() ) {
					gd->setColor( Color::White );
				} else {
					gd->setColor( fg );
				}

				gd->setDrawMode( glyph.mode & ATTR_ITALIC ? GlyphDrawable::DrawMode::TextItalic
														  : GlyphDrawable::DrawMode::Text );

				if ( mVBForeground ) {
					gd->drawIntoVertexBuffer( mVBForeground, mCurGridPos, { x, y } );
				} else {
					gd->draw( { x, y } );
				}
				dirtyFG = true;

				if ( mVBStyles.empty() ) {
					if ( glyph.mode & ATTR_UNDERLINE ) {
						mPrimitives.setColor( fg );
						mPrimitives.drawRectangle( Rectf( { x, y + lineHeight - cursorThickness },
														  { advanceX, cursorThickness } ) );
					} else if ( glyph.mode & ATTR_STRUCK ) {
						mPrimitives.setColor( fg );
						mPrimitives.drawRectangle( Rectf( { x, y + strikeThroughOffset },
														  { advanceX, cursorThickness } ) );
					}
				} else {
					if ( glyph.mode & ATTR_UNDERLINE ) {
						mVBStyles[j]->addQuad( { x, y + lineHeight - cursorThickness },
											   { advanceX, cursorThickness }, fg );
					} else if ( glyph.mode & ATTR_STRUCK ) {
						mVBStyles[j]->addQuad( { x, y + strikeThroughOffset },
											   { advanceX, cursorThickness }, fg );
					}
				}
			}

			x += advanceX;
		}

		y += lineHeight;

		if ( j == (Uint32)mCursor.y )
			invalidateCursor();
	}

	bool redrawCursor =
		!mEmulator->isScrolling() && !IS_SET( MODE_HIDE ) && ( !mUseFrameBuffer || mDirtyCursor );
	bool mustRenderUnderline = false;

	if ( redrawCursor ) {
		mDirtyCursor = false;
		Color drawcol;

		if ( IS_SET( MODE_REVERSE ) ) {
			if ( mEmulator->isSelected( mCursor.x, mCursor.y ) ) {
				drawcol = mColorScheme.getCursor();
			} else {
				drawcol = mColorScheme.getBackground();
			}
		} else {
			drawcol = mEmulator->isSelected( mCursor.x, mCursor.y ) ? mColorScheme.getBackground()
																	: mColorScheme.getCursor();
		}

		mPrimitives.setColor( drawcol );

		/* draw the new one */
		if ( IS_SET( MODE_FOCUSED ) ) {
			switch ( mCursorMode ) {
				case StExtension: {
					auto* gd = mFont->getGlyphDrawable(
						mCursorMode == StExtension ? 0xFB04 /* TUX */ : mCursorGlyph.u, mFontSize );
					gd->setColor( termColor( mCursorGlyph.fg, mColors ) );
					gd->setDrawMode( GlyphDrawable::DrawMode::Text );
					gd->draw(
						{ pos.x + mCursor.x * spaceCharAdvanceX, pos.y + mCursor.y * lineHeight } );
					break;
				}
				case BlinkUnderline: {
					if ( !( mMode & MODE_BLINK ) )
						break;
				}
				case SteadyUnderline: {
					mustRenderUnderline = true;
					break;
				}
				case BlinkBar: {
					if ( !( mMode & MODE_BLINK ) )
						break;
				}
				case SteadyBar: {
					mPrimitives.drawRectangle( Rectf(
						{ pos.x + mCursor.x * spaceCharAdvanceX, pos.y + mCursor.y * lineHeight },
						{ std::ceil( PixelDensity::dpToPx( 1 ) ), lineHeight } ) );
					break;
				}
				case BlinkingBlock:
				case BlinkingBlockDefault: {
					if ( !( mMode & MODE_BLINK ) )
						break;
				}
				case SteadyBlock:
				case TerminalCursorMode::MAX_CURSOR: {
					mPrimitives.drawRectangle( Rectf(
						{ pos.x + mCursor.x * spaceCharAdvanceX, pos.y + mCursor.y * lineHeight },
						{ spaceCharAdvanceX, lineHeight } ) );
					break;
				}
			}
		} else {
			Vector2f cpos{ pos.x + mCursor.x * spaceCharAdvanceX, pos.y + mCursor.y * lineHeight };
			mPrimitives.setColor( drawcol );
			mPrimitives.drawRectangle(
				Rectf( Vector2f( cpos.x, cpos.y ), { spaceCharAdvanceX, cursorThickness } ) );
			mPrimitives.drawRectangle(
				Rectf( Vector2f( cpos.x, cpos.y ), { cursorThickness, lineHeight } ) );
			mPrimitives.drawRectangle( Rectf( Vector2f( cpos.x + spaceCharAdvanceX, cpos.y ),
											  { cursorThickness, lineHeight } ) );
			mPrimitives.drawRectangle(
				Rectf( Vector2f( cpos.x, cpos.y + lineHeight - cursorThickness ),
					   { spaceCharAdvanceX, cursorThickness } ) );
		}
	}

	if ( mVBForeground ) {
		mFont->getTexture( mFontSize )->bind();
		if ( dirtyFG )
			mVBForeground->update( VERTEX_FLAGS_DEFAULT, false );
		mVBForeground->bind();
		mVBForeground->draw();
		mVBForeground->unbind();
	}

	if ( !mVBStyles.empty() ) {
		if ( dirtyFG ) {
			for ( auto vbo : mVBStyles ) {
				if ( vbo->getVertexCount() )
					vbo->update( VERTEX_FLAGS_PRIMITIVE, false );
			}
		}
		for ( auto vbo : mVBStyles ) {
			if ( vbo->getVertexCount() == 0 )
				continue;
			vbo->bind();
			vbo->draw();
			vbo->unbind();
		}
	}

	// Underline is rendered after foreground render because it usually clashes with the underlines
	// decorations and ends up being not visible, I prefer to do this even it it's not standard.
	if ( mustRenderUnderline ) {
		mPrimitives.drawRectangle(
			Rectf( { pos.x + mCursor.x * spaceCharAdvanceX,
					 pos.y + mCursor.y * lineHeight + lineHeight - cursorThickness },
				   { spaceCharAdvanceX, cursorThickness } ) );
	}

	if ( mFrameBuffer && pos.y + lineHeight * mRows < mSize.getHeight() ) {
		mPrimitives.setColor( defaultBg );
		mPrimitives.drawRectangle(
			{ { pos.x, pos.y + lineHeight * mRows },
			  { mSize.getWidth(), mSize.getHeight() - pos.y + lineHeight * mRows } } );
	}

	if ( mFrameBuffer )
		mFrameBuffer->unbind();
}

void TerminalDisplay::drawBg( bool toFBO ) {
	auto defaultBg = termColor( mEmulator->getDefaultBackground(), mColors );
	Primitives p;
	p.setForceDraw( toFBO );
	p.setColor( defaultBg );

	if ( toFBO ) {
		p.drawRectangle( Rectf( Vector2f::Zero, mFrameBuffer->getSize().asFloat() ) );
	} else {
		p.drawRectangle( Rectf( mPosition.floor().asFloat(), mSize.asFloat() ) );
	}
}

void TerminalDisplay::draw( const Vector2f& pos ) {
	if ( !mEmulator || !mTerminal )
		return;

	mDrawing = true;

	drawBg();

	if ( !mFrameBuffer || mDirty )
		drawGrid( pos );

	if ( mFrameBuffer )
		drawFrameBuffer();

	if ( hasFocus() && mWindow->getIME().isEditing() ) {
		Rectf r = updateIMELocation();
		FontStyleConfig config;
		config.Font = mFont;
		config.CharacterSize = mFontSize;
		config.FontColor = termColor( mCursorGlyph.bg, mColors );
		mWindow->getIME().draw( r.getPosition(), r.getHeight(), config, config.FontColor,
								termColor( mCursorGlyph.fg, mColors ), true );
	}

	mDrawing = false;
	mDirty = false;
	mFullDirty = false;
}

Vector2i TerminalDisplay::positionToGrid( const Vector2i& pos ) {
	Vector2f relPos = { pos.x - mPosition.x - mPadding.Left, pos.y - mPosition.y - mPadding.Top };
	int mouseX = 0;
	int mouseY = 0;

	auto fontSize = (Float)mFont->getFontHeight( mFontSize );
	auto spaceCharAdvanceX = mFont->getGlyph( 'A', mFontSize, false, false ).advance;

	auto clipColumns = (int)std::floor( std::max( 1.0f, mSize.getWidth() / spaceCharAdvanceX ) );
	auto clipRows = (int)std::floor( std::max( 1.0f, mSize.getHeight() / fontSize ) );

	if ( pos.x <= 0.0f || pos.y <= 0.0f ) {
		mouseX = 0;
		mouseY = 0;
	} else if ( relPos.x >= 0.0f && relPos.y >= 0.0f ) {
		mouseX = eeclamp( (int)std::floor( relPos.x / spaceCharAdvanceX ), 0, clipColumns );
		mouseY = eeclamp( (int)std::floor( relPos.y / fontSize ), 0, clipRows - 1 );
	}

	// All these checks are because there's a very rare bug I cannot find how it happens
	auto termSize = mTerminal->getSize();

	eeASSERT( mouseX >= 0 && mouseX <= mTerminal->getSize().x );
	eeASSERT( mouseY >= 0 && mouseY <= mTerminal->getSize().y );

	mouseX = eeclamp( mouseX, 0, termSize.x );
	mouseY = eeclamp( mouseY, 0, termSize.y );

	return { mouseX, mouseY };
}

void TerminalDisplay::onSizeChange() {
	Sizei gridSize( gridSizeFromTermDimensions(
		mFont, mFontSize,
		mSize - Vector2f( mPadding.Left + mPadding.Right, mPadding.Top + mPadding.Bottom ) ) );

	if ( mTerminal ) {
		if ( gridSize.getWidth() != mTerminal->getNumColumns() ||
			 gridSize.getHeight() != mTerminal->getNumRows() ) {
			mTerminal->resize( gridSize.getWidth(), gridSize.getHeight() );
			mDirtyLines.resize( gridSize.getHeight(), 1 );
		}
	} else if ( mEmulator ) {
		if ( gridSize.getWidth() != mEmulator->getNumColumns() ||
			 gridSize.getHeight() != mEmulator->getNumRows() ) {
			mEmulator->resize( gridSize.getWidth(), gridSize.getHeight() );
			mDirtyLines.resize( gridSize.getHeight(), 1 );
		}
	}

	if ( mFrameBuffer && ( mFrameBuffer->getWidth() < mSize.getWidth() ||
						   mFrameBuffer->getHeight() < mSize.getHeight() ) ) {
		if ( nullptr == mFrameBuffer ) {
			createFrameBuffer();
		} else {
			Sizei fboSize( getFrameBufferSize() );
			mFrameBuffer->resize( fboSize.getWidth(), fboSize.getHeight() );
		}
	}
	invalidateLines();
}

void TerminalDisplay::onProcessExit( int exitCode ) {
	sendEvent( { EventType::PROCESS_EXIT, String::toString( exitCode ) } );

	if ( !mTerminal || mProgram.empty() || exitCode != 0 || !mKeepAlive )
		return;

	auto processFactory = eeNew( ProcessFactory, () );

	Sizei termSize( mColumns, mRows );
	std::unique_ptr<IPseudoTerminal> pseudoTerminal = nullptr;

	auto process =
		processFactory->createWithPseudoTerminal( mProgram, mArgs, mWorkingDir, termSize.getWidth(),
												  termSize.getHeight(), pseudoTerminal, mEnv );

	if ( !pseudoTerminal ) {
		eeSAFE_DELETE( processFactory );
		fprintf( stderr, "TerminalDisplay::onProcessExit: Failed to create pseudo terminal\n" );
	}

	if ( !process ) {
		eeSAFE_DELETE( processFactory );
		fprintf( stderr, "TerminalDisplay::onProcessExit: Failed to spawn process\n" );
	}

	mTerminal->clearHistory();
	mTerminal->setPtyAndProcess( std::move( pseudoTerminal ), std::move( process ) );

	eeSAFE_DELETE( processFactory );
}

void TerminalDisplay::onScrollPositionChange() {
	sendEvent( { EventType::SCROLL_HISTORY } );
}

void TerminalDisplay::onTextInput( const Uint32& chr ) {
	if ( !mTerminal )
		return;
	String input;
	input.push_back( chr );
	std::string utf8Input( input.toUtf8() );
	mTerminal->ttywrite( utf8Input.c_str(), utf8Input.size(), 1 );
	mDirty = true;
}

void TerminalDisplay::onTextEditing( const String&, const Int32&, const Int32& ) {
	if ( !mTerminal )
		return;
	invalidateCursor();
	updateIMELocation();
}

void TerminalDisplay::onKeyDown( const Keycode& keyCode, const Uint32& /*chr*/, const Uint32& mod,
								 const Scancode& scancode ) {
	if ( mWindow->getIME().isEditing() )
		return;
	Uint32 smod = sanitizeMod( mod );

	auto scIt = terminalKeyMap.Shortcuts().find( keyCode );
	if ( scIt != terminalKeyMap.Shortcuts().end() ) {
		for ( auto& k : scIt->second ) {
			if ( k.mask == KEYMOD_CTRL_SHIFT_ALT_META || k.mask == smod ) {
				if ( IS_SET( MODE_APPKEYPAD ) ? k.appkey < 0 : k.appkey > 0 )
					continue;

				if ( IS_SET( MODE_NUMLOCK ) && k.appkey == 2 )
					continue;

				if ( IS_SET( MODE_APPCURSOR ) ? k.appcursor < 0 : k.appcursor > 0 )
					continue;

				if ( !k.altscrn || ( k.altscrn == ( mEmulator->tisaltscr() ? 1 : -1 ) ) ) {
					action( k.action );
					return;
				}
			}
		}
	}

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

			mTerminal->ttywrite( &tmp, 1, 1 );
			return;
		}
	}

	auto kvIt = terminalKeyMap.KeyMap().find( keyCode );
	if ( kvIt != terminalKeyMap.KeyMap().end() ) {
		for ( auto& k : kvIt->second ) {
			if ( k.mask == KEYMOD_CTRL_SHIFT_ALT_META || k.mask == smod ) {
				if ( IS_SET( MODE_APPKEYPAD ) ? k.appkey < 0 : k.appkey > 0 )
					continue;

				if ( IS_SET( MODE_NUMLOCK ) && k.appkey == 2 )
					continue;

				if ( IS_SET( MODE_APPCURSOR ) ? k.appcursor < 0 : k.appcursor > 0 )
					continue;

				if ( k.string.size() > 0 ) {
					mTerminal->ttywrite( k.string.c_str(), k.string.size(), 1 );
					return;
				}
				break;
			}
		}
	}

	auto pkmIt = terminalKeyMap.PlatformKeyMap().find( scancode );
	if ( pkmIt != terminalKeyMap.PlatformKeyMap().end() ) {
		for ( auto& k : pkmIt->second ) {
			if ( k.mask == KEYMOD_CTRL_SHIFT_ALT_META || k.mask == smod ) {
				if ( IS_SET( MODE_APPKEYPAD ) ? k.appkey < 0 : k.appkey > 0 )
					continue;

				if ( IS_SET( MODE_NUMLOCK ) && k.appkey == 2 )
					continue;

				if ( IS_SET( MODE_APPCURSOR ) ? k.appcursor < 0 : k.appcursor > 0 )
					continue;

				if ( k.string.size() > 0 ) {
					mTerminal->ttywrite( k.string.c_str(), k.string.size(), 1 );
					return;
				}
				break;
			}
		}
	}
}

Font* TerminalDisplay::getFont() const {
	return mFont;
}

void TerminalDisplay::setFont( Font* font ) {
	if ( mFont != font ) {
		mFont = font;
		onSizeChange();
	}
}

const Float& TerminalDisplay::getFontSize() const {
	return mFontSize;
}

void TerminalDisplay::setFontSize( const Float& fontSize ) {
	Float fontSizeClamp = eeclamp( fontSize, 4.f, 120.f );
	if ( mFontSize != fontSizeClamp ) {
		mFontSize = fontSizeClamp;
		onSizeChange();
	}
}

const Vector2f& TerminalDisplay::getPosition() const {
	return mPosition;
}

void TerminalDisplay::setPosition( const Vector2f& position ) {
	mPosition = position;
	invalidate();
}

const Sizef& TerminalDisplay::getSize() const {
	return mSize;
}

void TerminalDisplay::setSize( const Sizef& size ) {
	if ( mSize != size ) {
		mSize = size;
		onSizeChange();
	}
}

void TerminalDisplay::invalidate() {
	mDirty = true;
}

void TerminalDisplay::invalidateCursor() {
	invalidateLine( mCursor.y );
	mDirtyCursor = true;
	mDirty = true;
}

void TerminalDisplay::invalidateLine( const int& line ) {
	if ( line >= (int)mDirtyLines.size() ) {
		mDirtyLines.resize( line + 1 );
	}
	mDirtyLines[line] = true;
	mDirty = true;
}

void TerminalDisplay::invalidateLines() {
	mDirtyLines.assign( mDirtyLines.size(), 1 );
	mDirty = true;
	mFullDirty = true;
}
void TerminalDisplay::setFocus( bool focus ) {
	if ( focus )
		updateIMELocation();

	if ( focus == mFocus )
		return;
	mFocus = focus;
	bool modeFocus = mMode & MODE_FOCUSED;
	if ( mFocus != modeFocus ) {
		if ( mFocus ) {
			mMode |= MODE_FOCUSED | MODE_FOCUS;
			mWindow->startTextInput();
		} else {
			mMode ^= MODE_FOCUS | MODE_FOCUSED;
		}
	} else {
		mMode ^= MODE_FOCUS;
	}
	invalidateCursor();
}

Sizei TerminalDisplay::getFrameBufferSize() {
	return Sizei( Math::nextPowOfTwo( (int)mSize.getWidth() ),
				  Math::nextPowOfTwo( (int)mSize.getHeight() ) );
}

void TerminalDisplay::createFrameBuffer() {
	eeSAFE_DELETE( mFrameBuffer );
	Sizei fboSize( getFrameBufferSize() );
	if ( fboSize.getWidth() < 1 )
		fboSize.setWidth( 1 );
	if ( fboSize.getHeight() < 1 )
		fboSize.setHeight( 1 );
	mFrameBuffer = FrameBuffer::New( fboSize.getWidth(), fboSize.getHeight(), true );

	// Frame buffer failed to create?
	if ( !mFrameBuffer->created() )
		eeSAFE_DELETE( mFrameBuffer );
}

void TerminalDisplay::drawFrameBuffer() {
	if ( mFrameBuffer ) {
		Rect r( 0, 0, mSize.getWidth(), mSize.getHeight() );
		TextureRegion textureRegion( mFrameBuffer->getTexture(), r, r.getSize().asFloat() );
		textureRegion.draw( mPosition.floor().x, mPosition.floor().y );
	}
}

VertexBuffer* TerminalDisplay::createRowVBO( bool usesTexCoords ) {
	auto* VBO = VertexBuffer::NewVertexArray(
		usesTexCoords ? VERTEX_FLAGS_DEFAULT : VERTEX_FLAGS_PRIMITIVE,
		mQuadVertex == 6 ? EE::Graphics::PRIMITIVE_TRIANGLES : EE::Graphics::PRIMITIVE_QUADS,
		mColumns * mQuadVertex, 0, VertexBufferUsageType::Stream );
	VBO->setGridSize( Sizei( mColumns, 1 ) );
	return VBO;
}

void TerminalDisplay::createVBO( VertexBuffer** vbo, bool usesTexCoords ) {
	eeSAFE_DELETE( ( *vbo ) );
	( *vbo ) = VertexBuffer::New(
		usesTexCoords ? VERTEX_FLAGS_DEFAULT : VERTEX_FLAGS_PRIMITIVE,
		mQuadVertex == 6 ? EE::Graphics::PRIMITIVE_TRIANGLES : EE::Graphics::PRIMITIVE_QUADS,
		mRows * mColumns * mQuadVertex, 0, VertexBufferUsageType::Stream );
	( *vbo )->resizeArray( VERTEX_FLAG_POSITION, mRows * mColumns * mQuadVertex );
	( *vbo )->resizeArray( VERTEX_FLAG_COLOR, mRows * mColumns * mQuadVertex );
	( *vbo )->setGridSize( Sizei( mColumns, mRows ) );
	if ( usesTexCoords )
		( *vbo )->resizeArray( VERTEX_FLAG_TEXTURE0, mRows * mColumns * mQuadVertex );
}

void TerminalDisplay::initVBOs() {
	createVBO( &mVBBackground, false );
	createVBO( &mVBForeground, true );
	for ( VertexBuffer* vb : mVBStyles )
		eeSAFE_DELETE( vb );
	mVBStyles.clear();
	for ( Uint32 i = 0; i < mRows; ++i )
		mVBStyles.emplace_back( createRowVBO( false ) );
}

Rectf TerminalDisplay::updateIMELocation() {
	if ( !Engine::isMainThread() )
		return {};
	Float fontSize = mFont->getFontHeight( mFontSize );
	Float spaceCharAdvanceX = mFont->getGlyph( 'A', mFontSize, false, false ).advance;
	auto pos = mPosition.floor() + Vector2f( mPadding.Left, mPadding.Top );
	Rectf r( { pos.x + mCursor.x * spaceCharAdvanceX, pos.y + mCursor.y * fontSize },
			 { spaceCharAdvanceX, fontSize } );
	mWindow->getIME().setLocation( r.asInt() );
	return r;
}

bool TerminalDisplay::useFrameBuffer() const {
	return mUseFrameBuffer;
}

}} // namespace eterm::Terminal
