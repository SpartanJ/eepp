#include "utest.h"

#include <eepp/system/filesystem.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/ui/accessibility/accessibilitymanager.hpp>
#include <eepp/ui/uiapplication.hpp>
#include <eepp/ui/uicheckbox.hpp>
#include <eepp/ui/uipushbutton.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uiselectbutton.hpp>
#include <eepp/ui/uislider.hpp>

using namespace EE;
using namespace EE::System;
using namespace EE::UI;
using namespace EE::Window;

UTEST( Accessibility, LiveProjectionIdentityActionsAndInvalidation ) {
	UIApplication app(
		WindowSettings( 320, 240, "eepp - Accessibility Test", WindowStyle::Default,
						WindowBackend::Default, 32, {}, 1, false, true ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
	auto scene = app.getUI();
	auto manager = scene->getAccessibilityManager();

	UIWidget* ignoredContainer = UIWidget::New();
	ignoredContainer->setParent( scene->getRoot() );
	UIPushButton* button = UIPushButton::New();
	button->setText( "Save" );
	button->setParent( ignoredContainer );
	UICheckBox* checkbox = UICheckBox::New();
	checkbox->setText( "Autosave" );
	checkbox->setParent( scene->getRoot() );

	auto root = manager->getRoot();
	auto buttonRef = manager->getNodeRef( button );
	auto checkboxRef = manager->getNodeRef( checkbox );
	EXPECT_TRUE( root.isValid() );
	EXPECT_EQ( manager->getNodeInfo( root ).role, AccessibilityRole::Application );
	EXPECT_TRUE( manager->getNodeInfo( root ).name == String( "eepp - Accessibility Test" ) );
	EXPECT_TRUE( buttonRef == manager->getNodeRef( button ) );
	EXPECT_EQ( manager->getChildCount( root ), 2u );
	EXPECT_TRUE( manager->getChild( root, 0 ) == buttonRef );
	EXPECT_TRUE( manager->getParent( buttonRef ) == root );
	ignoredContainer->setAccessibilityHidden( true );
	EXPECT_EQ( manager->getChildCount( root ), 1u );
	ignoredContainer->setAccessibilityHidden( false );

	auto buttonInfo = manager->getNodeInfo( buttonRef );
	EXPECT_EQ( buttonInfo.role, AccessibilityRole::Button );
	EXPECT_TRUE( buttonInfo.name == String( "Save" ) );
	EXPECT_TRUE( buttonInfo.actions & accessibilityActionMask( AccessibilityAction::Press ) );

	EXPECT_FALSE( checkbox->isChecked() );
	EXPECT_TRUE( manager->performAction( checkboxRef, { AccessibilityAction::Toggle, {} } ) );
	EXPECT_TRUE( checkbox->isChecked() );
	UISlider* slider = UISlider::NewHorizontal();
	slider->setMaxValue( 100.f );
	slider->setParent( scene->getRoot() );
	auto sliderRef = manager->getNodeRef( slider );
	auto sliderInfo = manager->getNodeInfo( sliderRef );
	EXPECT_TRUE( sliderInfo.range.valid );
	EXPECT_EQ( sliderInfo.range.minimum, 0. );
	EXPECT_EQ( sliderInfo.range.maximum, 100. );
	EXPECT_TRUE(
		manager->performAction( sliderRef, { AccessibilityAction::SetValue, String( "42" ) } ) );
	EXPECT_EQ( slider->getValue(), 42.f );
	UISelectButton* selectable = UISelectButton::New();
	selectable->setParent( scene->getRoot() );
	auto selectableRef = manager->getNodeRef( selectable );
	EXPECT_TRUE( manager->getNodeInfo( selectableRef ).actions &
				 accessibilityActionMask( AccessibilityAction::Select ) );
	EXPECT_TRUE(
		manager->performAction( selectableRef, { AccessibilityAction::Select, String() } ) );
	EXPECT_TRUE( selectable->isSelected() );
	EXPECT_TRUE( static_cast<Uint64>( manager->getNodeInfo( selectableRef ).states ) &
				 static_cast<Uint64>( AccessibilityState::Selected ) );

	manager->clearPendingEvents();
	button->setAccessibilityLabel( "Save project" );
	EXPECT_TRUE( manager->getNodeInfo( buttonRef ).name == String( "Save project" ) );
	manager->notify( buttonRef, AccessibilityEvent::NameChanged );
	manager->notify( buttonRef, AccessibilityEvent::NameChanged );
	EXPECT_EQ( manager->getPendingEvents().size(), 1u );

	eeDelete( button );
	EXPECT_FALSE( manager->isValid( buttonRef ) );
	EXPECT_EQ( manager->getPendingEvents().size(), 2u );
	EXPECT_EQ( manager->getPendingEvents().back().type, AccessibilityEvent::Destroyed );
}
