#include "utest.h"

#include <eepp/system/filesystem.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/ui/accessibility/accessibilitymanager.hpp>
#include <eepp/ui/models/itemlistmodel.hpp>
#include <eepp/ui/models/stringmapmodel.hpp>
#include <eepp/ui/uiapplication.hpp>
#include <eepp/ui/uicheckbox.hpp>
#include <eepp/ui/uicombobox.hpp>
#include <eepp/ui/uilistview.hpp>
#include <eepp/ui/uimenu.hpp>
#include <eepp/ui/uiprogressbar.hpp>
#include <eepp/ui/uipushbutton.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uiselectbutton.hpp>
#include <eepp/ui/uislider.hpp>
#include <eepp/ui/uispinbox.hpp>
#include <eepp/ui/uitableview.hpp>
#include <eepp/ui/uitabwidget.hpp>
#include <eepp/ui/uitextedit.hpp>
#include <eepp/ui/uitreeview.hpp>

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
	auto rootChildren = manager->getChildren( root );
	EXPECT_EQ( rootChildren.size(), 2u );
	EXPECT_TRUE( rootChildren[0] == buttonRef );
	EXPECT_TRUE( rootChildren[1] == checkboxRef );
	EXPECT_TRUE( manager->getParent( buttonRef ) == root );
	ignoredContainer->applyProperty( CSS::StyleSheetProperty( "aria-hidden", "true" ) );
	EXPECT_EQ( manager->getChildCount( root ), 1u );
	ignoredContainer->applyProperty( CSS::StyleSheetProperty( "aria-hidden", "false" ) );

	auto buttonInfo = manager->getNodeInfo( buttonRef );
	EXPECT_EQ( buttonInfo.role, AccessibilityRole::Button );
	EXPECT_TRUE( buttonInfo.name == String( "Save" ) );
	EXPECT_TRUE( buttonInfo.actions & accessibilityActionMask( AccessibilityAction::Press ) );
	button->applyProperty( CSS::StyleSheetProperty( "aria-label", "Save item" ) );
	button->applyProperty( CSS::StyleSheetProperty( "aria-description", "Saves the item" ) );
	EXPECT_TRUE( manager->getNodeInfo( buttonRef ).name == String( "Save item" ) );
	EXPECT_TRUE( manager->getNodeInfo( buttonRef ).description == String( "Saves the item" ) );
	button->setAccessibilityLabel( {} );

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

	UITextInput* textInput = UITextInput::New();
	textInput->setText( "Project" );
	textInput->getDocument().setSelection( { 0, 3 } );
	textInput->setParent( scene->getRoot() );
	auto textInputRef = manager->getNodeRef( textInput );
	auto textInputInfo = manager->getNodeInfo( textInputRef );
	EXPECT_TRUE( textInputInfo.text.valid );
	EXPECT_EQ( textInputInfo.text.caretOffset, 3 );
	EXPECT_EQ( textInputInfo.text.selectionStart, 3 );
	EXPECT_EQ( textInputInfo.text.selectionEnd, 3 );
	textInput->setText( String::fromUtf8( std::string( "A😀é" ) ) );
	textInput->getDocument().setSelection( { 0, 4 } );
	textInputInfo = manager->getNodeInfo( textInputRef );
	EXPECT_EQ( textInputInfo.text.caretOffset, 4 );
	EXPECT_TRUE( textInputInfo.actions &
				 accessibilityActionMask( AccessibilityAction::SetTextSelection ) );
	EXPECT_TRUE( manager->performAction(
		textInputRef, { AccessibilityAction::SetTextSelection, String( "2:4" ) } ) );
	textInputInfo = manager->getNodeInfo( textInputRef );
	EXPECT_EQ( textInputInfo.text.selectionStart, 2 );
	EXPECT_EQ( textInputInfo.text.selectionEnd, 4 );
	textInput->setMode( UITextInput::TextInputMode::Password );
	textInputInfo = manager->getNodeInfo( textInputRef );
	EXPECT_TRUE( static_cast<Uint64>( textInputInfo.states ) &
				 static_cast<Uint64>( AccessibilityState::Protected ) );
	EXPECT_FALSE( textInputInfo.text.valid );
	EXPECT_FALSE( textInputInfo.actions &
				  accessibilityActionMask( AccessibilityAction::SetTextSelection ) );

	UITextEdit* textEdit = UITextEdit::New();
	textEdit->setParent( scene->getRoot() );
	auto textEditRef = manager->getNodeRef( textEdit );
	EXPECT_EQ( manager->getNodeInfo( textEditRef ).role, AccessibilityRole::TextBox );
	EXPECT_TRUE( manager->performAction( textEditRef,
										 { AccessibilityAction::SetText, String( "Notes" ) } ) );
	EXPECT_TRUE( manager->getNodeInfo( textEditRef ).value == String( "Notes" ) );
	EXPECT_TRUE( manager->getNodeInfo( textEditRef ).name.empty() );
	EXPECT_TRUE( manager->getNodeInfo( textEditRef, false ).value.empty() );
	textEdit->setText( "first\nsecond" );
	textEdit->getDocument().setSelection( { 0, 2 }, { 1, 3 } );
	auto textEditInfo = manager->getNodeInfo( textEditRef );
	EXPECT_TRUE( textEditInfo.text.valid );
	EXPECT_EQ( textEditInfo.text.selectionStart, 2 );
	EXPECT_EQ( textEditInfo.text.selectionEnd, 9 );
	EXPECT_TRUE( manager->performAction(
		textEditRef, { AccessibilityAction::SetTextSelection, String( "6:8" ) } ) );
	auto textEditSelection = textEdit->getDocument().getSelection( true );
	EXPECT_EQ( textEditSelection.start().line(), 1 );
	EXPECT_EQ( textEditSelection.start().column(), 0 );
	EXPECT_EQ( textEditSelection.end().line(), 1 );
	EXPECT_EQ( textEditSelection.end().column(), 2 );
	EXPECT_TRUE( manager->performAction(
		textEditRef, { AccessibilityAction::SetTextSelection, String( "12:12" ) } ) );
	textEditSelection = textEdit->getDocument().getSelection( true );
	EXPECT_EQ( textEditSelection.start().line(), 1 );
	EXPECT_EQ( textEditSelection.start().column(), 6 );
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

	UITableView* table = UITableView::New();
	table->setParent( scene->getRoot() );
	auto tableModel = ItemPairListOwnerModel<std::string, std::string>::create(
		{ { "Alpha", "One" }, { "Beta", "Two" } } );
	tableModel->setColumnName( 0, "Name" );
	tableModel->setColumnName( 1, "Value" );
	table->setModel( tableModel );
	auto tableRef = manager->getNodeRef( table );
	EXPECT_EQ( manager->getNodeInfo( tableRef ).role, AccessibilityRole::Table );
	EXPECT_EQ( manager->getChildCount( tableRef ), 2u );
	auto firstRowRef = manager->getChild( tableRef, 0 );
	EXPECT_TRUE( firstRowRef.source != tableRef.source );
	EXPECT_EQ( manager->getNodeInfo( firstRowRef ).role, AccessibilityRole::Row );
	EXPECT_EQ( manager->getChildCount( firstRowRef ), 2u );
	auto firstCellRef = manager->getChild( firstRowRef, 0 );
	EXPECT_EQ( manager->getNodeInfo( firstCellRef ).role, AccessibilityRole::Cell );
	EXPECT_TRUE( manager->getNodeInfo( firstCellRef ).name == String( "Name" ) );
	EXPECT_TRUE( manager->getNodeInfo( firstCellRef ).value == String( "Alpha" ) );
	EXPECT_TRUE( manager->getParent( firstCellRef ) == firstRowRef );
	EXPECT_TRUE( manager->performAction( firstRowRef, { AccessibilityAction::Select, String() } ) );
	EXPECT_TRUE( table->getSelection().containsRow( 0 ) );
	auto replacementModel = ItemPairListOwnerModel<std::string, std::string>::create(
		{ { "Gamma", "Three" }, { "Delta", "Four" } } );
	replacementModel->setColumnName( 0, "Name" );
	replacementModel->setColumnName( 1, "Value" );
	table->setModel( replacementModel );
	manager->notify( tableRef, AccessibilityEvent::ModelChanged );
	EXPECT_FALSE( manager->isValid( firstRowRef ) );
	firstRowRef = manager->getChild( tableRef, 0 );
	EXPECT_TRUE( manager->getNodeInfo( firstRowRef ).name == String( "Gamma" ) );

	UIListView* list = UIListView::New();
	list->setParent( scene->getRoot() );
	list->setModel( ItemListOwnerModel<std::string>::create( { "Red", "Green", "Blue" } ) );
	auto listRef = manager->getNodeRef( list );
	EXPECT_EQ( manager->getNodeInfo( listRef ).role, AccessibilityRole::List );
	EXPECT_EQ( manager->getChildCount( listRef ), 3u );
	EXPECT_EQ( manager->getChildren( listRef ).size(), 3u );
	auto listItemRef = manager->getChild( listRef, 1 );
	EXPECT_TRUE( manager->getChild( listRef, 1 ) == listItemRef );
	EXPECT_EQ( manager->getNodeInfo( listItemRef ).role, AccessibilityRole::ListItem );
	EXPECT_TRUE( manager->getNodeInfo( listItemRef ).name == String( "Green" ) );
	EXPECT_TRUE(
		manager->performAction( listItemRef, { AccessibilityAction::ScrollTo, String() } ) );
	EXPECT_TRUE( list->getSelection().isEmpty() );

	UITreeView* tree = UITreeView::New();
	tree->setParent( scene->getRoot() );
	std::map<std::string, std::vector<std::string>> treeItems{ { "Parent", { "Child" } } };
	tree->setModel( StringMapModel<>::create( treeItems ) );
	auto treeRef = manager->getNodeRef( tree );
	EXPECT_EQ( manager->getNodeInfo( treeRef ).role, AccessibilityRole::Tree );
	auto treeItemRef = manager->getChild( treeRef, 0 );
	EXPECT_EQ( manager->getNodeInfo( treeItemRef ).role, AccessibilityRole::TreeItem );
	EXPECT_EQ( manager->getChildCount( treeRef ), 1u );
	EXPECT_EQ( manager->getChildCount( treeItemRef ), 0u );
	EXPECT_TRUE( manager->getParent( treeItemRef ) == treeRef );
	EXPECT_TRUE( manager->getNodeInfo( treeItemRef ).actions &
				 accessibilityActionMask( AccessibilityAction::Expand ) );
	EXPECT_TRUE( manager->performAction( treeItemRef, { AccessibilityAction::Expand, String() } ) );
	EXPECT_TRUE( static_cast<Uint64>( manager->getNodeInfo( treeItemRef ).states ) &
				 static_cast<Uint64>( AccessibilityState::Expanded ) );
	EXPECT_EQ( manager->getChildCount( treeRef ), 2u );
	auto childTreeItemRef = manager->getChild( treeRef, 1 );
	EXPECT_EQ( manager->getNodeInfo( childTreeItemRef ).role, AccessibilityRole::TreeItem );
	EXPECT_TRUE( manager->getParent( childTreeItemRef ) == treeRef );
	EXPECT_TRUE(
		manager->performAction( treeItemRef, { AccessibilityAction::Collapse, String() } ) );
	EXPECT_EQ( manager->getChildCount( treeRef ), 1u );

	manager->clearPendingEvents();
	auto dynamicButton = UIPushButton::New();
	dynamicButton->setText( "Dynamic" );
	manager->clearPendingEvents();
	dynamicButton->setParent( ignoredContainer );
	manager->onWidgetParentChange( dynamicButton );
	auto dynamicButtonRef = manager->getNodeRef( dynamicButton );
	bool createdEventFound = false;
	for ( const auto& event : manager->getPendingEvents() ) {
		createdEventFound |= event.ref == root && event.related == dynamicButtonRef &&
							 event.type == AccessibilityEvent::Created && event.index >= 0;
	}
	EXPECT_TRUE( createdEventFound );

	manager->clearPendingEvents();
	eeDelete( dynamicButton );
	bool removedEventFound = false;
	for ( const auto& event : manager->getPendingEvents() ) {
		removedEventFound |= event.ref == root && event.related == dynamicButtonRef &&
							 event.type == AccessibilityEvent::Destroyed && event.index >= 0;
	}
	EXPECT_TRUE( removedEventFound );

	manager->clearPendingEvents();
	button->setAccessibilityLabel( "Save project" );
	EXPECT_TRUE( manager->getNodeInfo( buttonRef ).name == String( "Save project" ) );
	manager->notify( buttonRef, AccessibilityEvent::NameChanged );
	manager->notify( buttonRef, AccessibilityEvent::NameChanged );
	EXPECT_EQ( manager->getPendingEvents().size(), 1u );

	eeDelete( button );
	EXPECT_FALSE( manager->isValid( buttonRef ) );
	bool destroyedEventFound = false;
	for ( const auto& event : manager->getPendingEvents() )
		destroyedEventFound |=
			event.ref == buttonRef && event.type == AccessibilityEvent::Destroyed;
	EXPECT_TRUE( destroyedEventFound );
}
