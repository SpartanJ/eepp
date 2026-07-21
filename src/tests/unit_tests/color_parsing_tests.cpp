#include "utest.hpp"
#include <eepp/system/color.hpp>

using namespace EE;
using namespace EE::System;

#define EXPECT_COLOREQ( expected, actual )                                               \
	do {                                                                                \
		Color e = ( expected );                                                         \
		Color a = ( actual );                                                           \
		bool eq = ( e == a );                                                           \
		if ( !eq ) {                                                                    \
			UTEST_PRINTF( "%s:%i: Failure\n", __FILE__, __LINE__ );                     \
			UTEST_PRINTF( "  Expected : #%08X (%d,%d,%d,%d)\n", e.getValue(), e.r,     \
						  e.g, e.b, e.a );                                              \
			UTEST_PRINTF( "    Actual : #%08X (%d,%d,%d,%d)\n", a.getValue(), a.r,     \
						  a.g, a.b, a.a );                                              \
			*utest_result = UTEST_TEST_FAILURE;                                         \
		}                                                                               \
	} while ( 0 )

UTEST( ColorParsing, legacyRgbComma ) {
	EXPECT_COLOREQ( Color( 255, 0, 0, 255 ), Color::fromString( "rgb(255, 0, 0)" ) );
	EXPECT_COLOREQ( Color( 0, 255, 0, 255 ), Color::fromString( "rgb(0, 255, 0)" ) );
	EXPECT_COLOREQ( Color( 0, 0, 255, 255 ), Color::fromString( "rgb(0, 0, 255)" ) );
	EXPECT_COLOREQ( Color( 128, 64, 32, 255 ), Color::fromString( "rgb(128, 64, 32)" ) );
	EXPECT_COLOREQ( Color( 0, 0, 0, 255 ), Color::fromString( "rgb(0, 0, 0)" ) );
	EXPECT_COLOREQ( Color( 255, 255, 255, 255 ),
					Color::fromString( "rgb(255, 255, 255)" ) );
}

UTEST( ColorParsing, legacyRgbaComma ) {
	EXPECT_COLOREQ( Color( 255, 0, 0, 128 ),
					Color::fromString( "rgba(255, 0, 0, 0.5)" ) );
	EXPECT_COLOREQ( Color( 255, 0, 0, 0 ),
					Color::fromString( "rgba(255, 0, 0, 0.0)" ) );
	EXPECT_COLOREQ( Color( 255, 0, 0, 255 ),
					Color::fromString( "rgba(255, 0, 0, 1.0)" ) );
	EXPECT_COLOREQ( Color( 0, 0, 0, 64 ), Color::fromString( "rgba(0, 0, 0, 0.25)" ) );
}

UTEST( ColorParsing, modernRgbSpaceSeparated ) {
	EXPECT_COLOREQ( Color( 255, 0, 0, 255 ), Color::fromString( "rgb(255 0 0)" ) );
	EXPECT_COLOREQ( Color( 0, 255, 0, 255 ), Color::fromString( "rgb(0 255 0)" ) );
	EXPECT_COLOREQ( Color( 0, 0, 255, 255 ), Color::fromString( "rgb(0 0 255)" ) );
	EXPECT_COLOREQ( Color( 128, 64, 32, 255 ), Color::fromString( "rgb(128 64 32)" ) );
	EXPECT_COLOREQ( Color( 0, 0, 0, 255 ), Color::fromString( "rgb(0 0 0)" ) );
	EXPECT_COLOREQ( Color( 255, 255, 255, 255 ),
					Color::fromString( "rgb(255 255 255)" ) );
}

UTEST( ColorParsing, modernRgbSpaceWithAlpha ) {
	EXPECT_COLOREQ( Color( 255, 0, 0, 128 ),
					Color::fromString( "rgb(255 0 0 / 0.5)" ) );
	EXPECT_COLOREQ( Color( 255, 0, 0, 0 ),
					Color::fromString( "rgb(255 0 0 / 0.0)" ) );
	EXPECT_COLOREQ( Color( 255, 0, 0, 255 ),
					Color::fromString( "rgb(255 0 0 / 1.0)" ) );
	EXPECT_COLOREQ( Color( 0, 0, 0, 64 ),
					Color::fromString( "rgb(0 0 0 / 0.25)" ) );
	EXPECT_COLOREQ( Color( 10, 20, 30, 204 ),
					Color::fromString( "rgb(10 20 30 / 0.8)" ) );
}

UTEST( ColorParsing, modernRgbaSpaceSeparated ) {
	EXPECT_COLOREQ( Color( 255, 0, 0, 255 ), Color::fromString( "rgba(255 0 0)" ) );
	EXPECT_COLOREQ( Color( 255, 0, 0, 128 ),
					Color::fromString( "rgba(255 0 0 / 0.5)" ) );
}

UTEST( ColorParsing, percentageValues ) {
	EXPECT_COLOREQ( Color( 255, 0, 0, 255 ),
					Color::fromString( "rgb(100% 0% 0%)" ) );
	EXPECT_COLOREQ( Color( 0, 255, 0, 255 ),
					Color::fromString( "rgb(0% 100% 0%)" ) );
	EXPECT_COLOREQ( Color( 128, 128, 128, 255 ),
					Color::fromString( "rgb(50% 50% 50%)" ) );
	EXPECT_COLOREQ( Color( 255, 128, 64, 128 ),
					Color::fromString( "rgb(100% 50% 25% / 50%)" ) );
	EXPECT_COLOREQ( Color( 255, 0, 0, 128 ),
					Color::fromString( "rgba(100% 0% 0% / 0.5)" ) );
}

UTEST( ColorParsing, percentageCommaLegacy ) {
	EXPECT_COLOREQ( Color( 255, 0, 0, 255 ),
					Color::fromString( "rgb(100%, 0%, 0%)" ) );
	EXPECT_COLOREQ( Color( 128, 128, 128, 255 ),
					Color::fromString( "rgb(50%, 50%, 50%)" ) );
	EXPECT_COLOREQ( Color( 0, 255, 0, 255 ),
					Color::fromString( "rgb(0%, 100%, 0%)" ) );
}

UTEST( ColorParsing, noneKeyword ){
	EXPECT_COLOREQ( Color( 0, 128, 255, 255 ),
					Color::fromString( "rgb(none 128 255)" ) );
	EXPECT_COLOREQ( Color( 255, 0, 255, 255 ),
					Color::fromString( "rgb(255 none 255)" ) );
	EXPECT_COLOREQ( Color( 255, 128, 0, 255 ),
					Color::fromString( "rgb(255 128 none)" ) );
	EXPECT_COLOREQ( Color( 0, 0, 0, 255 ), Color::fromString( "rgb(none none none)" ) );
}

UTEST( ColorParsing, noneKeywordLegacy ) {
	EXPECT_COLOREQ( Color( 0, 128, 255, 255 ),
					Color::fromString( "rgb(none, 128, 255)" ) );
	EXPECT_COLOREQ( Color( 255, 0, 0, 128 ),
					Color::fromString( "rgba(255, none, 0, 0.5)" ) );
}

UTEST( ColorParsing, rgbWithAlphaLegacy ) {
	EXPECT_COLOREQ( Color( 255, 0, 0, 128 ),
					Color::fromString( "rgb(255, 0, 0, 0.5)" ) );
	EXPECT_COLOREQ( Color( 10, 20, 30, 204 ),
					Color::fromString( "rgb(10, 20, 30, 0.8)" ) );
}

UTEST( ColorParsing, rgbaWithThreeLegacy ) {
	EXPECT_COLOREQ( Color( 255, 128, 64, 255 ),
					Color::fromString( "rgba(255, 128, 64)" ) );
}

UTEST( ColorParsing, whitespaceHandling ) {
	EXPECT_COLOREQ( Color( 255, 0, 0, 255 ),
					Color::fromString( "rgb(  255   0   0  )" ) );
	EXPECT_COLOREQ( Color( 255, 0, 0, 128 ),
					Color::fromString( "rgb( 255  0  0  /  0.5 )" ) );
	EXPECT_COLOREQ( Color( 255, 0, 0, 255 ),
					Color::fromString( "rgb( 255 , 0 , 0 )" ) );
	EXPECT_COLOREQ( Color( 255, 0, 0, 128 ),
					Color::fromString( "rgba( 255 , 0 , 0 , 0.5 )" ) );
}

UTEST( ColorParsing, invalidReturnsTransparent ) {
	EXPECT_COLOREQ( Color::Transparent, Color::fromString( "rgb(255)" ) );
	EXPECT_COLOREQ( Color::Transparent, Color::fromString( "rgb(255 0)" ) );
	EXPECT_COLOREQ( Color::Transparent, Color::fromString( "rgb()" ) );
	EXPECT_COLOREQ( Color::Transparent, Color::fromString( "rgb(   )" ) );
	EXPECT_COLOREQ( Color::Transparent, Color::fromString( "rgb(abc, def, ghi)" ) );
	EXPECT_COLOREQ( Color::Transparent, Color::fromString( "rgb(abc def ghi)" ) );
}

UTEST( ColorParsing, rgbaHexNotation ) {
	EXPECT_COLOREQ( Color( 0xAA, 0xBB, 0xCC, 128 ),
					Color::fromString( "rgba(#AABBCC, 0.5)" ) );
	EXPECT_COLOREQ( Color( 0xAA, 0xBB, 0xCC, 128 ),
					Color::fromString( "rgb(#AABBCC, 0.5)" ) );
}

UTEST( ColorParsing, alphaPercentage ) {
	EXPECT_COLOREQ( Color( 255, 0, 0, 128 ),
					Color::fromString( "rgb(255 0 0 / 50%)" ) );
	EXPECT_COLOREQ( Color( 255, 0, 0, 0 ),
					Color::fromString( "rgb(255 0 0 / 0%)" ) );
	EXPECT_COLOREQ( Color( 255, 0, 0, 255 ),
					Color::fromString( "rgb(255 0 0 / 100%)" ) );
}

UTEST( ColorParsing, clampingOutOfRange ) {
	EXPECT_COLOREQ( Color( 255, 0, 0, 255 ),
					Color::fromString( "rgb(300 0 0)" ) );
	EXPECT_COLOREQ( Color( 0, 0, 0, 255 ),
					Color::fromString( "rgb(0 -50 0)" ) );
	EXPECT_COLOREQ( Color( 255, 0, 0, 255 ),
					Color::fromString( "rgb(300, 0, 0)" ) );
}

UTEST( ColorParsing, mixedModernRgbaSyntax ) {
	EXPECT_COLOREQ( Color( 10, 20, 30, 128 ),
					Color::fromString( "rgb(10 20 30 / 0.5)" ) );
	EXPECT_COLOREQ( Color( 100, 150, 200, 64 ),
					Color::fromString( "rgba(100 150 200 / 0.25)" ) );
}

UTEST( ColorParsing, legacyHslComma ) {
	EXPECT_COLOREQ( Color( 255, 0, 0, 255 ),
					Color::fromString( "hsl(0, 100%, 50%)" ) );
	EXPECT_COLOREQ( Color( 0, 255, 0, 255 ),
					Color::fromString( "hsl(120, 100%, 50%)" ) );
	EXPECT_COLOREQ( Color( 0, 0, 255, 255 ),
					Color::fromString( "hsl(240, 100%, 50%)" ) );
}

UTEST( ColorParsing, legacyHslaComma ) {
	EXPECT_COLOREQ( Color( 255, 0, 0, 128 ),
					Color::fromString( "hsla(0, 100%, 50%, 0.5)" ) );
	EXPECT_COLOREQ( Color( 0, 255, 0, 64 ),
					Color::fromString( "hsla(120, 100%, 50%, 0.25)" ) );
}

UTEST( ColorParsing, modernHslSpaceSeparated ) {
	EXPECT_COLOREQ( Color( 255, 0, 0, 255 ),
					Color::fromString( "hsl(0 100% 50%)" ) );
	EXPECT_COLOREQ( Color( 0, 255, 0, 255 ),
					Color::fromString( "hsl(120 100% 50%)" ) );
	EXPECT_COLOREQ( Color( 0, 0, 255, 255 ),
					Color::fromString( "hsl(240 100% 50%)" ) );
}

UTEST( ColorParsing, modernHslSpaceWithAlpha ) {
	EXPECT_COLOREQ( Color( 255, 0, 0, 128 ),
					Color::fromString( "hsl(0 100% 50% / 0.5)" ) );
	EXPECT_COLOREQ( Color( 0, 255, 0, 64 ),
					Color::fromString( "hsl(120 100% 50% / 0.25)" ) );
	EXPECT_COLOREQ( Color( 0, 0, 255, 255 ),
					Color::fromString( "hsl(240 100% 50% / 1.0)" ) );
}

UTEST( ColorParsing, modernHslaSpaceSeparated ) {
	EXPECT_COLOREQ( Color( 255, 0, 0, 255 ),
					Color::fromString( "hsla(0 100% 50%)" ) );
	EXPECT_COLOREQ( Color( 255, 0, 0, 128 ),
					Color::fromString( "hsla(0 100% 50% / 0.5)" ) );
}

UTEST( ColorParsing, hslNoneKeyword ) {
	EXPECT_COLOREQ( Color( 255, 0, 0, 255 ),
					Color::fromString( "hsl(none 100% 50%)" ) );
	EXPECT_COLOREQ( Color( 0, 0, 0, 255 ),
					Color::fromString( "hsl(0 0% none)" ) );
	EXPECT_COLOREQ( Color( 255, 0, 0, 255 ),
					Color::fromString( "hsl(none, 100%, 50%)" ) );
}

UTEST( ColorParsing, hslWithAlphaLegacy ) {
	EXPECT_COLOREQ( Color( 255, 0, 0, 128 ),
					Color::fromString( "hsl(0, 100%, 50%, 0.5)" ) );
}

UTEST( ColorParsing, hslaWithThreeLegacy ) {
	EXPECT_COLOREQ( Color( 255, 0, 0, 255 ),
					Color::fromString( "hsla(0, 100%, 50%)" ) );
}

UTEST( ColorParsing, legacyHsvComma ) {
	EXPECT_COLOREQ( Color( 255, 0, 0, 255 ),
					Color::fromString( "hsv(0, 100%, 100%)" ) );
	EXPECT_COLOREQ( Color( 0, 255, 0, 255 ),
					Color::fromString( "hsv(120, 100%, 100%)" ) );
	EXPECT_COLOREQ( Color( 0, 0, 0, 255 ),
					Color::fromString( "hsv(0, 0%, 0%)" ) );
}

UTEST( ColorParsing, legacyHsvaComma ) {
	EXPECT_COLOREQ( Color( 255, 0, 0, 128 ),
					Color::fromString( "hsva(0, 100%, 100%, 0.5)" ) );
}

UTEST( ColorParsing, modernHsvSpaceSeparated ) {
	EXPECT_COLOREQ( Color( 255, 0, 0, 255 ),
					Color::fromString( "hsv(0 100% 100%)" ) );
	EXPECT_COLOREQ( Color( 0, 255, 0, 255 ),
					Color::fromString( "hsv(120 100% 100%)" ) );
}

UTEST( ColorParsing, modernHsvSpaceWithAlpha ) {
	EXPECT_COLOREQ( Color( 255, 0, 0, 128 ),
					Color::fromString( "hsv(0 100% 100% / 0.5)" ) );
	EXPECT_COLOREQ( Color( 0, 255, 0, 64 ),
					Color::fromString( "hsv(120 100% 100% / 0.25)" ) );
}

UTEST( ColorParsing, modernHsvaSpaceSeparated ) {
	EXPECT_COLOREQ( Color( 255, 0, 0, 255 ),
					Color::fromString( "hsva(0 100% 100%)" ) );
	EXPECT_COLOREQ( Color( 255, 0, 0, 128 ),
					Color::fromString( "hsva(0 100% 100% / 0.5)" ) );
}

UTEST( ColorParsing, hsvNoneKeyword ) {
	EXPECT_COLOREQ( Color( 255, 0, 0, 255 ),
					Color::fromString( "hsv(none 100% 100%)" ) );
}

UTEST( ColorParsing, hsvWithAlphaLegacy ) {
	EXPECT_COLOREQ( Color( 255, 0, 0, 128 ),
					Color::fromString( "hsv(0, 100%, 100%, 0.5)" ) );
}

UTEST( ColorParsing, hsvaWithThreeLegacy ) {
	EXPECT_COLOREQ( Color( 255, 0, 0, 255 ),
					Color::fromString( "hsva(0, 100%, 100%)" ) );
}
