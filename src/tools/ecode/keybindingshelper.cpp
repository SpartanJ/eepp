#include "keybindingshelper.hpp"

namespace ecode {

void KeybindingsHelper::updateKeybindings(
	IniFile& ini, const std::string& group, Input* input,
	std::unordered_map<std::string, std::string>& keybindings,
	const std::unordered_map<std::string, std::string>& defKeybindings, bool forceRebind,
	const std::map<std::string, std::string>& migrateKeyindings, IniFile& iniState ) {
	KeyBindings bindings( input );
	bool added = false;
	bool migrated = false;

	if ( ini.findKey( group ) != IniFile::noID ) {
		keybindings = ini.getKeyUnorderedMap( group );
	} else {
		for ( const auto& it : defKeybindings )
			ini.setValue( group, it.first, it.second );
		added = true;
	}
	std::unordered_map<std::string, std::string> invertedKeybindings;
	for ( const auto& key : keybindings )
		invertedKeybindings[key.second] = key.first;

	if ( !added && forceRebind ) {
		for ( const auto& migrate : migrateKeyindings ) {
			auto foundCmd = invertedKeybindings.find( migrate.first );
			if ( foundCmd != invertedKeybindings.end() && foundCmd->second == migrate.second ) {
				std::string shortcut;
				for ( const auto& defKb : defKeybindings ) {
					if ( defKb.second == foundCmd->first ) {
						shortcut = defKb.first;
						break;
					}
				}
				if ( !shortcut.empty() &&
					 !iniState.keyValueExists( "migrated_keybindings_" + group, migrate.first ) ) {
					ini.setValue( group, shortcut, foundCmd->first );
					ini.deleteValue( group, migrate.second );
					keybindings.erase( migrate.second );
					invertedKeybindings[foundCmd->first] = shortcut;
					iniState.setValue( "migrated_keybindings_" + group, migrate.first,
									   migrate.second );
					added = true;
					migrated = true;
				}
			}
		}
	}

	if ( defKeybindings.size() != keybindings.size() || forceRebind ) {
		for ( const auto& key : defKeybindings ) {
			auto foundCmd = invertedKeybindings.find( key.second );
			auto& shortcutStr = key.first;
			if ( foundCmd == invertedKeybindings.end() &&
				 keybindings.find( shortcutStr ) == keybindings.end() ) {
				keybindings[shortcutStr] = key.second;
				invertedKeybindings[key.second] = shortcutStr;
				ini.setValue( group, shortcutStr, key.second );
				added = true;
			} else if ( foundCmd == invertedKeybindings.end() ) {
				// Override the shortcut if the command that holds that
				// shortcut does not exists anymore
				auto kb = keybindings.find( shortcutStr );
				if ( kb != keybindings.end() ) {
					bool found = false;
					for ( const auto& val : defKeybindings )
						if ( val.second == kb->second )
							found = true;
					if ( !found ) {
						keybindings[shortcutStr] = key.second;
						invertedKeybindings[key.second] = shortcutStr;
						ini.setValue( group, shortcutStr, key.second );
						added = true;
					}
				}
			}
		}
	}

	if ( migrated )
		iniState.writeFile();
	if ( added )
		ini.writeFile();
}

void KeybindingsHelper::updateKeybindings(
	IniFile& ini, const std::string& group, Input* input,
	std::unordered_map<std::string, std::string>& keybindings,
	std::unordered_map<std::string, std::string>& invertedKeybindings,
	const std::map<KeyBindings::Shortcut, std::string>& defKeybindings, bool forceRebind,
	const std::map<std::string, std::string>& migrateKeyindings, IniFile& iniState ) {
	KeyBindings bindings( input );
	bool added = false;
	bool migrated = false;

	if ( ini.findKey( group ) != IniFile::noID ) {
		keybindings = ini.getKeyUnorderedMap( group );
	} else {
		for ( const auto& it : defKeybindings )
			ini.setValue( group, bindings.getShortcutString( it.first ), it.second );
		added = true;
	}
	for ( const auto& key : keybindings )
		invertedKeybindings[key.second] = key.first;

	if ( !added && forceRebind ) {
		for ( const auto& migrate : migrateKeyindings ) {
			auto foundCmd = invertedKeybindings.find( migrate.first );
			if ( foundCmd != invertedKeybindings.end() && foundCmd->second == migrate.second ) {
				KeyBindings::Shortcut shortcut;
				for ( const auto& defKb : defKeybindings ) {
					if ( defKb.second == foundCmd->first ) {
						shortcut = defKb.first;
						break;
					}
				}
				if ( !iniState.keyValueExists( "migrated_keybindings_" + group, migrate.first ) ) {
					if ( !shortcut.empty() ) {
						auto newShortcutStr = bindings.getShortcutString( shortcut );
						ini.setValue( group, newShortcutStr, foundCmd->first );
						invertedKeybindings[foundCmd->first] = newShortcutStr;
					} else {
						invertedKeybindings.erase( foundCmd->first );
					}
					ini.deleteValue( group, migrate.second );
					keybindings.erase( migrate.second );
					iniState.setValue( "migrated_keybindings_" + group, migrate.first,
									   migrate.second );
					added = true;
					migrated = true;
				}
			}
		}
	}

	bool keybindingsWereEmpty = keybindings.empty();

	if ( defKeybindings.size() != keybindings.size() || forceRebind ) {
		for ( auto& key : defKeybindings ) {
			auto foundCmd = invertedKeybindings.find( key.second );
			auto shortcutStr = bindings.getShortcutString( key.first );

			if ( ( foundCmd == invertedKeybindings.end() || keybindingsWereEmpty ) &&
				 keybindings.find( shortcutStr ) == keybindings.end() ) {
				keybindings[shortcutStr] = key.second;
				invertedKeybindings[key.second] = shortcutStr;
				ini.setValue( group, shortcutStr, key.second );
				added = true;
			} else if ( foundCmd == invertedKeybindings.end() ) {
				// Override the shortcut if the command that holds that
				// shortcut does not exists anymore
				auto kb = keybindings.find( shortcutStr );
				if ( kb != keybindings.end() ) {
					bool found = false;
					for ( const auto& val : defKeybindings )
						if ( val.second == kb->second )
							found = true;
					if ( !found ) {
						keybindings[shortcutStr] = key.second;
						invertedKeybindings[key.second] = shortcutStr;
						ini.setValue( group, shortcutStr, key.second );
						added = true;
					}
				}
			}
		}
	}
	if ( migrated )
		iniState.writeFile();
	if ( added )
		ini.writeFile();
}

void KeybindingsHelper::updateKeybindings(
	IniFile& ini, const std::string& group, Input* input,
	std::unordered_map<std::string, std::string>& keybindings,
	std::unordered_map<std::string, std::string>& invertedKeybindings,
	const MouseBindings::ShortcutMap& defKeybindings, bool forceRebind,
	const std::map<std::string, std::string>& migrateKeyindings, IniFile& iniState ) {
	MouseBindings bindings;
	bool added = false;
	bool migrated = false;

	if ( ini.findKey( group ) != IniFile::noID ) {
		keybindings = ini.getKeyUnorderedMap( group );
	} else {
		for ( const auto& it : defKeybindings )
			ini.setValue( group, bindings.getShortcutString( it.first ), it.second );
		added = true;
	}
	for ( const auto& key : keybindings )
		invertedKeybindings[key.second] = key.first;

	if ( !added && forceRebind ) {
		for ( const auto& migrate : migrateKeyindings ) {
			auto foundCmd = invertedKeybindings.find( migrate.first );
			if ( foundCmd != invertedKeybindings.end() && foundCmd->second == migrate.second ) {
				MouseBindings::Shortcut shortcut;
				for ( const auto& defKb : defKeybindings ) {
					if ( defKb.second == foundCmd->first ) {
						shortcut = defKb.first;
						break;
					}
				}
				if ( !iniState.keyValueExists( "migrated_keybindings_" + group, migrate.first ) ) {
					if ( !shortcut.empty() ) {
						auto newShortcutStr = bindings.getShortcutString( shortcut );
						ini.setValue( group, newShortcutStr, foundCmd->first );
						invertedKeybindings[foundCmd->first] = newShortcutStr;
					} else {
						invertedKeybindings.erase( foundCmd->first );
					}
					ini.deleteValue( group, migrate.second );
					keybindings.erase( migrate.second );
					iniState.setValue( "migrated_keybindings_" + group, migrate.first,
									   migrate.second );
					added = true;
					migrated = true;
				}
			}
		}
	}

	bool keybindingsWereEmpty = keybindings.empty();

	if ( defKeybindings.size() != keybindings.size() || forceRebind ) {
		for ( auto& key : defKeybindings ) {
			auto foundCmd = invertedKeybindings.find( key.second );
			auto shortcutStr = bindings.getShortcutString( key.first );

			if ( ( foundCmd == invertedKeybindings.end() || keybindingsWereEmpty ) &&
				 keybindings.find( shortcutStr ) == keybindings.end() ) {
				keybindings[shortcutStr] = key.second;
				invertedKeybindings[key.second] = shortcutStr;
				ini.setValue( group, shortcutStr, key.second );
				added = true;
			} else if ( foundCmd == invertedKeybindings.end() ) {
				// Override the shortcut if the command that holds that
				// shortcut does not exists anymore
				auto kb = keybindings.find( shortcutStr );
				if ( kb != keybindings.end() ) {
					bool found = false;
					for ( const auto& val : defKeybindings )
						if ( val.second == kb->second )
							found = true;
					if ( !found ) {
						keybindings[shortcutStr] = key.second;
						invertedKeybindings[key.second] = shortcutStr;
						ini.setValue( group, shortcutStr, key.second );
						added = true;
					}
				}
			}
		}
	}
	if ( migrated )
		iniState.writeFile();
	if ( added )
		ini.writeFile();
}

} // namespace ecode
