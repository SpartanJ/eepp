#ifndef EE_UI_TOOLS_KEYBINDINGSCONFIG_HPP
#define EE_UI_TOOLS_KEYBINDINGSCONFIG_HPP

#include <eepp/system/inifile.hpp>
#include <eepp/ui/keyboardshortcut.hpp>
#include <eepp/ui/mouseshortcut.hpp>
#include <eepp/ui/tools/keybindingsconfigdata.hpp>
#include <functional>
#include <map>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace EE::UI::Tools {

using namespace EE::System;

/** INI binding data and operations independent of UI widgets and application migration policy.
 * Reading and validation never alter process-wide modifiers or installed shortcuts. */
class EE_API KeybindingsConfig {
  public:
	enum class InputType : Uint8 { Keyboard, Mouse };

	/** Names are borrowed for the duration of validate(); the caller defines every context. */
	struct Context {
		std::string_view name;
		InputType type{ InputType::Keyboard };
	};

	struct Binding {
		std::string command;
		Uint32 priority{ 1 }; // Internal order; mappings on disk are plain commands.
	};

	struct EE_API Section {
		std::map<std::string, Binding> bindings;
		std::vector<std::string> unbound;

		bool isUnbound( const std::string& command ) const;

		std::vector<std::string> orderedBindings( const std::string& command ) const;

		/** Replace output maps, ignoring explicitly unbound commands. */
		void apply( std::unordered_map<std::string, std::string>& target,
					std::unordered_map<std::string, std::string>* inverted = nullptr ) const;

		/** Add the bindings to `target`, skipping explicitly unbound commands and, when a filter
		 * is given, the commands it rejects. Each command's Main shortcut becomes the one
		 * target.getCommandKeybindString() returns, as shown in menus and tooltips. */
		void apply( KeyBindings& target,
					const std::function<bool( const std::string& )>& filter = {} ) const;
	};

	/** Result of loadFile(): the tolerant configuration with normalized modifier entries. */
	struct EE_API LoadedFile {
		IniFile ini;
		std::vector<std::string> warnings;

		/** False for a defaults-only load or when entries were skipped. Skipped entries stay on
		 * disk for manual repair, so recovered changes must not overwrite them. */
		bool canSave() const;

		/** Write the INI when canSave() and it differs from the last loaded or saved contents.
		 * Return false only when a required write failed. */
		bool save();

	  private:
		friend class KeybindingsConfig;
		std::string mLoaded;
	};

	/** Builds an editor command list from a parsed file. Each context lists its defaults, the
	 * assignable commands without one, and the file's bindings and explicit unbindings. */
	class EE_API Catalog {
	  public:
		Catalog( const IniFile& ini, const Input* input, KeybindingsConfigData& output );

		/** Defaults keep getOrderedShortcuts() order for Main, Alternative, and extras. */
		void add( std::string_view context, const KeyBindings::ShortcutMap& defaults,
				  const std::vector<std::string>& commands = {}, const String& name = {} );

		/** Expression defaults are ordered lexically. */
		void add( std::string_view context,
				  const std::unordered_map<std::string, std::string>& defaults,
				  const std::vector<std::string>& commands = {}, const String& name = {} );

	  private:
		const IniFile& mIni;
		const Input* mInput;
		KeybindingsConfigData& mOutput;

		void add( std::string_view context,
				  std::vector<std::pair<std::string, std::string>> orderedDefaults,
				  const std::vector<std::string>& commands, const String& name );
	};

	/** Read plain commands and [context.state] order/unbound arrays. Failure leaves output
	 * untouched. */
	static bool readSection( const IniFile& ini, std::string_view group, Section& output,
							 std::string* error = nullptr );

	/** Update the INI in memory. Preserve comments/unknown state entries; return whether it
	 * changed. */
	static bool writeSection( IniFile& ini, std::string_view group, const Section& section );

	/** Write the modifiers and every keyboard context that has commands in `configuration`.
	 * Unchanged commands keep their stored order. Mouse and absent contexts are untouched. */
	static void writeConfiguration( IniFile& ini, const KeybindingsConfigData& configuration,
									std::span<const Context> contexts );

	/** Read physical modifier preferences, deriving missing values from KeyMod's current defaults.
	 * Failure leaves both outputs untouched. Does not modify KeyMod or write the INI. */
	static bool readModifiers( const IniFile& ini, std::string& primary, std::string& secondary,
							   std::string& error );

	/** Validate only caller-supplied contexts. With input, resolve keyboard names and collisions.
	 * Mouse contexts are decoded without imposing keyboard validation on their expressions. */
	static bool validate( const IniFile& ini, std::span<const Context> contexts, std::string& error,
						  const Input* input = nullptr );

	/** Prepare an in-memory load without discarding usable shortcuts. Skip unknown keyboard
	 * names and conflicting aliases, retaining the lexically first expression for each physical
	 * shortcut. Report every skipped entry. Malformed state/modifiers still fail transactionally.
	 * Callers must not persist the recovered INI when warnings are present: retain the original
	 * file for manual repair. This never changes global modifiers or installed bindings. */
	static bool prepareForLoad( IniFile& ini, std::span<const Context> contexts, std::string& error,
								std::vector<std::string>& warnings, const Input* input );

	/** Runtime load. An empty path loads no file (compiled defaults only). Reads the file,
	 * applies prepareForLoad() and logs each warning, validates the modifiers, then applies them
	 * to KeyMod and normalizes `modifier.mod`/`mod2` in the INI. Failure changes nothing. */
	static bool loadFile( const std::string& path, std::span<const Context> contexts,
						  const Input* input, LoadedFile& output, std::string& error );

	/** Runtime load for an application with a single keyboard context: loadFile(), restore
	 * missing defaults (every default alias while the section doesn't exist yet), save the
	 * recovered file, and replace `section` with the active configuration; install it with
	 * Section::apply(). `defaults` runs after the file's modifiers are applied, so symbolic
	 * defaults use them. Failure changes nothing. */
	static bool loadContext( const std::string& path, std::string_view context,
							 const std::function<KeyBindings::ShortcutMap()>& defaults,
							 const Input* input, Section& section, std::string& error );

	/** Opt-in recovery. Never overwrite occupied physical shortcuts, explicit assignments, or
	 * unbindings. By default add one shortcut per missing command; restoreAllAliases also adds
	 * every supplied alias for missing commands. Return whether the section changed; no I/O.
	 * Symbolic expressions use the current KeyMod preferences, which callers apply explicitly.
	 * Pass input to resolve keyboard aliases; null checks keyboard occupancy by expression only. */
	static bool mergeDefaults( Section& section,
							   const std::unordered_map<std::string, std::string>& defaults,
							   const Input* input, InputType type = InputType::Keyboard,
							   bool restoreAllAliases = false );

	/** Typed defaults are considered in getOrderedShortcuts() order. A null input leaves
	 * the section unchanged because keyboard names cannot be serialized without it. */
	static bool mergeDefaults( Section& section, const KeyBindings::ShortcutMap& defaults,
							   const Input* input, bool restoreAllAliases = false );

	static bool mergeDefaults( Section& section, const MouseBindings::ShortcutMap& defaults,
							   bool restoreAllAliases = false );

	/** Replace the managed layer while retaining widget/plugin overrides. */
	static void replaceBindings( KeyBindings& target, const KeyBindings::ShortcutMap& previous,
								 const KeyBindings::ShortcutMap& next );

	/** Reject changed assignments shadowed by unmanaged bindings. */
	static bool validateOverrides( const KeyBindings::ShortcutMap& effective,
								   const KeyBindings::ShortcutMap& previous,
								   const KeyBindings::ShortcutMap& next,
								   std::string& conflictingCommand );

  private:
	template <typename Range>
	static bool mergeDefaultsOrdered( Section& section, const Range& defaults, const Input* input,
									  InputType type, bool restoreAllAliases );
};

} // namespace EE::UI::Tools

#endif
