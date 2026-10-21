#include "utest.h"
#include <eepp/system/scopedop.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/ui/uiapplication.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/window/runtime.hpp>

using namespace EE;
using namespace EE::System;
using namespace EE::UI;
using namespace EE::Window;

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
		window->show();
		window->raise();
		window->restore();
		window->maximize();
		window->toggleFullscreen();
		window->setSize( 360, 260, false );
		EXPECT_FALSE( window->isVisible() );
		EXPECT_TRUE( window->isWindowed() );
	}
}
