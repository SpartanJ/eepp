#include "utest.h"
#include <cstdlib>
#include <eepp/graphics/font.hpp>
#include <eepp/graphics/fontservice.hpp>
#include <eepp/graphics/pixeldensity.hpp>
#include <eepp/graphics/resourcescope.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/ui/uiapplication.hpp>
#include <eepp/ui/uiscenenode.hpp>
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

UTEST( UIEnvironment, fontOverridesAcceptOnlyKnownValues ) {
	const std::string previousHinting = Sys::getEnv( "EEPP_FONT_HINTING" );
	const std::string previousAntialiasing = Sys::getEnv( "EEPP_FONT_ANTIALIASING" );
	EXPECT_TRUE( Sys::setEnv( "EEPP_FONT_HINTING", "slight" ) );
	EXPECT_TRUE( Sys::setEnv( "EEPP_FONT_ANTIALIASING", "subpixel" ) );
	EXPECT_EQ( Font::fontHintingFromEnvironment( FontHinting::Full ), FontHinting::Slight );
	EXPECT_EQ( Font::fontAntialiasingFromEnvironment( FontAntialiasing::Grayscale ),
			   FontAntialiasing::Subpixel );
	{
		UIApplication app( WindowSettings{ 320, 240, "Environment Font Test" } );
		ASSERT_TRUE( app.getUI() );
		const auto& fonts = app.getUI()->getResourceScope()->getFontService();
		EXPECT_EQ( fonts.getHinting(), FontHinting::Slight );
		EXPECT_EQ( fonts.getAntialiasing(), FontAntialiasing::Subpixel );
	}
	EXPECT_TRUE( Sys::setEnv( "EEPP_FONT_HINTING", "unknown" ) );
	EXPECT_TRUE( Sys::setEnv( "EEPP_FONT_ANTIALIASING", "unknown" ) );
	EXPECT_EQ( Font::fontHintingFromEnvironment( FontHinting::Full ), FontHinting::Full );
	EXPECT_EQ( Font::fontAntialiasingFromEnvironment( FontAntialiasing::Grayscale ),
			   FontAntialiasing::Grayscale );
	EXPECT_TRUE( Sys::setEnv( "EEPP_FONT_HINTING", previousHinting.c_str() ) );
	EXPECT_TRUE( Sys::setEnv( "EEPP_FONT_ANTIALIASING", previousAntialiasing.c_str() ) );
}

UTEST( UIEnvironment, colorSchemeOverridesApplicationPreference ) {
	const std::string previous = Sys::getEnv( "EEPP_COLOR_SCHEME" );
	EXPECT_TRUE( Sys::setEnv( "EEPP_COLOR_SCHEME", "light" ) );
	{
		UIApplication app( WindowSettings{ 320, 240, "Environment Theme Test" } );
		ASSERT_TRUE( app.getUI() );
		EXPECT_EQ( app.getUI()->getColorSchemePreference(), ColorSchemePreference::Light );
		app.getUI()->setColorSchemePreference( ColorSchemePreference::Dark );
		EXPECT_EQ( app.getUI()->getColorSchemePreference(), ColorSchemePreference::Light );
	}
	EXPECT_TRUE( Sys::setEnv( "EEPP_COLOR_SCHEME", previous.c_str() ) );
}
