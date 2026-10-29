#include "utest.h"

#include "../../eepp/window/platform/macos/platformhelper.hpp"
#include <eepp/system/scopedop.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/ui/uiapplication.hpp>
#include <eepp/ui/uimenubar.hpp>
#include <eepp/ui/uimenuitem.hpp>
#include <eepp/ui/uipopupmenu.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/window/engine.hpp>
#include <eepp/window/input.hpp>

#if defined( EE_SDL_VERSION_2 )
#include "../../eepp/window/backend/SDL2/base.hpp"
#elif defined( EE_SDL_VERSION_3 )
#if __has_include( <SDL3/SDL.h> )
#include <SDL3/SDL.h>
#else
#include <SDL.h>
#endif
#endif

#define Rect AppleRect
#import <AppKit/AppKit.h>
#undef Rect
#undef BSD

using namespace EE;
using namespace EE::System;
using namespace EE::UI;
using namespace EE::Window;
using EE::Window::Private::WindowVisibility;

// Simulate the asynchronous native transition without creating a visible fullscreen Space.
@interface EEPPFullscreenTestWindow : NSWindow
@property( nonatomic ) BOOL simulatedFullscreen;
@property( nonatomic ) NSUInteger toggleCount;
@end

@implementation EEPPFullscreenTestWindow

- (NSWindowStyleMask)styleMask {
	return [super styleMask] | ( self.simulatedFullscreen ? NSWindowStyleMaskFullScreen : 0 );
}

- (void)toggleFullScreen:(id)sender {
	(void)sender;
	++self.toggleCount;
}

@end

namespace {

class ReopenTestApplication : public UIApplication {
  public:
	using UIApplication::UIApplication;

	void markRunning() { mRunning = true; }
};

} // namespace

UTEST( UIApplicationMacOS, RetainedHideWaitsForFullscreenExitAndCanBeCancelled ) {
	[NSApplication sharedApplication];
	EEPPFullscreenTestWindow* native =
		[[EEPPFullscreenTestWindow alloc] initWithContentRect:NSMakeRect( 0, 0, 320, 240 )
													styleMask:NSWindowStyleMaskTitled
													  backing:NSBackingStoreBuffered
														defer:NO];
	[native setReleasedWhenClosed:NO];
	static Uint32 completedId;
	completedId = 0;
	auto complete = +[]( Uint32 id ) { completedId = id; };
	EXPECT_FALSE( WindowVisibility::deferHide( native, 7, complete ) );
	native.simulatedFullscreen = YES;
	EXPECT_TRUE( WindowVisibility::deferHide( native, 7, complete ) );
	[[NSNotificationCenter defaultCenter]
		postNotificationName:NSWindowWillExitFullScreenNotification
					  object:native];
	EXPECT_TRUE( WindowVisibility::deferHide( native, 7, complete ) );
	EXPECT_EQ( native.toggleCount, 1u );
	EXPECT_EQ( completedId, 0u );
	native.simulatedFullscreen = NO;
	[[NSNotificationCenter defaultCenter] postNotificationName:NSWindowDidExitFullScreenNotification
														object:native];
	EXPECT_EQ( completedId, 7u );
	// The observer is removed after completion, so another exit cannot hide a reopened window.
	completedId = 0;
	[[NSNotificationCenter defaultCenter] postNotificationName:NSWindowDidExitFullScreenNotification
														object:native];
	EXPECT_EQ( completedId, 0u );
	native.simulatedFullscreen = YES;
	EXPECT_TRUE( WindowVisibility::deferHide( native, 8, complete ) );
	WindowVisibility::cancelHide( native );
	[[NSNotificationCenter defaultCenter] postNotificationName:NSWindowDidExitFullScreenNotification
														object:native];
	EXPECT_EQ( completedId, 0u );
	EXPECT_TRUE( WindowVisibility::deferHide( native, 9, complete ) );
	[[NSNotificationCenter defaultCenter]
		postNotificationName:@"NSWindowDidFailToExitFullScreenNotification"
					  object:native];
	EXPECT_EQ( completedId, 0u );
	EXPECT_TRUE( WindowVisibility::deferHide( native, 10, complete ) );
	EXPECT_EQ( native.toggleCount, 4u );
	[native close];
	[[NSNotificationCenter defaultCenter] postNotificationName:NSWindowDidExitFullScreenNotification
														object:native];
	EXPECT_EQ( completedId, 0u );
#if !__has_feature( objc_arc )
	[native release];
#endif
}

UTEST( UIApplicationMacOS, DeferredHideDistinguishesBackendExitFromIgnoredNativeRequest ) {
	[NSApplication sharedApplication];
	EEPPFullscreenTestWindow* native =
		[[EEPPFullscreenTestWindow alloc] initWithContentRect:NSMakeRect( 0, 0, 320, 240 )
													styleMask:NSWindowStyleMaskTitled
													  backing:NSBackingStoreBuffered
														defer:NO];
	[native setReleasedWhenClosed:NO];
	native.simulatedFullscreen = YES;
	static Uint32 completedId;
	completedId = 0;
	auto complete = +[]( Uint32 id ) { completedId = id; };
	// SDL3 has already queued the exit. Neither this call nor a repeated close may toggle it.
	EXPECT_TRUE( WindowVisibility::deferHide( native, 21, complete, false ) );
	EXPECT_TRUE( WindowVisibility::deferHide( native, 21, complete ) );
	EXPECT_EQ( native.toggleCount, 0u );
	[[NSNotificationCenter defaultCenter] postNotificationName:NSWindowDidExitFullScreenNotification
														object:native];
	EXPECT_EQ( completedId, 21u );
	completedId = 0;
	// An ignored native toggle sends no WillExit. The next close can retry it.
	EXPECT_TRUE( WindowVisibility::deferHide( native, 22, complete ) );
	EXPECT_TRUE( WindowVisibility::deferHide( native, 22, complete ) );
	EXPECT_EQ( native.toggleCount, 2u );
	[[NSNotificationCenter defaultCenter]
		postNotificationName:NSWindowWillExitFullScreenNotification
					  object:native];
	EXPECT_TRUE( WindowVisibility::deferHide( native, 22, complete ) );
	EXPECT_EQ( native.toggleCount, 2u );
	[[NSNotificationCenter defaultCenter] postNotificationName:NSWindowDidExitFullScreenNotification
														object:native];
	EXPECT_EQ( completedId, 22u );
	[native close];
#if !__has_feature( objc_arc )
	[native release];
#endif
}

#if defined( EE_SDL_VERSION_2 ) || defined( EE_SDL_VERSION_3 )
UTEST( UIApplicationMacOS, HiddenSDLRequestedFullscreenCloseClearsBackendFlags ) {
	const std::string previous = Sys::getEnv( "EEPP_WINDOW_HIDDEN" );
	ScopedOp environment( [] { Sys::setEnv( "EEPP_WINDOW_HIDDEN", "" ); },
						  [&previous] { Sys::setEnv( "EEPP_WINDOW_HIDDEN", previous.c_str() ); } );
	UIApplication::Settings settings( Sys::getProcessPath(), 1.f, false );
	settings.exitOnClose = false;
	UIApplication app( WindowSettings( 320, 240, "SDL Fullscreen Flags",
									   WindowStyle::Default | WindowStyle::Hidden ),
					   settings );
	auto* window = app.getWindow();
	ASSERT_TRUE( window != nullptr );
	SDL_Window* sdl = SDL_GetWindowFromID( window->getWindowID() );
	ASSERT_TRUE( sdl != nullptr );
	ASSERT_FALSE( window->isVisible() );
	// SDL records requested fullscreen even for a hidden window. This exercises real backend
	// flags without entering a visible Space; native deferred hiding is tested separately.
	window->toggleFullscreen();
	EXPECT_FALSE( window->isWindowed() );
#if defined( EE_SDL_VERSION_2 )
	EXPECT_TRUE( ( SDL_GetWindowFlags( sdl ) & SDL_WINDOW_FULLSCREEN_DESKTOP ) != 0 );
#endif
	EXPECT_FALSE( app.handleCloseRequest( window ) );
	EXPECT_TRUE( window->isWindowed() );
	EXPECT_EQ( SDL_GetWindowFlags( sdl ) & SDL_WINDOW_FULLSCREEN, 0u );
	ASSERT_FALSE( window->isVisible() );
	Sys::setEnv( "EEPP_WINDOW_HIDDEN", "1" );
	EXPECT_TRUE( app.reopen() );
	EXPECT_TRUE( window->isWindowed() );
	EXPECT_EQ( SDL_GetWindowFlags( sdl ) & SDL_WINDOW_FULLSCREEN, 0u );
}
#endif

UTEST( UIApplicationMacOS, DockReopenRestoresRetainedWindowAndHandlerIsClearedOnDestruction ) {
	const std::string previous = Sys::getEnv( "EEPP_WINDOW_HIDDEN" );
	ScopedOp environment( [] { Sys::setEnv( "EEPP_WINDOW_HIDDEN", "1" ); },
						  [&previous] { Sys::setEnv( "EEPP_WINDOW_HIDDEN", previous.c_str() ); } );
	{
		UIApplication::Settings settings( Sys::getProcessPath(), 1.f, false );
		settings.exitOnClose = false;
		ReopenTestApplication app( WindowSettings( 320, 240, "Dock Reopen" ), settings );
		ASSERT_TRUE( app.getWindow() != nullptr );
		app.markRunning();
		// Exercise SDL's native close path too: a synthetic eepp close event cannot catch
		// an unwanted SDL_QUIT automatically queued after the final window-close request.
		NSWindow* native = static_cast<NSWindow*>( app.getWindow()->getWindowHandler() );
		[native performClose:nil];
		app.getWindow()->getInput()->update();
		EXPECT_TRUE( app.isRunning() );
		EXPECT_TRUE( app.getWindow()->isOpen() );
		id<NSApplicationDelegate> delegate = [NSApp delegate];
		ASSERT_TRUE( [delegate
			respondsToSelector:@selector( applicationShouldHandleReopen:hasVisibleWindows: )] );
		EXPECT_FALSE( [delegate applicationShouldHandleReopen:NSApp hasVisibleWindows:NO] );
		EXPECT_FALSE( app.reopen() );
		EXPECT_TRUE( app.getWindow()->isOpen() );
	}
	// Recreate the SDL delegate and verify it cannot call the destroyed application.
	UIApplication app( WindowSettings( 320, 240, "Next Application" ),
					   UIApplication::Settings( Sys::getProcessPath(), 1.f, false ) );
	ASSERT_TRUE( app.getWindow() != nullptr );
	Engine::instance()->getPlatformHelper()->setApplicationReopenCallback( {} );
	id<NSApplicationDelegate> delegate = [NSApp delegate];
	EXPECT_TRUE( [delegate applicationShouldHandleReopen:NSApp hasVisibleWindows:NO] );
}

UTEST( UIApplicationMacOS, NativeQuitMenuMatchesConfiguredShortcut ) {
	UIApplication app( WindowSettings( 320, 240, "Quit Shortcut" ) );
	ASSERT_TRUE( app.getUI() != nullptr );
	auto* bar = UIMenuBar::New();
	bar->setParent( app.getUI() );
	auto* popup = UIPopUpMenu::New();
	popup->add( "Quit" )->setMenuRole( MenuRole::Quit )->setId( "close-app" );
	bar->addMenuButton( "File", popup );
	int activations = 0;
	popup->on( Event::OnItemClicked, [&]( const Event* ) { ++activations; } );
	KeyBindings bindings( app.getWindow()->getInput() );
	auto key = []( NSString* character, NSEventModifierFlags modifiers ) {
		return [NSEvent keyEventWithType:NSEventTypeKeyDown
								location:NSZeroPoint
						   modifierFlags:modifiers
							   timestamp:0
							windowNumber:0
								 context:nil
							  characters:character
			 charactersIgnoringModifiers:character
							   isARepeat:NO
								 keyCode:12];
	};
	// A customized Quit shortcut must not advertise and consume an unregistered Command-Q.
	bindings.addKeybind( { KEY_X, KEYMOD_META | KEYMOD_ALT }, "close-app" );
	bar->updateShortcuts( bindings );
	bar->setGlobalMenuBarEnabled( true );
	NSMenuItem* nativeQuit = [NSApp mainMenu].itemArray.firstObject.submenu.itemArray.lastObject;
	EXPECT_EQ(
		static_cast<unsigned long>( nativeQuit.keyEquivalentModifierMask ),
		static_cast<unsigned long>( NSEventModifierFlagCommand | NSEventModifierFlagOption ) );
	EXPECT_FALSE( [[NSApp mainMenu] performKeyEquivalent:key( @"q", NSEventModifierFlagCommand )] );
	EXPECT_EQ( activations, 0 );
	EXPECT_TRUE( [[NSApp mainMenu]
		performKeyEquivalent:key( @"x", NSEventModifierFlagCommand | NSEventModifierFlagOption )] );
	EXPECT_EQ( activations, 1 );
	bar->setGlobalMenuBarEnabled( false );
	bindings.replaceKeybind( { KEY_Q, KEYMOD_META }, "close-app" );
	bar->updateShortcuts( bindings );
	bar->setGlobalMenuBarEnabled( true );
	EXPECT_TRUE( [[NSApp mainMenu] performKeyEquivalent:key( @"q", NSEventModifierFlagCommand )] );
	EXPECT_EQ( activations, 2 );
}
