#import <AppKit/AppKit.h>
#include <eepp/window/platform/macos/platformhelper.hpp>

namespace EE { namespace Window { namespace Private {

bool setNativeScrollMomentumEnabled( bool enabled ) {
	[[NSUserDefaults standardUserDefaults] setBool:enabled ? YES : NO
											forKey:@"AppleMomentumScrollSupported"];
	return true;
}

bool setWindowTitleBarSeparatorVisible( void* window, bool visible ) {
	if ( nullptr == window )
		return false;

	if ( @available( macOS 11.0, * ) ) {
		NSWindow* nsWindow = static_cast<NSWindow*>( window );
		[nsWindow setTitlebarSeparatorStyle:visible ? NSTitlebarSeparatorStyleAutomatic
													: NSTitlebarSeparatorStyleNone];
		return true;
	}
	return false;
}

bool setWindowTitleBarColor( void* window, Uint8 red, Uint8 green, Uint8 blue ) {
	if ( nullptr == window )
		return false;

	NSWindow* nsWindow = static_cast<NSWindow*>( window );
	nsWindow.titlebarAppearsTransparent = YES;
	nsWindow.backgroundColor = [NSColor colorWithRed:static_cast<CGFloat>( red ) / 255.f
											   green:static_cast<CGFloat>( green ) / 255.f
												blue:static_cast<CGFloat>( blue ) / 255.f
											   alpha:1.f];
	return true;
}

}}} // namespace EE::Window::Private
