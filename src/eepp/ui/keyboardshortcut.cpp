#include <algorithm>
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
	const Uint32 alt = shortcut.mod & KEYMOD_ALT;
	if ( alt == KEYMOD_ALT )
		sanitized.mod |= KEYMOD_LALT;
	else
		sanitized.mod |= alt;
	return sanitized;
}

std::vector<KeyBindings::Shortcut>
KeyBindings::getOrderedShortcuts( const KeyBindings::ShortcutMap& bindings ) {
	std::vector<Shortcut> shortcuts;
	shortcuts.reserve( bindings.size() );
	for ( const auto& binding : bindings )
		shortcuts.emplace_back( binding.first );
	std::sort( shortcuts.begin(), shortcuts.end() );
	return shortcuts;
}

std::shared_ptr<KeyBindings::Storage> KeyBindings::getEmptyStorage() {
	static auto storage = std::make_shared<Storage>();
	return storage;
}

KeyBindings::KeyBindings( const Window::Input* input ) :
	mInput( input ), mStorage( getEmptyStorage() ) {}

void KeyBindings::setKeybinds( const KeyBindings& bindings ) {
	mStorage = bindings.mStorage;
}

void KeyBindings::ensureUniqueStorage() {
	if ( mStorage.use_count() != 1 )
		mStorage = std::make_shared<Storage>( *mStorage );
}

void KeyBindings::addKeybindsString( const std::map<std::string, std::string>& binds ) {
	for ( auto& bind : binds ) {
		addKeybindString( bind.first, bind.second );
	}
}

void KeyBindings::addKeybinds( const KeyBindings::ShortcutMap& binds ) {
	for ( const auto& shortcut : getOrderedShortcuts( binds ) ) {
		addKeybind( shortcut, binds.find( shortcut )->second );
	}
}

void KeyBindings::addKeybindsStringUnordered(
	const std::unordered_map<std::string, std::string>& binds ) {
	for ( auto& bind : binds ) {
		addKeybindString( bind.first, bind.second );
	}
}

void KeyBindings::addKeybindString( const std::string& key, const std::string& command ) {
	addKeybind( getShortcutFromString( key ), command );
}

void KeyBindings::addKeybind( const KeyBindings::Shortcut& key, const std::string& command ) {
	ensureUniqueStorage();
	auto shortcut = sanitizeShortcut( key );
	mStorage->shortcuts[shortcut] = command;
	mStorage->keybindingsInvert[command] = shortcut;
}

void KeyBindings::replaceKeybindString( const std::string& keys, const std::string& command ) {
	replaceKeybind( getShortcutFromString( keys ), command );
}

void KeyBindings::replaceKeybind( const KeyBindings::Shortcut& keys, const std::string& command ) {
	ensureUniqueStorage();
	bool erased;
	do {
		erased = false;
		auto it = mStorage->shortcuts.find( sanitizeShortcut( keys ) );
		if ( it != mStorage->shortcuts.end() ) {
			mStorage->shortcuts.erase( it );
			mStorage->keybindingsInvert.erase( it->second );
			erased = true;
		}
	} while ( erased );
	mStorage->shortcuts[sanitizeShortcut( keys )] = command;
	mStorage->keybindingsInvert[command] = sanitizeShortcut( keys );
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
	ensureUniqueStorage();
	auto it = mStorage->shortcuts.find( keys );
	if ( it != mStorage->shortcuts.end() ) {
		mStorage->shortcuts.erase( it );
	}
}

void KeyBindings::removeKeybind( const std::string& kb ) {
	removeKeybind( getShortcutFromString( kb ) );
}

bool KeyBindings::existsKeybind( const KeyBindings::Shortcut& keys ) {
	return mStorage->shortcuts.find( keys ) != mStorage->shortcuts.end();
}

bool KeyBindings::hasCommand( const std::string& command ) {
	return mStorage->keybindingsInvert.find( command ) != mStorage->keybindingsInvert.end();
}

KeyBindings::Shortcut KeyBindings::getShortcutFromCommand( const std::string& cmd ) const {
	auto it = mStorage->keybindingsInvert.find( cmd );
	if ( it != mStorage->keybindingsInvert.end() )
		return it->second;
	return {};
}

void KeyBindings::removeCommandKeybind( const std::string& command ) {
	ensureUniqueStorage();
	auto kbIt = mStorage->keybindingsInvert.find( command );
	if ( kbIt != mStorage->keybindingsInvert.end() ) {
		removeKeybind( kbIt->second );
		mStorage->keybindingsInvert.erase( command );
	}
}

void KeyBindings::removeCommandsKeybind( const std::vector<std::string>& commands ) {
	for ( auto& cmd : commands )
		removeCommandKeybind( cmd );
}

std::string KeyBindings::getCommandFromKeyBind( const KeyBindings::Shortcut& keys ) {
	auto it = mStorage->shortcuts.find( sanitizeShortcut( keys ) );
	if ( it != mStorage->shortcuts.end() ) {
		return it->second;
	}
	return "";
}

std::string KeyBindings::keybindFormat( std::string str ) {
	if ( !str.empty() ) {
		String::replace( str, "mod2", KeyMod::getDefaultSecondaryModifierString() );
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
	auto it = mStorage->keybindingsInvert.find( command );
	if ( it == mStorage->keybindingsInvert.end() )
		return "";
	return keybindFormat( getShortcutString( it->second ) );
}

void KeyBindings::reset() {
	mStorage = getEmptyStorage();
}

const KeyBindings::ShortcutMap& KeyBindings::getShortcutMap() const {
	return mStorage->shortcuts;
}

const std::map<std::string, KeyBindings::Shortcut>& KeyBindings::getKeybindings() const {
	return mStorage->keybindingsInvert;
}

std::string KeyBindings::fromShortcut( const Window::Input* input, KeyBindings::Shortcut shortcut,
									   bool format ) {
	shortcut = sanitizeShortcut( shortcut );
	std::vector<std::string> mods;
	std::string keyname( String::toLower( input->getKeyName( shortcut.key ) ) );
	const auto& MOD_MAP = KeyMod::getModMap();
	if ( shortcut.mod & MOD_MAP.at( "mod" ) )
		mods.emplace_back( "mod" );
	if ( shortcut.mod & MOD_MAP.at( "mod2" ) )
		mods.emplace_back( "mod2" );
	const Uint32 abstractMods = MOD_MAP.at( "mod" ) | MOD_MAP.at( "mod2" );
	if ( ( shortcut.mod & KEYMOD_CTRL ) && !( KEYMOD_CTRL & abstractMods ) )
		mods.emplace_back( "ctrl" );
	if ( ( shortcut.mod & KEYMOD_SHIFT ) && !( KEYMOD_SHIFT & abstractMods ) )
		mods.emplace_back( "shift" );
	if ( ( shortcut.mod & KEYMOD_LALT ) && !( KEYMOD_LALT & abstractMods ) )
		mods.emplace_back( "alt" );
	if ( ( shortcut.mod & KEYMOD_RALT ) && !( KEYMOD_RALT & abstractMods ) )
		mods.emplace_back( "altgr" );
	if ( ( shortcut.mod & KEYMOD_META ) && !( KEYMOD_META & abstractMods ) )
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
