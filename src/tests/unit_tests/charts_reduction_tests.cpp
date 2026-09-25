#include "utest.h"
#include <cmath>
#include <eepp/ui/charts/chartreduction.hpp>
#include <limits>

using namespace EE::UI::Charts;

UTEST( ChartReduction, preservesSpikesAndProvenance ) {
	OwnedXYDataSource source;
	std::vector<ChartPoint> points;
	points.reserve( 1000 );
	for ( size_t i = 0; i < 1000; ++i )
		points.emplace_back( ChartPoint{ static_cast<double>( i ), i == 503 ? 100.0 : 0.0 } );
	source.setPoints( std::move( points ) );
	auto read = source.acquireRead();
	auto reduced = reducePixelBuckets( read, { 0, 1000 }, { 0, 999 }, 10 );
	EXPECT_TRUE( reduced.size() <= 40 );
	bool spike = false;
	for ( const auto& point : reduced )
		spike |= point.sourceIndex == 503 && point.y == 100;
	EXPECT_TRUE( spike );
	EXPECT_EQ( reduced.front().sourceIndex, 0u );
	EXPECT_EQ( reduced.back().sourceIndex, 999u );
}

UTEST( ChartReduction, gapsBreakConnectivity ) {
	OwnedXYDataSource source;
	source.setPoints(
		{ { 0, 0 }, { 1, 1 }, { 2, std::numeric_limits<double>::quiet_NaN() }, { 3, 3 } } );
	auto read = source.acquireRead();
	auto reduced = reducePixelBuckets( read, { 0, 4 }, { 0, 3 }, 4 );
	EXPECT_EQ( reduced.size(), 4u );
	EXPECT_TRUE( reduced[2].gap );
	EXPECT_EQ( reduced[2].sourceIndex, 2u );
}

UTEST( ChartReduction, reusableBufferReplacesPreviousOutput ) {
	OwnedXYDataSource source;
	source.setPoints( { { 0, 1 }, { 1, 2 }, { 2, 3 } } );
	auto read = source.acquireRead();
	std::vector<ReducedPoint> reduced;
	reducePixelBuckets( read, { 0, 3 }, { 0, 2 }, 3, reduced );
	EXPECT_EQ( reduced.size(), 3u );
	const auto capacity = reduced.capacity();
	reducePixelBuckets( read, { 1, 2 }, { 0, 2 }, 3, reduced );
	EXPECT_EQ( reduced.size(), 1u );
	EXPECT_EQ( reduced.front().sourceIndex, 1u );
	EXPECT_EQ( reduced.capacity(), capacity );
	reducePixelBuckets( read, { 0, 0 }, { 0, 2 }, 3, reduced );
	EXPECT_TRUE( reduced.empty() );
	EXPECT_EQ( reduced.capacity(), capacity );
}

UTEST( ChartReduction, wrappedRingKeepsChronologicalCurve ) {
	RingXYDataSource ring( 1000 );
	for ( int i = 0; i < 1080; ++i ) {
		const double x = static_cast<double>( i );
		ring.append( { x, std::sin( x * 0.03 ) + 0.2 * std::sin( x * 0.17 ) } );
	}
	const auto read = ring.acquireRead();
	EXPECT_EQ( read.chunks().size(), 2u );
	EXPECT_EQ( pointAt( read, 0 ).x, 80 );
	EXPECT_EQ( pointAt( read, 999 ).x, 1079 );
	auto visible = visibleLogicalRange( read, 829, 1079 );
	visible.begin = visible.begin ? visible.begin - 1 : 0;
	auto reduced = reducePixelBuckets( read, visible, { 829, 1079 }, 500 );
	EXPECT_EQ( reduced.size(), visible.end - visible.begin );
	for ( size_t i = 1; i < reduced.size(); ++i ) {
		EXPECT_TRUE( reduced[i].sourceIndex > reduced[i - 1].sourceIndex );
		EXPECT_TRUE( reduced[i].x > reduced[i - 1].x );
		EXPECT_TRUE( reduced[i].x - reduced[i - 1].x <= 1 );
	}
}
