#include <eepp/ui/mouseshortcut.hpp>

namespace EE { namespace UI {

MouseButtonsMask MouseBindings::getMouseButtonsMask( const std::string& strc ) {
	MouseButtonsMask mask;
	String::readBySeparator(
		strc,
		[&mask]( std::string_view str ) {
			if ( "mouseleft" == str || "mousebtn1" == str )
				mask |= EE_BUTTON_LMASK;
			if ( "mouseright" == str || "mousebtn2" == str )
				mask |= EE_BUTTON_RMASK;
			if ( "mousemiddle" == str || "mousebtn3" == str )
				mask |= EE_BUTTON_MMASK;
			if ( "mousebtn4" == str )
				mask |= EE_BUTTON_4MASK;
			if ( "mousebtn5" == str )
				mask |= EE_BUTTON_5MASK;
			if ( "mousebtn6" == str )
				mask |= EE_BUTTON_6MASK;
			if ( "mousebtn7" == str )
				mask |= EE_BUTTON_7MASK;
			if ( "mousebtn8" == str )
				mask |= EE_BUTTON_8MASK;
			if ( "mousewheelup" == str )
				mask |= EE_BUTTON_WUMASK;
			if ( "mousewheeldown" == str )
				mask |= EE_BUTTON_WDMASK;
			if ( "mousewheelleft" == str )
				mask |= EE_BUTTON_WLMASK;
			if ( "mousewheelright" == str )
				mask |= EE_BUTTON_WRMASK;
			if ( "mousewheelupdown" == str )
				mask |= EE_BUTTONS_WUWD;
			if ( "mousewheelleftright" == str )
				mask |= EE_BUTTONS_WLWR;
		},
		'+' );
	return mask;
}

std::string MouseBindings::getMouseButtonsName( MouseButtonsMask mask ) {
	std::vector<std::string> v;
	if ( mask & EE_BUTTON_LMASK )
		v.push_back( "mouseleft" );
	if ( mask & EE_BUTTON_RMASK )
		v.push_back( "mouseright" );
	if ( mask & EE_BUTTON_MMASK )
		v.push_back( "mousemiddle" );
	if ( mask & EE_BUTTON_4MASK )
		v.push_back( "mousebtn4" );
	if ( mask & EE_BUTTON_5MASK )
		v.push_back( "mousebtn5" );
	if ( mask & EE_BUTTON_6MASK )
		v.push_back( "mousebtn6" );
	if ( mask & EE_BUTTON_7MASK )
		v.push_back( "mousebtn7" );
	if ( mask & EE_BUTTON_8MASK )
		v.push_back( "mousebtn8" );
	if ( mask & EE_BUTTON_WUMASK )
		v.push_back( "mousewheelup" );
	if ( mask & EE_BUTTON_WDMASK )
		v.push_back( "mousewheeldown" );
	if ( mask & EE_BUTTON_WLMASK )
		v.push_back( "mousewheelleft" );
	if ( mask & EE_BUTTON_WRMASK )
		v.push_back( "mousewheelrigt" );
	return String::join( v, '+' );
}

MouseBindings::Shortcut MouseBindings::sanitizeShortcut( const MouseBindings::Shortcut& shortcut ) {
	MouseBindings::Shortcut sanitized( shortcut.action, shortcut.key, 0 );
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

MouseBindings::MouseBindings() {}

void MouseBindings::addMousebindsString( const std::map<std::string, std::string>& binds ) {
	for ( auto& bind : binds ) {
		addMousebindString( bind.first, bind.second );
	}
}

void MouseBindings::addMousebinds( const std::map<MouseBindings::Shortcut, std::string>& binds ) {
	for ( auto& bind : binds ) {
		addMousebind( bind.first, bind.second );
	}
}

void MouseBindings::addMousebindsStringUnordered(
	const std::unordered_map<std::string, std::string>& binds ) {
	for ( auto& bind : binds ) {
		addMousebindString( bind.first, bind.second );
	}
}

void MouseBindings::addMousebindsUnordered(
	const std::unordered_map<MouseBindings::Shortcut, std::string>& binds ) {
	for ( auto& bind : binds ) {
		addMousebind( bind.first, bind.second );
	}
}

void MouseBindings::addMousebindString( const std::string& key, const std::string& command ) {
	addMousebind( getShortcutFromString( key ), command );
}

void MouseBindings::addMousebind( const MouseBindings::Shortcut& key, const std::string& command ) {
	mShortcuts[sanitizeShortcut( key )] = command;
	mMousebindingsInvert[command] = sanitizeShortcut( key );
}

void MouseBindings::replaceMousebindString( const std::string& keys, const std::string& command ) {
	replaceMousebind( getShortcutFromString( keys ), command );
}

void MouseBindings::replaceMousebind( const MouseBindings::Shortcut& keys,
									  const std::string& command ) {
	bool erased;
	do {
		erased = false;
		auto it = mShortcuts.find( sanitizeShortcut( keys ) );
		if ( it != mShortcuts.end() ) {
			mShortcuts.erase( it );
			mMousebindingsInvert.erase( it->second );
			erased = true;
		}
	} while ( erased );
	mShortcuts[sanitizeShortcut( keys )] = command;
	mMousebindingsInvert[command] = sanitizeShortcut( keys );
}

MouseBindings::Shortcut MouseBindings::toShortcut( const std::string& keys ) {
	Shortcut shortcut;
	Uint32 mod = 0;
	auto keysSplit = String::split( keys, '+' );
	for ( auto& part : keysSplit ) {
		if ( ( mod = KeyMod::getKeyMod( part ) ) ) {
			shortcut.mod |= mod;
		} else {
			shortcut.key |= getMouseButtonsMask( part );
		}
	}
	return shortcut;
}

MouseBindings::Shortcut MouseBindings::getShortcutFromString( const std::string& keys ) {
	return toShortcut( keys );
}

void MouseBindings::removeMousebind( const MouseBindings::Shortcut& keys ) {
	auto it = mShortcuts.find( keys );
	if ( it != mShortcuts.end() ) {
		mShortcuts.erase( it );
	}
}

void MouseBindings::removeMousebind( const std::string& kb ) {
	removeMousebind( getShortcutFromString( kb ) );
}

bool MouseBindings::existsMousebind( const MouseBindings::Shortcut& keys ) {
	return mShortcuts.find( keys ) != mShortcuts.end();
}

bool MouseBindings::hasCommand( const std::string& command ) {
	return mMousebindingsInvert.find( command ) != mMousebindingsInvert.end();
}

MouseBindings::Shortcut MouseBindings::getShortcutFromCommand( const std::string& cmd ) const {
	auto it = mMousebindingsInvert.find( cmd );
	if ( it != mMousebindingsInvert.end() )
		return it->second;
	return {};
}

void MouseBindings::removeCommandMousebind( const std::string& command ) {
	auto kbIt = mMousebindingsInvert.find( command );
	if ( kbIt != mMousebindingsInvert.end() ) {
		removeMousebind( kbIt->second );
		mMousebindingsInvert.erase( command );
	}
}

void MouseBindings::removeCommandsMousebind( const std::vector<std::string>& commands ) {
	for ( auto& cmd : commands )
		removeCommandMousebind( cmd );
}

std::string MouseBindings::getCommandFromMousebind( const MouseBindings::Shortcut& keys ) {
	auto it = mShortcuts.find( sanitizeShortcut( keys ) );
	if ( it != mShortcuts.end() ) {
		return it->second;
	}
	return "";
}

std::string MouseBindings::mousebindFormat( std::string str ) {
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

std::string MouseBindings::getCommandMousebindString( const std::string& command ) const {
	auto it = mMousebindingsInvert.find( command );
	if ( it == mMousebindingsInvert.end() )
		return "";
	return mousebindFormat( getShortcutString( Shortcut( it->second ) ) );
}

void MouseBindings::reset() {
	mShortcuts.clear();
	mMousebindingsInvert.clear();
}

const MouseBindings::ShortcutMap& MouseBindings::getShortcutMap() const {
	return mShortcuts;
}

const std::map<std::string, MouseBindings::Shortcut> MouseBindings::getMousebindings() const {
	return mMousebindingsInvert;
}

std::string MouseBindings::fromShortcut( MouseBindings::Shortcut shortcut, bool format ) {
	std::vector<std::string> mods;
	std::string keyname( String::toLower( getMouseButtonsName( shortcut.key ) ) );
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
		return format ? mousebindFormat( keyname ) : keyname;
	auto ret = String::join( mods, '+' ) + "+" + keyname;
	return format ? mousebindFormat( ret ) : ret;
}

std::string MouseBindings::getShortcutString( MouseBindings::Shortcut shortcut,
											  bool format ) const {
	return fromShortcut( shortcut, format );
}

}} // namespace EE::UI
