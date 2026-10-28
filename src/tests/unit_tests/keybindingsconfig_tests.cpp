#include "utest.hpp"
#include <array>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/ui/tools/keybindingsconfig.hpp>
#include <eepp/ui/tools/keybindingsconfigdata.hpp>
#include <eepp/ui/uiapplication.hpp>

using namespace EE;
using namespace EE::System;
using namespace EE::UI;
using namespace EE::UI::Tools;

UTEST( KeybindingsConfig, OptionalStateIsAppliedBeforeRuntimeDispatch ) {
	IniFile ini( "", false );
	ini.setValue( "editor", "ctrl+c", "copy" );
	ini.setValue( "editor", "ctrl+v", "paste" );
	ini.setValue( "editor", "shift+insert", "paste" );
	ini.setValue( "editor.state", "order", R"(["shift+insert","ctrl+v"])" );
	ini.setValue( "editor.state", "unbound", R"(["copy","cut","cut"] )" );
	KeybindingsConfig::Section section;
	ASSERT_TRUE( KeybindingsConfig::readSection( ini, "editor", section ) );
	EXPECT_TRUE( section.isUnbound( "copy" ) );
	EXPECT_TRUE( section.isUnbound( "cut" ) );
	EXPECT_EQ( 2u, section.unbound.size() );
	auto ordered = section.orderedBindings( "paste" );
	ASSERT_EQ( 2u, ordered.size() );
	EXPECT_TRUE( ordered[0] == "shift+insert" );
	std::unordered_map<std::string, std::string> runtime, inverted;
	section.apply( runtime, &inverted );
	EXPECT_TRUE( runtime["ctrl+v"] == "paste" );
	EXPECT_TRUE( inverted["paste"] == "shift+insert" );
	EXPECT_FALSE( runtime.count( "unbound" ) );
	KeybindingsConfig::writeSection( ini, "editor", section );
	EXPECT_FALSE( runtime.count( "ctrl+c" ) );
	EXPECT_FALSE( ini.keyValueExists( "editor", "unbound" ) );
	EXPECT_TRUE( ini.getValue( "editor.state", "unbound" ) == R"(["copy","cut"])" );
	EXPECT_TRUE( ini.getValue( "editor", "ctrl+v" ) == "paste" );
	EXPECT_TRUE( ini.getValue( "editor.state", "order" ) == R"(["shift+insert","ctrl+v"])" );
}

UTEST( KeybindingsConfig, WritesPreserveCommentsAndUnknownStateEntries ) {
	IniFile ini( "", false );
	ini.addHeaderComment( "user shortcuts" );
	ini.setValue( "editor", "ctrl+c", "copy" );
	ini.addKeyComment( "editor", "custom commands" );
	ini.setValue( "editor", "ctrl+v", "paste" );
	ini.setValue( "editor", "shift+insert", "paste" );
	ini.setValue( "editor.state", "order", R"(["shift+insert","ctrl+v"])" );
	ini.setValue( "editor.state", "unbound", "[]" );
	ini.setValue( "editor.state", "future", "preserve me" );
	KeybindingsConfig::Section section;
	ASSERT_TRUE( KeybindingsConfig::readSection( ini, "editor", section ) );
	EXPECT_TRUE( KeybindingsConfig::writeSection( ini, "editor", section ) );
	EXPECT_TRUE( ini.getValue( "editor", "ctrl+v" ) == "paste" );
	EXPECT_TRUE( ini.getValue( "editor.state", "order" ) == R"(["shift+insert","ctrl+v"])" );
	EXPECT_FALSE( KeybindingsConfig::writeSection( ini, "editor", section ) );
	section.bindings["ctrl+k"] = { "command,with\"quotes\\and=equals", 1 };
	section.unbound = { "cut" };
	KeybindingsConfig::writeSection( ini, "editor", section );
	KeybindingsConfig::Section roundtrip;
	ASSERT_TRUE( KeybindingsConfig::readSection( ini, "editor", roundtrip ) );
	EXPECT_TRUE( roundtrip.bindings.at( "ctrl+k" ).command ==
				 section.bindings.at( "ctrl+k" ).command );
	section.unbound.clear();
	section.bindings.erase( "shift+insert" );
	KeybindingsConfig::writeSection( ini, "editor", section );
	EXPECT_FALSE( ini.keyValueExists( "editor.state", "unbound" ) );
	EXPECT_FALSE( ini.keyValueExists( "editor.state", "order" ) );
	EXPECT_TRUE( ini.getValue( "editor.state", "future" ) == "preserve me" );
	EXPECT_TRUE( ini.getKeyComment( "editor", 0 ) == "custom commands" );
	EXPECT_TRUE( ini.getHeaderComment( 0 ) == "user shortcuts" );
	ini.deleteValue( "editor.state", "future" );
	KeybindingsConfig::writeSection( ini, "editor", section );
	EXPECT_EQ( IniFile::noID, ini.findKey( "editor.state" ) );
}

UTEST( KeybindingsConfig, StateOrderHandlesStaleNewAndDuplicateShortcuts ) {
	IniFile ini( "", false );
	ini.setValue( "editor", "a", "command" );
	ini.setValue( "editor", "b", "command" );
	ini.setValue( "editor", "c", "command" );
	ini.setValue( "editor.state", "order", R"(["missing","c","c","b"])" );
	KeybindingsConfig::Section section;
	ASSERT_TRUE( KeybindingsConfig::readSection( ini, "editor", section ) );
	EXPECT_TRUE( section.orderedBindings( "command" ) ==
				 std::vector<std::string>( { "c", "b", "a" } ) );
	std::unordered_map<std::string, std::string> runtime, inverted;
	section.apply( runtime, &inverted );
	EXPECT_TRUE( inverted["command"] == "c" );
	KeybindingsConfig::writeSection( ini, "editor", section );
	EXPECT_TRUE( ini.getValue( "editor.state", "order" ) == R"(["c","b","a"])" );
	for ( const auto& [shortcut, command] : ini.getKeyUnorderedMap( "editor" ) )
		EXPECT_TRUE( command == "command" );
}

UTEST( KeybindingsConfig, ReplacingManagedBindingsPreservesPluginAndWidgetOverrides ) {
	KeyBindings target( nullptr );
	const KeyBindings::ShortcutMap previous{ { { KEY_C, KEYMOD_CTRL }, "copy" },
											 { { KEY_INSERT, KEYMOD_CTRL }, "copy" },
											 { { KEY_V, KEYMOD_CTRL }, "paste" } };
	target.addKeybinds( previous );
	target.addKeybind( { KEY_F5 }, "plugin-debug" );
	target.addKeybind( { KEY_V, KEYMOD_CTRL }, "plugin-paste" );
	const KeyBindings::ShortcutMap next{ { { KEY_P, KEYMOD_CTRL }, "paste" } };
	KeybindingsConfig::replaceBindings( target, previous, next );
	EXPECT_TRUE( target.getCommandFromKeyBind( { KEY_C, KEYMOD_CTRL } ).empty() );
	EXPECT_TRUE( target.getCommandFromKeyBind( { KEY_INSERT, KEYMOD_CTRL } ).empty() );
	EXPECT_TRUE( target.getCommandFromKeyBind( { KEY_P, KEYMOD_CTRL } ) == "paste" );
	EXPECT_TRUE( target.getCommandFromKeyBind( { KEY_F5 } ) == "plugin-debug" );
	EXPECT_TRUE( target.getCommandFromKeyBind( { KEY_V, KEYMOD_CTRL } ) == "plugin-paste" );
}

UTEST( KeybindingsConfig, NewAssignmentsCannotSilentlyShadowUnmanagedPluginBindings ) {
	const KeyBindings::ShortcutMap previous{ { { KEY_C, KEYMOD_CTRL }, "copy" } };
	const KeyBindings::ShortcutMap effective{ { { KEY_C, KEYMOD_CTRL }, "plugin-copy" },
											  { { KEY_F5 }, "plugin-debug" } };
	std::string command;
	EXPECT_TRUE( KeybindingsConfig::validateOverrides( effective, previous, previous, command ) );
	const KeyBindings::ShortcutMap next{ { { KEY_F5 }, "copy" } };
	EXPECT_FALSE( KeybindingsConfig::validateOverrides( effective, previous, next, command ) );
	EXPECT_TRUE( command == "plugin-debug" );
	const KeyBindings::ShortcutMap sameOwner{ { { KEY_F5 }, "plugin-debug" } };
	EXPECT_TRUE( KeybindingsConfig::validateOverrides( effective, previous, sameOwner, command ) );
}

UTEST( KeybindingsConfig, ContextNamesAndInputTypesAreChosenByTheConsumer ) {
	UIApplication app( WindowSettings{ 320, 240, "consumer binding contexts" } );
	IniFile ini( "", false );
	ini.setValue( "viewer", "mod+f", "fullscreen" );
	ini.setValue( "gestures", "mousedown,mod+mouseleft", "pan" );
	// Unrelated application data is not interpreted as a binding context.
	ini.setValue( "editor.state", "unbound", "invalid JSON" );
	const std::array<KeybindingsConfig::Context, 2> contexts{
		{ { "viewer" }, { "gestures", KeybindingsConfig::InputType::Mouse } } };
	const auto previousModifier = KeyMod::getDefaultModifier();
	std::string error;
	EXPECT_TRUE( KeybindingsConfig::validate( ini, contexts, error, app.getWindow()->getInput() ) );
	ini.setValue( "viewer", "mod+this-key-does-not-exist", "fullscreen" );
	EXPECT_FALSE(
		KeybindingsConfig::validate( ini, contexts, error, app.getWindow()->getInput() ) );
	EXPECT_TRUE( error.find( "[viewer]" ) == 0 );
	EXPECT_EQ( previousModifier, KeyMod::getDefaultModifier() );
	// A keyboard context retains keyboard validation regardless of its name.
	const std::array<KeybindingsConfig::Context, 1> keyboard{ { { "editor_mouse_bindings" } } };
	ini.setValue( "editor_mouse_bindings", "mod+this-key-does-not-exist", "pan" );
	EXPECT_FALSE(
		KeybindingsConfig::validate( ini, keyboard, error, app.getWindow()->getInput() ) );
}

UTEST( KeybindingsConfig, ModifierReadingIsIndependentAndTransactional ) {
	IniFile ini( "", false );
	ini.setValue( "modifier", "mod", "alt" );
	const auto global = KeyMod::getDefaultModifier();
	const auto globalSecondary = KeyMod::getDefaultSecondaryModifier();
	std::string primary, secondary, error;
	ASSERT_TRUE( KeybindingsConfig::readModifiers( ini, primary, secondary, error ) );
	EXPECT_TRUE( primary == "alt" );
	EXPECT_EQ( 0u, KeyMod::getKeyMod( primary ) & KeyMod::getKeyMod( secondary ) );
	const auto expectedPrimary = primary, expectedSecondary = secondary;
	ini.setValue( "modifier", "mod2", "alt" );
	EXPECT_FALSE( KeybindingsConfig::readModifiers( ini, primary, secondary, error ) );
	EXPECT_TRUE( primary == expectedPrimary );
	EXPECT_TRUE( secondary == expectedSecondary );
	EXPECT_EQ( global, KeyMod::getDefaultModifier() );
	EXPECT_EQ( globalSecondary, KeyMod::getDefaultSecondaryModifier() );
}

UTEST( KeybindingsConfig, TolerantLoadsKeepCustomBindingsAndChooseDeterministicAliasOwners ) {
	UIApplication app( WindowSettings{ 320, 240, "legacy shortcut recovery" } );
	IniFile ini( "", false );
	ini.setValue( "modifier", "mod", "ctrl" );
	ini.setValue( "modifier", "mod2", "alt" );
	ini.setValue( "viewer", "mod+s", "save" );
	ini.setValue( "viewer", "ctrl+s", "custom-save" );
	ini.setValue( "viewer", "mod+k", "custom-action" );
	ini.setValue( "viewer", "mod+this-key-does-not-exist", "unknown" );
	ini.setValue( "viewer", "mod+c", "copy" );
	ini.setValue( "viewer", "ctrl+c", "copy" );
	ini.setValue( "search", "mod+s", "search" );
	ini.setValue( "gestures", "mousedown,mod+mouseleft", "pan" );
	const std::array<KeybindingsConfig::Context, 3> contexts{
		{ { "viewer" }, { "search" }, { "gestures", KeybindingsConfig::InputType::Mouse } } };
	std::string error;
	std::vector<std::string> warnings;
	const auto modifier = KeyMod::getDefaultModifier();
	EXPECT_FALSE(
		KeybindingsConfig::validate( ini, contexts, error, app.getWindow()->getInput() ) );
	ASSERT_TRUE( KeybindingsConfig::prepareForLoad( ini, contexts, error, warnings,
													app.getWindow()->getInput() ) );
	EXPECT_EQ( 2u, warnings.size() );
	EXPECT_TRUE( ini.getValue( "viewer", "ctrl+s" ) == "custom-save" );
	EXPECT_FALSE( ini.keyValueExists( "viewer", "mod+s" ) );
	EXPECT_FALSE( ini.keyValueExists( "viewer", "mod+this-key-does-not-exist" ) );
	EXPECT_TRUE( ini.getValue( "viewer", "mod+k" ) == "custom-action" );
	EXPECT_TRUE( ini.getValue( "viewer", "mod+c" ) == "copy" );
	EXPECT_TRUE( ini.getValue( "viewer", "ctrl+c" ) == "copy" );
	EXPECT_TRUE( ini.getValue( "search", "mod+s" ) == "search" );
	EXPECT_TRUE( ini.getValue( "gestures", "mousedown,mod+mouseleft" ) == "pan" );
	EXPECT_EQ( modifier, KeyMod::getDefaultModifier() );
	EXPECT_TRUE( KeybindingsConfig::validate( ini, contexts, error, app.getWindow()->getInput() ) );
}

UTEST( KeybindingsConfig, MalformedMetadataCannotPartiallyApplyATolerantLoad ) {
	UIApplication app( WindowSettings{ 320, 240, "transactional shortcut recovery" } );
	IniFile ini( "", false );
	ini.setValue( "viewer", "mod+this-key-does-not-exist", "custom" );
	ini.setValue( "search.state", "order", "[broken JSON]" );
	const std::array<KeybindingsConfig::Context, 2> contexts{ { { "viewer" }, { "search" } } };
	std::string error;
	std::vector<std::string> warnings{ "previous warning" };
	EXPECT_FALSE( KeybindingsConfig::prepareForLoad( ini, contexts, error, warnings,
													 app.getWindow()->getInput() ) );
	EXPECT_TRUE( ini.getValue( "viewer", "mod+this-key-does-not-exist" ) == "custom" );
	EXPECT_TRUE( warnings == std::vector<std::string>{ "previous warning" } );
}

UTEST( KeybindingsConfig, DefaultRecoveryIsOptInAndKeepsAssignmentsAndExplicitUnbindings ) {
	IniFile ini( "", false );
	ini.setValue( "viewer", "f10", "open" );
	ini.setValue( "viewer.state", "unbound", R"(["close"])" );
	KeybindingsConfig::Section section;
	ASSERT_TRUE( KeybindingsConfig::readSection( ini, "viewer", section ) );
	EXPECT_EQ( 1u, section.bindings.size() );
	const std::unordered_map<std::string, std::string> defaults{
		{ "f1", "open" }, { "f2", "close" }, { "f3", "refresh" }, { "f4", "refresh" } };
	EXPECT_TRUE( KeybindingsConfig::mergeDefaults( section, defaults, nullptr ) );
	EXPECT_EQ( 2u, section.bindings.size() );
	EXPECT_TRUE( section.bindings.at( "f10" ).command == "open" );
	EXPECT_TRUE( section.bindings.at( "f3" ).command == "refresh" );
	EXPECT_FALSE( section.bindings.count( "f1" ) );
	EXPECT_FALSE( section.bindings.count( "f2" ) );
	EXPECT_FALSE( section.bindings.count( "f4" ) );
	EXPECT_FALSE( KeybindingsConfig::mergeDefaults( section, defaults, nullptr ) );
	// Recovery changes the section only; persistence remains an explicit consumer action.
	EXPECT_FALSE( ini.keyValueExists( "viewer", "f3" ) );
}

UTEST( KeybindingsConfig, FirstRunAliasRestorationNeverAddsAliasesToExistingAssignments ) {
	KeybindingsConfig::Section section;
	section.bindings["f10"] = { "open", 1 };
	section.unbound = { "close" };
	const std::unordered_map<std::string, std::string> defaults{
		{ "f1", "open" }, { "f2", "close" }, { "f3", "refresh" }, { "f4", "refresh" } };
	EXPECT_TRUE( KeybindingsConfig::mergeDefaults( section, defaults, nullptr,
												   KeybindingsConfig::InputType::Keyboard, true ) );
	EXPECT_EQ( 3u, section.bindings.size() );
	EXPECT_FALSE( section.bindings.count( "f1" ) );
	EXPECT_FALSE( section.bindings.count( "f2" ) );
	EXPECT_TRUE( section.bindings.at( "f3" ).command == "refresh" );
	EXPECT_TRUE( section.bindings.at( "f4" ).command == "refresh" );
}

UTEST( KeybindingsConfig, TypedDefaultOrderingPreservesTheTerminalRecoveryChoice ) {
	UIApplication app( WindowSettings{ 320, 240, "typed binding defaults" } );
	const KeyBindings::ShortcutMap defaults{ { { KEY_PAGEDOWN, KEYMOD_CTRL }, "next-tab" },
											 { { KEY_TAB, KEYMOD_CTRL }, "next-tab" } };
	KeybindingsConfig::Section section;
	ASSERT_TRUE(
		KeybindingsConfig::mergeDefaults( section, defaults, app.getWindow()->getInput() ) );
	ASSERT_EQ( 1u, section.bindings.size() );
	const auto shortcut = KeyBindings::sanitizeShortcut(
		KeyBindings::toShortcut( app.getWindow()->getInput(), section.bindings.begin()->first ) );
	EXPECT_EQ( KEY_TAB, shortcut.key );
	EXPECT_EQ( static_cast<Uint32>( KEYMOD_CTRL ), shortcut.mod );
	KeybindingsConfig::Section firstRun;
	EXPECT_TRUE(
		KeybindingsConfig::mergeDefaults( firstRun, defaults, app.getWindow()->getInput(), true ) );
	EXPECT_EQ( 2u, firstRun.bindings.size() );
}

UTEST( KeybindingsConfig, MouseRecoveryUsesAnExplicitTypeAndPreservesPhysicalOwnership ) {
	KeybindingsConfig::Section section;
	const auto physicalModifier = KeyMod::getDefaultModifierString();
	const auto existing = "mousedown," + physicalModifier + "+mouseleft";
	section.bindings[existing] = { "pan", 1 };
	section.unbound = { "context-menu" };
	const MouseBindings::ShortcutMap defaults{
		{ { MouseAction::Down, EE_BUTTON_LMASK, KeyMod::getDefaultModifier() }, "select" },
		{ { MouseAction::Click, EE_BUTTON_RMASK, KEYMOD_NONE }, "context-menu" } };
	EXPECT_FALSE( KeybindingsConfig::mergeDefaults( section, defaults ) );
	EXPECT_EQ( 1u, section.bindings.size() );
	EXPECT_TRUE( section.bindings.at( existing ).command == "pan" );
}

UTEST( KeybindingsConfig, UnmodifiedMouseDefaultsRetainTheirActionAcrossSerialization ) {
	const MouseBindings::ShortcutMap defaults{
		{ { MouseAction::DoubleClick, EE_BUTTON_LMASK, KEYMOD_NONE }, "select-word" },
		{ { MouseAction::Click, EE_BUTTON_RMASK, KEYMOD_NONE }, "context-menu" } };
	KeybindingsConfig::Section section;
	ASSERT_TRUE( KeybindingsConfig::mergeDefaults( section, defaults, true ) );
	ASSERT_EQ( 2u, section.bindings.size() );
	IniFile ini( "", false );
	KeybindingsConfig::writeSection( ini, "viewer_gestures", section );
	KeybindingsConfig::Section reloaded;
	ASSERT_TRUE( KeybindingsConfig::readSection( ini, "viewer_gestures", reloaded ) );
	std::unordered_map<std::string, std::string> expressions;
	reloaded.apply( expressions );
	MouseBindings runtime;
	runtime.addMousebindsString( expressions );
	EXPECT_TRUE( runtime.getCommandFromMousebind( { MouseAction::DoubleClick, EE_BUTTON_LMASK,
													KEYMOD_NONE } ) == "select-word" );
	EXPECT_TRUE( runtime.getCommandFromMousebind(
					 { MouseAction::Click, EE_BUTTON_RMASK, KEYMOD_NONE } ) == "context-menu" );
}

UTEST( KeybindingsConfig, CatalogMergesDefaultsAssignableCommandsAndFileState ) {
	IniFile ini( "", false );
	ini.setValue( "viewer", "ctrl+k", "zoom" );
	ini.setValue( "viewer", "ctrl+o", "open" );
	ini.setValue( "viewer.state", "unbound", R"(["close"])" );
	KeybindingsConfigData data;
	KeybindingsConfig::Catalog catalog( ini, nullptr, data );
	catalog.add( "viewer",
				 std::unordered_map<std::string, std::string>{
					 { "ctrl+w", "close" }, { "ctrl+q", "close" }, { "ctrl+o", "open" } },
				 { "rotate" }, "Image viewer" );
	ASSERT_EQ( 4u, data.commands.size() );
	EXPECT_TRUE( data.commands[0].command == "close" );
	EXPECT_TRUE( ( data.commands[0].defaults == std::vector<std::string>{ "ctrl+q", "ctrl+w" } ) );
	EXPECT_TRUE( data.commands[0].unbound );
	EXPECT_TRUE( data.commands[0].bindings.empty() );
	EXPECT_TRUE( data.commands[1].bindings == std::vector<std::string>{ "ctrl+o" } );
	EXPECT_TRUE( data.commands[2].command == "rotate" );
	EXPECT_TRUE( data.commands[2].defaults.empty() );
	EXPECT_TRUE( data.commands[3].command == "zoom" );
	EXPECT_TRUE( data.commands[3].context == "viewer" );
	EXPECT_TRUE( data.contextNames.at( "viewer" ) == "Image viewer" );
}

UTEST( KeybindingsConfig, WriteConfigurationKeepsOtherContextsAndStoredOrder ) {
	IniFile ini( "", false );
	ini.setValue( "viewer", "ctrl+v", "paste" );
	ini.setValue( "viewer", "shift+insert", "paste" );
	ini.setValue( "viewer.state", "order", R"(["shift+insert","ctrl+v"])" );
	ini.setValue( "viewer", "ctrl+c", "copy" );
	ini.setValue( "search", "ctrl+f", "find" );
	ini.setValue( "gestures", "mousedown,mouseleft", "pan" );
	const std::array<KeybindingsConfig::Context, 3> contexts{
		{ { "viewer" }, { "search" }, { "gestures", KeybindingsConfig::InputType::Mouse } } };
	KeybindingsConfigData data{ { { "viewer", "paste", {}, { "shift+insert", "ctrl+v" }, {} },
								  { "viewer", "copy", {}, { "ctrl+k", "ctrl+c" }, {} },
								  { "viewer", "cut", {}, {}, {}, true },
								  { "gestures", "pan", {}, {}, {}, true } },
								"meta",
								"alt" };
	KeybindingsConfig::writeConfiguration( ini, data, contexts );
	EXPECT_TRUE( ini.getValue( "modifier", "mod" ) == "meta" );
	EXPECT_TRUE( ini.getValue( "modifier", "mod2" ) == "alt" );
	EXPECT_TRUE( ini.getValue( "viewer", "ctrl+k" ) == "copy" );
	EXPECT_TRUE( ini.getValue( "viewer.state", "order" ) ==
				 R"(["ctrl+k","ctrl+c","shift+insert","ctrl+v"])" );
	EXPECT_TRUE( ini.getValue( "viewer.state", "unbound" ) == R"(["cut"])" );
	// The editor showed no search commands, and mouse mappings are never edited.
	EXPECT_TRUE( ini.getValue( "search", "ctrl+f" ) == "find" );
	EXPECT_TRUE( ini.getValue( "gestures", "mousedown,mouseleft" ) == "pan" );
	EXPECT_FALSE( ini.keyValueExists( "gestures.state", "unbound" ) );
}

UTEST( KeybindingsConfig, LoadFileSavesRecoveryOnlyWhenNothingWasSkipped ) {
	UIApplication app( WindowSettings{ 320, 240, "keybindings file load" } );
	auto* input = app.getWindow()->getInput();
	const auto modifier = KeyMod::getDefaultModifier();
	const auto secondary = KeyMod::getDefaultSecondaryModifier();
	const auto path = Sys::getTempPath() + "eepp_keybindings_load_file.cfg";
	const std::array<KeybindingsConfig::Context, 1> contexts{ { { "viewer" } } };
	KeybindingsConfig::LoadedFile file;
	std::string error;

	// Filling in the missing modifier entries is a recovered change worth saving.
	FileSystem::fileWrite( path, std::string_view( "[viewer]\nctrl+o=open\n" ) );
	ASSERT_TRUE( KeybindingsConfig::loadFile( path, contexts, input, file, error ) );
	EXPECT_TRUE( file.canSave() );
	ASSERT_TRUE( file.save() );
	EXPECT_TRUE( IniFile( path ).keyValueExists( "modifier", "mod" ) );

	// Saving again compares with what is on disk now, so reverting a saved change is written too.
	// Reload first: the file is now normalized, so the reverted INI equals the loaded snapshot.
	ASSERT_TRUE( KeybindingsConfig::loadFile( path, contexts, input, file, error ) );
	file.ini.setValue( "viewer", "ctrl+n", "new" );
	ASSERT_TRUE( file.save() );
	EXPECT_TRUE( IniFile( path ).keyValueExists( "viewer", "ctrl+n" ) );
	file.ini.deleteValue( "viewer", "ctrl+n" );
	ASSERT_TRUE( file.save() );
	EXPECT_FALSE( IniFile( path ).keyValueExists( "viewer", "ctrl+n" ) );

	// A skipped entry keeps the original file for manual repair.
	const std::string broken = "[viewer]\nctrl+o=open\nctrl+this-key-does-not-exist=broken\n";
	FileSystem::fileWrite( path, std::string_view( broken ) );
	ASSERT_TRUE( KeybindingsConfig::loadFile( path, contexts, input, file, error ) );
	EXPECT_EQ( 1u, file.warnings.size() );
	EXPECT_FALSE( file.ini.keyValueExists( "viewer", "ctrl+this-key-does-not-exist" ) );
	EXPECT_FALSE( file.canSave() );
	file.ini.setValue( "viewer", "ctrl+n", "new" );
	EXPECT_TRUE( file.save() );
	std::string contents;
	FileSystem::fileGet( path, contents );
	EXPECT_TRUE( contents == broken );

	// A defaults-only load never writes.
	ASSERT_TRUE( KeybindingsConfig::loadFile( "", contexts, input, file, error ) );
	EXPECT_FALSE( file.canSave() );

	KeyMod::setDefaultModifier( modifier );
	KeyMod::setDefaultSecondaryModifier( secondary );
	FileSystem::fileRemove( path );
}
