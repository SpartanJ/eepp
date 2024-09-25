#include <eepp/ui/keyboardshortcut.hpp>
#include <eepp/window/input.hpp>

using namespace EE::Window;

namespace EE { namespace UI {

KeyBindings::Shortcut KeyBindings::sanitizeShortcut( const KeyBindings::Shortcut& shortcut ) {
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

void KeyBindings::addKeybindsStringUnordered(
	const std::unordered_map<std::string, std::string>& binds ) {
	for ( auto& bind : binds ) {
		addKeybindString( bind.first, bind.second );
	}
}

void KeyBindings::addKeybindsUnordered(
	const std::unordered_map<KeyBindings::Shortcut, std::string>& binds ) {
	for ( auto& bind : binds ) {
		addKeybind( bind.first, bind.second );
	}
}

void KeyBindings::addKeybindString( const std::string& key, const std::string& command ) {
	addKeybind( getShortcutFromString( key ), command );
}

void KeyBindings::addKeybind( const KeyBindings::Shortcut& key, const std::string& command ) {
	mShortcuts[sanitizeShortcut( key )] = command;
	mKeybindingsInvert[command] = sanitizeShortcut( key );
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
			mKeybindingsInvert.erase( it->second );
			erased = true;
		}
	} while ( erased );
	mShortcuts[sanitizeShortcut( keys )] = command;
	mKeybindingsInvert[command] = sanitizeShortcut( keys );
}

KeyBindings::Shortcut KeyBindings::toShortcut( const Window::Input* input,
											   const std::string& keys ) {
	Shortcut shortcut;
	Uint32 mod = 0;
	auto keysSplit = String::split( keys, '+' );
	if ( keysSplit.size() == 1 && KeyMod::getKeyMod( keysSplit[0] ) && keys.find( "++" ) )
		keysSplit.emplace_back( "+" );
	if ( keysSplit.size() == 2 && KeyMod::getKeyMod( keysSplit[0] ) &&
		 keys.find( " +" ) != std::string::npos )
		keysSplit[1] += "+";
	for ( auto& part : keysSplit ) {
		if ( ( mod = KeyMod::getKeyMod( part ) ) ) {
			shortcut.mod |= mod;
		} else {
			shortcut.key = input->getKeyFromName( part );
		}
	}
	return shortcut;
}

KeyBindings::Shortcut KeyBindings::getShortcutFromString( const std::string& keys ) {
	return toShortcut( mInput, keys );
}

void KeyBindings::removeKeybind( const KeyBindings::Shortcut& keys ) {
	auto it = mShortcuts.find( keys.toUint64() );
	if ( it != mShortcuts.end() ) {
		mShortcuts.erase( it );
	}
}

bool KeyBindings::existsKeybind( const KeyBindings::Shortcut& keys ) {
	return mShortcuts.find( keys.toUint64() ) != mShortcuts.end();
}

void KeyBindings::removeCommandKeybind( const std::string& command ) {
	auto kbIt = mKeybindingsInvert.find( command );
	if ( kbIt != mKeybindingsInvert.end() ) {
		removeKeybind( kbIt->second );
		mKeybindingsInvert.erase( command );
	}
}

void KeyBindings::removeCommandsKeybind( const std::vector<std::string>& commands ) {
	for ( auto& cmd : commands )
		removeCommandKeybind( cmd );
}

std::string KeyBindings::getCommandFromKeyBind( const KeyBindings::Shortcut& keys ) {
	auto it = mShortcuts.find( sanitizeShortcut( keys ) );
	if ( it != mShortcuts.end() ) {
		return it->second;
	}
	return "";
}

std::string KeyBindings::keybindFormat( std::string str ) {
	if ( !str.empty() ) {
		String::replace( str, "mod", KeyMod::getDefaultModifierString() );
		str[0] = std::toupper( str[0] );
		size_t found = str.find_first_of( '+' );
		while ( found != std::string::npos ) {
			if ( found + 1 < str.size() ) {
				str[found + 1] = std::toupper( str[found + 1] );
			}
			found = str.find_first_of( '+', found + 1 );
		}
		return str;
	}
	return "";
}

std::string KeyBindings::getCommandKeybindString( const std::string& command ) const {
	auto it = mKeybindingsInvert.find( command );
	if ( it == mKeybindingsInvert.end() )
		return "";
	return keybindFormat( getShortcutString( Shortcut( it->second ) ) );
}

void KeyBindings::reset() {
	mShortcuts.clear();
	mKeybindingsInvert.clear();
}

const ShortcutMap& KeyBindings::getShortcutMap() const {
	return mShortcuts;
}

const std::map<std::string, Uint64> KeyBindings::getKeybindings() const {
	return mKeybindingsInvert;
}

std::string KeyBindings::fromShortcut( const Window::Input* input, KeyBindings::Shortcut shortcut,
									   bool format ) {
	std::vector<std::string> mods;
	std::string keyname( String::toLower( input->getKeyName( shortcut.key ) ) );
	const auto& MOD_MAP = KeyMod::getModMap();
	if ( shortcut.mod & MOD_MAP.at( "mod" ) )
		mods.emplace_back( "mod" );
	if ( ( shortcut.mod & KEYMOD_CTRL ) && KEYMOD_CTRL != MOD_MAP.at( "mod" ) )
		mods.emplace_back( "ctrl" );
	if ( ( shortcut.mod & KEYMOD_SHIFT ) && KEYMOD_SHIFT != MOD_MAP.at( "mod" ) )
		mods.emplace_back( "shift" );
	if ( ( shortcut.mod & KEYMOD_LALT ) && KEYMOD_LALT != MOD_MAP.at( "mod" ) )
		mods.emplace_back( "alt" );
	if ( ( shortcut.mod & KEYMOD_RALT ) && KEYMOD_RALT != MOD_MAP.at( "mod" ) )
		mods.emplace_back( "altgr" );
	if ( ( shortcut.mod & KEYMOD_META ) && KEYMOD_META != MOD_MAP.at( "mod" ) )
		mods.emplace_back( "meta" );
	if ( mods.empty() )
		return format ? keybindFormat( keyname ) : keyname;
	auto ret = String::join( mods, '+' ) + "+" + keyname;
	return format ? keybindFormat( ret ) : ret;
}

std::string KeyBindings::getShortcutString( KeyBindings::Shortcut shortcut, bool format ) const {
	return fromShortcut( mInput, shortcut, format );
}

}} // namespace EE::UI
