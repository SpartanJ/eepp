#include "utest.h"

#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <memory>

// Isolate a copy so the numeric boundary cases can exercise the static helpers
// without exporting test hooks from eepp or changing the public NanoSVG ABI.
#define NANOSVG_CPLUSPLUS
#define NANOSVGRAST_CPLUSPLUS
#define NANOSVG_IMPLEMENTATION
#define NANOSVGRAST_IMPLEMENTATION
namespace NanoSVGSecurity {

#include <nanosvg/nanosvg.h>
#include <nanosvg/nanosvgrast.h>

} // namespace NanoSVGSecurity

namespace {

using namespace NanoSVGSecurity;
using SVGImage = std::unique_ptr<NSVGimage, decltype( &nsvgDelete )>;
using SVGRasterizer = std::unique_ptr<NSVGrasterizer, decltype( &nsvgDeleteRasterizer )>;

} // namespace

UTEST( NanoSVGSecurity, ExtremeArcRadiiProduceFinitePaths ) {
	char inputs[][200] = {
		R"(<svg width="16" height="16"><path d="M1 1 A57.1E2857 3 0 0 1 12 12 L1 12 Z"/></svg>)",
		R"(<svg width="16" height="16"><path d="M1 1 A1e30 1e30 0 0 1 12 12 L1 12 Z"/></svg>)",
		R"(<svg width="16" height="16"><path d="M1 1 A0 3 0 0 1 12 12 L1 12 Z"/></svg>)",
		R"(<svg width="16" height="16"><path d="M1 1 A3 3 0 0 1 1e999 12 L1 12 Z"/></svg>)" };
	for ( auto& input : inputs ) {
		SVGImage image( nsvgParse( input, "px", 96, 0xFFFFFFFF ), &nsvgDelete );
		ASSERT_TRUE( image != nullptr );
		ASSERT_TRUE( image->shapes != nullptr );
		for ( auto* shape = image->shapes; shape; shape = shape->next ) {
			for ( auto* path = shape->paths; path; path = path->next ) {
				for ( int i = 0; i < path->npts * 2; ++i )
					EXPECT_TRUE( std::isfinite( path->pts[i] ) );
			}
		}
	}
}

UTEST( NanoSVGSecurity, ExtremeStrokeTessellationIsBounded ) {
	for ( float radius :
		  { 0.f, 1.f, 5.3310536385e9f, std::numeric_limits<float>::max(),
			std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN() } ) {
		int count = nsvg__curveDivs( radius, NSVG_PI, 0.25f );
		EXPECT_GE( count, 2 );
		EXPECT_LE( count, 1024 );
	}
	EXPECT_EQ( nsvg__curveDivs( 1.f, NSVG_PI, 0.25f ), 3 );
}

UTEST( NanoSVGSecurity, UnrepresentableActiveEdgesDoNotConsumePoolEntries ) {
	SVGRasterizer rasterizer( nsvgCreateRasterizer(), &nsvgDeleteRasterizer );
	ASSERT_TRUE( rasterizer != nullptr );
	NSVGactiveEdge spare{};
	rasterizer->freelist = &spare;
	const float nan = std::numeric_limits<float>::quiet_NaN();
	const float inf = std::numeric_limits<float>::infinity();
	NSVGedge edges[] = {
		{ -3e7f, 0, -3e7f, 10, 1, nullptr }, { 2097152.f, 0, 2097152.f, 10, 1, nullptr },
		{ 0, 0, 1e30f, 1, 1, nullptr },		 { nan, 0, 1, 10, 1, nullptr },
		{ 0, 0, inf, 10, 1, nullptr },		 { 0, 1, 1, 1, 1, nullptr } };
	for ( auto& edge : edges ) {
		EXPECT_TRUE( nsvg__addActive( rasterizer.get(), &edge, 0.5f ) == nullptr );
		EXPECT_TRUE( rasterizer->freelist == &spare );
	}
	NSVGedge valid{ 1, 0, 3, 10, 1, nullptr };
	auto* active = nsvg__addActive( rasterizer.get(), &valid, 0.5f );
	EXPECT_TRUE( active == &spare );
	EXPECT_TRUE( rasterizer->freelist == nullptr );
	EXPECT_EQ( active->x, 1126 );
	EXPECT_EQ( active->dx, 205 );
}

UTEST( NanoSVGSecurity, ScanlineAdvanceRejectsBothSignedOverflowDirections ) {
	for ( float sign : { -1.f, 1.f } ) {
		SVGRasterizer rasterizer( nsvgCreateRasterizer(), &nsvgDeleteRasterizer );
		ASSERT_TRUE( rasterizer != nullptr );
		NSVGedge edge{ sign * 2097150.f, 0, sign * 2097170.f, 10, 1, nullptr };
		unsigned char scanline[1]{};
		unsigned char pixels[4]{};
		NSVGcachedPaint paint{};
		paint.type = NSVG_PAINT_COLOR;
		paint.colors[0] = 0xFFFFFFFF;
		rasterizer->edges = &edge;
		rasterizer->nedges = 1;
		rasterizer->width = 1;
		rasterizer->height = 1;
		rasterizer->stride = 4;
		rasterizer->scanline = scanline;
		rasterizer->bitmap = pixels;
		nsvg__rasterizeSortedEdges( rasterizer.get(), 0, 0, 1, 1, &paint, NSVG_FILLRULE_NONZERO );
		// These buffers are borrowed; deletion must only release the pool pages.
		rasterizer->edges = nullptr;
		rasterizer->scanline = nullptr;
		ASSERT_TRUE( rasterizer->freelist != nullptr );
		EXPECT_EQ( rasterizer->freelist->x, sign > 0 ? 2147482624 : -2147482624 );
	}
}

UTEST( NanoSVGSecurity, InvalidGeometryDoesNotSuppressFollowingStyledShapes ) {
	char input[] = R"(<svg width="16" height="16">
<path d="M-3e7 0 L-3e7 16 L-4e7 8 Z"/>
<style>.icon { fill: currentColor; }</style>
<rect class="icon" x="2" y="2" width="12" height="12"/></svg>)";
	SVGImage image( nsvgParse( input, "px", 96, 0xFF332211 ), &nsvgDelete );
	SVGRasterizer rasterizer( nsvgCreateRasterizer(), &nsvgDeleteRasterizer );
	ASSERT_TRUE( image != nullptr );
	ASSERT_TRUE( rasterizer != nullptr );
	std::array<unsigned char, 16 * 16 * 4> pixels{};
	nsvgRasterize( rasterizer.get(), image.get(), 0, 0, 1, pixels.data(), 16, 16, 16 * 4 );
	const size_t center = ( 8 * 16 + 8 ) * 4;
	EXPECT_EQ( pixels[center], 0x11 );
	EXPECT_EQ( pixels[center + 1], 0x22 );
	EXPECT_EQ( pixels[center + 2], 0x33 );
	EXPECT_EQ( pixels[center + 3], 255 );
}

UTEST( NanoSVGSecurity, SingleStopGradientFillsTheRampFromItsOnlyStop ) {
	// The gradient allocation holds one stop; the color ramp must not read past it.
	char input[] = R"svg(<svg width="16" height="16"><defs><linearGradient id="g">
<stop offset="0.5" stop-color="#ff0000"/></linearGradient></defs>
<rect width="16" height="16" fill="url(#g)"/></svg>)svg";
	SVGImage image( nsvgParse( input, "px", 96, 0xFFFFFFFF ), &nsvgDelete );
	SVGRasterizer rasterizer( nsvgCreateRasterizer(), &nsvgDeleteRasterizer );
	ASSERT_TRUE( image != nullptr );
	ASSERT_TRUE( rasterizer != nullptr );
	std::array<unsigned char, 16 * 16 * 4> pixels{};
	nsvgRasterize( rasterizer.get(), image.get(), 0, 0, 1, pixels.data(), 16, 16, 16 * 4 );
	for ( size_t x : { 1, 8, 14 } ) {
		const size_t pixel = ( 8 * 16 + x ) * 4;
		EXPECT_EQ( pixels[pixel], 255 );
		EXPECT_EQ( pixels[pixel + 1], 0 );
		EXPECT_EQ( pixels[pixel + 2], 0 );
		EXPECT_EQ( pixels[pixel + 3], 255 );
	}
}

UTEST( NanoSVGSecurity, ClampMapsNaNToTheLowerBound ) {
	// Callers convert the clamped value to int and index the gradient color ramp with it.
	EXPECT_EQ( nsvg__clampf( std::numeric_limits<float>::quiet_NaN(), 0.f, 255.f ), 0.f );
	EXPECT_EQ( nsvg__clampf( -1.f, 0.f, 255.f ), 0.f );
	EXPECT_EQ( nsvg__clampf( 300.f, 0.f, 255.f ), 255.f );
	EXPECT_EQ( nsvg__clampf( 7.5f, 0.f, 255.f ), 7.5f );
}

UTEST( NanoSVGSecurity, DashedStrokesAreBoundedPerImage ) {
	// A 1-unit dash along a 1e9-unit line would emit about 1e9 dashes, and beyond the float
	// precision of the coordinates the dash position stops advancing.
	char input[] = R"(<svg width="32" height="32">
<line x1="0" y1="16" x2="1000000000" y2="16" stroke="#000" stroke-width="4" stroke-dasharray="1"/>
<line x1="0" y1="16" x2="1000000000" y2="16" stroke="#000" stroke-width="4" stroke-dasharray="1"/>
</svg>)";
	SVGImage image( nsvgParse( input, "px", 96, 0xFFFFFFFF ), &nsvgDelete );
	SVGRasterizer rasterizer( nsvgCreateRasterizer(), &nsvgDeleteRasterizer );
	ASSERT_TRUE( image != nullptr );
	ASSERT_TRUE( rasterizer != nullptr );
	std::array<unsigned char, 32 * 32 * 4> pixels{};
	nsvgRasterize( rasterizer.get(), image.get(), 0, 0, 1, pixels.data(), 32, 32, 32 * 4 );
	// The budget is shared by both lines: the first exhausts it, and each line stops at its
	// first dash past the budget.
	EXPECT_EQ( rasterizer->dashSegments, NSVG__MAX_DASH_SEGMENTS + 2 );
	// Dashes before the budget ran out are still drawn.
	EXPECT_GT( pixels[( 16 * 32 + 0 ) * 4 + 3], 0 );
}
