#include <algorithm>
#include <cmath>
#include <eepp/ui/charts/chartreduction.hpp>

namespace EE { namespace UI { namespace Charts {

namespace {

struct Bucket {
	ReducedPoint first;
	ReducedPoint minimum;
	ReducedPoint maximum;
	ReducedPoint last;
	bool used{ false };
};

void emit( std::vector<ReducedPoint>& output, const Bucket& bucket ) {
	if ( !bucket.used )
		return;
	ReducedPoint candidates[4] = { bucket.first, bucket.minimum, bucket.maximum, bucket.last };
	std::sort( std::begin( candidates ), std::end( candidates ),
			   []( const ReducedPoint& a, const ReducedPoint& b ) {
				   return a.sourceIndex < b.sourceIndex;
			   } );
	for ( const auto& candidate : candidates ) {
		if ( output.empty() || output.back().sourceIndex != candidate.sourceIndex )
			output.emplace_back( candidate );
	}
}

} // namespace

void reducePixelBuckets( const XYDataRead& read, IndexRange visible, DataRange xRange,
						 size_t pixelWidth, std::vector<ReducedPoint>& output ) {
	output.clear();
	if ( read.order() != DataOrder::AscendingX || visible.begin >= visible.end ||
		 visible.end > read.size() || pixelWidth == 0 || xRange.min >= xRange.max )
		return;
	if ( visible.end - visible.begin <= pixelWidth ) {
		output.reserve( visible.end - visible.begin );
		for ( const auto& chunk : read.chunks() ) {
			const size_t begin = std::max( visible.begin, chunk.logicalBegin );
			const size_t end = std::min( visible.end, chunk.logicalBegin + chunk.size() );
			if ( begin >= end )
				continue;
			std::visit(
				[&]( const auto& xs, const auto& ys ) {
					for ( size_t index = begin; index < end; ++index ) {
						const size_t i = index - chunk.logicalBegin;
						const double x = xs[i];
						const double y = ys[i];
						output.emplace_back( ReducedPoint{
							x, y, index, !std::isfinite( x ) || !std::isfinite( y ) } );
					}
				},
				chunk.xs, chunk.ys );
		}
		return;
	}
	output.reserve( std::min( visible.end - visible.begin, pixelWidth * 4 + 2 ) );
	const double pixelsPerUnit = static_cast<double>( pixelWidth ) / ( xRange.max - xRange.min );
	Bucket bucket;
	size_t currentPixel = 0;
	bool havePixel = false;
	for ( const auto& chunk : read.chunks() ) {
		const size_t begin = std::max( visible.begin, chunk.logicalBegin );
		const size_t end = std::min( visible.end, chunk.logicalBegin + chunk.size() );
		if ( begin >= end )
			continue;
		std::visit(
			[&]( const auto& xs, const auto& ys ) {
				for ( size_t index = begin; index < end; ++index ) {
					const size_t i = index - chunk.logicalBegin;
					const double x = xs[i];
					const double y = ys[i];
					if ( !std::isfinite( x ) || !std::isfinite( y ) ) {
						emit( output, bucket );
						bucket = {};
						havePixel = false;
						if ( output.empty() || !output.back().gap )
							output.emplace_back( ReducedPoint{ x, y, index, true } );
						continue;
					}
					const double relative = ( x - xRange.min ) * pixelsPerUnit;
					const size_t pixel = static_cast<size_t>(
						std::clamp( relative, 0.0, static_cast<double>( pixelWidth - 1 ) ) );
					if ( havePixel && pixel != currentPixel ) {
						emit( output, bucket );
						bucket = {};
					}
					currentPixel = pixel;
					havePixel = true;
					const ReducedPoint point{ x, y, index, false };
					if ( !bucket.used ) {
						bucket = { point, point, point, point, true };
					} else {
						bucket.last = point;
						if ( y < bucket.minimum.y )
							bucket.minimum = point;
						if ( y > bucket.maximum.y )
							bucket.maximum = point;
					}
				}
			},
			chunk.xs, chunk.ys );
	}
	emit( output, bucket );
}

std::vector<ReducedPoint> reducePixelBuckets( const XYDataRead& read, IndexRange visible,
											  DataRange xRange, size_t pixelWidth ) {
	std::vector<ReducedPoint> output;
	reducePixelBuckets( read, visible, xRange, pixelWidth, output );
	return output;
}

}}} // namespace EE::UI::Charts
