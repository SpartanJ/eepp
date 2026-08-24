#include "utest.h"

#include <eepp/system/filesystem.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/ui/uiapplication.hpp>
#include <eepp/ui/uimenu.hpp>
#include <eepp/ui/uipopupmenu.hpp>
#include <eepp/ui/uiscenenode.hpp>

using namespace EE;
using namespace EE::System;
using namespace EE::UI;
using namespace EE::Window;

UTEST( UIMenu, SemanticActivationLifecycleAndRoles ) {
	UIApplication app(
		WindowSettings( 320, 240, "eepp - UIMenu Test", WindowStyle::Default,
						WindowBackend::Default, 32, {}, 1, false, true ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
	UIMenu* menu = UIMenu::New();
	menu->setParent( app.getUI() );

	int clicked = 0;
	int shown = 0;
	int hidden = 0;
	menu->on( Event::OnItemClicked, [&clicked]( const Event* ) { ++clicked; } );
	menu->on( Event::OnMenuShow, [&shown]( const Event* ) { ++shown; } );
	menu->on( Event::OnMenuHide, [&hidden]( const Event* ) { ++hidden; } );

	UIMenuItem* item = menu->add( "Item" );
	EXPECT_EQ( item->getMenuRole(), MenuRole::NoRole );
	EXPECT_EQ( item->setMenuRole( MenuRole::About ), item );
	EXPECT_EQ( item->getMenuRole(), MenuRole::About );
	item->activate();
	EXPECT_EQ( clicked, 1 );
	item->setEnabled( false );
	item->activate();
	EXPECT_EQ( clicked, 1 );
	item->setEnabled( true );

	menu->show();
	item->setOnShouldCloseCb( []( UIMenuItem* ) { return false; } );
	item->activate();
	EXPECT_TRUE( menu->isVisible() );
	EXPECT_EQ( clicked, 2 );
	item->setOnShouldCloseCb( {} );
	menu->hide();

	UIMenuCheckBox* checkBox = menu->addCheckBox( "Check" );
	EXPECT_FALSE( checkBox->isActive() );
	checkBox->activate();
	EXPECT_TRUE( checkBox->isActive() );
	checkBox->activate();
	EXPECT_FALSE( checkBox->isActive() );
	EXPECT_EQ( clicked, 4 );

	UIMenuRadioButton* radioA = menu->addRadioButton( "A", true );
	UIMenuRadioButton* radioB = menu->addRadioButton( "B" );
	radioB->activate();
	EXPECT_FALSE( radioA->isActive() );
	EXPECT_TRUE( radioB->isActive() );
	radioB->activate();
	EXPECT_TRUE( radioB->isActive() );
	EXPECT_EQ( clicked, 6 );

	menu->notifyMenuWillShow();
	menu->notifyMenuDidHide();
	EXPECT_EQ( shown, 2 );
	EXPECT_EQ( hidden, 3 );

	EXPECT_EQ( menu->getMenuBarRole(), MenuBarRole::Normal );
	EXPECT_EQ( menu->setMenuBarRole( MenuBarRole::Help ), menu );
	EXPECT_EQ( menu->getMenuBarRole(), MenuBarRole::Help );

	UIPopUpMenu* childMenu = UIPopUpMenu::New();
	childMenu->setParent( app.getUI() );
	UIMenuSubMenu* subMenu = menu->addSubMenu( "Submenu", {}, childMenu );
	int subMenuShowStep = 0;
	subMenu->on( Event::OnMenuShow, [&subMenuShowStep]( const Event* ) {
		subMenuShowStep = 0 == subMenuShowStep ? 1 : -1;
	} );
	childMenu->on( Event::OnMenuShow, [&subMenuShowStep]( const Event* ) {
		subMenuShowStep = 1 == subMenuShowStep ? 2 : -1;
	} );
	subMenu->showSubMenu();
	EXPECT_EQ( subMenuShowStep, 2 );
}
