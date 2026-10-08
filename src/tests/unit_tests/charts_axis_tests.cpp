#include "utest.h"
#include <cmath>
#include <eepp/ui/charts/chartaxis.hpp>

using namespace EE::UI::Charts;

UTEST( ChartAxis, fitZoomPanAndTransform ) {
	ChartAxis axis( AxisPosition::Bottom );
	AxisViewport viewport;
	viewport.fit( DataRange{ 10, 20 }, axis );
	EXPECT_EQ( viewport.range().min, 10 );
	EXPECT_EQ( viewport.range().max, 20 );
	EXPECT_EQ( viewport.toPixel( 15, 100, 200, false, axis ), 200 );
	EXPECT_EQ( viewport.fromPixel( 200, 100, 200, false, axis ), 15 );
	EXPECT_EQ( viewport.toPixel( 10, 100, 200, true, axis ), 300 );
	viewport.zoom( 15, 2, axis );
	EXPECT_EQ( viewport.range().min, 12.5 );
	EXPECT_EQ( viewport.range().max, 17.5 );
	viewport.pan( 2, axis );
	EXPECT_EQ( viewport.range().min, 14.5 );
	EXPECT_TRUE( viewport.manual() );
}

UTEST( ChartAxis, constraintsAndDegenerateRanges ) {
	ChartAxis axis( AxisPosition::Left );
	axis.setConstraints( RangeConstraints{ 0, 100, 10, 40 } );
	AxisViewport viewport;
	viewport.setRange( { 95, 96 }, axis );
	EXPECT_EQ( viewport.range().max, 100 );
	EXPECT_EQ( viewport.range().min, 90 );
	viewport.setRange( { -100, 200 }, axis );
	EXPECT_EQ( viewport.range().max - viewport.range().min, 40 );
	viewport.fit( DataRange{ 3, 3 }, axis );
	EXPECT_TRUE( viewport.range().min < 3 );
	EXPECT_TRUE( viewport.range().max > 3 );
	EXPECT_FALSE( viewport.manual() );
}

UTEST( ChartAxis, niceTicks ) {
	auto ticks = linearTicks( { -2, 3 }, 500, 100 );
	EXPECT_TRUE( ticks.size() >= 4 );
	EXPECT_TRUE( ticks.size() <= 8 );
	bool zero = false;
	for ( double tick : ticks )
		zero |= tick == 0;
	EXPECT_TRUE( zero );
	EXPECT_TRUE( linearTicks( { 1, 1 }, 500 ).empty() );
	EXPECT_FALSE( formatNumericTick( 1.25, 0.25 ).empty() );
	ChartAxis axis( AxisPosition::Bottom );
	axis.setLabel( EE::String( "Time (s)" ) );
	EXPECT_TRUE( axis.label() == EE::String( "Time (s)" ) );
	axis.setFormatter( []( double value, double ) { return EE::String::toString( value ) + "%"; } );
	EXPECT_TRUE( axis.formatTick( 12, 1 ).toUtf8().find( '%' ) != std::string::npos );
}
