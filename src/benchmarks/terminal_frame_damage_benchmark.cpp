#include <tabulate/tabulate.hpp>

#include "../eepp/window/terminal/framedamage.hpp"
#include "../tests/unit_tests/utest.hpp"

#include <eepp/core/string.hpp>
#include <eepp/system/clock.hpp>

#include <algorithm>
#include <array>
#include <cstring>
#include <functional>
#include <vector>

using namespace EE;
using namespace EE::System;
using namespace EE::Window::Private;

namespace {

struct DamageRectangle {
	Int32 x;
	Int32 y;
	Int32 width;
	Int32 height;
};

struct BenchmarkResult {
	double microsecondsPerFrame;
	size_t transferPixels;
	Uint64 checksum;
};

using DamageFunction =
	std::function<FrameDamageResult( const std::vector<Uint8>&, const std::vector<Uint8>&,
									 const Math::Sizei&, std::vector<DamageRectangle>& )>;

FrameDamageResult findFrameDamageWithEqualityCheck( const std::vector<Uint8>& current,
													const std::vector<Uint8>& previous,
													const Math::Sizei& size,
													std::vector<DamageRectangle>& rectangles ) {
	if ( 0 == std::memcmp( current.data(), previous.data(), current.size() ) ) {
		rectangles.clear();
		return FrameDamageResult::None;
	}
	return findFrameDamage( current, previous, size, rectangles );
}

FrameDamageResult findFrameDamageRowMajor( const std::vector<Uint8>& current,
										   const std::vector<Uint8>& previous,
										   const Math::Sizei& size,
										   std::vector<DamageRectangle>& rectangles,
										   std::vector<Uint8>& changedTiles ) {
	rectangles.clear();
	const Int32 tilesX = ( size.x + FrameDamageTileSize - 1 ) / FrameDamageTileSize;
	const Int32 tilesY = ( size.y + FrameDamageTileSize - 1 ) / FrameDamageTileSize;
	const size_t rowBytes = static_cast<size_t>( size.x ) * 3;
	Int32 left = size.x;
	Int32 top = size.y;
	Int32 right = 0;
	Int32 bottom = 0;
	changedTiles.resize( static_cast<size_t>( tilesX ) );

	for ( Int32 tileY = 0; tileY < tilesY; ++tileY ) {
		const Int32 y = tileY * FrameDamageTileSize;
		const Int32 height = eemin( FrameDamageTileSize, size.y - y );
		std::fill( changedTiles.begin(), changedTiles.end(), 0 );
		Int32 remainingTiles = tilesX;
		for ( Int32 row = 0; row < height && remainingTiles; ++row ) {
			const Int32 sourceRow = size.y - 1 - ( y + row );
			const Uint8* currentRow = current.data() + static_cast<size_t>( sourceRow ) * rowBytes;
			const Uint8* previousRow =
				previous.data() + static_cast<size_t>( sourceRow ) * rowBytes;
			for ( Int32 tileX = 0; tileX < tilesX; ++tileX ) {
				if ( changedTiles[static_cast<size_t>( tileX )] )
					continue;
				const Int32 x = tileX * FrameDamageTileSize;
				const Int32 width = eemin( FrameDamageTileSize, size.x - x );
				const size_t offset = static_cast<size_t>( x ) * 3;
				if ( 0 != std::memcmp( currentRow + offset, previousRow + offset,
									   static_cast<size_t>( width ) * 3 ) ) {
					changedTiles[static_cast<size_t>( tileX )] = 1;
					--remainingTiles;
					left = eemin( left, x );
					top = eemin( top, y );
					right = eemax( right, x + width );
					bottom = eemax( bottom, y + height );
				}
			}
		}
	}

	if ( left < right && top < bottom ) {
		rectangles.push_back( { left, top, right - left, bottom - top } );
		return left == 0 && top == 0 && right == size.x && bottom == size.y
				   ? FrameDamageResult::Full
				   : FrameDamageResult::Rectangle;
	}
	return FrameDamageResult::None;
}

void changeRectangle( std::vector<Uint8>& pixels, const Math::Sizei& size, Int32 x, Int32 y,
					  Int32 width, Int32 height ) {
	const size_t rowBytes = static_cast<size_t>( size.x ) * 3;
	for ( Int32 row = y; row < y + height; ++row ) {
		Uint8* pixel =
			pixels.data() + static_cast<size_t>( row ) * rowBytes + static_cast<size_t>( x ) * 3;
		for ( Int32 column = 0; column < width * 3; ++column )
			pixel[column] ^= static_cast<Uint8>( 0x5A + ( column & 7 ) );
	}
}

void changeRandomTiles( std::vector<Uint8>& pixels, const Math::Sizei& size, Int32 percentage ) {
	Uint32 state = 0x12345678;
	const Int32 tilesX = ( size.x + FrameDamageTileSize - 1 ) / FrameDamageTileSize;
	const Int32 tilesY = ( size.y + FrameDamageTileSize - 1 ) / FrameDamageTileSize;
	for ( Int32 tileY = 0; tileY < tilesY; ++tileY ) {
		for ( Int32 tileX = 0; tileX < tilesX; ++tileX ) {
			state ^= state << 13;
			state ^= state >> 17;
			state ^= state << 5;
			if ( static_cast<Int32>( state % 100 ) >= percentage )
				continue;
			const Int32 x = tileX * FrameDamageTileSize;
			const Int32 y = tileY * FrameDamageTileSize;
			changeRectangle( pixels, size, x, y, eemin( FrameDamageTileSize, size.x - x ),
							 eemin( FrameDamageTileSize, size.y - y ) );
		}
	}
}

BenchmarkResult runBenchmark( const DamageFunction& function, const std::vector<Uint8>& current,
							  const std::vector<Uint8>& previous, const Math::Sizei& size,
							  Int32 iterations ) {
	static constexpr size_t SampleCount = 5;
	std::vector<DamageRectangle> rectangles;
	FrameDamageResult result = FrameDamageResult::None;
	Uint64 checksum = 0;
	function( current, previous, size, rectangles );
	std::array<double, SampleCount> samples;
	for ( double& sample : samples ) {
		Clock clock;
		for ( Int32 iteration = 0; iteration < iterations; ++iteration ) {
			result = function( current, previous, size, rectangles );
			checksum += 1 + static_cast<Uint8>( result ) + rectangles.size();
		}
		sample = static_cast<double>( clock.getElapsedTime().asMicroseconds() ) / iterations;
	}
	std::sort( samples.begin(), samples.end() );
	size_t transferPixels = 0;
	if ( result == FrameDamageResult::Full ) {
		transferPixels = static_cast<size_t>( size.x ) * size.y;
	} else {
		for ( const DamageRectangle& rectangle : rectangles )
			transferPixels += static_cast<size_t>( rectangle.width ) * rectangle.height;
	}
	return { samples[SampleCount / 2], transferPixels, checksum };
}

} // namespace

UTEST( Benchmark, TerminalFrameDamage ) {
	struct Resolution {
		Math::Sizei size;
		Int32 iterations;
	};
	struct Scenario {
		const char* name;
		std::function<void( std::vector<Uint8>&, const Math::Sizei& )> change;
	};

	const std::vector<Resolution> resolutions = {
		{ { 1280, 720 }, 150 }, { { 1920, 1080 }, 80 }, { { 3840, 2160 }, 20 } };
	const std::vector<Scenario> scenarios = {
		{ "identical", []( auto&, const auto& ) {} },
		{ "caret",
		  []( auto& pixels, const auto& size ) {
			  changeRectangle( pixels, size, size.x / 2, size.y / 2, 3, 24 );
		  } },
		{ "full-width-row",
		  []( auto& pixels, const auto& size ) {
			  changeRectangle( pixels, size, 0, size.y / 2, size.x, 32 );
		  } },
		{ "distant-corners",
		  []( auto& pixels, const auto& size ) {
			  changeRectangle( pixels, size, 0, 0, 32, 32 );
			  changeRectangle( pixels, size, size.x - 32, size.y - 32, 32, 32 );
		  } },
		{ "scroll-region",
		  []( auto& pixels, const auto& size ) {
			  changeRectangle( pixels, size, size.x / 5, size.y / 5, size.x * 3 / 5,
							   size.y * 3 / 5 );
		  } },
		{ "random-10pct",
		  []( auto& pixels, const auto& size ) { changeRandomTiles( pixels, size, 10 ); } },
		{ "random-50pct",
		  []( auto& pixels, const auto& size ) { changeRandomTiles( pixels, size, 50 ); } },
		{ "random-100pct",
		  []( auto& pixels, const auto& size ) { changeRandomTiles( pixels, size, 100 ); } },
	};

	std::vector<Uint8> changedTiles;
	const DamageFunction currentAlgorithm = []( const auto& current, const auto& previous,
												const auto& size, auto& rectangles ) {
		return findFrameDamage( current, previous, size, rectangles );
	};
	const DamageFunction equalityFastPath = findFrameDamageWithEqualityCheck;
	const DamageFunction rowMajor = [&changedTiles]( const auto& current, const auto& previous,
													 const auto& size, auto& rectangles ) {
		return findFrameDamageRowMajor( current, previous, size, rectangles, changedTiles );
	};
	const DamageFunction equalityRowMajor = [&changedTiles]( const auto& current,
															 const auto& previous, const auto& size,
															 auto& rectangles ) {
		if ( 0 == std::memcmp( current.data(), previous.data(), current.size() ) ) {
			rectangles.clear();
			return FrameDamageResult::None;
		}
		return findFrameDamageRowMajor( current, previous, size, rectangles, changedTiles );
	};
	const DamageFunction adaptive = [&changedTiles]( const auto& current, const auto& previous,
													 const auto& size, auto& rectangles ) {
		return findFrameDamageAdaptive( current, previous, size, rectangles, changedTiles );
	};

	tabulate::Table table;
	table.add_row( { "Resolution", "Scenario", "Tile-major us", "memcmp + tile us", "Row-major us",
					 "memcmp + rows us", "Production", "Production us", "Gap to fastest",
					 "Transfer MiB" } );
	for ( size_t column = 0; column < table[0].size(); ++column ) {
		table[0][column]
			.format()
			.font_align( tabulate::FontAlign::center )
			.font_style( { tabulate::FontStyle::bold } );
	}
	for ( const Resolution& resolution : resolutions ) {
		const size_t frameBytes = static_cast<size_t>( resolution.size.x ) * resolution.size.y * 3;
		std::vector<Uint8> previous( frameBytes, 0x35 );
		for ( const Scenario& scenario : scenarios ) {
			std::vector<Uint8> current = previous;
			scenario.change( current, resolution.size );
			std::vector<DamageRectangle> expectedRectangles;
			std::vector<DamageRectangle> candidateRectangles;
			const FrameDamageResult expected =
				currentAlgorithm( current, previous, resolution.size, expectedRectangles );
			for ( const DamageFunction* candidate :
				  { &equalityFastPath, &rowMajor, &equalityRowMajor, &adaptive } ) {
				const FrameDamageResult actual =
					( *candidate )( current, previous, resolution.size, candidateRectangles );
				EXPECT_EQ( static_cast<int>( expected ), static_cast<int>( actual ) );
				EXPECT_EQ( expectedRectangles.size(), candidateRectangles.size() );
				if ( !expectedRectangles.empty() ) {
					EXPECT_EQ( expectedRectangles[0].x, candidateRectangles[0].x );
					EXPECT_EQ( expectedRectangles[0].y, candidateRectangles[0].y );
					EXPECT_EQ( expectedRectangles[0].width, candidateRectangles[0].width );
					EXPECT_EQ( expectedRectangles[0].height, candidateRectangles[0].height );
				}
			}
			const BenchmarkResult baseline = runBenchmark( currentAlgorithm, current, previous,
														   resolution.size, resolution.iterations );
			const BenchmarkResult equality = runBenchmark( equalityFastPath, current, previous,
														   resolution.size, resolution.iterations );
			const BenchmarkResult rows =
				runBenchmark( rowMajor, current, previous, resolution.size, resolution.iterations );
			const BenchmarkResult equalityRows = runBenchmark(
				equalityRowMajor, current, previous, resolution.size, resolution.iterations );
			const bool productionUsesRows = frameBytes >= FrameDamageRowMajorThreshold;
			const double productionTime =
				productionUsesRows ? rows.microsecondsPerFrame : baseline.microsecondsPerFrame;
			const double fastestTime =
				eemin( eemin( baseline.microsecondsPerFrame, equality.microsecondsPerFrame ),
					   eemin( rows.microsecondsPerFrame, equalityRows.microsecondsPerFrame ) );
			const double gapToFastest = ( productionTime / fastestTime - 1.0 ) * 100.0;
			const double transferMiB =
				static_cast<double>( baseline.transferPixels * 3 ) / ( 1024.0 * 1024.0 );
			table.add_row( { String::format( "%dx%d", resolution.size.x, resolution.size.y ),
							 scenario.name, String::format( "%.2f", baseline.microsecondsPerFrame ),
							 String::format( "%.2f", equality.microsecondsPerFrame ),
							 String::format( "%.2f", rows.microsecondsPerFrame ),
							 String::format( "%.2f", equalityRows.microsecondsPerFrame ),
							 productionUsesRows ? "row-major" : "tile-major",
							 String::format( "%.2f", productionTime ),
							 String::format( "%.1f%%", gapToFastest ),
							 String::format( "%.2f", transferMiB ) } );
			EXPECT_GT(
				baseline.checksum + equality.checksum + rows.checksum + equalityRows.checksum, 0u );
		}
	}
	for ( size_t column = 2; column < table[0].size(); ++column )
		table.column( column ).format().font_align( tabulate::FontAlign::right );
	UTEST_PRINT_INFO( ( "\n" + table.str() ).c_str() );
}
