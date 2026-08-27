#include "utest.h"
#include <eepp/ui/keyboardshortcut.hpp>
#include <eepp/ui/mouseshortcut.hpp>
#include <eepp/ui/uiapplication.hpp>
#include <eepp/ui/uiscenenode.hpp>

using namespace EE;
using namespace EE::UI;
using namespace EE::Window;

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
