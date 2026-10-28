#include "keybindingshelper.hpp"
#include <algorithm>
#include <array>
#include <eepp/system/log.hpp>
#include <eepp/ui/tools/keybindingsconfig.hpp>
#include <vector>

using namespace EE;
using namespace EE::UI::Tools;

namespace ecode {

using Section = KeybindingsConfig::Section;

static constexpr std::array<KeybindingsConfig::Context, 5> keybindingContexts{
	{ { "editor" },
	  { "global_search" },
	  { "document_search" },
	  { "statusbar" },
	  { "editor_mouse_bindings", KeybindingsConfig::InputType::Mouse } } };

std::span<const KeybindingsConfig::Context> KeybindingsHelper::contexts() {
	return keybindingContexts;
}

bool KeybindingsHelper::validate( const IniFile& ini, std::string& error, const Input* input ) {
	return KeybindingsConfig::validate( ini, keybindingContexts, error, input );
}

void KeybindingsHelper::updateSection( IniFile& ini, const std::string& group, Input* input,
									   std::unordered_map<std::string, std::string>& keybindings,
									   const std::unordered_map<std::string, std::string>& defaults,
									   bool forceRebind,
									   const std::map<std::string, std::string>& migrations,
									   IniFile& state, bool persist,
									   std::unordered_map<std::string, std::string>* outputInverted,
									   const KeyBindings::ShortcutMap* keyboardDefaults ) {
	Section section;
	std::string error;
	if ( !KeybindingsConfig::readSection( ini, group, section, &error ) ) {
		Log::error( "%s", error.c_str() );
		return;
	}
	// An older ecode can restore disabled defaults into the mapping section. Unbound
	// remains authoritative, and those mappings must not reserve shortcuts during recovery.
	std::erase_if( section.bindings,
				   [&]( const auto& entry ) { return section.isUnbound( entry.second.command ); } );
	bool changed = false;
	std::vector<std::pair<std::string_view, std::string_view>> migrated;
	const bool newSection = ini.findKey( group ) == IniFile::noID;
	std::unordered_map<std::string, std::string> inverted;
	section.apply( keybindings, &inverted );
	if ( !newSection && forceRebind ) {
		for ( const auto& [command, previous] : migrations ) {
			auto current = inverted.find( command );
			if ( current == inverted.end() || current->second != previous ||
				 section.bindings.at( previous ).priority > 1 ||
				 state.keyValueExists( "migrated_keybindings_" + group, command ) )
				continue;
			auto next = std::min_element(
				defaults.begin(), defaults.end(), [&]( const auto& a, const auto& b ) {
					bool am = a.second == command, bm = b.second == command;
					return am != bm ? am : a.first < b.first;
				} );
			if ( next == defaults.end() || next->second != command ||
				 ( next->first != previous &&
				   std::any_of(
					   section.bindings.begin(), section.bindings.end(), [&]( const auto& entry ) {
						   if ( entry.first == previous )
							   return false;
						   if ( group == "editor_mouse_bindings" )
							   return MouseBindings::sanitizeShortcut(
										  MouseBindings::toShortcut( entry.first ) ) ==
									  MouseBindings::sanitizeShortcut(
										  MouseBindings::toShortcut( next->first ) );
						   return input ? KeyBindings::sanitizeShortcut(
											  KeyBindings::toShortcut( input, entry.first ) ) ==
											  KeyBindings::sanitizeShortcut(
												  KeyBindings::toShortcut( input, next->first ) )
										: entry.first == next->first;
					   } ) ) )
				continue;
			auto binding = section.bindings.at( previous );
			section.bindings.erase( previous );
			section.bindings[next->first] = std::move( binding );
			migrated.emplace_back( command, previous );
			changed = true;
		}
	}
	if ( keyboardDefaults ) {
		changed |= KeybindingsConfig::mergeDefaults( section, *keyboardDefaults, input,
													 section.bindings.empty() );
	} else {
		changed |= KeybindingsConfig::mergeDefaults( section, defaults, input,
													 group == "editor_mouse_bindings"
														 ? KeybindingsConfig::InputType::Mouse
														 : KeybindingsConfig::InputType::Keyboard,
													 section.bindings.empty() );
	}
	changed |= KeybindingsConfig::writeSection( ini, group, section );
	section.apply( keybindings, outputInverted );
	// Persist the binding changes before recording the version migration as complete.
	if ( persist && changed && !ini.writeFileAtomic() ) {
		Log::error( "Could not save keybindings: %s", ini.path().c_str() );
		return;
	}
	for ( const auto& [command, previous] : migrated )
		state.setValue( "migrated_keybindings_" + group, command, previous );
	if ( persist && !migrated.empty() && !state.writeFileAtomic() )
		Log::error( "Could not save keybinding migration state: %s", state.path().c_str() );
}

void KeybindingsHelper::updateKeybindings(
	IniFile& ini, const std::string& group, Input* input,
	std::unordered_map<std::string, std::string>& keybindings,
	const std::unordered_map<std::string, std::string>& defaults, bool forceRebind,
	const std::map<std::string, std::string>& migrations, IniFile& state, bool persist ) {
	updateSection( ini, group, input, keybindings, defaults, forceRebind, migrations, state,
				   persist, nullptr, nullptr );
}

void KeybindingsHelper::updateKeybindings(
	IniFile& ini, const std::string& group, Input* input,
	std::unordered_map<std::string, std::string>& keybindings,
	std::unordered_map<std::string, std::string>& inverted,
	const KeyBindings::ShortcutMap& defaults, bool forceRebind,
	const std::map<std::string, std::string>& migrations, IniFile& state, bool persist ) {
	std::unordered_map<std::string, std::string> expressions;
	if ( forceRebind && !migrations.empty() ) {
		for ( const auto& shortcut : KeyBindings::getOrderedShortcuts( defaults ) )
			expressions[KeyBindings::fromShortcut( input, shortcut )] = defaults.at( shortcut );
	}
	updateSection( ini, group, input, keybindings, expressions, forceRebind, migrations, state,
				   persist, &inverted, &defaults );
}

void KeybindingsHelper::updateKeybindings(
	IniFile& ini, const std::string& group, Input* input,
	std::unordered_map<std::string, std::string>& keybindings,
	std::unordered_map<std::string, std::string>& inverted,
	const MouseBindings::ShortcutMap& defaults, bool forceRebind,
	const std::map<std::string, std::string>& migrations, IniFile& state, bool persist ) {
	std::unordered_map<std::string, std::string> expressions;
	for ( const auto& [shortcut, command] : defaults )
		expressions[MouseBindings::fromShortcut( shortcut )] = command;
	updateSection( ini, group, input, keybindings, expressions, forceRebind, migrations, state,
				   persist, &inverted, nullptr );
}

} // namespace ecode
