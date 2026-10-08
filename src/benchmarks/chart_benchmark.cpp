#include "../tests/unit_tests/utest.hpp"
#include <cmath>
#include <eepp/core/string.hpp>
#include <eepp/system/clock.hpp>
#include <eepp/ui/charts/chartreduction.hpp>

using namespace EE;
using namespace EE::UI::Charts;

UTEST( Benchmark, ChartReduction ) {
	for ( size_t count : { 10000u, 100000u, 1000000u, 5000000u } ) {
		std::vector<ChartPoint> points;
		points.reserve( count );
		for ( size_t i = 0; i < count; ++i ) {
			const double x = static_cast<double>( i );
			const double y = std::sin( x * 0.003 ) + ( i % 997 == 0 ? 5.0 : 0.0 );
			points.emplace_back( ChartPoint{ x, y } );
		}
		OwnedXYDataSource source;
		source.setPoints( std::move( points ) );
		auto read = source.acquireRead();
		const DataRange full{ 0.0, static_cast<double>( count - 1 ) };
		const DataRange middle{ count * 0.25, count * 0.75 };
		Uint64 emitted = 0;
		std::vector<ReducedPoint> reduced;
		System::Clock reductionClock;
		for ( int run = 0; run < 5; ++run ) {
			const auto visible = visibleLogicalRange( read, full.min, full.max );
			reducePixelBuckets( read, visible, full, 1200, reduced );
			emitted += reduced.size();
		}
		const auto reductionTime = reductionClock.getElapsedTime();
		System::Clock panClock;
		for ( int run = 0; run < 5; ++run ) {
			const auto visible =
				visibleLogicalRange( read, middle.min + run * 100, middle.max + run * 100 );
			reducePixelBuckets( read, visible, middle, 1200, reduced );
			emitted += reduced.size();
		}
		const auto panTime = panClock.getElapsedTime();
		System::Clock yClock;
		for ( int run = 0; run < 5; ++run )
			EXPECT_TRUE( visibleYExtent( read, middle ).has_value() );
		const auto yTime = yClock.getElapsedTime();
		UTEST_PRINT_INFO(
			String::format( "chart %zu points: full %.2f ms, pan %.2f ms, visible Y %.2f ms, "
							"emitted %llu",
							count, reductionTime.asMilliseconds() / 5.0,
							panTime.asMilliseconds() / 5.0, yTime.asMilliseconds() / 5.0,
							static_cast<unsigned long long>( emitted / 10 ) )
				.c_str() );
	}
}
