#import <AppKit/AppKit.h>
#include <Cocoa/Cocoa.h>

#include "macos.hpp"

/* setAppleMenu disappeared from the headers in 10.4 */
@interface NSApplication(NSAppleMenu)
- (void)setAppleMenu:(NSMenu *)menu;
@end

// Recreates the menubar replacing the default SDL menubar
void macOS_createApplicationMenus() {
	NSString *appName;
	NSString *title;
	NSMenu *appleMenu;
	NSMenu *windowMenu;
	NSMenuItem *menuItem;
	NSMenu *mainMenu;

	if (NSApp == nil) {
		return;
	}

	mainMenu = [[NSApplication sharedApplication] mainMenu];
	[mainMenu removeAllItems];

	appName = @"ecode";
	appleMenu = [[NSMenu alloc] initWithTitle:@""];

	title = [@"About " stringByAppendingString:appName];
	[appleMenu addItemWithTitle:title action:@selector(orderFrontStandardAboutPanel:) keyEquivalent:@""];

	[appleMenu addItem:[NSMenuItem separatorItem]];

	[appleMenu addItemWithTitle:@"Show All" action:@selector(unhideAllApplications:) keyEquivalent:@""];

	[appleMenu addItem:[NSMenuItem separatorItem]];

	title = [@"Quit " stringByAppendingString:appName];
	[appleMenu addItemWithTitle:title action:@selector(performClose:) keyEquivalent:@"q"];

	menuItem = [[NSMenuItem alloc] initWithTitle:@"" action:nil keyEquivalent:@""];
	[menuItem setSubmenu:appleMenu];

	[[NSApp mainMenu] addItem:menuItem];

	[NSApp setAppleMenu:appleMenu];

	windowMenu = [[NSMenu alloc] initWithTitle:@"Window"];

	menuItem = [[NSMenuItem alloc] initWithTitle:@"Minimize" action:@selector(miniaturize:) keyEquivalent:@"m"];
	[menuItem setKeyEquivalentModifierMask:NSEventModifierFlagCommand];
	[windowMenu addItem:menuItem];

	menuItem = [[NSMenuItem alloc] initWithTitle:@"Toggle Full Screen" action:@selector(toggleFullScreen:) keyEquivalent:@"f"];
	[menuItem setKeyEquivalentModifierMask:NSEventModifierFlagControl | NSEventModifierFlagCommand];
	[windowMenu addItem:menuItem];

	menuItem = [[NSMenuItem alloc] initWithTitle:@"Window" action:nil keyEquivalent:@""];
	[menuItem setSubmenu:windowMenu];

	[[NSApp mainMenu] addItem:menuItem];

	[NSApp setWindowsMenu:windowMenu];
}

void macOS_enableScrollMomentum() {
	[[NSUserDefaults standardUserDefaults] setBool:YES forKey:@"AppleMomentumScrollSupported"];
}

void macOS_removeTitleBarSeparator( void* window ) {
	NSWindow* nsWindow = window;
	[nsWindow setTitlebarSeparatorStyle:NSTitlebarSeparatorStyleNone];
}

void macOS_changeTitleBarColor( void* window, double red, double green, double blue ) {
	NSWindow* nsWindow = window;
	nsWindow.titlebarAppearsTransparent = YES;
	nsWindow.backgroundColor = [NSColor colorWithRed:red green:green blue:blue alpha:1.];
}
