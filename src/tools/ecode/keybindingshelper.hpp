#ifndef KEYBINDINGS_HELPER_HPP
#define KEYBINDINGS_HELPER_HPP

#include <eepp/core/containers.hpp>
#include <eepp/system/inifile.hpp>
#include <eepp/ui/keyboardshortcut.hpp>
#include <eepp/ui/mouseshortcut.hpp>
#include <eepp/ui/tools/keybindingsconfig.hpp>
#include <map>
#include <span>
#include <string>
#include <unordered_map>

using namespace EE::System;
using namespace EE::UI;

namespace ecode {

class KeybindingsHelper {
  public:
	/** ecode's complete context schema; migration and recovery policy remains local. */
	static std::span<const EE::UI::Tools::KeybindingsConfig::Context> contexts();

	static bool validate( const IniFile& ini, std::string& error, const Input* input = nullptr );

	static void
	updateKeybindings( IniFile& ini, const std::string& group, Input* input,
					   std::unordered_map<std::string, std::string>& keybindings,
					   const std::unordered_map<std::string, std::string>& defKeybindings,
					   bool forceRebind,
					   const std::map<std::string, std::string>& migrateKeyindings,
					   IniFile& iniState, bool persist = true );

	static void
	updateKeybindings( IniFile& ini, const std::string& group, Input* input,
					   std::unordered_map<std::string, std::string>& keybindings,
					   std::unordered_map<std::string, std::string>& invertedKeybindings,
					   const KeyBindings::ShortcutMap& defKeybindings, bool forceRebind,
					   const std::map<std::string, std::string>& migrateKeyindings,
					   IniFile& iniState, bool persist = true );

	static void
	updateKeybindings( IniFile& ini, const std::string& group, Input* input,
					   std::unordered_map<std::string, std::string>& keybindings,
					   std::unordered_map<std::string, std::string>& invertedKeybindings,
					   const MouseBindings::ShortcutMap& defKeybindings, bool forceRebind,
					   const std::map<std::string, std::string>& migrateKeyindings,
					   IniFile& iniState, bool persist = true );

  private:
	static void updateSection( IniFile& ini, const std::string& group, Input* input,
							   std::unordered_map<std::string, std::string>& keybindings,
							   const std::unordered_map<std::string, std::string>& defaults,
							   bool forceRebind,
							   const std::map<std::string, std::string>& migrations, IniFile& state,
							   bool persist, std::unordered_map<std::string, std::string>* inverted,
							   const KeyBindings::ShortcutMap* keyboardDefaults );
};

} // namespace ecode

#endif
