#include "utest.h"
#include <cstdlib>
#include <eepp/graphics/pixeldensity.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/ui/uiapplication.hpp>
#include <eepp/window/window.hpp>
#include <string>

using namespace EE;
using namespace EE::Graphics;
using namespace EE::System;
using namespace EE::UI;
using namespace EE::Window;

UTEST( PixelDensity, environmentDefaultIsStrictAndDoesNotScaleWindowSettings ) {
	const char* prior = std::getenv( "EEPP_PIXEL_DENSITY" );
	const std::string previous = prior ? prior : "";
	EXPECT_TRUE( Sys::setEnv( "EEPP_PIXEL_DENSITY", "1.75" ) );
	EXPECT_EQ( PixelDensity::getEnvironmentPixelDensity(), 1.75f );
	WindowSettings window( 320, 240, "Density Test" );
	EXPECT_EQ( window.Width, 320u );
	EXPECT_EQ( window.Height, 240u );
	EXPECT_EQ( window.PixelDensity, 0.f );
	EXPECT_TRUE( Sys::setEnv( "EEPP_PIXEL_DENSITY", "1.75oops" ) );
	EXPECT_EQ( PixelDensity::getEnvironmentPixelDensity(), 0.f );
	EXPECT_TRUE( Sys::setEnv( "EEPP_PIXEL_DENSITY", "-2" ) );
	EXPECT_EQ( PixelDensity::getEnvironmentPixelDensity(), 0.f );
	EXPECT_TRUE( Sys::setEnv( "EEPP_PIXEL_DENSITY", "nan" ) );
	EXPECT_EQ( PixelDensity::getEnvironmentPixelDensity(), 0.f );
	EXPECT_TRUE( Sys::setEnv( "EEPP_PIXEL_DENSITY", previous.c_str() ) );
}

UTEST( PixelDensity, environmentDefaultAppliesToUIWithoutResizingWindow ) {
	const char* prior = std::getenv( "EEPP_PIXEL_DENSITY" );
	const std::string previous = prior ? prior : "";
	const Float priorDensity = PixelDensity::getPixelDensity();
	EXPECT_TRUE( Sys::setEnv( "EEPP_PIXEL_DENSITY", "1.75" ) );
	{
		UIApplication app( WindowSettings{ 320, 240, "Environment Density Test" } );
		EXPECT_EQ( PixelDensity::getPixelDensity(), 1.75f );
		EXPECT_EQ( app.getWindow()->getWindowInfo()->WindowConfig.Width, 320u );
		EXPECT_EQ( app.getWindow()->getWindowInfo()->WindowConfig.Height, 240u );
	}
	EXPECT_TRUE( Sys::setEnv( "EEPP_PIXEL_DENSITY", previous.c_str() ) );
	PixelDensity::setPixelDensity( priorDensity );
}
