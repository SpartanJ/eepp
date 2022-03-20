#include <eepp/ui/keyboardshortcut.hpp>
#include <eepp/window/input.hpp>

using namespace EE::Window;

namespace EE { namespace UI {

std::map<std::string, Uint32> MOD_MAP = { { "lshift", KEYMOD_SHIFT },
										  { "rshift", KEYMOD_SHIFT },
										  { "left shift", KEYMOD_SHIFT },
										  { "right shift", KEYMOD_SHIFT },
										  { "shift", KEYMOD_SHIFT },
										  { "lctrl", KEYMOD_CTRL },
										  { "rctrl", KEYMOD_CTRL },
										  { "left ctrl", KEYMOD_CTRL },
										  { "right ctrl", KEYMOD_CTRL },
										  { "ctrl", KEYMOD_CTRL },
										  { "lalt", KEYMOD_ALT },
										  { "ralt", KEYMOD_RALT },
										  { "left alt", KEYMOD_LALT },
										  { "right alt", KEYMOD_RALT },
										  { "altgr", KEYMOD_RALT },
										  { "alt", KEYMOD_LALT },
										  { "lmeta", KEYMOD_META },
										  { "lmeta", KEYMOD_META },
										  { "left meta", KEYMOD_META },
										  { "right meta", KEYMOD_META },
										  { "meta", KEYMOD_META },
										  { "mod", KEYMOD_DEFAULT_MODIFIER },
										  { "modifier", KEYMOD_DEFAULT_MODIFIER } };

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
		}
	} while ( erased );
	mShortcuts[sanitizeShortcut( keys )] = command;
}

KeyBindings::Shortcut KeyBindings::getShortcutFromString( const std::string& keys ) {
	Shortcut shortcut;
	Uint32 mod = 0;
	auto keysSplit = String::split( keys, '+' );
	if ( keysSplit.size() == 1 && getKeyMod( keysSplit[0] ) && keys.find( "++" ) )
		keysSplit.emplace_back( "+" );
	if ( keysSplit.size() == 2 && getKeyMod( keysSplit[0] ) &&
		 keys.find( " +" ) != std::string::npos )
		keysSplit[1] += "+";
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

void KeyBindings::reset() {
	mShortcuts.clear();
}

const ShortcutMap& KeyBindings::getShortcutMap() const {
	return mShortcuts;
}

std::string KeyBindings::getShortcutString( KeyBindings::Shortcut shortcut ) {
	std::vector<std::string> mods;
	std::string keyname( String::toLower( mInput->getKeyName( shortcut.key ) ) );
	if ( shortcut.mod & KEYMOD_CTRL )
		mods.emplace_back( "ctrl" );
	if ( shortcut.mod & KEYMOD_SHIFT )
		mods.emplace_back( "shift" );
	if ( shortcut.mod & KEYMOD_LALT )
		mods.emplace_back( "alt" );
	if ( shortcut.mod & KEYMOD_RALT )
		mods.emplace_back( "altgr" );
	if ( shortcut.mod & KEYMOD_META )
		mods.emplace_back( "meta" );
	if ( mods.empty() )
		return keyname;
	return String::join( mods, '+' ) + "+" + keyname;
}

Uint32 KeyBindings::getDefaultModifier() const {
	return MOD_MAP["mod"];
}

void KeyBindings::setDefaultModifier( Uint32 newDefaultModifier ) {
	MOD_MAP["mod"] = newDefaultModifier;
	MOD_MAP["modifier"] = newDefaultModifier;
}

}} // namespace EE::UI
