#include "utest.h"
#include <array>
#include <atomic>
#include <eepp/ui/charts/modelxydatasource.hpp>
#include <eepp/ui/charts/xydatasource.hpp>
#include <limits>
#include <thread>

using namespace EE;
using namespace EE::UI::Charts;

UTEST( ChartData, contiguousAndStridedViews ) {
	std::vector<double> values{ 1, 2, 3 };
	std::array<double, 3> array{ 4, 5, 6 };
	SmallVector<double, 4> small{ 7, 8, 9 };
	std::span<const double> span( values );
	StridedSpan<double> a( values );
	StridedSpan<double> b( array );
	StridedSpan<double> c( small );
	StridedSpan<double> d( span );
	EXPECT_EQ( a[2], 3 );
	EXPECT_EQ( b[1], 5 );
	EXPECT_EQ( c[0], 7 );
	EXPECT_EQ( d.span()[2], 3 );
	EXPECT_TRUE( a.contiguous() );

	struct Sample {
		double x;
		float y;
		Uint32 flags;
	};
	std::array<Sample, 3> samples{ Sample{ 1, 10, 0 }, Sample{ 2, 20, 0 }, Sample{ 3, 30, 0 } };
	auto xs = memberSpan( std::span<const Sample>( samples ), &Sample::x );
	auto ys = memberSpan( std::span<const Sample>( samples ), &Sample::y );
	EXPECT_FALSE( xs.contiguous() );
	EXPECT_EQ( xs.strideBytes(), sizeof( Sample ) );
	EXPECT_EQ( xs[2], 3 );
	EXPECT_EQ( ys[1], 20 );
}

UTEST( ChartData, chunkViewsShareLogicalResults ) {
	std::array<double, 4> xs{ 0, 1, 2, 3 };
	std::array<float, 4> ys{ 2, 4, 8, 16 };
	SmallVector<XYDataChunk, 2> chunks;
	chunks.emplace_back( XYDataChunk{ 0, StridedSpan<double>( xs ), StridedSpan<float>( ys ) } );
	XYDataRead soa( 1, DataOrder::AscendingX, std::move( chunks ) );
	EXPECT_EQ( soa.size(), 4u );
	EXPECT_EQ( pointAt( soa, 3 ).y, 16 );
	EXPECT_EQ( lowerBoundX( soa, 1.5 ), 2u );
	auto visible = visibleLogicalRange( soa, 1, 2 );
	EXPECT_EQ( visible.begin, 1u );
	EXPECT_EQ( visible.end, 3u );
	EXPECT_EQ( yExtent( soa )->max, 16 );

	SmallVector<XYDataChunk, 2> linearChunks;
	linearChunks.emplace_back( XYDataChunk{ 0, LinearSpan{ 0, 1, 4 }, StridedSpan<float>( ys ) } );
	XYDataRead linear( 2, DataOrder::AscendingX, std::move( linearChunks ) );
	for ( size_t i = 0; i < soa.size(); ++i ) {
		EXPECT_EQ( pointAt( soa, i ).x, pointAt( linear, i ).x );
		EXPECT_EQ( pointAt( soa, i ).y, pointAt( linear, i ).y );
	}
}

UTEST( ChartData, immutableFactoriesRetainSharedStorage ) {
	auto xs = std::make_shared<const std::vector<double>>( std::vector<double>{ 1, 2, 3 } );
	auto ys = std::make_shared<const std::vector<float>>( std::vector<float>{ 4, 5, 6 } );
	auto arrays = makeArrayXYDataSource( xs, ys );
	auto read = arrays->acquireRead();
	arrays.reset();
	xs.reset();
	ys.reset();
	EXPECT_EQ( read.order(), DataOrder::AscendingX );
	EXPECT_EQ( pointAt( read, 2 ).y, 6 );

	struct Sample {
		double x;
		float y;
	};
	auto samples =
		std::make_shared<const std::vector<Sample>>( std::vector<Sample>{ { 1, 10 }, { 2, 20 } } );
	auto members = makeMemberXYDataSource( samples, &Sample::x, &Sample::y );
	const auto memberRead = members->acquireRead();
	EXPECT_EQ( memberRead.order(), DataOrder::AscendingX );
	EXPECT_EQ( pointAt( memberRead, 1 ).y, 20 );
}

UTEST( ChartData, finiteExtentsAndDuplicates ) {
	OwnedXYDataSource source;
	source.setPoints(
		{ { 1, 2 }, { 1, 4 }, { 2, std::numeric_limits<double>::quiet_NaN() }, { 3, 8 } } );
	auto read = source.acquireRead();
	EXPECT_EQ( read.order(), DataOrder::AscendingX );
	EXPECT_EQ( lowerBoundX( read, 1 ), 0u );
	EXPECT_EQ( visibleLogicalRange( read, 1, 1 ).end, 2u );
	EXPECT_EQ( xExtent( read )->max, 3 );
	EXPECT_EQ( yExtent( read )->min, 2 );
	EXPECT_EQ( yExtent( read )->max, 8 );
}

UTEST( ChartData, ownedReadSurvivesSourceDestruction ) {
	XYDataRead read;
	{
		OwnedXYDataSource source;
		source.append( { 42, 7 } );
		read = source.acquireRead();
	}
	EXPECT_EQ( pointAt( read, 0 ).x, 42 );
}

UTEST( ChartData, deltasDescribeLatestMutation ) {
	OwnedXYDataSource owned;
	owned.setPoints( { { 1, 2 } } );
	{
		const auto read = owned.acquireRead();
		EXPECT_TRUE( read.delta().has_value() );
		EXPECT_TRUE( read.delta()->reset );
	}
	owned.append( { 2, 3 } );
	{
		const auto read = owned.acquireRead();
		EXPECT_FALSE( read.delta()->reset );
		EXPECT_EQ( read.delta()->fromGeneration + 1, read.generation() );
		EXPECT_EQ( read.delta()->appended.begin, 1u );
		EXPECT_EQ( read.delta()->appended.end, 2u );
	}
	RingXYDataSource ring( 1 );
	ring.append( { 1, 2 } );
	ring.append( { 2, 3 } );
	const auto read = ring.acquireRead();
	EXPECT_EQ( read.delta()->removedFront, 1u );
	EXPECT_EQ( read.delta()->appended.begin, 0u );
	EXPECT_EQ( read.delta()->appended.end, 1u );
}

UTEST( ChartData, ringExposesWrappedChunks ) {
	RingXYDataSource ring( 4 );
	for ( int i = 0; i < 6; ++i )
		ring.append( { static_cast<double>( i ), static_cast<double>( i * 2 ) } );
	auto read = ring.acquireRead();
	EXPECT_EQ( read.size(), 4u );
	EXPECT_EQ( read.chunks().size(), 2u );
	EXPECT_EQ( read.chunks()[1].logicalBegin, 2u );
	for ( size_t i = 0; i < 4; ++i ) {
		EXPECT_EQ( pointAt( read, i ).x, static_cast<double>( i + 2 ) );
		EXPECT_EQ( pointAt( read, i ).y, static_cast<double>( ( i + 2 ) * 2 ) );
	}
	EXPECT_EQ( lowerBoundX( read, 4 ), 2u );
	EXPECT_EQ( visibleYExtent( read, { 3, 4 } )->min, 6 );
	EXPECT_EQ( visibleYExtent( read, { 3, 4 } )->max, 8 );
}

UTEST( ChartData, concurrentRingReadsAndNotifications ) {
	RingXYDataSource ring( 32 );
	std::atomic<int> notifications{ 0 };
	auto connection = ring.onChanged( [&]( Uint64 ) { ++notifications; } );
	std::thread writer( [&] {
		for ( int i = 0; i < 200; ++i )
			ring.append( { static_cast<double>( i ), static_cast<double>( i ) } );
	} );
	for ( int i = 0; i < 200; ++i ) {
		auto read = ring.acquireRead();
		for ( size_t j = 1; j < read.size(); ++j )
			EXPECT_TRUE( pointAt( read, j - 1 ).x < pointAt( read, j ).x );
	}
	writer.join();
	EXPECT_EQ( notifications.load(), 200 );
	connection.disconnect();
	ring.append( { 200, 200 } );
	EXPECT_EQ( notifications.load(), 200 );
}

namespace {
class NumericChartModel : public EE::UI::Models::Model {
  public:
	size_t rowCount( const EE::UI::Models::ModelIndex& = {} ) const override {
		return values.size();
	}

	size_t columnCount( const EE::UI::Models::ModelIndex& = {} ) const override { return 2; }

	EE::UI::Models::Variant
	data( const EE::UI::Models::ModelIndex& index,
		  EE::UI::Models::ModelRole = EE::UI::Models::ModelRole::Display ) const override {
		return index.column() == 0 ? EE::UI::Models::Variant( values[index.row()].first )
								   : EE::UI::Models::Variant( values[index.row()].second );
	}

	std::vector<std::pair<float, float>> values{ { 1, 2 }, { 2, 4 } };
};
} // namespace

UTEST( ChartData, modelAdapterPublishesTypedSnapshot ) {
	auto model = std::make_shared<NumericChartModel>();
	ModelXYDataSource source( model, 0, 1 );
	{
		auto read = source.acquireRead();
		EXPECT_EQ( read.size(), 2u );
		EXPECT_EQ( pointAt( read, 1 ).y, 4 );
	}
	model->values.emplace_back( 3, 9 );
	model->update();
	auto read = source.acquireRead();
	EXPECT_EQ( read.size(), 3u );
	EXPECT_EQ( pointAt( read, 2 ).x, 3 );
	EXPECT_EQ( pointAt( read, 2 ).y, 9 );
}
