#include "utest.hpp"

#include <eepp/system/filesystem.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/ui/tools/uisettingspanel.hpp>
#include <eepp/ui/uiapplication.hpp>
#include <eepp/ui/uicheckbox.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uitextview.hpp>

using namespace EE;
using namespace EE::System;
using namespace EE::UI;
using namespace EE::UI::Tools;
using namespace EE::Window;

UTEST( UISettingsPanelModel, validatesCategoriesGroupsAndSettings ) {
	SettingsModel model;

	EXPECT_FALSE( model.addCategory( { {}, "General", "Behavior" } ) );
	EXPECT_TRUE( model.addCategory( { "general.behavior", "General", "Behavior" } ) );
	EXPECT_FALSE( model.addCategory( { "general.behavior", "General", "Other" } ) );
	EXPECT_TRUE( model.hasCategory( "general.behavior" ) );
	EXPECT_FALSE( model.hasCategory( "general.missing" ) );

	EXPECT_FALSE( model.addGroup( { "general.missing", "Display", 0 } ) );
	EXPECT_TRUE( model.addGroup( { "general.behavior", "Display", 0 } ) );

	bool value = false;
	EXPECT_FALSE( model.addSetting( { { {}, "general.behavior", "Empty", "Empty identifier", {} },
									  BoolPointerSetting{ &value, {} } } ) );
	EXPECT_FALSE(
		model.addSetting( { { "missing", "general.missing", "Missing", "Missing category", {} },
							BoolPointerSetting{ &value, {} } } ) );
	EXPECT_TRUE( model.addSetting(
		{ { "enabled", "general.behavior", "Enabled", "Enable the option", "Display" },
		  BoolPointerSetting{ &value, {} } } ) );
	EXPECT_FALSE( model.addSetting(
		{ { "enabled", "general.behavior", "Duplicate", "Duplicate identifier", {} },
		  BoolPointerSetting{ &value, {} } } ) );

	EXPECT_EQ( 1u, model.categories().size() );
	EXPECT_EQ( 1u, model.groups().size() );
	EXPECT_EQ( 1u, model.settings().size() );
	EXPECT_STDSTREQ( "enabled", model.settings().front().descriptor.id );
}

UTEST( UISettingsPanelModel, clearRemovesAllRules ) {
	SettingsModel model;
	bool value = true;
	EXPECT_TRUE( model.addCategory( { "editor.document", "Editor", "Document" } ) );
	EXPECT_TRUE( model.addGroup( { "editor.document", "Files", 0 } ) );
	EXPECT_TRUE(
		model.addSetting( { { "trim", "editor.document", "Trim", "Trim whitespace", "Files" },
							BoolPointerSetting{ &value, {} } } ) );

	model.clear();

	EXPECT_TRUE( model.categories().empty() );
	EXPECT_TRUE( model.groups().empty() );
	EXPECT_TRUE( model.settings().empty() );
}

UTEST( UISettingsPanel, buildsAndMaterializesCategoriesLazily ) {
	UIApplication app(
		WindowSettings( 800, 600, "eepp - UISettingsPanel Test", WindowStyle::Default,
						WindowBackend::Default, 32 ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
	auto* panel = UISettingsPanel::New( app.getUI()->getRoot() );
	bool firstValue = false;
	bool secondValue = true;

	EXPECT_TRUE( panel->addCategory( "general.behavior", "General", "Behavior" ) );
	EXPECT_TRUE( panel->addCategory( "editor.display", "Editor", "Display" ) );
	EXPECT_TRUE( panel->addBool( { "first", "general.behavior", "First", "The first setting", {} },
								 &firstValue ) );
	EXPECT_TRUE( panel->addBool( { "second", "editor.display", "Second", "The second setting", {} },
								 &secondValue ) );
	EXPECT_EQ( nullptr, panel->find<UIWidget>( "setting_first" ) );
	EXPECT_EQ( nullptr, panel->find<UIWidget>( "setting_second" ) );

	panel->build();

	EXPECT_TRUE( panel->isBuilt() );
	auto* firstRow = panel->find<UIWidget>( "setting_first" );
	EXPECT_NE( nullptr, firstRow );
	EXPECT_TRUE( firstRow->isVisible() );
	EXPECT_EQ( nullptr, panel->find<UIWidget>( "setting_second" ) );
	EXPECT_FALSE( panel->addCategory( "late.category", "Late", "Category" ) );

	panel->selectCategory( "editor.display" );
	auto* secondRow = panel->find<UIWidget>( "setting_second" );
	EXPECT_NE( nullptr, secondRow );
	EXPECT_FALSE( firstRow->isVisible() );
	EXPECT_TRUE( secondRow->isVisible() );
	EXPECT_STDSTREQ( "Display",
					 panel->find<UITextView>( "settings_page_title" )->getText().toUtf8() );

	panel->setCategoryEnabled( "editor.display", false );
	EXPECT_FALSE( secondRow->isEnabled() );
	panel->setCategoryEnabled( "editor.display", true );
	EXPECT_TRUE( secondRow->isEnabled() );
}

UTEST( UISettingsPanel, filtersAcrossUnmaterializedCategories ) {
	UIApplication app(
		WindowSettings( 800, 600, "eepp - UISettingsPanel Filter Test", WindowStyle::Default,
						WindowBackend::Default, 32 ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
	auto* panel = UISettingsPanel::New( app.getUI()->getRoot() );
	bool firstValue = false;
	bool secondValue = false;
	panel->setSearchResultsText( "Filtered Settings" );
	EXPECT_TRUE( panel->addCategory( "general.behavior", "General", "Behavior" ) );
	EXPECT_TRUE( panel->addCategory( "editor.display", "Editor", "Display" ) );
	EXPECT_TRUE( panel->addBool( { "first", "general.behavior", "First", "Ordinary option", {} },
								 &firstValue ) );
	EXPECT_TRUE( panel->addBool(
		{ "second", "editor.display", "Second", "Unique searchable phrase", {} }, &secondValue ) );
	panel->build();

	panel->setFilter( "searchable" );

	auto* firstRow = panel->find<UIWidget>( "setting_first" );
	auto* secondRow = panel->find<UIWidget>( "setting_second" );
	EXPECT_NE( nullptr, secondRow );
	EXPECT_FALSE( firstRow->isVisible() );
	EXPECT_TRUE( secondRow->isVisible() );
	EXPECT_STDSTREQ( "Filtered Settings",
					 panel->find<UITextView>( "settings_page_title" )->getText().toUtf8() );

	panel->setFilter( {} );
	EXPECT_TRUE( firstRow->isVisible() );
	EXPECT_FALSE( secondRow->isVisible() );
}
