#include "utest.h"

#include <eepp/ui/accessibility/accessibilitymanager.hpp>
#include <eepp/ui/models/itemlistmodel.hpp>
#include <eepp/ui/uiapplication.hpp>
#include <eepp/ui/uicheckbox.hpp>
#include <eepp/ui/uipushbutton.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uitableview.hpp>
#include <eepp/ui/uitextedit.hpp>
#include <eepp/ui/uitextinput.hpp>

#define FileInfo AppleFileInfo
#define Rect AppleRect
#define TextRange AppleTextRange
#import <AppKit/AppKit.h>
#undef TextRange
#undef Rect
#undef FileInfo
#undef BSD

using namespace EE;
using namespace EE::System;
using namespace EE::UI;
using namespace EE::Window;

namespace {

class AccessibilityTestApplication : public UIApplication {
  public:
	using UIApplication::UIApplication;

	void destroyPendingWindows() { processPendingWindowDestruction(); }
};

NSObject<NSAccessibility>* semanticRootForWindow( NSWindow* window ) {
	for ( NSObject<NSAccessibility>* child in [[window contentView] accessibilityChildren] ) {
		if ( [[child accessibilityIdentifier] hasPrefix:@"eepp.window."] )
			return child;
	}
	return nil;
}

} // namespace

UTEST( AccessibilityMacOS, ApplicationWindowBridgeIdentityActionTextAndLifecycle ) {
	AccessibilityTestApplication app( WindowSettings( 320, 240, "eepp - macOS Accessibility Test" ),
									  UIApplication::Settings( Sys::getProcessPath() ) );
	if ( !app.getUI() )
		UTEST_SKIP( "a logged-in macOS graphical session is required" );
	app.setQuitPolicy( UIApplication::QuitPolicy::OnLastWindowClosed );
	auto scene = app.getUI();
	auto manager = scene->getAccessibilityManager();
	bool pressed = false;
	auto button = UIPushButton::New();
	button->setText( "Press me" );
	button->onClick( [&pressed]( auto ) { pressed = true; } );
	button->setParent( scene->getRoot() );
	auto checkbox = UICheckBox::New();
	checkbox->setText( "Enable feature" );
	checkbox->setParent( scene->getRoot() );
	auto textInput = UITextInput::New();
	textInput->setText( String::fromUtf8( std::string( "A😀é" ) ) );
	textInput->getDocument().setSelection( { 0, 1 }, { 0, 4 } );
	textInput->setParent( scene->getRoot() );
	auto textEdit = UITextEdit::New();
	textEdit->setText( String::fromUtf8( std::string( "one\n😀x" ) ) );
	textEdit->getDocument().setSelection( { 0, 1 }, { 1, 1 } );
	textEdit->setParent( scene->getRoot() );
	auto table = UITableView::New();
	table->setModel( Models::ItemPairListOwnerModel<std::string, std::string>::create(
		{ { "Alpha", "One" }, { "Beta", "Two" } } ) );
	table->setParent( scene->getRoot() );
	manager->update();

	auto secondary =
		app.createWindow( WindowSettings( 240, 160, "eepp - Secondary Accessibility Test" ) );
	ASSERT_TRUE( secondary != nullptr );
	auto secondaryManager = secondary->getAccessibilityManager();
	UIPushButton* secondaryButton = nullptr;
	{
		auto context = secondary->makeCurrent();
		secondaryButton = UIPushButton::New();
		secondaryButton->setText( "Secondary" );
		secondaryButton->setParent( secondary->getRoot() );
		secondaryManager->update();
	}

	EXPECT_TRUE( NSApp != nil );
	NSWindow* window = reinterpret_cast<NSWindow*>( scene->getWindow()->getWindowHandler() );
	EXPECT_TRUE( window != nil );
	EXPECT_TRUE( [window.accessibilityRole isEqualToString:NSAccessibilityWindowRole] );
	NSObject<NSAccessibility>* windowRoot = semanticRootForWindow( window );
	EXPECT_TRUE( windowRoot != nil );
	[windowRoot retain];

	NSArray* children = [windowRoot accessibilityChildren];
	EXPECT_TRUE( children != nil );
	EXPECT_EQ( children.count, 5u );
	NSObject<NSAccessibility>* element = children.firstObject;
	NSObject<NSAccessibility>* sameElement = [windowRoot.accessibilityChildren firstObject];
	EXPECT_TRUE( element == sameElement );
	EXPECT_TRUE( [element.accessibilityRole isEqualToString:NSAccessibilityButtonRole] );
	EXPECT_TRUE( element.accessibilityIdentifier.length > 0 );
	EXPECT_TRUE( [element accessibilityPerformPress] );
	EXPECT_TRUE( pressed );
	scene->update( Time::Zero );
	element.accessibilityFocused = YES;
	EXPECT_TRUE( button->hasFocus() );
	scene->update( Time::Zero );
	EXPECT_TRUE( [NSApp accessibilityApplicationFocusedUIElement] == element );
	EXPECT_TRUE( [NSApp accessibilityFocusedWindow] == window );

	NSObject<NSAccessibility>* checkboxElement = children[1];
	EXPECT_TRUE( [checkboxElement.accessibilityRole isEqualToString:NSAccessibilityCheckBoxRole] );
	EXPECT_FALSE( checkbox->isChecked() );
	EXPECT_TRUE( [checkboxElement accessibilityPerformPress] );
	EXPECT_TRUE( checkbox->isChecked() );

	NSObject<NSAccessibility>* textElement = children[2];
	EXPECT_TRUE( [textElement.accessibilityRole isEqualToString:NSAccessibilityTextFieldRole] );
	EXPECT_EQ( textElement.accessibilitySelectedTextRange.location, 1u );
	EXPECT_EQ( textElement.accessibilitySelectedTextRange.length, 4u );
	textElement.accessibilitySelectedTextRange = NSMakeRange( 3, 2 );
	auto textInfo = manager->getNodeInfo( manager->getNodeRef( textInput ) ).text;
	EXPECT_EQ( textInfo.selectionStart, 2 );
	EXPECT_EQ( textInfo.selectionEnd, 4 );
	textInput->setMode( UITextInput::TextInputMode::Password );
	EXPECT_TRUE(
		[textElement.accessibilitySubrole isEqualToString:NSAccessibilitySecureTextFieldSubrole] );
	EXPECT_TRUE( textElement.isAccessibilityProtectedContent );

	NSObject<NSAccessibility>* multilineElement = children[3];
	EXPECT_TRUE(
		[multilineElement.accessibilityRole isEqualToString:NSAccessibilityTextFieldRole] );
	EXPECT_EQ( multilineElement.accessibilityNumberOfCharacters, 7 );
	EXPECT_EQ( multilineElement.accessibilitySelectedTextRange.location, 1u );
	EXPECT_EQ( multilineElement.accessibilitySelectedTextRange.length, 5u );
	multilineElement.accessibilitySelectedTextRange = NSMakeRange( 4, 2 );
	auto multilineInfo = manager->getNodeInfo( manager->getNodeRef( textEdit ) ).text;
	EXPECT_EQ( multilineInfo.selectionStart, 4 );
	EXPECT_EQ( multilineInfo.selectionEnd, 5 );

	NSObject<NSAccessibility>* tableElement = children[4];
	NSArray* rows = tableElement.accessibilityRows;
	EXPECT_EQ( rows.count, 2u );
	NSObject<NSAccessibility>* staleRow = rows.firstObject;
	EXPECT_TRUE( [staleRow.accessibilityValue isEqualToString:@"Alpha"] );
	table->setModel( Models::ItemPairListOwnerModel<std::string, std::string>::create(
		{ { "Gamma", "Three" } } ) );
	EXPECT_FALSE( staleRow.isAccessibilityElement );
	NSArray* replacementRows = tableElement.accessibilityRows;
	EXPECT_EQ( replacementRows.count, 1u );
	EXPECT_TRUE( replacementRows.firstObject != staleRow );
	EXPECT_TRUE( [[replacementRows.firstObject accessibilityValue] isEqualToString:@"Gamma"] );

	AccessibilityNodeRef buttonRef = manager->getNodeRef( button );
	eeDelete( button );
	EXPECT_FALSE( manager->isValid( buttonRef ) );
	EXPECT_FALSE( [element isAccessibilityElement] );

	app.closeWindow( scene->getWindow() );
	app.destroyPendingWindows();
	EXPECT_EQ( app.getWindowCount(), 1u );
	EXPECT_TRUE( app.getUI() == nullptr );
	EXPECT_FALSE( [windowRoot isAccessibilityElement] );
	NSWindow* survivingWindow =
		reinterpret_cast<NSWindow*>( secondary->getWindow()->getWindowHandler() );
	EXPECT_TRUE( survivingWindow != nil );
	EXPECT_TRUE( [survivingWindow.accessibilityTitle
		isEqualToString:@"eepp - Secondary Accessibility Test"] );
	NSObject<NSAccessibility>* survivingRoot = semanticRootForWindow( survivingWindow );
	EXPECT_TRUE( survivingRoot != nil );
	EXPECT_EQ( survivingRoot.accessibilityChildren.count, 1u );
	EXPECT_TRUE( secondaryManager->isValid( secondaryManager->getNodeRef( secondaryButton ) ) );
	[windowRoot release];
}
