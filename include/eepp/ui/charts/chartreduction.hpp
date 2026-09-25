#ifndef EE_UI_CHARTS_CHARTREDUCTION_HPP
#define EE_UI_CHARTS_CHARTREDUCTION_HPP

#include <eepp/ui/charts/xydatasource.hpp>

namespace EE { namespace UI { namespace Charts {

struct ReducedPoint {
	double x{ 0.0 };
	double y{ 0.0 };
	size_t sourceIndex{ 0 };
	bool gap{ false };
};

/**
 * Reduces an ascending-X visible interval to at most four source samples per pixel bucket.
 * Each point retains its logical index. Invalid values split line connectivity.
 */
EE_API std::vector<ReducedPoint> reducePixelBuckets( const XYDataRead& read, IndexRange visible,
													 DataRange xRange, size_t pixelWidth );

/** Writes into a reusable buffer, retaining its capacity across data updates. */
EE_API void reducePixelBuckets( const XYDataRead& read, IndexRange visible, DataRange xRange,
								size_t pixelWidth, std::vector<ReducedPoint>& output );

}}} // namespace EE::UI::Charts

#endif
