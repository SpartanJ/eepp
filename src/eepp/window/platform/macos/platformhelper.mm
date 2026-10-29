#import <AppKit/AppKit.h>
#include <eepp/window/platform/macos/platformhelper.hpp>
#import <objc/runtime.h>

// The native window owns this observer only while its fullscreen exit is pending. Keep an ID,
// rather than a borrowed C++ Window pointer, so closing the window cannot leave a dangling
// callback.
@interface EEPPDeferredWindowHide : NSObject {
	void ( *mComplete )( EE::Uint32 );
	EE::Uint32 mWindowId;
}
@property( nonatomic ) BOOL requestNativeExit;
@property( nonatomic ) BOOL exiting;

- (instancetype)initWithWindow:(NSWindow*)window
					  windowId:(EE::Uint32)windowId
					  complete:(void ( * )( EE::Uint32 ))complete;

- (void)didExitFullScreen:(NSNotification*)notification;

- (void)willExitFullScreen:(NSNotification*)notification;

- (void)requestExit:(NSWindow*)window;

- (void)didFailFullScreen:(NSNotification*)notification;

- (void)willClose:(NSNotification*)notification;
@end

@implementation EEPPDeferredWindowHide

- (instancetype)initWithWindow:(NSWindow*)window
					  windowId:(EE::Uint32)windowId
					  complete:(void ( * )( EE::Uint32 ))complete {
	self = [super init];
	if ( self ) {
		mComplete = complete;
		mWindowId = windowId;
		NSNotificationCenter* center = [NSNotificationCenter defaultCenter];
		[center addObserver:self
				   selector:@selector( willExitFullScreen: )
					   name:NSWindowWillExitFullScreenNotification
					 object:window];
		[center addObserver:self
				   selector:@selector( didExitFullScreen: )
					   name:NSWindowDidExitFullScreenNotification
					 object:window];
		// AppKit has no exported constants for these names; SDL observes them too.
		[center addObserver:self
				   selector:@selector( didFailFullScreen: )
					   name:@"NSWindowDidFailToEnterFullScreenNotification"
					 object:window];
		[center addObserver:self
				   selector:@selector( didFailFullScreen: )
					   name:@"NSWindowDidFailToExitFullScreenNotification"
					 object:window];
		[center addObserver:self
				   selector:@selector( willClose: )
					   name:NSWindowWillCloseNotification
					 object:window];
	}
	return self;
}

- (void)didExitFullScreen:(NSNotification*)notification {
	auto complete = mComplete;
	auto windowId = mWindowId;
	EE::Window::Private::WindowVisibility::cancelHide( notification.object );
	complete( windowId );
}

- (void)willExitFullScreen:(NSNotification*)notification {
	(void)notification;
	self.exiting = YES;
}

- (void)requestExit:(NSWindow*)window {
	// A retry can recover an ignored native request, but must not reverse an exit that started.
	// When SDL owns the request, it also owns any queued toggle and retry handling.
	if ( self.requestNativeExit && !self.exiting &&
		 ( window.styleMask & NSWindowStyleMaskFullScreen ) )
		[window toggleFullScreen:nil];
}

- (void)didFailFullScreen:(NSNotification*)notification {
	// Keep a failed transition visible and responsive, and allow the next close to retry.
	EE::Window::Private::WindowVisibility::cancelHide( notification.object );
}

- (void)willClose:(NSNotification*)notification {
	EE::Window::Private::WindowVisibility::cancelHide( notification.object );
}

- (void)dealloc {
	[[NSNotificationCenter defaultCenter] removeObserver:self];
#if !__has_feature( objc_arc )
	[super dealloc];
#endif
}

@end

namespace EE { namespace Window { namespace Private {

static char deferredHideKey;

bool WindowVisibility::deferHide( void* window, Uint32 windowId, void ( *complete )( Uint32 ),
								  bool requestNativeExit ) {
	NSWindow* native = static_cast<NSWindow*>( window );
	if ( !native )
		return false;
	EEPPDeferredWindowHide* pending = objc_getAssociatedObject( native, &deferredHideKey );
	if ( pending ) {
		[pending requestExit:native];
		return true;
	}
	if ( !( native.styleMask & NSWindowStyleMaskFullScreen ) )
		return false;
	EEPPDeferredWindowHide* observer = [[EEPPDeferredWindowHide alloc] initWithWindow:native
																			 windowId:windowId
																			 complete:complete];
	objc_setAssociatedObject( native, &deferredHideKey, observer,
							  OBJC_ASSOCIATION_RETAIN_NONATOMIC );
	observer.requestNativeExit = requestNativeExit;
	// orderOut: during native fullscreen leaves its Space empty. Wait for AppKit's completed
	// transition instead of blocking the event loop or guessing an animation delay.
	[observer requestExit:native];
#if !__has_feature( objc_arc )
	[observer release];
#endif
	return true;
}

void WindowVisibility::cancelHide( void* window ) {
	if ( window )
		objc_setAssociatedObject( static_cast<NSWindow*>( window ), &deferredHideKey, nil,
								  OBJC_ASSOCIATION_RETAIN_NONATOMIC );
}

bool ApplicationReopenHandler::setCallback( std::function<bool()> callback ) {
	static std::function<bool()> handler;
	static IMP previousHandler = nullptr;
	auto dispatch = +[]( id delegate, SEL selector, NSApplication* app, BOOL visible ) -> BOOL {
		if ( handler && handler() )
			return NO;
		if ( previousHandler ) {
			using Handler = BOOL ( * )( id, SEL, NSApplication*, BOOL );
			return reinterpret_cast<Handler>( previousHandler )( delegate, selector, app, visible );
		}
		return YES;
	};
	// Keep SDL's application delegate and its file-open/termination handling intact. The hook
	// lives with the delegate class; clearing the callback restores its normal reopen behavior.
	if ( callback ) {
		id delegate = [NSApp delegate];
		if ( !delegate )
			return false;
		Class delegateClass = object_getClass( delegate );
		SEL selector = @selector( applicationShouldHandleReopen:hasVisibleWindows: );
		Method method = class_getInstanceMethod( delegateClass, selector );
		if ( !method || method_getImplementation( method ) != reinterpret_cast<IMP>( dispatch ) ) {
			const char* encoding =
				method ? method_getTypeEncoding( method )
					   : protocol_getMethodDescription( @protocol( NSApplicationDelegate ),
														selector, NO, YES )
							 .types;
			if ( !encoding )
				return false;
			previousHandler = method ? method_getImplementation( method ) : nullptr;
			class_replaceMethod( delegateClass, selector, reinterpret_cast<IMP>( dispatch ),
								 encoding );
			// Refresh AppKit's cached optional delegate selectors after adding the method.
			[NSApp setDelegate:delegate];
		}
	}
	handler = std::move( callback );
	return true;
}

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
