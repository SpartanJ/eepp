#import <AppKit/AppKit.h>
#include <Cocoa/Cocoa.h>

#include "macos.hpp"

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
