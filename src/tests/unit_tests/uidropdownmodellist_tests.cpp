#include "utest.h"

#include <eepp/scene/actionmanager.hpp>
#include <eepp/scene/scenemanager.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/ui/models/itemlistmodel.hpp>
#include <eepp/ui/uiapplication.hpp>
#include <eepp/ui/uidropdownlist.hpp>
#include <eepp/ui/uidropdownmodellist.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uiscrollbar.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <eepp/ui/uiwindow.hpp>
#include <eepp/window/engine.hpp>

using namespace EE;
using namespace EE::Window;
using namespace EE::Scene;
using namespace EE::UI;
using namespace EE::UI::Models;

UTEST( UIDropDownModelList, basicFunctionality ) {
	UIApplication app(
		WindowSettings( 800, 600, "eepp - UIDropDownModelList Test", WindowStyle::Default,
						WindowBackend::Default, 32, {}, 1, false, true ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1.5 ) );
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	UISceneNode* sceneNode = app.getUI();

	UIDropDownModelList* dropDown = UIDropDownModelList::New();
	dropDown->setParent( sceneNode );

	std::vector<std::string> items = { "Item 1", "Item 2", "Item 3" };
	auto model = ItemListOwnerModel<std::string>::create( items );
	dropDown->setModel( model );

	// Model should be set
	EXPECT_TRUE( dropDown->getModel() == model );

	// Items count should match
	EXPECT_EQ( dropDown->getListView()->getModel()->rowCount(), 3ul );

	// Max visible items
	dropDown->setSize( 200, 30 );
	dropDown->setMaxNumVisibleItems( 3 );
	dropDown->showList();
	EXPECT_FALSE( dropDown->getListView()->getVerticalScrollBar()->isVisible() );

	dropDown->getListView()->setVisible( false );
	dropDown->setMaxNumVisibleItems( 2 );
	dropDown->showList();
	EXPECT_EQ( dropDown->getMaxNumVisibleItems(), 2ul );
	EXPECT_TRUE( dropDown->getListView()->getVerticalScrollBar()->isVisible() );
}

UTEST( UIDropDown, PlacementParentingAndFocusCompatibility ) {
	for ( Float density : { 1.f, 2.f } ) {
		UIApplication app(
			WindowSettings( 640, 480, "eepp - Dropdown Placement Test", WindowStyle::Default,
							WindowBackend::Default, 32, {}, 1, false, true ),
			UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(),
									 density ) );
		auto* ui = app.getUI();
		ui->getUIThemeManager()->setDefaultEffectsEnabled( false );
		auto* window = UIWindow::New();
		window->setParent( ui->getRoot() );
		window->setPosition( 20, 15 );
		window->setSize( 250, 180 );
		for ( bool modelBased : { false, true } ) {
			UIDropDown* dropdown;
			UIWidget* popup;
			if ( modelBased ) {
				auto* modelDropdown = UIDropDownModelList::New();
				modelDropdown->setModel( ItemListOwnerModel<std::string>::create(
					{ "A reasonably wide dropdown item", "Second item", "Third item" } ) );
				modelDropdown->getListView()->setRowHeight( 20 );
				dropdown = modelDropdown;
				popup = modelDropdown->getListView();
			} else {
				auto* listDropdown = UIDropDownList::New();
				listDropdown->getListBox()->addListBoxItems(
					{ "A reasonably wide dropdown item", "Second item", "Third item" } );
				listDropdown->getListBox()->setRowHeight( 20 );
				dropdown = listDropdown;
				popup = listDropdown->getListBox();
			}
			dropdown->setParent( window->getContainer() );
			dropdown->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
			dropdown->setSize( 100, 20 );
			dropdown->setPosition( 30, 30 );
			dropdown->setMaxNumVisibleItems( 3 );
			const Rectf bounds( ui->getWorldBounds() );
			for ( bool toRoot : { false, true } ) {
				dropdown->setPopUpToRoot( toRoot );
				for ( auto mode :
					  { UIDropDown::MenuWidthMode::DropDown, UIDropDown::MenuWidthMode::Contents,
						UIDropDown::MenuWidthMode::ContentsCentered,
						UIDropDown::MenuWidthMode::ExpandIfNeeded,
						UIDropDown::MenuWidthMode::ExpandIfNeededCentered } ) {
					dropdown->setMenuWidthMode( mode );
					dropdown->showList();
					const Rectf field( dropdown->getScreenRect() );
					const Float width = popup->getPixelsSize().getWidth();
					const bool centered = mode == UIDropDown::MenuWidthMode::ContentsCentered ||
										  mode == UIDropDown::MenuWidthMode::ExpandIfNeededCentered;
					Float x = centered ? field.Left + eefloor( ( field.getWidth() - width ) / 2 )
									   : field.Left;
					x = eeclamp( x, bounds.Left, eemax( bounds.Left, bounds.Right - width ) );
					EXPECT_TRUE( popup->getScreenPos() == Vector2f( x, field.Bottom ) );
					EXPECT_TRUE( popup->getParent() ==
								 ( toRoot ? ui->getRoot() : window->getContainer() ) );
					EXPECT_TRUE( popup->isVisible() );
					EXPECT_TRUE( popup->isEnabled() );
					EXPECT_TRUE( popup->hasFocus() );
					if ( mode == UIDropDown::MenuWidthMode::DropDown )
						EXPECT_EQ( width, field.getWidth() );
					popup->setVisible( false );
				}
			}
			// A list wider than the scene still belongs below the field when its height fits.
			dropdown->setMenuWidthMode( UIDropDown::MenuWidthMode::DropDown );
			dropdown->setPixelsSize( bounds.getWidth() + 100, 40 );
			dropdown->showList();
			EXPECT_TRUE( popup->getScreenPos() ==
						 Vector2f( bounds.Left, dropdown->getScreenRect().Bottom ) );
			popup->setVisible( false );
			dropdown->setSize( 100, 20 );
			Vector2f bottomPosition( bounds.Right - dropdown->getPixelsSize().getWidth(),
									 bounds.Bottom - dropdown->getPixelsSize().getHeight() );
			dropdown->getParent()->worldToNode( bottomPosition );
			dropdown->setPosition( bottomPosition );
			dropdown->showList();
			EXPECT_EQ( popup->getScreenRect().Bottom, dropdown->getScreenRect().Top );
			EXPECT_EQ( popup->getScreenRect().Right, bounds.Right );
			popup->getFirstChild()->setFocus();
			EXPECT_TRUE( popup->isVisible() );
			dropdown->setFocus();
			EXPECT_TRUE( popup->isVisible() );
			popup->setFocus();
			auto* other = UITextInput::New();
			other->setParent( ui->getRoot() );
			other->setFocus();
			EXPECT_FALSE( popup->isVisible() );
			EXPECT_FALSE( popup->isEnabled() );
			eeDelete( dropdown );
			eeDelete( other );
		}
	}
}

UTEST( UIDropDown, ReopenDuringFadeDoesNotHideTheNewPopup ) {
	UIApplication app(
		WindowSettings( 640, 480, "eepp - Dropdown Fade Test", WindowStyle::Default,
						WindowBackend::Default, 32, {}, 1, false, true ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
	auto* ui = app.getUI();
	ui->getUIThemeManager()->setDefaultEffectsEnabled( true );
	ui->getUIThemeManager()->setWidgetsFadeOutTime( Milliseconds( 50 ) );
	auto* dropdown = UIDropDownList::New();
	dropdown->setParent( ui->getRoot() );
	dropdown->setSize( 100, 20 );
	dropdown->getListBox()->addListBoxItems( { "First item", "Second item" } );
	auto* popup = dropdown->getListBox();
	auto* other = UITextInput::New();
	other->setParent( ui->getRoot() );
	dropdown->showList();
	EXPECT_TRUE( popup->isVisible() );
	other->setFocus();
	EXPECT_FALSE( popup->isEnabled() );
	EXPECT_TRUE( popup->isVisible() );
	dropdown->showList();
	// A visible fading popup first toggles closed; reopen after its fade completes.
	for ( int i = 0; i < 3; ++i )
		ui->getActionManager()->update( Milliseconds( 100 ) );
	dropdown->showList();
	other->setFocus();
	popup->setVisible( false );
	dropdown->showList();
	for ( int i = 0; i < 3; ++i )
		ui->getActionManager()->update( Milliseconds( 100 ) );
	EXPECT_TRUE( popup->isVisible() );
	EXPECT_TRUE( popup->isEnabled() );
	EXPECT_EQ( popup->getAlpha(), 255.f );
}

UTEST( UIDropDown, ReopenAfterDisablingEffectsRestoresOpacity ) {
	UIApplication app(
		WindowSettings( 640, 480, "eepp - Dropdown Effects Test", WindowStyle::Default,
						WindowBackend::Default, 32, {}, 1, false, true ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
	auto* ui = app.getUI();
	ui->getUIThemeManager()->setDefaultEffectsEnabled( true );
	ui->getUIThemeManager()->setWidgetsFadeOutTime( Milliseconds( 50 ) );
	auto* dropdown = UIDropDownList::New();
	dropdown->setParent( ui->getRoot() );
	dropdown->setSize( 100, 20 );
	dropdown->getListBox()->addListBoxItems( { "First item", "Second item" } );
	auto* other = UITextInput::New();
	other->setParent( ui->getRoot() );
	dropdown->showList();
	other->setFocus();
	for ( int i = 0; i < 3; ++i )
		ui->getActionManager()->update( Milliseconds( 100 ) );
	auto* popup = dropdown->getListBox();
	EXPECT_FALSE( popup->isVisible() );
	EXPECT_EQ( popup->getAlpha(), 0.f );
	ui->getUIThemeManager()->setDefaultEffectsEnabled( false );
	dropdown->showList();
	EXPECT_TRUE( popup->isVisible() );
	EXPECT_TRUE( popup->isEnabled() );
	EXPECT_EQ( popup->getAlpha(), 255.f );
}
