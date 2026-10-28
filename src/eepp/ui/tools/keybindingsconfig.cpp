#include <eepp/ui/tools/keybindingsconfig.hpp>

#include <algorithm>
#include <eepp/core/containers.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/iostreamstring.hpp>
#include <eepp/system/log.hpp>
#include <limits>
#include <map>
#include <nlohmann/json.hpp>

using namespace EE::System;

namespace EE::UI::Tools {

bool KeybindingsConfig::Section::isUnbound( const std::string& command ) const {
	return std::find( unbound.begin(), unbound.end(), command ) != unbound.end();
}

std::vector<std::string>
KeybindingsConfig::Section::orderedBindings( const std::string& command ) const {
	std::vector<std::string> result;
	if ( isUnbound( command ) )
		return result;
	for ( const auto& [shortcut, binding] : bindings ) {
		if ( binding.command == command )
			result.push_back( shortcut );
	}
	std::sort( result.begin(), result.end(), [this]( const auto& a, const auto& b ) {
		const auto pa = bindings.at( a ).priority;
		const auto pb = bindings.at( b ).priority;
		if ( pa != pb )
			return pa < pb;
		return a < b;
	} );
	return result;
}

void KeybindingsConfig::Section::apply(
	std::unordered_map<std::string, std::string>& target,
	std::unordered_map<std::string, std::string>* inverted ) const {
	target.clear();
	if ( inverted )
		inverted->clear();
	for ( const auto& [shortcut, binding] : bindings ) {
		if ( isUnbound( binding.command ) )
			continue;
		target[shortcut] = binding.command;
		if ( !inverted )
			continue;
		auto [it, inserted] = inverted->try_emplace( binding.command, shortcut );
		if ( !inserted ) {
			Uint32 previous = bindings.at( it->second ).priority;
			if ( binding.priority < previous )
				it->second = shortcut;
		}
	}
}

bool KeybindingsConfig::LoadedFile::canSave() const {
	return !ini.path().empty() && warnings.empty();
}

bool KeybindingsConfig::LoadedFile::save() {
	if ( !canSave() )
		return true;
	IOStreamString contents;
	ini.writeStream( contents );
	if ( contents.getStream() == mLoaded )
		return true;
	if ( !ini.writeFileAtomic() )
		return false;
	// Later saves compare against what is now on disk, not what was originally loaded.
	mLoaded = contents.getStream();
	return true;
}

KeybindingsConfig::Catalog::Catalog( const IniFile& ini, const Input* input,
									 KeybindingsConfigData& output ) :
	mIni( ini ), mInput( input ), mOutput( output ) {}

void KeybindingsConfig::Catalog::add( std::string_view context,
									  const KeyBindings::ShortcutMap& defaults,
									  const std::vector<std::string>& commands,
									  const String& name ) {
	std::vector<std::pair<std::string, std::string>> ordered;
	ordered.reserve( defaults.size() );
	for ( const auto& shortcut : KeyBindings::getOrderedShortcuts( defaults ) )
		ordered.emplace_back( KeyBindings::fromShortcut( mInput, shortcut ),
							  defaults.at( shortcut ) );
	add( context, std::move( ordered ), commands, name );
}

void KeybindingsConfig::Catalog::add( std::string_view context,
									  const std::unordered_map<std::string, std::string>& defaults,
									  const std::vector<std::string>& commands,
									  const String& name ) {
	std::vector<std::pair<std::string, std::string>> ordered( defaults.begin(), defaults.end() );
	std::sort( ordered.begin(), ordered.end() );
	add( context, std::move( ordered ), commands, name );
}

void KeybindingsConfig::Catalog::add(
	std::string_view context, std::vector<std::pair<std::string, std::string>> orderedDefaults,
	const std::vector<std::string>& commands, const String& name ) {
	// Callers validate the file before building a catalog; an unreadable section is empty.
	Section section;
	readSection( mIni, context, section );
	std::map<std::string, KeybindingsCommand> catalog;
	for ( auto& [expression, command] : orderedDefaults )
		catalog[command].defaults.push_back( std::move( expression ) );
	for ( const auto& command : commands )
		catalog.try_emplace( command );
	for ( const auto& [shortcut, binding] : section.bindings )
		catalog.try_emplace( binding.command );
	for ( const auto& command : section.unbound )
		catalog.try_emplace( command );
	mOutput.commands.reserve( mOutput.commands.size() + catalog.size() );
	for ( auto& [command, entry] : catalog ) {
		entry.context = context;
		entry.command = command;
		entry.bindings = section.orderedBindings( command );
		entry.unbound = section.isUnbound( command );
		mOutput.commands.push_back( std::move( entry ) );
	}
	if ( !name.empty() )
		mOutput.contextNames[std::string( context )] = name;
}

bool KeybindingsConfig::readSection( const IniFile& ini, std::string_view group, Section& output,
									 std::string* error ) {
	Section section;
	auto fail = [&]( const std::string& key, const std::string& reason ) {
		if ( error )
			*error = "[" + std::string( group ) + "] " + key + ": " + reason;
		return false;
	};
	for ( const auto& [shortcut, raw] : ini.getKeyUnorderedMap( group ) ) {
		std::string_view value = String::trim( std::string_view( raw ), " \t\r\n" );
		if ( value.empty() )
			return fail( shortcut, "command must not be empty" );
		Binding binding{ std::string( value ), 1 };
		section.bindings.emplace( shortcut, std::move( binding ) );
	}
	const auto stateGroup = std::string( group ) + ".state";
	auto readArray = [&]( const std::string& key, std::vector<std::string>& values ) {
		if ( !ini.keyValueExists( stateGroup, key ) )
			return true;
		auto stateError = [&]( const char* reason ) {
			if ( error )
				*error = "[" + stateGroup + "] " + key + ": " + reason;
			return false;
		};
		auto json = nlohmann::json::parse( ini.getValue( stateGroup, key ), nullptr, false );
		if ( !json.is_array() )
			return stateError( "expected a JSON array of strings" );
		for ( const auto& value : json ) {
			if ( !value.is_string() || value.get_ref<const std::string&>().empty() )
				return stateError( "each entry must be a nonempty string" );
			values.push_back( value.get<std::string>() );
		}
		return true;
	};
	std::vector<std::string> order;
	if ( !readArray( "unbound", section.unbound ) || !readArray( "order", order ) )
		return false;
	UnorderedMap<std::string, Uint32> orderedCommands;
	for ( const auto& shortcut : order ) {
		auto found = section.bindings.find( shortcut );
		if ( found != section.bindings.end() )
			orderedCommands.try_emplace( found->second.command, 0 );
	}
	// Listed shortcuts come before unlisted ones. Stale entries are harmless when a user
	// edits only the mappings, and new shortcuts have a deterministic lexical fallback.
	for ( auto& [shortcut, binding] : section.bindings ) {
		if ( orderedCommands.count( binding.command ) )
			binding.priority = std::numeric_limits<Uint32>::max();
	}
	for ( const auto& shortcut : order ) {
		auto found = section.bindings.find( shortcut );
		if ( found != section.bindings.end() &&
			 found->second.priority == std::numeric_limits<Uint32>::max() )
			found->second.priority = ++orderedCommands.at( found->second.command );
	}
	// Older editors can restore disabled defaults. Keep unbound authoritative in
	// apply(), orderedBindings(), and writeSection().
	std::sort( section.unbound.begin(), section.unbound.end() );
	section.unbound.erase( std::unique( section.unbound.begin(), section.unbound.end() ),
						   section.unbound.end() );
	output = std::move( section );
	return true;
}

bool KeybindingsConfig::writeSection( IniFile& ini, std::string_view group,
									  const Section& section ) {
	bool changed = false;
	auto set = [&]( std::string_view targetGroup, const std::string& key,
					const std::string& value ) {
		if ( !ini.keyValueExists( targetGroup, key ) ||
			 ini.getValue( targetGroup, key ) != value ) {
			ini.setValue( targetGroup, key, value );
			changed = true;
		}
	};
	for ( const auto& [shortcut, raw] : ini.getKeyUnorderedMap( group ) ) {
		auto found = section.bindings.find( shortcut );
		if ( found == section.bindings.end() || section.isUnbound( found->second.command ) )
			changed |= ini.deleteValue( group, shortcut );
	}
	UnorderedSet<std::string> commands;
	for ( const auto& [shortcut, binding] : section.bindings ) {
		if ( section.isUnbound( binding.command ) )
			continue;
		set( group, shortcut, binding.command );
		if ( binding.priority > 1 )
			commands.insert( binding.command );
	}
	std::vector<std::string> order;
	// Only store ordering when lexical order would change Main/Alternative. Default
	// files and commands with a single shortcut need no state at all.
	std::vector<std::string> sortedCommands( commands.begin(), commands.end() );
	std::sort( sortedCommands.begin(), sortedCommands.end() );
	for ( const auto& command : sortedCommands ) {
		auto shortcuts = section.orderedBindings( command );
		if ( !std::is_sorted( shortcuts.begin(), shortcuts.end() ) )
			order.insert( order.end(), shortcuts.begin(), shortcuts.end() );
	}
	auto unbound = section.unbound;
	std::sort( unbound.begin(), unbound.end() );
	unbound.erase( std::unique( unbound.begin(), unbound.end() ), unbound.end() );
	const auto stateGroup = std::string( group ) + ".state";
	if ( unbound.empty() )
		changed |= ini.deleteValue( stateGroup, "unbound" );
	else
		set( stateGroup, "unbound", nlohmann::json( unbound ).dump() );
	if ( order.empty() )
		changed |= ini.deleteValue( stateGroup, "order" );
	else
		set( stateGroup, "order", nlohmann::json( order ).dump() );
	if ( ini.findKey( stateGroup ) != IniFile::noID && ini.getNumValues( stateGroup ) == 0 &&
		 ini.getNumKeyComments( stateGroup ) == 0 )
		changed |= ini.deleteKey( stateGroup );
	return changed;
}

void KeybindingsConfig::writeConfiguration( IniFile& ini,
											const KeybindingsConfigData& configuration,
											std::span<const Context> contexts ) {
	ini.setValue( "modifier", "mod", configuration.modifier );
	ini.setValue( "modifier", "mod2", configuration.secondaryModifier );
	for ( const auto& context : contexts ) {
		const auto inContext = [&]( const KeybindingsCommand& entry ) {
			return entry.context == context.name;
		};
		// A context the editor did not show must keep its file contents.
		if ( context.type == InputType::Mouse ||
			 std::none_of( configuration.commands.begin(), configuration.commands.end(),
						   inContext ) )
			continue;
		Section current, desired;
		readSection( ini, context.name, current );
		for ( const auto& entry : configuration.commands ) {
			if ( !inContext( entry ) )
				continue;
			if ( entry.bindings.empty() && entry.unbound )
				desired.unbound.push_back( entry.command );
			// Unchanged commands keep their stored priorities; edited ones follow the new order.
			const bool unchanged = entry.bindings == current.orderedBindings( entry.command );
			for ( size_t i = 0; i < entry.bindings.size(); ++i ) {
				const auto& shortcut = entry.bindings[i];
				desired.bindings[shortcut] =
					unchanged ? current.bindings.at( shortcut )
							  : Binding{ entry.command, static_cast<Uint32>( i + 1 ) };
			}
		}
		writeSection( ini, context.name, desired );
	}
}

bool KeybindingsConfig::readModifiers( const IniFile& ini, std::string& primary,
									   std::string& secondaryModifier, std::string& error ) {
	auto modifier = ini.getValue( "modifier", "mod", KeyMod::getDefaultModifierString() );
	auto secondary =
		ini.getValue( "modifier", "mod2", KeyMod::getDefaultSecondaryModifierString() );
	for ( const auto& name : { modifier, secondary } ) {
		const auto lower = String::toLower( name );
		if ( !KeyMod::getKeyMod( name ) || lower == "mod" || lower == "mod2" ||
			 lower == "modifier" || lower == "modifier2" ) {
			error = "[modifier] Invalid modifier: " + name;
			return false;
		}
	}
	if ( KeyMod::getKeyMod( modifier ) & KeyMod::getKeyMod( secondary ) ) {
		if ( ini.keyValueExists( "modifier", "mod2" ) ) {
			error = "[modifier] Default and secondary modifiers overlap";
			return false;
		}
		secondary = KeyMod::getKeyMod( modifier ) & KEYMOD_ALT ? "ctrl" : "alt";
	}
	primary = std::move( modifier );
	secondaryModifier = std::move( secondary );
	return true;
}

bool KeybindingsConfig::validate( const IniFile& ini, std::span<const Context> contexts,
								  std::string& error, const Input* input ) {
	std::string modifier, secondary;
	if ( !readModifiers( ini, modifier, secondary, error ) )
		return false;
	KeybindingsConfigData resolver{ {}, std::move( modifier ), std::move( secondary ) };
	for ( const auto& context : contexts ) {
		Section section;
		if ( !readSection( ini, context.name, section, &error ) )
			return false;
		if ( !input || context.type == InputType::Mouse )
			continue;
		KeyBindings::ShortcutMap owners;
		for ( const auto& [expression, binding] : section.bindings ) {
			auto shortcut = resolver.resolve( input, expression );
			if ( shortcut.key == KEY_UNKNOWN || KeyMod::getKeyMod( expression ) ) {
				error = "[" + std::string( context.name ) + "] Invalid shortcut: " + expression;
				return false;
			}
			if ( section.isUnbound( binding.command ) )
				continue;
			auto [owner, inserted] = owners.emplace( shortcut, binding.command );
			if ( !inserted && owner->second != binding.command ) {
				error = "[" + std::string( context.name ) +
						"] Conflicting shortcut: " + expression + " (" + owner->second + ", " +
						binding.command + ")";
				return false;
			}
		}
	}
	return true;
}

bool KeybindingsConfig::prepareForLoad( IniFile& ini, std::span<const Context> contexts,
										std::string& error, std::vector<std::string>& warnings,
										const Input* input ) {
	// Validate metadata before making any changes, including in earlier contexts.
	if ( !validate( ini, contexts, error ) )
		return false;
	std::string modifier, secondary;
	if ( !readModifiers( ini, modifier, secondary, error ) )
		return false;
	KeybindingsConfigData resolver{ {}, std::move( modifier ), std::move( secondary ) };
	std::vector<std::string> issues;
	for ( const auto& context : contexts ) {
		if ( !input || context.type == InputType::Mouse )
			continue;
		Section section;
		readSection( ini, context.name, section );
		KeyBindings::ShortcutMap owners;
		bool changed = false;
		for ( auto it = section.bindings.begin(); it != section.bindings.end(); ) {
			const auto& [expression, binding] = *it;
			if ( section.isUnbound( binding.command ) ) {
				++it;
				continue;
			}
			auto shortcut = resolver.resolve( input, expression );
			std::string reason;
			if ( shortcut.key == KEY_UNKNOWN || KeyMod::getKeyMod( expression ) ) {
				reason = "Invalid shortcut: " + expression;
			} else {
				auto [owner, inserted] = owners.emplace( shortcut, binding.command );
				if ( !inserted && owner->second != binding.command ) {
					reason = "Conflicting shortcut: " + expression + " (keeping " + owner->second +
							 ", skipping " + binding.command + ")";
				}
			}
			if ( reason.empty() ) {
				++it;
			} else {
				issues.push_back( "[" + std::string( context.name ) + "] " + reason );
				it = section.bindings.erase( it );
				changed = true;
			}
		}
		if ( changed )
			writeSection( ini, context.name, section );
	}
	warnings = std::move( issues );
	return true;
}

bool KeybindingsConfig::loadFile( const std::string& path, std::span<const Context> contexts,
								  const Input* input, LoadedFile& output, std::string& error ) {
	LoadedFile file;
	file.ini = IniFile( path, false );
	if ( !path.empty() && FileSystem::fileExists( path ) && !file.ini.loadFromFile( path ) ) {
		error = "Could not read keybindings: " + path;
		return false;
	}
	std::string modifier, secondary;
	if ( !prepareForLoad( file.ini, contexts, error, file.warnings, input ) ||
		 !readModifiers( file.ini, modifier, secondary, error ) )
		return false;
	for ( const auto& warning : file.warnings )
		Log::warning( "%s", warning.c_str() );
	// Snapshot before normalizing, so filling in missing modifier entries counts as a change.
	IOStreamString loaded;
	file.ini.writeStream( loaded );
	file.mLoaded = loaded.getStream();
	KeyMod::setDefaultModifier( KeyMod::getKeyMod( modifier ) );
	KeyMod::setDefaultSecondaryModifier( KeyMod::getKeyMod( secondary ) );
	file.ini.setValue( "modifier", "mod", modifier );
	file.ini.setValue( "modifier", "mod2", secondary );
	output = std::move( file );
	return true;
}

template <typename Range>
bool KeybindingsConfig::mergeDefaultsOrdered( Section& section, const Range& defaults,
											  const Input* input, InputType type,
											  bool restoreAllAliases ) {
	bool changed = std::erase_if( section.bindings, [&]( const auto& entry ) {
					   return section.isUnbound( entry.second.command );
				   } ) != 0;
	UnorderedSet<std::string_view> assigned;
	UnorderedMap<KeyBindings::Shortcut, bool, KeyBindings::ShortcutHash> occupiedKeys;
	UnorderedMap<MouseBindings::Shortcut, bool, MouseBindings::ShortcutHash> occupiedMouse;
	auto occupy = [&]( const std::string& expression ) {
		if ( type == InputType::Mouse )
			return occupiedMouse
				.emplace(
					MouseBindings::sanitizeShortcut( MouseBindings::toShortcut( expression ) ),
					true )
				.second;
		if ( input )
			return occupiedKeys
				.emplace(
					KeyBindings::sanitizeShortcut( KeyBindings::toShortcut( input, expression ) ),
					true )
				.second;
		return true;
	};
	for ( const auto& [expression, binding] : section.bindings ) {
		assigned.insert( binding.command );
		occupy( expression );
	}
	// Preserve all existing command assignments, even when restoring every default alias.
	// Borrow names from map nodes: std::map insertions do not invalidate these views.
	for ( const auto& [expression, command] : defaults ) {
		if ( section.isUnbound( command ) || section.bindings.count( expression ) ||
			 assigned.count( command ) || !occupy( expression ) )
			continue;
		auto added = section.bindings.emplace( expression, Binding{ command, 1 } );
		if ( !restoreAllAliases )
			assigned.insert( added.first->second.command );
		changed = true;
	}
	return changed;
}

bool KeybindingsConfig::mergeDefaults( Section& section,
									   const std::unordered_map<std::string, std::string>& defaults,
									   const Input* input, InputType type,
									   bool restoreAllAliases ) {
	std::map<std::string, std::string> ordered( defaults.begin(), defaults.end() );
	return mergeDefaultsOrdered( section, ordered, input, type, restoreAllAliases );
}

bool KeybindingsConfig::mergeDefaults( Section& section, const KeyBindings::ShortcutMap& defaults,
									   const Input* input, bool restoreAllAliases ) {
	if ( !input )
		return false;
	std::vector<std::pair<std::string, std::string>> ordered;
	ordered.reserve( defaults.size() );
	for ( const auto& shortcut : KeyBindings::getOrderedShortcuts( defaults ) )
		ordered.emplace_back( KeyBindings::fromShortcut( input, shortcut ),
							  defaults.at( shortcut ) );
	return mergeDefaultsOrdered( section, ordered, input, InputType::Keyboard, restoreAllAliases );
}

bool KeybindingsConfig::mergeDefaults( Section& section, const MouseBindings::ShortcutMap& defaults,
									   bool restoreAllAliases ) {
	std::map<std::string, std::string> ordered;
	for ( const auto& [shortcut, command] : defaults )
		ordered[MouseBindings::fromShortcut( shortcut )] = command;
	return mergeDefaultsOrdered( section, ordered, nullptr, InputType::Mouse, restoreAllAliases );
}

void KeybindingsConfig::replaceBindings( KeyBindings& target,
										 const KeyBindings::ShortcutMap& previous,
										 const KeyBindings::ShortcutMap& next ) {
	// Copy only the externally installed layer. A plugin may intentionally override a
	// managed shortcut; it must retain the same precedence after a settings reload.
	KeyBindings::ShortcutMap overlays;
	for ( const auto& [shortcut, command] : target.getShortcutMap() ) {
		auto found = previous.find( shortcut );
		if ( found == previous.end() || found->second != command )
			overlays.emplace( shortcut, command );
	}
	target.reset();
	target.addKeybinds( next );
	target.addKeybinds( overlays );
}

bool KeybindingsConfig::validateOverrides( const KeyBindings::ShortcutMap& effective,
										   const KeyBindings::ShortcutMap& previous,
										   const KeyBindings::ShortcutMap& next,
										   std::string& conflictingCommand ) {
	for ( const auto& [shortcut, command] : next ) {
		auto old = previous.find( shortcut );
		if ( old != previous.end() && old->second == command )
			continue;
		auto active = effective.find( shortcut );
		if ( active != effective.end() && active->second != command &&
			 ( old == previous.end() || old->second != active->second ) ) {
			conflictingCommand = active->second;
			return false;
		}
	}
	return true;
}

} // namespace EE::UI::Tools
