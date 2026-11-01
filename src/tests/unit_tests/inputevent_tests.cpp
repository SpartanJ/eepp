#include "../../eepp/window/draganddrop/dragaccess.hpp"
#include "utest.h"
#include <eepp/ui/uiapplication.hpp>
#include <eepp/window/engine.hpp>
#include <eepp/window/input.hpp>

#if EE_PLATFORM == EE_PLATFORM_MACOS && defined( EE_SDL_VERSION_2 )
#include "../../eepp/window/backend/SDL2/base.hpp"
#endif

#if defined( EE_SDL_VERSION_3 )
#include "../../eepp/window/backend/SDL3/base.hpp"
#endif

using namespace EE;
using namespace EE::UI;
using namespace EE::Window;
using EE::Window::Private::NativeDragAccess;

#if EE_PLATFORM == EE_PLATFORM_MACOS && defined( EE_SDL_VERSION_2 )
UTEST( InputEvent, cocoaSDL2FileDropUsesTargetPointerInsteadOfCachedPosition ) {
	WindowSettings settings{ 320, 240, "File Drop Position Test" };
	settings.Style |= WindowStyle::Hidden;
	UIApplication app( settings );
	auto* pollingInput = app.getWindow()->getInput();
	auto* target = Engine::instance()->createWindow( settings );
	ASSERT_TRUE( target != nullptr );
	auto* targetInput = target->getInput();
	// Different hidden-window origins expose accidentally using the polling window's position.
	SDL_SetWindowPosition( SDL_GetWindowFromID( app.getWindow()->getWindowID() ), 10, 20 );
	SDL_SetWindowPosition( SDL_GetWindowFromID( target->getWindowID() ), 110, 220 );
	pollingInput->update();
	targetInput->setMousePos( { -10000, -10000 } );
	unsigned received = 0;
	const auto callback = targetInput->pushCallback( [&]( InputEvent* event ) {
		if ( event->Type != InputEvent::FileDropped )
			return;
		++received;
		EXPECT_TRUE( event->file.hasPosition );
		const Vector2f position = targetInput->queryMousePos().asFloat() * target->getScale();
		EXPECT_NEAR( event->file.x, position.x, 2.f * target->getScale() );
		EXPECT_NEAR( event->file.y, position.y, 2.f * target->getScale() );
		EXPECT_NE( event->file.x, -10000.f );
		EXPECT_NE( event->file.y, -10000.f );
	} );
	SDL_Event drop{};
	drop.type = SDL_DROPFILE;
	drop.drop.windowID = target->getWindowID();
	drop.drop.file = SDL_strdup( "/tmp/eefm-drop-position-test" );
	const int pushed = SDL_PushEvent( &drop );
	if ( pushed != 1 )
		SDL_free( drop.drop.file );
	EXPECT_EQ( pushed, 1 );
	pollingInput->update();
	EXPECT_EQ( received, 1u );
	targetInput->popCallback( callback );
	Engine::instance()->destroyWindow( target );
}
#endif

UTEST( InputEvent, modalReleaseEndsGestureWithoutClickOrOtherButtonChanges ) {
	WindowSettings settings{ 320, 240, "Modal Input State Test" };
	settings.Style |= WindowStyle::Hidden;
	UIApplication app( settings );
	auto* input = app.getWindow()->getInput();
	unsigned callbacks = 0;
	const auto callback = input->pushCallback( [&]( InputEvent* ) { ++callbacks; } );
	input->injectButtonPress( EE_BUTTON_LEFT );
	input->injectButtonPress( EE_BUTTON_RIGHT );
	input->cleanStates();
	NativeDragAccess::resetNativeDragInput( *input );
	EXPECT_EQ( input->getPressTrigger(), EE_BUTTON_RMASK );
	EXPECT_EQ( input->getReleaseTrigger(), EE_BUTTON_LMASK );
	EXPECT_EQ( input->getLastPressTrigger(), EE_BUTTON_LMASK | EE_BUTTON_RMASK );
	EXPECT_EQ( input->getClickTrigger(), 0u );
	EXPECT_EQ( input->getDoubleClickTrigger(), 0u );
	EXPECT_EQ( callbacks, 0u );
	EXPECT_TRUE( NativeDragAccess::consumeNativeDragRelease( *input ) );
	EXPECT_FALSE( NativeDragAccess::consumeNativeDragRelease( *input ) );
	input->popCallback( callback );
}

UTEST( InputEvent, newLeftPressClearsOnlyItsWindowsPendingNativeRelease ) {
	WindowSettings settings{ 320, 240, "Native Release Scope Test" };
	settings.Style |= WindowStyle::Hidden;
	UIApplication app( settings );
	auto* input = app.getWindow()->getInput();
	auto* otherWindow = Engine::instance()->createWindow( settings );
	ASSERT_TRUE( otherWindow != nullptr );
	auto* otherInput = otherWindow->getInput();
	EXPECT_FALSE( NativeDragAccess::consumeNativeDragRelease( *input ) );
	NativeDragAccess::resetNativeDragInput( *input );
	EXPECT_FALSE( NativeDragAccess::consumeNativeDragRelease( *otherInput ) );
	EXPECT_TRUE( NativeDragAccess::consumeNativeDragRelease( *input ) );

	// Injected events use the same press path as translated native events.
	InputEvent press{};
	press.Type = InputEvent::MouseButtonDown;
	press.WinID = app.getWindow()->getWindowID();
	press.button.button = EE_BUTTON_LEFT;
	NativeDragAccess::resetNativeDragInput( *input );
	NativeDragAccess::resetNativeDragInput( *otherInput );
	EXPECT_TRUE( otherInput->pushEvent( press ) );
	EXPECT_FALSE( NativeDragAccess::consumeNativeDragRelease( *input ) );
	EXPECT_EQ( input->getPressTrigger(), EE_BUTTON_LMASK );
	EXPECT_TRUE( NativeDragAccess::consumeNativeDragRelease( *otherInput ) );
	input->injectButtonRelease( EE_BUTTON_LEFT );

	NativeDragAccess::resetNativeDragInput( *input );
	input->injectButtonPress( EE_BUTTON_RIGHT );
	EXPECT_TRUE( NativeDragAccess::consumeNativeDragRelease( *input ) );
	Engine::instance()->destroyWindow( otherWindow );
}

UTEST( InputEvent, clipboardChangedReachesCallbacksWithoutWindowID ) {
	UIApplication app( WindowSettings{ 320, 240, "Clipboard Event Test" } );
	auto* input = app.getWindow()->getInput();
	unsigned int received = 0;
	Uint32 windowID = 1;
	InputEvent::ClipboardOwner owner = InputEvent::ClipboardOwner::Self;
	const auto callbackID = input->pushCallback( [&]( InputEvent* event ) {
		if ( event->Type == InputEvent::ClipboardChanged ) {
			++received;
			windowID = event->WinID;
			owner = event->clipboard.owner;
		}
	} );

	InputEvent event{};
	event.Type = InputEvent::ClipboardChanged;
	event.clipboard.owner = InputEvent::ClipboardOwner::Unknown;
	EXPECT_TRUE( input->pushEvent( event ) );
	EXPECT_EQ( received, 1u );
	EXPECT_EQ( windowID, 0u );
	EXPECT_EQ( owner, InputEvent::ClipboardOwner::Unknown );

	event.clipboard.owner = InputEvent::ClipboardOwner::Self;
	EXPECT_TRUE( input->pushEvent( event ) );
	EXPECT_EQ( received, 2u );
	EXPECT_EQ( owner, InputEvent::ClipboardOwner::Self );

	event.clipboard.owner = InputEvent::ClipboardOwner::External;
	EXPECT_TRUE( input->pushEvent( event ) );
	EXPECT_EQ( received, 3u );
	EXPECT_EQ( owner, InputEvent::ClipboardOwner::External );

	input->popCallback( callbackID );
	EXPECT_TRUE( input->pushEvent( event ) );
	EXPECT_EQ( received, 3u );
}

UTEST( InputEvent, fractionalWheelRetainsBothAxesAndTargetWindow ) {
	WindowSettings settings{ 320, 240, "Precise Wheel Test" };
	settings.Style |= WindowStyle::Hidden;
	UIApplication app( settings );
	auto* input = app.getWindow()->getInput();
	int received = 0;
	const auto callback = input->pushCallback( [&]( InputEvent* event ) {
		if ( event->Type != InputEvent::MouseWheel )
			return;
		++received;
		EXPECT_EQ( app.getWindow()->getWindowID(), event->WinID );
		EXPECT_NEAR( .125f, event->wheel.x, .00001f );
		EXPECT_NEAR( -.25f, event->wheel.y, .00001f );
	} );
	InputEvent wheel( InputEvent::MouseWheel );
	wheel.WinID = app.getWindow()->getWindowID();
	wheel.wheel = { 0, .125f, -.25f, InputEvent::WheelEvent::Normal };
	EXPECT_TRUE( input->pushEvent( wheel ) );
	EXPECT_EQ( 1, received );
	wheel.wheel.x = wheel.wheel.y = 0.f;
	EXPECT_TRUE( input->pushEvent( wheel ) );
	EXPECT_EQ( 1, received );
	input->popCallback( callback );
}

#if defined( EE_SDL_VERSION_3 )
UTEST( InputEvent, sdl3QueuedWheelPreservesWholeTicksAndDropsZeroMotion ) {
	WindowSettings settings{ 320, 240, "SDL3 Wheel Queue Test" };
	settings.Style |= WindowStyle::Hidden;
	UIApplication app( settings );
	auto* input = app.getWindow()->getInput();
	input->update();
	unsigned wheels = 0, presses = 0, releases = 0;
	const auto callback = input->pushCallback( [&]( InputEvent* event ) {
		if ( event->WinID != app.getWindow()->getWindowID() )
			return;
		if ( event->Type == InputEvent::MouseWheel ) {
			++wheels;
			EXPECT_NE( 0.f, event->wheel.y );
		} else if ( event->Type == InputEvent::MouseButtonDown &&
					event->button.button == EE_BUTTON_WHEELUP ) {
			++presses;
		} else if ( event->Type == InputEvent::MouseButtonUp &&
					event->button.button == EE_BUTTON_WHEELUP ) {
			++releases;
		}
	} );
	SDL_Event wheel{};
	wheel.type = SDL_EVENT_MOUSE_WHEEL;
	wheel.wheel.windowID = app.getWindow()->getWindowID();
	wheel.wheel.direction = SDL_MOUSEWHEEL_NORMAL;
	wheel.wheel.y = 1.f;
#if SDL_VERSION_ATLEAST( 3, 2, 12 )
	wheel.wheel.integer_y = 1;
#endif
	ASSERT_TRUE( SDL_PushEvent( &wheel ) );
	input->update();
	EXPECT_EQ( 1u, wheels );
	EXPECT_EQ( 1u, presses );
	EXPECT_EQ( 1u, releases );

	wheel.wheel.y = 0.f;
#if SDL_VERSION_ATLEAST( 3, 2, 12 )
	wheel.wheel.integer_y = 0;
#endif
	ASSERT_TRUE( SDL_PushEvent( &wheel ) );
	input->update();
	EXPECT_EQ( 1u, wheels );
	EXPECT_EQ( 1u, presses );
	EXPECT_EQ( 1u, releases );

	wheel.wheel.y = .25f;
	ASSERT_TRUE( SDL_PushEvent( &wheel ) );
	input->update();
	EXPECT_EQ( 2u, wheels );
#if SDL_VERSION_ATLEAST( 3, 2, 12 )
	EXPECT_EQ( 1u, presses );
	EXPECT_EQ( 1u, releases );
#else
	EXPECT_EQ( 2u, presses );
	EXPECT_EQ( 2u, releases );
#endif
	input->popCallback( callback );
}
#endif
