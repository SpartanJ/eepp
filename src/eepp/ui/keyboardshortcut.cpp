#include <eepp/ui/keyboardshortcut.hpp>
#include <eepp/window/input.hpp>

using namespace EE::Window;

namespace EE { namespace UI {

static std::map<std::string, Uint32> MOD_MAP = {
	{"lshift", KEYMOD_SHIFT},	   {"rshift", KEYMOD_SHIFT},	{"left shift", KEYMOD_SHIFT},
	{"right shift", KEYMOD_SHIFT}, {"shift", KEYMOD_SHIFT},		{"lctrl", KEYMOD_CTRL},
	{"rctrl", KEYMOD_CTRL},		   {"left ctrl", KEYMOD_CTRL},	{"right ctrl", KEYMOD_CTRL},
	{"ctrl", KEYMOD_CTRL},		   {"lalt", KEYMOD_ALT},		{"ralt", KEYMOD_RALT},
	{"left alt", KEYMOD_LALT},	   {"right alt", KEYMOD_RALT},	{"altgr", KEYMOD_RALT},
	{"alt", KEYMOD_LALT},		   {"lmeta", KEYMOD_META},		{"lmeta", KEYMOD_META},
	{"left meta", KEYMOD_META},	   {"right meta", KEYMOD_META}, {"meta", KEYMOD_META},
};

Uint32 KeyBindings::getKeyMod( std::string key ) {
	String::toLowerInPlace( key );
	auto it = MOD_MAP.find( key );
	if ( ( it != MOD_MAP.end() ) ) {
		return it->second;
	}
	return 0;
}

static KeyBindings::Shortcut sanitizeShortcut( const KeyBindings::Shortcut& shortcut ) {
	KeyBindings::Shortcut sanitized( shortcut.key, 0 );
	if ( shortcut.mod & KEYMOD_CTRL )
		sanitized.mod |= KEYMOD_CTRL;
	if ( shortcut.mod & KEYMOD_SHIFT )
		sanitized.mod |= KEYMOD_SHIFT;
	if ( shortcut.mod & KEYMOD_META )
		sanitized.mod |= KEYMOD_META;
	if ( shortcut.mod & KEYMOD_LALT )
		sanitized.mod |= KEYMOD_LALT;
	if ( shortcut.mod & KEYMOD_RALT )
		sanitized.mod |= KEYMOD_RALT;
	return sanitized;
}

KeyBindings::KeyBindings( const Window::Input* input ) : mInput( input ) {}

void KeyBindings::addKeybindsString( const std::map<std::string, std::string>& binds ) {
	for ( auto& bind : binds ) {
		addKeybindString( bind.first, bind.second );
	}
}

void KeyBindings::addKeybinds( const std::map<KeyBindings::Shortcut, std::string>& binds ) {
	for ( auto& bind : binds ) {
		addKeybind( bind.first, bind.second );
	}
}

void KeyBindings::addKeybindString( const std::string& keys, const std::string& command ) {
	addKeybind( getShortcutFromString( keys ), command );
}

void KeyBindings::addKeybind( const KeyBindings::Shortcut& keys, const std::string& command ) {
	mShortcuts[sanitizeShortcut( keys )] = command;
}

void KeyBindings::replaceKeybindString( const std::string& keys, const std::string& command ) {
	replaceKeybind( getShortcutFromString( keys ), command );
}

void KeyBindings::replaceKeybind( const KeyBindings::Shortcut& keys, const std::string& command ) {
	bool erased;
	do {
		erased = false;
		auto it = mShortcuts.find( sanitizeShortcut( keys ) );
		if ( it != mShortcuts.end() ) {
			mShortcuts.erase( it );
			erased = true;
			break;
		}
	} while ( erased );
	mShortcuts[sanitizeShortcut( keys )] = command;
}

KeyBindings::Shortcut KeyBindings::getShortcutFromString( const std::string& keys ) {
	Shortcut shortcut;
	Uint32 mod = 0;
	auto keysSplit = String::split( keys, '+' );
	for ( auto& part : keysSplit ) {
		if ( ( mod = getKeyMod( part ) ) ) {
			shortcut.mod |= mod;
		} else {
			shortcut.key = mInput->getKeyFromName( part );
		}
	}
	return shortcut;
}

void KeyBindings::removeKeybind( const KeyBindings::Shortcut& keys ) {
	auto it = mShortcuts.find( keys.key );
	if ( it != mShortcuts.end() ) {
		mShortcuts.erase( it );
	}
}

bool KeyBindings::existsKeybind( const KeyBindings::Shortcut& keys ) {
	return mShortcuts.find( keys.toUint64() ) != mShortcuts.end();
}

std::string KeyBindings::getCommandFromKeyBind( const KeyBindings::Shortcut& keys ) {
	auto it = mShortcuts.find( sanitizeShortcut( keys ) );
	if ( it != mShortcuts.end() ) {
		return it->second;
	}
	return "";
}

}} // namespace EE::UI
