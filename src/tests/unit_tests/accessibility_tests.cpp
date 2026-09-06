#include "utest.h"

#include <eepp/system/filesystem.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/ui/accessibility/accessibilitymanager.hpp>
#include <eepp/ui/uiapplication.hpp>
#include <eepp/ui/uicheckbox.hpp>
#include <eepp/ui/uicombobox.hpp>
#include <eepp/ui/uimenu.hpp>
#include <eepp/ui/uiprogressbar.hpp>
#include <eepp/ui/uipushbutton.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uiselectbutton.hpp>
#include <eepp/ui/uislider.hpp>
#include <eepp/ui/uispinbox.hpp>
#include <eepp/ui/uitabwidget.hpp>
#include <eepp/ui/uitextedit.hpp>

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

	UISpinBox* spinBox = UISpinBox::New();
	spinBox->setParent( scene->getRoot() );
	auto spinBoxRef = manager->getNodeRef( spinBox );
	EXPECT_EQ( manager->getNodeInfo( spinBoxRef ).role, AccessibilityRole::SpinButton );
	EXPECT_TRUE(
		manager->performAction( spinBoxRef, { AccessibilityAction::SetValue, String( "7.5" ) } ) );
	EXPECT_EQ( spinBox->getValue(), 7.5 );

	UIProgressBar* progressBar = UIProgressBar::New();
	progressBar->setProgress( 25.f );
	progressBar->setParent( scene->getRoot() );
	auto progressInfo = manager->getNodeInfo( manager->getNodeRef( progressBar ) );
	EXPECT_EQ( progressInfo.role, AccessibilityRole::ProgressBar );
	EXPECT_TRUE( progressInfo.range.valid );
	Float progressValue = 0;
	EXPECT_TRUE( String::fromString( progressValue, progressInfo.value.toUtf8() ) );
	EXPECT_EQ( progressValue, 25.f );

	UITextEdit* textEdit = UITextEdit::New();
	textEdit->setParent( scene->getRoot() );
	auto textEditRef = manager->getNodeRef( textEdit );
	EXPECT_EQ( manager->getNodeInfo( textEditRef ).role, AccessibilityRole::TextBox );
	EXPECT_TRUE( manager->performAction( textEditRef,
										 { AccessibilityAction::SetText, String( "Notes" ) } ) );
	EXPECT_TRUE( manager->getNodeInfo( textEditRef ).value == String( "Notes" ) );
	textEdit->setLocked( true );
	EXPECT_TRUE( static_cast<Uint64>( manager->getNodeInfo( textEditRef ).states ) &
				 static_cast<Uint64>( AccessibilityState::ReadOnly ) );
	EXPECT_FALSE( manager->performAction( textEditRef,
										  { AccessibilityAction::SetText, String( "Blocked" ) } ) );

	UIMenu* menu = UIMenu::New();
	menu->setParent( scene->getRoot() );
	UIMenuCheckBox* menuCheckBox = menu->addCheckBox( "Line numbers" );
	UIMenuRadioButton* menuRadioButton = menu->addRadioButton( "Dark theme" );
	auto menuCheckBoxRef = manager->getNodeRef( menuCheckBox );
	auto menuRadioButtonRef = manager->getNodeRef( menuRadioButton );
	EXPECT_EQ( manager->getNodeInfo( menuCheckBoxRef ).role, AccessibilityRole::CheckMenuItem );
	EXPECT_TRUE(
		manager->performAction( menuCheckBoxRef, { AccessibilityAction::Toggle, String() } ) );
	EXPECT_TRUE( menuCheckBox->isActive() );
	EXPECT_TRUE( static_cast<Uint64>( manager->getNodeInfo( menuCheckBoxRef ).states ) &
				 static_cast<Uint64>( AccessibilityState::Checked ) );
	EXPECT_TRUE(
		manager->performAction( menuRadioButtonRef, { AccessibilityAction::Select, String() } ) );
	EXPECT_TRUE( menuRadioButton->isActive() );

	UIComboBox* comboBox = UIComboBox::New();
	comboBox->setParent( scene->getRoot() );
	comboBox->getListBox()->addListBoxItem( "First" );
	comboBox->setText( "First" );
	auto comboBoxRef = manager->getNodeRef( comboBox );
	auto comboBoxInfo = manager->getNodeInfo( comboBoxRef );
	EXPECT_EQ( comboBoxInfo.role, AccessibilityRole::ComboBox );
	EXPECT_TRUE( comboBoxInfo.value == String( "First" ) );
	EXPECT_TRUE( comboBoxInfo.actions & accessibilityActionMask( AccessibilityAction::Expand ) );
	EXPECT_TRUE( manager->performAction( comboBoxRef, { AccessibilityAction::Expand, String() } ) );
	comboBoxInfo = manager->getNodeInfo( comboBoxRef );
	EXPECT_TRUE( static_cast<Uint64>( comboBoxInfo.states ) &
				 static_cast<Uint64>( AccessibilityState::Expanded ) );
	EXPECT_TRUE( comboBoxInfo.actions & accessibilityActionMask( AccessibilityAction::Collapse ) );
	EXPECT_TRUE(
		manager->performAction( comboBoxRef, { AccessibilityAction::Collapse, String() } ) );

	UITabWidget* tabs = UITabWidget::New();
	tabs->setParent( scene->getRoot() );
	UITab* firstTab = tabs->add( "General", UIWidget::New() );
	UITab* secondTab = tabs->add( "Advanced", UIWidget::New() );
	tabs->setTabSelected( secondTab );
	auto firstTabInfo = manager->getNodeInfo( manager->getNodeRef( firstTab ) );
	auto secondTabInfo = manager->getNodeInfo( manager->getNodeRef( secondTab ) );
	EXPECT_EQ( firstTabInfo.role, AccessibilityRole::Tab );
	EXPECT_FALSE( static_cast<Uint64>( firstTabInfo.states ) &
				  static_cast<Uint64>( AccessibilityState::Selected ) );
	EXPECT_TRUE( static_cast<Uint64>( secondTabInfo.states ) &
				 static_cast<Uint64>( AccessibilityState::Selected ) );
	EXPECT_TRUE( firstTabInfo.actions & accessibilityActionMask( AccessibilityAction::Select ) );
	EXPECT_TRUE( manager->performAction( manager->getNodeRef( firstTab ),
										 { AccessibilityAction::Select, String() } ) );
	EXPECT_TRUE( tabs->getTabSelected() == firstTab );

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
