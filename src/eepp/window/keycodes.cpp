#include <eepp/window/keycodes.hpp>

namespace EE { namespace Window {

Uint32 KeyMod::defaultModifier = KEYMOD_DEFAULT_MODIFIER;

static std::map<std::string, Uint32> MOD_MAP = { { "lshift", KEYMOD_SHIFT },
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
												 { "mod", KeyMod::getDefaultModifier() },
												 { "modifier", KeyMod::getDefaultModifier() } };

Uint32 KeyMod::getDefaultModifier() {
	return defaultModifier;
}

void KeyMod::setDefaultModifier( const Uint32& mod ) {
	defaultModifier = mod;
	MOD_MAP["mod"] = mod;
	MOD_MAP["modifier"] = mod;
}

std::string KeyMod::getDefaultModifierString() {
	switch ( defaultModifier ) {
		case KEYMOD_SHIFT:
			return "shift";
		case KEYMOD_LALT:
			return "alt";
		case KEYMOD_RALT:
			return "altgr";
		case KEYMOD_META:
			return "meta";
		case KEYMOD_CTRL:
		default:
			return "ctrl";
	}
}

Uint32 KeyMod::getKeyMod( std::string key ) {
	String::toLowerInPlace( key );
	auto it = MOD_MAP.find( key );
	if ( ( it != MOD_MAP.end() ) )
		return it->second;
	return 0;
}

const std::map<std::string, Uint32>& KeyMod::getModMap() {
	return MOD_MAP;
}

std::vector<Keycode> KeyMod::getKeyCodesFromModifier( Uint32 mod ) {
	std::vector<Keycode> codes;
	if ( mod & KEYMOD_LSHIFT ) {
		codes.push_back( KEY_LSHIFT );
	}
	if ( mod & KEYMOD_RSHIFT ) {
		codes.push_back( KEY_RSHIFT );
	}
	if ( mod & KEYMOD_LCTRL ) {
		codes.push_back( KEY_LCTRL );
	}
	if ( mod & KEYMOD_RCTRL ) {
		codes.push_back( KEY_RCTRL );
	}
	if ( mod & KEYMOD_LALT ) {
		codes.push_back( KEY_LALT );
	}
	if ( mod & KEYMOD_RALT ) {
		codes.push_back( KEY_RALT );
	}
	if ( mod & KEYMOD_LMETA ) {
		codes.push_back( KEY_LGUI );
	}
	if ( mod & KEYMOD_RMETA ) {
		codes.push_back( KEY_RGUI );
	}
	return codes;
}

}} // namespace EE::Window
