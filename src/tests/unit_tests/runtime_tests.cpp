#include "../../eepp/window/draganddrop/waylandlibrary.hpp"
#include "utest.h"
#include <eepp/system/scopedop.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/ui/uiapplication.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/window/engine.hpp>
#include <eepp/window/runtime.hpp>

#if EE_PLATFORM == EE_PLATFORM_LINUX && defined( EE_HAS_WAYLAND_CLIENT )
#include <chrono>
#include <cstdlib>
#include <eepp/network/locallistener.hpp>
#include <eepp/network/socketselector.hpp>
#include <future>
#include <unistd.h>
#endif

using namespace EE;
using namespace EE::System;
using namespace EE::UI;
using namespace EE::Window;

#if EE_PLATFORM == EE_PLATFORM_LINUX && defined( EE_HAS_WAYLAND_CLIENT )
UTEST( Runtime, waylandRegistryProbeTimesOutWhenServerDoesNotReply ) {
	using namespace EE::Network;
	if ( std::getenv( "WAYLAND_SOCKET" ) )
		return;
	void* wayland = Sys::loadObject( "libwayland-client.so.0", false );
	if ( !wayland )
		return;
	ScopedOp releaseWayland( nullptr, [&] { Sys::unloadObject( wayland ); } );
	char directory[] = "/tmp/eepp-wayland-probe-XXXXXX";
	ASSERT_TRUE( mkdtemp( directory ) != nullptr );
	ScopedOp removeDirectory( nullptr, [&] { rmdir( directory ); } );
	const auto path = std::string( directory ) + "/wayland";
	const bool hadDisplay = std::getenv( "WAYLAND_DISPLAY" ) != nullptr;
	const auto previousDisplay = Sys::getEnv( "WAYLAND_DISPLAY" );
	ASSERT_TRUE( Sys::setEnv( "WAYLAND_DISPLAY", path.c_str() ) );
	ScopedOp restoreDisplay( nullptr, [&] {
		if ( hadDisplay )
			Sys::setEnv( "WAYLAND_DISPLAY", previousDisplay.c_str() );
		else
			unsetenv( "WAYLAND_DISPLAY" );
	} );

	LocalListener listener;
	ASSERT_EQ( listener.listen( path ), Socket::Done );
	SocketSelector selector;
	selector.add( listener );
	auto probe = std::async( std::launch::async, [] {
		return EE::Window::Private::WaylandLibrary::hasGlobals( { "wp_fifo_manager_v1" } );
	} );
	LocalSocket peer;
	const bool connected = selector.wait( Seconds( 2 ) ) && listener.accept( peer ) == Socket::Done;
	listener.close();
	const auto status = probe.wait_for( std::chrono::seconds( 2 ) );
	// Disconnect even if the probe regresses, so the test can report the timeout failure.
	peer.disconnect();
	EXPECT_TRUE( connected );
	EXPECT_TRUE( status == std::future_status::ready );
	EXPECT_FALSE( probe.get() );
}
#endif

UTEST( Runtime, hiddenWindowOverrideRequiresOneAndPreservesRuntime ) {
	const std::string previous = Sys::getEnv( "EEPP_WINDOW_HIDDEN" );
	ScopedOp environment( nullptr,
						  [&previous] { Sys::setEnv( "EEPP_WINDOW_HIDDEN", previous.c_str() ); } );
	const auto mode = Runtime::mode();
	for ( const char* value : { "", "0", "true", "false", "01", "1" } ) {
		EXPECT_TRUE( Sys::setEnv( "EEPP_WINDOW_HIDDEN", value ) );
		EXPECT_EQ( Runtime::forceHiddenWindows(), value[0] == '1' && value[1] == '\0' );
		EXPECT_EQ( Runtime::mode(), mode );
	}
}

UTEST( Runtime, softwareRenderingOverrideUsesNonemptyNonzeroValues ) {
	const std::string previous = Sys::getEnv( "EEPP_FORCE_SOFTWARE_RENDERING" );
	ScopedOp environment( nullptr, [&previous] {
		Sys::setEnv( "EEPP_FORCE_SOFTWARE_RENDERING", previous.c_str() );
	} );
	for ( const char* value : { "", "0" } ) {
		EXPECT_TRUE( Sys::setEnv( "EEPP_FORCE_SOFTWARE_RENDERING", value ) );
		EXPECT_FALSE( Runtime::forceSoftwareRendering() );
	}
	for ( const char* value : { "1", "true", "false", "off", "00" } ) {
		EXPECT_TRUE( Sys::setEnv( "EEPP_FORCE_SOFTWARE_RENDERING", value ) );
		EXPECT_TRUE( Runtime::forceSoftwareRendering() );
	}
}

UTEST( Runtime, multisampleOverrideParsesLevelsAndRejectsInvalidValues ) {
	const std::string previous = Sys::getEnv( "EEPP_MSAA" );
	ScopedOp environment( nullptr, [&previous] { Sys::setEnv( "EEPP_MSAA", previous.c_str() ); } );
	const auto mode = Runtime::mode();
	for ( Uint32 samples : { 0u, 1u, 2u, 4u, 8u, 16u, 2147483647u } ) {
		EXPECT_TRUE( Sys::setEnv( "EEPP_MSAA", String::toString( samples ).c_str() ) );
		EXPECT_EQ( Runtime::resolveMultisamples( 8 ), samples );
		EXPECT_EQ( Runtime::resolveMultisamples( 0 ), samples );
		EXPECT_EQ( Runtime::mode(), mode );
	}
	for ( const char* value :
		  { "", "true", "-1", "+4", " 4", "4 ", "4x", "0x4", "4.0", "2147483648", "4294967295",
			"4294967296", "9999999999999999999999999999999999999999" } ) {
		EXPECT_TRUE( Sys::setEnv( "EEPP_MSAA", value ) );
		EXPECT_EQ( Runtime::resolveMultisamples( 8 ), 8u );
		EXPECT_EQ( Runtime::resolveMultisamples( 0 ), 0u );
		EXPECT_EQ( Runtime::mode(), mode );
	}
}

UTEST( Runtime, multisampleOverrideAppliesToPrimaryAndSecondaryWindows ) {
	if ( Runtime::mode() == RuntimeMode::Terminal )
		return;
	const std::string previous = Sys::getEnv( "EEPP_MSAA" );
	ScopedOp environment( [] { Sys::setEnv( "EEPP_MSAA", "0" ); },
						  [&previous] { Sys::setEnv( "EEPP_MSAA", previous.c_str() ); } );
	ScopedOp cleanup( nullptr, [] { Engine::destroySingleton(); } );
	const ContextSettings context( false, 0, 4 );
	auto* primary =
		Engine::instance()->createWindow( { 64, 64, "MSAA Override Primary" }, context );
	ASSERT_TRUE( primary != nullptr && primary->isOpen() );
	auto* secondary =
		Engine::instance()->createWindow( { 64, 64, "MSAA Override Secondary" }, context );
	ASSERT_TRUE( secondary != nullptr && secondary->isOpen() );
	for ( auto* window : { primary, secondary } )
		EXPECT_EQ( window->getWindowInfo()->ContextConfig.Multisamples, 0u );
	EXPECT_EQ( context.Multisamples, 4u );
}

UTEST( UIApplication, forcedHiddenWindowsIgnorePresentationRequests ) {
	if ( Runtime::mode() == RuntimeMode::Terminal )
		return;
	const std::string previous = Sys::getEnv( "EEPP_WINDOW_HIDDEN" );
	ScopedOp environment( [] { Sys::setEnv( "EEPP_WINDOW_HIDDEN", "1" ); },
						  [&previous] { Sys::setEnv( "EEPP_WINDOW_HIDDEN", previous.c_str() ); } );
	WindowSettings settings{ 320, 240, "Forced Hidden Window Test" };
	// Do not inherit the unit harness's hidden default: the environment must force both windows
	// hidden and suppress the requested fullscreen desktop mode before native creation.
	settings.Style = WindowStyle::Titlebar | WindowStyle::Resize | WindowStyle::Fullscreen |
					 WindowStyle::UseDesktopResolution;
	UIApplication app( settings );
	auto* secondary = app.createWindow( settings );
	ASSERT_TRUE( secondary != nullptr );
	for ( auto* window : { app.getWindow(), secondary->getWindow() } ) {
		EXPECT_TRUE( window->getWindowInfo()->WindowConfig.Style & WindowStyle::Hidden );
		EXPECT_FALSE( window->getWindowInfo()->WindowConfig.Style &
					  ( WindowStyle::Fullscreen | WindowStyle::UseDesktopResolution ) );
		EXPECT_FALSE( window->isVisible() );
		EXPECT_TRUE( window->isWindowed() );
		const auto position = window->getPosition();
		window->show();
		window->raise();
		window->restore();
		window->maximize();
		EXPECT_EQ( window->getPosition().x, position.x );
		EXPECT_EQ( window->getPosition().y, position.y );
		window->toggleFullscreen();
		window->setSize( 360, 260, false );
		EXPECT_FALSE( window->isVisible() );
		EXPECT_TRUE( window->isWindowed() );
	}
}
