#ifndef EE_UI_TOOLS_KEYBINDINGSCONFIGDATA_HPP
#define EE_UI_TOOLS_KEYBINDINGSCONFIGDATA_HPP

#include <eepp/core/containers.hpp>
#include <eepp/ui/keyboardshortcut.hpp>
#include <string>
#include <string_view>
#include <vector>

namespace EE::UI::Tools {

/** Application-owned command catalog. Binding expressions retain symbolic mod/mod2 tokens.
 * Binding order defines Main, Alternative, then additional shortcuts. */
struct EE_API KeybindingsCommand {
	std::string context;
	std::string command;
	String name;
	std::vector<std::string> bindings;
	std::vector<std::string> defaults;
	bool unbound{ false }; // Explicitly cleared; distinct from a command with no default shortcut.
};

struct EE_API KeybindingsConfigData {
	std::vector<KeybindingsCommand> commands;
	std::string modifier;
	std::string secondaryModifier;
	/** Optional context headings; a context without one shows its capitalized name. */
	UnorderedMap<std::string, String> contextNames;

	/** Resolve without changing the process-wide KeyMod configuration. */
	KeyBindings::Shortcut resolve( const Input* input, std::string expression ) const;

	/** Resolved shortcuts of one context, excluding explicitly unbound commands. */
	KeyBindings::ShortcutMap shortcuts( const Input* input, std::string_view context ) const;
};

} // namespace EE::UI::Tools

#endif
