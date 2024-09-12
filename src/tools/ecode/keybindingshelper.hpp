#ifndef KEYBINDINGS_HELPER_HPP
#define KEYBINDINGS_HELPER_HPP

#include <eepp/system/inifile.hpp>
#include <eepp/ui/keyboardshortcut.hpp>
#include <string>

using namespace EE::System;
using namespace EE::UI;

namespace ecode {

class KeybindingsHelper {
  public:
	static void updateKeybindings(
		IniFile& ini, const std::string& group, Input* input,
		std::unordered_map<std::string, std::string>& keybindings,
		const std::unordered_map<std::string, std::string>& defKeybindings, bool forceRebind,
		const std::map<std::string, std::string>& migrateKeyindings, IniFile& iniState );

	static void updateKeybindings(
		IniFile& ini, const std::string& group, Input* input,
		std::unordered_map<std::string, std::string>& keybindings,
		std::unordered_map<std::string, std::string>& invertedKeybindings,
		const std::map<KeyBindings::Shortcut, std::string>& defKeybindings, bool forceRebind,
		const std::map<std::string, std::string>& migrateKeyindings, IniFile& iniState );
};

} // namespace ecode

#endif
