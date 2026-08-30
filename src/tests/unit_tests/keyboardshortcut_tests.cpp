#include "utest.h"
#include <eepp/ui/keyboardshortcut.hpp>
#include <eepp/ui/mouseshortcut.hpp>
#include <eepp/ui/uiapplication.hpp>
#include <eepp/ui/uiscenenode.hpp>

using namespace EE;
using namespace EE::UI;
using namespace EE::Window;

UTEST( KeyboardShortcut, BulkBindingsPreserveLegacyOrderedReverseLookup ) {
	const KeyBindings::Shortcut first{ KEY_A, KEYMOD_NONE };
	const KeyBindings::Shortcut second{ KEY_B, KEYMOD_CTRL };
	const KeyBindings::Shortcut third{ KEY_C, KEYMOD_CTRL | KEYMOD_SHIFT };
	KeyBindings::ShortcutMap defaults;
	defaults.emplace( third, "duplicate-command" );
	defaults.emplace( first, "duplicate-command" );
	defaults.emplace( second, "duplicate-command" );

	auto ordered = KeyBindings::getOrderedShortcuts( defaults );
	EXPECT_EQ( first.toUint64(), ordered.front().toUint64() );
	EXPECT_EQ( third.toUint64(), ordered.back().toUint64() );

	KeyBindings bindings( nullptr );
	bindings.addKeybinds( defaults );
	EXPECT_EQ( third.toUint64(),
			   bindings.getShortcutFromCommand( "duplicate-command" ).toUint64() );
}

UTEST( KeyboardShortcut, SharedBindingsDetachOnMutation ) {
	KeyBindings defaults( nullptr );
	defaults.addKeybind( { KEY_A, KEYMOD_CTRL }, "select-all" );

	KeyBindings bindings( nullptr );
	bindings.setKeybinds( defaults );
	EXPECT_TRUE( &bindings.getShortcutMap() == &defaults.getShortcutMap() );
	EXPECT_TRUE( bindings.getCommandFromKeyBind( { KEY_A, KEYMOD_CTRL } ) == "select-all" );

	bindings.addKeybind( { KEY_C, KEYMOD_CTRL }, "copy" );
	EXPECT_TRUE( &bindings.getShortcutMap() != &defaults.getShortcutMap() );
	EXPECT_TRUE( bindings.getCommandFromKeyBind( { KEY_C, KEYMOD_CTRL } ) == "copy" );
	EXPECT_TRUE( defaults.getCommandFromKeyBind( { KEY_C, KEYMOD_CTRL } ).empty() );
	EXPECT_TRUE( defaults.getCommandFromKeyBind( { KEY_A, KEYMOD_CTRL } ) == "select-all" );
}

UTEST( KeyboardShortcut, AggregateAltUsesPrimaryAltAndKeepsAltGrDistinct ) {
	UIApplication app( WindowSettings{ 320, 240, "eepp - keyboard shortcut tests" } );
	auto& bindings = app.getUI()->getKeyBindings();

	bindings.addKeybind( { KEY_1, KEYMOD_ALT }, "primary-alt" );
	EXPECT_EQ( static_cast<Uint32>( KEYMOD_LALT ),
			   bindings.getShortcutFromCommand( "primary-alt" ).mod );
	EXPECT_TRUE( bindings.getCommandFromKeyBind( { KEY_1, KEYMOD_LALT } ) == "primary-alt" );
	EXPECT_TRUE( bindings.getCommandFromKeyBind( { KEY_1, KEYMOD_RALT } ).empty() );
	EXPECT_TRUE( bindings.getShortcutString( { KEY_1, KEYMOD_ALT }, true ) == "Alt+1" );

	bindings.addKeybind( { KEY_2, KEYMOD_RALT }, "altgr" );
	EXPECT_TRUE( bindings.getCommandFromKeyBind( { KEY_2, KEYMOD_RALT } ) == "altgr" );
	EXPECT_TRUE( bindings.getCommandFromKeyBind( { KEY_2, KEYMOD_LALT } ).empty() );
	EXPECT_EQ( static_cast<Uint32>( KEYMOD_LALT ), bindings.getShortcutFromString( "lalt+1" ).mod );
}

UTEST( KeyboardShortcut, DefaultSecondaryModifierIsSerializableAndDoesNotOverlapDefault ) {
	UIApplication app( WindowSettings{ 320, 240, "eepp - keyboard shortcut tests" } );
	auto& bindings = app.getUI()->getKeyBindings();
	const Uint32 defaultModifier = KeyMod::getDefaultModifier();
	const Uint32 secondaryModifier = KeyMod::getDefaultSecondaryModifier();
	EXPECT_EQ( 0u, defaultModifier & secondaryModifier );
	EXPECT_EQ( static_cast<Uint32>( defaultModifier & KEYMOD_ALT ? KEYMOD_CTRL : KEYMOD_LALT ),
			   secondaryModifier );
	EXPECT_EQ( secondaryModifier, KeyMod::getModMap().at( "mod2" ) );
	EXPECT_EQ( secondaryModifier, KeyMod::getModMap().at( "modifier2" ) );
	EXPECT_EQ( secondaryModifier, bindings.getShortcutFromString( "mod2+1" ).mod );
	EXPECT_TRUE( bindings.getShortcutString( { KEY_1, secondaryModifier } ) == "mod2+1" );
	EXPECT_TRUE( bindings.getShortcutString( { KEY_1, secondaryModifier }, true ) ==
				 KeyBindings::keybindFormat( KeyMod::getDefaultSecondaryModifierString() + "+1" ) );
	EXPECT_TRUE(
		MouseBindings::mousebindFormat( "mod2+mouseleft" ) ==
		KeyBindings::keybindFormat( KeyMod::getDefaultSecondaryModifierString() + "+mouseleft" ) );

	KeyMod::setDefaultSecondaryModifier( KEYMOD_META );
	EXPECT_EQ( static_cast<Uint32>( KEYMOD_META ), KeyMod::getDefaultSecondaryModifier() );
	EXPECT_EQ( static_cast<Uint32>( KEYMOD_META ), KeyMod::getModMap().at( "mod2" ) );
	EXPECT_EQ( static_cast<Uint32>( KEYMOD_META ), KeyMod::getModMap().at( "modifier2" ) );
	KeyMod::setDefaultSecondaryModifier( secondaryModifier );

	KeyMod::setDefaultSecondaryModifier( KEYMOD_LALT );
	KeyMod::setDefaultModifier( KEYMOD_LALT );
	EXPECT_EQ( static_cast<Uint32>( KEYMOD_CTRL ), KeyMod::getDefaultSecondaryModifier() );
	EXPECT_EQ( 0u, KeyMod::getDefaultModifier() & KeyMod::getDefaultSecondaryModifier() );
	EXPECT_EQ( KeyMod::getDefaultSecondaryModifier(), KeyMod::getModMap().at( "mod2" ) );
	KeyMod::setDefaultModifier( defaultModifier );
	KeyMod::setDefaultSecondaryModifier( secondaryModifier );
}
