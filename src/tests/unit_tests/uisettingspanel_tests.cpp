#include "scopedenvironmentvariable.hpp"
#include "utest.hpp"

#include <algorithm>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/ui/css/stylesheetparser.hpp>
#include <eepp/ui/tools/uisettingspanel.hpp>
#include <eepp/ui/uiapplication.hpp>
#include <eepp/ui/uicheckbox.hpp>
#include <eepp/ui/uidropdownmodellist.hpp>
#include <eepp/ui/uieventdispatcher.hpp>
#include <eepp/ui/uimessagebox.hpp>
#include <eepp/ui/uipushbutton.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uispinbox.hpp>
#include <eepp/ui/uitextinput.hpp>
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

UTEST( UISettingsPanel, appliesAndSavesWindowScaleModes ) {
	ScopedEnvironmentVariable environmentDensity( "EEPP_PIXEL_DENSITY", "1.75" );
	Float density = 0.f;
	PixelDensityMode mode = PixelDensityMode::DisplayScale;
	int saves = 0;
	Float savedDensity = 0.f;
	WindowSettings settings( 800, 600, "eepp - UI Scale Settings Test" );
	settings.DensityMode = mode;
	UIApplication app( settings, UIApplication::Settings( Sys::getProcessPath() + ".." +
														  FileSystem::getOSSlash() ) );
	auto* window = app.getWindow();
	ASSERT_NE( nullptr, window );
	auto* panel = UISettingsPanel::New( app.getUI()->getRoot() );
	ASSERT_TRUE( panel->addCategory( "appearance.fonts", "Appearance", "Fonts and Scale" ) );
	panel->addUIScaleSettings( "appearance.fonts", nullptr, density, mode, [&] { ++saves; } );
	panel->addUIScaleSettings( "appearance.fonts", window, density, mode, [&] {
		++saves;
		savedDensity = density;
	} );
	panel->build();
	panel->selectCategory( "appearance.fonts" );
	auto* modeRow = panel->find<UIWidget>( "setting_uiScaleMode" );
	auto* factorRow = panel->find<UIWidget>( "setting_uiScaleFactor" );
	ASSERT_NE( nullptr, modeRow );
	ASSERT_NE( nullptr, factorRow );
	auto* choice = modeRow->find<UIDropDownModelList>( "setting_control_widget" );
	auto* factor = factorRow->find<UISpinBox>( "setting_control_widget" );
	ASSERT_NE( nullptr, choice );
	ASSERT_NE( nullptr, factor );

	auto select = [choice]( int index ) {
		choice->getListView()->getSelection().set( choice->getModel()->index( index, 0 ) );
	};
	EXPECT_EQ( 3, choice->getListView()->getSelection().first().row() );
	EXPECT_EQ( 1.75f, window->getPixelDensityOverride() );
	EXPECT_EQ( 1.75f, factor->getValue() );
	EXPECT_EQ( 0.f, density );
	EXPECT_EQ( 0, saves );
	select( 2 );
	EXPECT_TRUE( mode == PixelDensityMode::PhysicalPPI );
	EXPECT_TRUE( window->getPixelDensityMode() == mode );
	EXPECT_EQ( 0.f, window->getPixelDensityOverride() );
	EXPECT_EQ( 0.f, savedDensity );
	EXPECT_STDSTREQ( "1.75", Sys::getEnv( "EEPP_PIXEL_DENSITY" ) );
	EXPECT_EQ( 1, saves );

	factor->setValue( 1.5 );
	EXPECT_EQ( 3, choice->getListView()->getSelection().first().row() );
	EXPECT_EQ( 1.5f, density );
	EXPECT_EQ( 1.5f, window->getPixelDensityOverride() );
	EXPECT_EQ( 2, saves );

	select( 1 );
	EXPECT_TRUE( mode == PixelDensityMode::DisplayScale );
	EXPECT_TRUE( window->getPixelDensityMode() == mode );
	EXPECT_EQ( 0.f, density );
	EXPECT_EQ( 0.f, window->getPixelDensityOverride() );
	EXPECT_EQ( 3, saves );
	EXPECT_EQ( factor->getValue(), std::clamp( window->getPixelDensity(), 1.f, 6.f ) );

	select( 0 );
	EXPECT_TRUE( mode == PixelDensityMode::BestFit );
	EXPECT_TRUE( window->getPixelDensityMode() == mode );
	EXPECT_EQ( 4, saves );

	const auto displayKey = window->getDisplayDensityKey();
	if ( !displayKey.empty() ) {
		ASSERT_TRUE( window->setDisplayDensityOverride( displayKey, 1.61f ) );
		EXPECT_EQ( 1.61f, window->getPixelDensity() );
	}
	const Float manualScale = std::clamp( window->getPixelDensity(), 1.f, 6.f );
	select( 3 );
	EXPECT_EQ( manualScale, density );
	EXPECT_EQ( manualScale, window->getPixelDensityOverride() );
	EXPECT_EQ( manualScale, factor->getValue() );
	EXPECT_EQ( 5, saves );
}

UTEST( UISettingsPanel, perDisplayOverrideStartsFromAutomaticScale ) {
	Float density = 0.f;
	PixelDensityMode mode = PixelDensityMode::PhysicalPPI;
	DisplayDensityOverrides overrides;
	WindowSettings settings( 800, 600, "eepp - Per-display Scale Test" );
	settings.DensityMode = mode;
	UIApplication app( settings, UIApplication::Settings( Sys::getProcessPath() + ".." +
														  FileSystem::getOSSlash() ) );
	auto* window = app.getWindow();
	ASSERT_NE( nullptr, window );
	const auto key = window->getDisplayDensityKey();
	if ( key.empty() )
		UTEST_SKIP( "The current display has no persistent identity" );
	const Float physicalDPI = window->getPhysicalDisplayDPI();
	if ( physicalDPI > 0.f )
		window->setPixelDensityReferencePPI( physicalDPI / 1.5f );
	const Float automatic = window->getPixelDensity();

	auto* scene = app.getUI();
	auto* panel = UISettingsPanel::New( scene->getRoot() );
	ASSERT_TRUE( panel->addCategory( "appearance.fonts", "Appearance", "Fonts and Scale" ) );
	panel->addUIScaleSettings( "appearance.fonts", window, density, mode, {}, &overrides );
	panel->build();
	panel->selectCategory( "appearance.fonts" );
	auto* row = panel->find<UIWidget>( "setting_uiDisplayScale" );
	ASSERT_NE( nullptr, row );
	auto* configure = row->find<UIPushButton>( "setting_control_widget" );
	ASSERT_NE( nullptr, configure );
	configure->sendMouseEvent( Event::MouseClick, {}, EE_BUTTON_LMASK );
	auto* dialog = scene->find<UIMessageBox>( "display_scale_dialog" );
	ASSERT_NE( nullptr, dialog );
	auto* factor = dialog->find<UISpinBox>( "display_scale_factor" );
	auto* custom = dialog->find<UICheckBox>( "display_scale_override" );
	ASSERT_NE( nullptr, factor );
	ASSERT_NE( nullptr, custom );
	EXPECT_FALSE( factor->isEnabled() );
	EXPECT_EQ( automatic, factor->getValue() );

	custom->setChecked( true );
	EXPECT_TRUE( factor->isEnabled() );
	factor->setValue( 3.1 );
	custom->setChecked( false );
	EXPECT_FALSE( factor->isEnabled() );
	EXPECT_EQ( automatic, factor->getValue() );
	EXPECT_TRUE( overrides.empty() );

	custom->setChecked( true );
	const Float seed = std::clamp( automatic, 1.f, 6.f );
	EXPECT_EQ( seed, factor->getValue() );
	dialog->sendCommonEvent( Event::OnConfirm );
	const auto saved = overrides.find( key );
	ASSERT_TRUE( saved != overrides.end() );
	EXPECT_EQ( seed, saved->second );
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
	auto* search = panel->find<UITextInput>( "settings_filter" );
	panel->focusSearch();
	app.getUI()->update( Milliseconds( 16 ) );
	app.getUI()->draw();
	EXPECT_EQ( search, app.getUI()->getUIEventDispatcher()->getFocusNode() );
	auto* firstRow = panel->find<UIWidget>( "setting_first" );
	EXPECT_NE( nullptr, firstRow );
	EXPECT_TRUE( firstRow->isVisible() );
	EXPECT_EQ( nullptr, panel->find<UIWidget>( "setting_second" ) );
	EXPECT_FALSE( panel->addCategory( "late.category", "Late", "Category" ) );
	EXPECT_FALSE( panel->addCustomWidget(
		{ "lateWidget", "general.behavior", "Late", "Late widget", {} }, []( UIWidget* parent ) {
			auto* widget = UITextView::New();
			widget->setParent( parent );
			return widget;
		} ) );

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

UTEST( UISettingsPanel, materializesCustomWidgets ) {
	UIApplication app(
		WindowSettings( 800, 600, "eepp - UISettingsPanel Custom Widget Test", WindowStyle::Default,
						WindowBackend::Default, 32 ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
	auto* panel = UISettingsPanel::New( app.getUI()->getRoot() );
	EXPECT_TRUE( panel->addCategory( "general.integration", "General", "Integration" ) );
	EXPECT_TRUE( panel->addCustomWidget(
		{ "associations", "general.integration", "Associations", "Select extensions", {} },
		[]( UIWidget* parent ) {
			auto* widget = UITextView::New();
			widget->setId( "custom_associations" );
			widget->setParent( parent );
			return widget;
		} ) );

	panel->build();

	EXPECT_NE( nullptr, panel->find<UIWidget>( "setting_associations" ) );
	EXPECT_NE( nullptr, panel->find<UIWidget>( "custom_associations" ) );
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

UTEST( UISettingsPanel, preservesStylesAcrossThemeReplacement ) {
	UIApplication app(
		WindowSettings( 800, 600, "eepp - UISettingsPanel Theme Test", WindowStyle::Default,
						WindowBackend::Default, 32 ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
	auto* scene = app.getUI();
	auto* panel = UISettingsPanel::New( scene->getRoot() );
	bool first = true;
	EXPECT_TRUE( panel->addCategory( "general.behavior", "General", "Behavior" ) );
	EXPECT_TRUE( panel->addCategory( "editor.document", "Editor", "Document" ) );
	EXPECT_TRUE(
		panel->addBool( { "first", "general.behavior", "First", "First option", {} }, &first ) );
	EXPECT_TRUE( panel->addChoice(
		{ "indent", "editor.document", "Indent", "Indent mode", {} }, { "None", "Smart" },
		[] { return size_t{ 1 }; }, []( size_t ) {} ) );
	panel->build();
	EXPECT_EQ( nullptr, panel->find<UIWidget>( "setting_indent" ) );

	// Theme replacement keeps marked component styles, as ecode does in App::setTheme().
	CSS::StyleSheetParser parser;
	ASSERT_TRUE( parser.loadFromFile( "assets/ui/breeze.css" ) );
	auto components = scene->getStyleSheet().getAllWithMarkers();
	scene->setStyleSheet( parser.getStyleSheet(), false );
	scene->getStyleSheet().combineStyleSheet( components );
	scene->getStyleSheet().updateMediaLists( scene->getMediaFeatures() );
	scene->reloadStyle( true, true, true );
	panel->selectCategory( "editor.document" );
	scene->update( Milliseconds( 16 ) );

	auto* row = panel->find<UIWidget>( "setting_indent" );
	ASSERT_NE( nullptr, row );
	EXPECT_GT( row->getPadding().Top, 0.f );
	EXPECT_EQ( 190.f, row->find<UIWidget>( "setting_control_widget" )->getSize().getWidth() );

	scene->setColorSchemePreference( ColorSchemePreference::Light );
	scene->update( Milliseconds( 16 ) );
	EXPECT_TRUE( panel->find<UIWidget>( "settings_sidebar" )->getBackgroundColor() ==
				 Color( "#ffffff" ) );
	EXPECT_EQ( 0u, row->find<UITextView>( "setting_name" )->getFontStyle() & Text::Shadow );
	EXPECT_EQ( 0u, row->find<UITextView>( "setting_description" )->getFontStyle() & Text::Shadow );
	// Existing rows must also adopt the light rules after replacing the theme.
	auto* firstRow = panel->find<UIWidget>( "setting_first" );
	EXPECT_EQ( 0u, firstRow->find<UITextView>( "setting_name" )->getFontStyle() & Text::Shadow );
	EXPECT_EQ( 0u,
			   firstRow->find<UITextView>( "setting_description" )->getFontStyle() & Text::Shadow );
}

UTEST( UISettingsPanel, updatesLightAndDarkStyles ) {
	UIApplication app(
		WindowSettings( 800, 600, "eepp - UISettingsPanel Color Scheme Test", WindowStyle::Default,
						WindowBackend::Default, 32 ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
	auto* scene = app.getUI();
	scene->setColorSchemePreference( ColorSchemePreference::Dark );
	auto* panel = UISettingsPanel::New( scene->getRoot() );
	bool value = true;
	EXPECT_TRUE( panel->addCategory( "general.behavior", "General", "Behavior" ) );
	EXPECT_TRUE( panel->addBool( { "enabled", "general.behavior", "Enabled", "Enable option", {} },
								 &value ) );
	panel->build();
	auto* sidebar = panel->find<UIWidget>( "settings_sidebar" );
	auto* name = panel->find<UITextView>( "setting_name" );
	auto* description = panel->find<UITextView>( "setting_description" );

	for ( auto scheme : { ColorSchemePreference::Dark, ColorSchemePreference::Light,
						  ColorSchemePreference::Dark, ColorSchemePreference::Light } ) {
		scene->setColorSchemePreference( scheme );
		scene->update( Milliseconds( 16 ) );
		const bool dark = scheme == ColorSchemePreference::Dark;
		EXPECT_TRUE( sidebar->getBackgroundColor() == Color( dark ? "#232629" : "#ffffff" ) );
		EXPECT_EQ( dark, ( name->getFontStyle() & Text::Shadow ) != 0 );
		EXPECT_EQ( 0u, description->getFontStyle() & Text::Shadow );
	}
}

UTEST( UISettingsPanel, fullSizeCustomWidgetUsesViewportAndRestoresSettingsScroll ) {
	UIApplication app( WindowSettings{ 1000, 760, "settings viewport test" } );
	auto* panel = UISettingsPanel::New( app.getUI()->getRoot() );
	panel->addCategory( "general.shortcuts", "General", "Shortcuts" );
	panel->addCategory( "general.behavior", "General", "Behavior" );
	panel->addCustomWidget(
		{ "shortcuts", "general.shortcuts", "Shortcuts", {} },
		[]( UIWidget* parent ) {
			auto* widget = UILinearLayout::NewVertical();
			widget->setParent( parent );
			widget->setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::MatchParent );
			return widget;
		},
		true );
	bool value = false;
	panel->addBool( { "enabled", "general.behavior", "Enabled", {} }, &value );
	panel->build();
	panel->selectCategory( "general.shortcuts" );
	app.getUI()->update( Milliseconds( 16 ) );
	auto* page = panel->find<UIWidget>( "settings_full_page" );
	auto* scroll = panel->find<UIWidget>( "settings_scroll" );
	EXPECT_TRUE( page->isVisible() );
	EXPECT_FALSE( scroll->isVisible() );
	auto* row = panel->find<UIWidget>( "setting_shortcuts" );
	EXPECT_EQ( page, row->getParent() );
	EXPECT_FALSE( row->find<UIWidget>( "setting_info" )->isVisible() );
	panel->selectCategory( "general.behavior" );
	EXPECT_FALSE( page->isVisible() );
	EXPECT_TRUE( scroll->isVisible() );
	panel->setFilter( "Shortcuts" );
	EXPECT_TRUE( scroll->isVisible() );
	EXPECT_NE( page, row->getParent() );
	panel->setFilter( "" );
	panel->selectCategory( "general.shortcuts" );
	EXPECT_TRUE( page->isVisible() );
	EXPECT_EQ( page, row->getParent() );
}
