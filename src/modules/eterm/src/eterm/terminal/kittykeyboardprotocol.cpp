#include <algorithm>
#include <cstdio>
#include <eterm/terminal/kittykeyboardprotocol.hpp>

namespace eterm { namespace Terminal {

static bool validCodepoint( Uint32 codepoint ) {
	return codepoint <= 0x10FFFF && !( codepoint >= 0xD800 && codepoint <= 0xDFFF );
}

void KittyKeyboardState::reset() {
	flags = 0;
	stack.clear();
}

void KittyKeyboardState::push( Uint32 requestedFlags ) {
	if ( stack.size() == MaxStackDepth )
		stack.erase( stack.begin() );
	stack.push_back( flags );
	flags = requestedFlags & KITTY_KEYBOARD_SUPPORTED_FLAGS;
}

void KittyKeyboardState::pop( size_t count ) {
	if ( count == 0 )
		count = 1;
	count = std::min( count, MaxStackDepth );
	while ( count-- ) {
		if ( stack.empty() ) {
			flags = 0;
			return;
		}
		flags = stack.back();
		stack.pop_back();
	}
}

void KittyKeyboardState::set( Uint32 requestedFlags, Uint32 mode ) {
	const Uint32 supported = requestedFlags & KITTY_KEYBOARD_SUPPORTED_FLAGS;
	switch ( mode ) {
		case 1:
			flags = supported;
			break;
		case 2:
			flags |= supported;
			break;
		case 3:
			flags &= ~supported;
			break;
	}
}

Uint32 KittyKeyboardEncoder::encodeModifiers( Uint32 modifiers ) {
	Uint32 result = 0;
	if ( modifiers & KEYMOD_SHIFT )
		result |= 1;
	if ( modifiers & KEYMOD_ALT )
		result |= 2;
	if ( modifiers & KEYMOD_CTRL )
		result |= 4;
	// EEPP's META is the platform GUI/Command key and maps to Kitty Super.
	if ( modifiers & KEYMOD_META )
		result |= 8;
	if ( modifiers & KEYMOD_CAPS )
		result |= 64;
	if ( modifiers & KEYMOD_NUM )
		result |= 128;
	return result + 1;
}

enum class LegacyShape : Uint8 { CsiU, Tilde, Letter };

struct MappedKey {
	Uint32 code{ 0 };
	LegacyShape shape{ LegacyShape::CsiU };
	char suffix{ 0 };
	bool resetKey{ false };
	bool printable{ false };
};

static MappedKey mapKey( const KittyKeyEvent& event ) {
	switch ( event.scancode ) {
		case SCANCODE_ESCAPE:
			return { 27, LegacyShape::CsiU, 0, false };
		case SCANCODE_RETURN:
		case SCANCODE_RETURN2:
			return { 13, LegacyShape::CsiU, 0, true };
		case SCANCODE_TAB:
		case SCANCODE_KP_TAB:
			return { 9, LegacyShape::CsiU, 0, true };
		case SCANCODE_BACKSPACE:
		case SCANCODE_KP_BACKSPACE:
			return { 127, LegacyShape::CsiU, 0, true };
		case SCANCODE_INSERT:
			return { 2, LegacyShape::Tilde };
		case SCANCODE_DELETE:
			return { 3, LegacyShape::Tilde };
		case SCANCODE_PAGEUP:
			return { 5, LegacyShape::Tilde };
		case SCANCODE_PAGEDOWN:
			return { 6, LegacyShape::Tilde };
		case SCANCODE_UP:
			return { 1, LegacyShape::Letter, 'A' };
		case SCANCODE_DOWN:
			return { 1, LegacyShape::Letter, 'B' };
		case SCANCODE_RIGHT:
			return { 1, LegacyShape::Letter, 'C' };
		case SCANCODE_LEFT:
			return { 1, LegacyShape::Letter, 'D' };
		case SCANCODE_END:
			return { 1, LegacyShape::Letter, 'F' };
		case SCANCODE_HOME:
			return { 1, LegacyShape::Letter, 'H' };
		case SCANCODE_F1:
			return { 1, LegacyShape::Letter, 'P' };
		case SCANCODE_F2:
			return { 1, LegacyShape::Letter, 'Q' };
		case SCANCODE_F3:
			// CSI R is a cursor-position report and was removed as an allowed F3 encoding.
			return { 13, LegacyShape::Tilde };
		case SCANCODE_F4:
			return { 1, LegacyShape::Letter, 'S' };
		case SCANCODE_F5:
			return { 15, LegacyShape::Tilde };
		case SCANCODE_F6:
			return { 17, LegacyShape::Tilde };
		case SCANCODE_F7:
			return { 18, LegacyShape::Tilde };
		case SCANCODE_F8:
			return { 19, LegacyShape::Tilde };
		case SCANCODE_F9:
			return { 20, LegacyShape::Tilde };
		case SCANCODE_F10:
			return { 21, LegacyShape::Tilde };
		case SCANCODE_F11:
			return { 23, LegacyShape::Tilde };
		case SCANCODE_F12:
			return { 24, LegacyShape::Tilde };
		case SCANCODE_CAPSLOCK:
			return { 57358 };
		case SCANCODE_SCROLLLOCK:
			return { 57359 };
		case SCANCODE_NUMLOCKCLEAR:
			return { 57360 };
		case SCANCODE_PRINTSCREEN:
			return { 57361 };
		case SCANCODE_PAUSE:
			return { 57362 };
		case SCANCODE_APPLICATION:
		case SCANCODE_MENU:
			return { 57363 };
		case SCANCODE_KP_0:
			return { 57399 };
		case SCANCODE_KP_1:
		case SCANCODE_KP_2:
		case SCANCODE_KP_3:
		case SCANCODE_KP_4:
		case SCANCODE_KP_5:
		case SCANCODE_KP_6:
		case SCANCODE_KP_7:
		case SCANCODE_KP_8:
		case SCANCODE_KP_9:
			return { 57400u + static_cast<Uint32>( event.scancode - SCANCODE_KP_1 ) };
		case SCANCODE_KP_PERIOD:
		case SCANCODE_KP_DECIMAL:
			return { 57409 };
		case SCANCODE_KP_DIVIDE:
			return { 57410 };
		case SCANCODE_KP_MULTIPLY:
			return { 57411 };
		case SCANCODE_KP_MINUS:
			return { 57412 };
		case SCANCODE_KP_PLUS:
			return { 57413 };
		case SCANCODE_KP_ENTER:
			return { 57414 };
		case SCANCODE_KP_EQUALS:
		case SCANCODE_KP_EQUALSAS400:
			return { 57415 };
		case SCANCODE_KP_COMMA:
		case SCANCODE_SEPARATOR:
			return { 57416 };
		case SCANCODE_LSHIFT:
			return { 57441 };
		case SCANCODE_LCTRL:
			return { 57442 };
		case SCANCODE_LALT:
			return { 57443 };
		case SCANCODE_LGUI:
			return { 57444 };
		case SCANCODE_RSHIFT:
			return { 57447 };
		case SCANCODE_RCTRL:
			return { 57448 };
		case SCANCODE_RALT:
			return { 57449 };
		case SCANCODE_RGUI:
			return { 57450 };
		default:
			if ( event.scancode >= SCANCODE_F13 && event.scancode <= SCANCODE_F24 )
				return { 57376u + static_cast<Uint32>( event.scancode - SCANCODE_F13 ) };
			break;
	}
	const auto keycode = static_cast<Uint32>( event.keycode );
	if ( keycode >= 32 && keycode <= 126 ) {
		const Uint32 unshifted = keycode >= 'A' && keycode <= 'Z' ? keycode + 32 : keycode;
		return { unshifted, LegacyShape::CsiU, 0, false, true };
	}
	if ( validCodepoint( event.character ) && event.character != 0 )
		return { event.character, LegacyShape::CsiU, 0, false, event.character >= 32 };
	return {};
}

static Uint32 baseLayoutCodepoint( Scancode scancode ) {
	if ( scancode >= SCANCODE_A && scancode <= SCANCODE_Z )
		return 'a' + static_cast<Uint32>( scancode - SCANCODE_A );
	if ( scancode >= SCANCODE_1 && scancode <= SCANCODE_9 )
		return '1' + static_cast<Uint32>( scancode - SCANCODE_1 );
	if ( scancode == SCANCODE_0 )
		return '0';
	static constexpr struct {
		Scancode scancode;
		Uint32 codepoint;
	} punctuation[] = {
		{ SCANCODE_SPACE, ' ' },	   { SCANCODE_MINUS, '-' },		   { SCANCODE_EQUALS, '=' },
		{ SCANCODE_LEFTBRACKET, '[' }, { SCANCODE_RIGHTBRACKET, ']' }, { SCANCODE_BACKSLASH, '\\' },
		{ SCANCODE_SEMICOLON, ';' },   { SCANCODE_APOSTROPHE, '\'' },  { SCANCODE_GRAVE, '`' },
		{ SCANCODE_COMMA, ',' },	   { SCANCODE_PERIOD, '.' },	   { SCANCODE_SLASH, '/' } };
	for ( const auto& entry : punctuation ) {
		if ( entry.scancode == scancode )
			return entry.codepoint;
	}
	return 0;
}

static std::string serialize( const MappedKey& key, Uint32 modifiers, KittyKeyEventType type,
							  bool reportEvents, bool reportAlternate, bool associatedText,
							  Uint32 text, Scancode scancode ) {
	char buf[96];
	char keyField[48];
	const Uint32 rawBaseLayout = reportAlternate ? baseLayoutCodepoint( scancode ) : 0;
	const Uint32 baseLayout = rawBaseLayout != key.code ? rawBaseLayout : 0;
	const bool shiftPressed = ( ( modifiers - 1 ) & 1u ) != 0;
	const Uint32 shifted =
		reportAlternate && shiftPressed && validCodepoint( text ) && text != key.code ? text : 0;
	if ( shifted && baseLayout )
		std::snprintf( keyField, sizeof( keyField ), "%u:%u:%u", key.code, shifted, baseLayout );
	else if ( shifted )
		std::snprintf( keyField, sizeof( keyField ), "%u:%u", key.code, shifted );
	else if ( baseLayout )
		std::snprintf( keyField, sizeof( keyField ), "%u::%u", key.code, baseLayout );
	else
		std::snprintf( keyField, sizeof( keyField ), "%u", key.code );
	const char suffix = key.shape == LegacyShape::Tilde	   ? '~'
						: key.shape == LegacyShape::Letter ? key.suffix
														   : 'u';
	int len;
	if ( associatedText && text >= 32 && !( text >= 127 && text <= 159 ) ) {
		len = reportEvents ? std::snprintf( buf, sizeof( buf ), "\033[%s;%u:%u;%uu", keyField,
											modifiers, static_cast<Uint32>( type ), text )
						   : std::snprintf( buf, sizeof( buf ), "\033[%s;%u;%uu", keyField,
											modifiers, text );
	} else if ( reportEvents ) {
		len = std::snprintf( buf, sizeof( buf ), "\033[%s;%u:%u%c", keyField, modifiers,
							 static_cast<Uint32>( type ), suffix );
	} else {
		len = std::snprintf( buf, sizeof( buf ), "\033[%s;%u%c", keyField, modifiers, suffix );
	}
	return len > 0 ? std::string( buf, static_cast<size_t>( len ) ) : std::string{};
}

KittyEncodedKey KittyKeyboardEncoder::encode( const KittyKeyEvent& event, Uint32 flags ) {
	KittyEncodedKey result;
	if ( flags == 0 )
		return result;
	const bool reportAll =
		flags & kittyKeyboardFlag( KittyKeyboardFlag::ReportAllKeysAsEscapeCodes );
	const bool disambiguate =
		flags & kittyKeyboardFlag( KittyKeyboardFlag::DisambiguateEscapeCodes );
	const bool reportEvents = flags & kittyKeyboardFlag( KittyKeyboardFlag::ReportEventTypes );
	if ( event.type != KittyKeyEventType::Press && !reportEvents )
		return result;
	const MappedKey key = mapKey( event );
	if ( key.code == 0 || ( key.resetKey && !reportAll ) )
		return result;
	const Uint32 modifiers = encodeModifiers( event.modifiers );
	// Shift-only printable keys still use committed text under disambiguation. Encoding them here
	// would duplicate the subsequent text-input event. Right Alt is AltGr in EEPP and is likewise a
	// text-producing layout selector, not a shortcut modifier. Some platforms also expose a
	// synthetic Ctrl while AltGr is held, so Ctrl only disambiguates when AltGr is absent.
	const bool altGr = event.modifiers & KEYMOD_RALT;
	const bool disambiguatingModifier = ( event.modifiers & ( KEYMOD_LALT | KEYMOD_META ) ) != 0 ||
										( !altGr && ( event.modifiers & KEYMOD_CTRL ) != 0 );
	if ( key.printable && !reportAll && !( disambiguate && disambiguatingModifier ) )
		return result;
	const bool mustEncode = reportAll || event.type != KittyKeyEventType::Press ||
							( disambiguate && ( disambiguatingModifier || key.code == 27 ) );
	if ( !mustEncode )
		return result;
	const bool associated =
		reportAll && ( flags & kittyKeyboardFlag( KittyKeyboardFlag::ReportAssociatedText ) ) &&
		validCodepoint( event.character );
	result.bytes = serialize( key, modifiers, event.type, reportEvents,
							  flags & kittyKeyboardFlag( KittyKeyboardFlag::ReportAlternateKeys ),
							  associated, event.character, event.scancode );
	result.expectedText = reportAll && event.character >= 32 ? event.character : 0;
	result.handled = !result.bytes.empty();
	return result;
}

std::string KittyKeyboardEncoder::encodeText( Uint32 codepoint, Uint32 flags ) {
	if ( !( flags & kittyKeyboardFlag( KittyKeyboardFlag::ReportAllKeysAsEscapeCodes ) ) ||
		 !validCodepoint( codepoint ) )
		return {};
	const bool associated = flags & kittyKeyboardFlag( KittyKeyboardFlag::ReportAssociatedText );
	MappedKey key{ associated ? 0 : codepoint };
	return serialize( key, 1, KittyKeyEventType::Press,
					  flags & kittyKeyboardFlag( KittyKeyboardFlag::ReportEventTypes ), false,
					  associated, codepoint, SCANCODE_UNKNOWN );
}

}} // namespace eterm::Terminal
