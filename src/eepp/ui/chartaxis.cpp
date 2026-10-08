#include <algorithm>
#include <cmath>
#include <cstdio>
#include <eepp/ui/charts/chartaxis.hpp>
#include <limits>

namespace EE { namespace UI { namespace Charts {

namespace {

DataRange expanded( DataRange range ) {
	if ( !std::isfinite( range.min ) || !std::isfinite( range.max ) || range.min > range.max )
		return { 0.0, 1.0 };
	if ( range.min == range.max ) {
		const double padding = std::max( 1.0, std::abs( range.min ) * 0.05 );
		range.min = std::nextafter( range.min - padding, -std::numeric_limits<double>::infinity() );
		range.max = std::nextafter( range.max + padding, std::numeric_limits<double>::infinity() );
	}
	return range;
}

double niceStep( double raw ) {
	if ( !std::isfinite( raw ) || raw <= 0 )
		return 1.0;
	const double power = std::pow( 10.0, std::floor( std::log10( raw ) ) );
	const double normalized = raw / power;
	if ( normalized <= 1.0 )
		return power;
	if ( normalized <= 2.0 )
		return 2.0 * power;
	if ( normalized <= 2.5 )
		return 2.5 * power;
	if ( normalized <= 5.0 )
		return 5.0 * power;
	return 10.0 * power;
}

} // namespace

String ChartAxis::formatTick( double value, double step ) const {
	return mFormatter ? mFormatter( value, step ) : formatNumericTick( value, step );
}

void AxisViewport::fit( std::optional<DataRange> extent, const ChartAxis& axis ) {
	apply( extent ? *extent : DataRange{ 0.0, 1.0 }, axis, false );
}

void AxisViewport::setRange( DataRange range, const ChartAxis& axis ) {
	apply( range, axis, true );
}

void AxisViewport::zoom( double anchor, double factor, const ChartAxis& axis ) {
	if ( !std::isfinite( factor ) || factor <= 0 )
		return;
	const double transformedAnchor = axis.scale().forward( anchor );
	const double low = axis.scale().forward( mRange.min );
	const double high = axis.scale().forward( mRange.max );
	apply( { axis.scale().inverse( transformedAnchor + ( low - transformedAnchor ) / factor ),
			 axis.scale().inverse( transformedAnchor + ( high - transformedAnchor ) / factor ) },
		   axis, true );
}

void AxisViewport::pan( double delta, const ChartAxis& axis ) {
	const double low = axis.scale().forward( mRange.min );
	const double high = axis.scale().forward( mRange.max );
	apply( { axis.scale().inverse( low + delta ), axis.scale().inverse( high + delta ) }, axis,
		   true );
}

void AxisViewport::apply( DataRange range, const ChartAxis& axis, bool manual ) {
	range = expanded( range );
	const auto& constraints = axis.constraints();
	double low = axis.scale().forward( range.min );
	double high = axis.scale().forward( range.max );
	if ( !std::isfinite( low ) || !std::isfinite( high ) || low >= high ) {
		low = 0.0;
		high = 1.0;
	}
	const double absoluteLow = constraints.absoluteMinimum
								   ? axis.scale().forward( *constraints.absoluteMinimum )
								   : -std::numeric_limits<double>::infinity();
	const double absoluteHigh = constraints.absoluteMaximum
									? axis.scale().forward( *constraints.absoluteMaximum )
									: std::numeric_limits<double>::infinity();
	if ( absoluteLow >= absoluteHigh )
		return;
	const double available = absoluteHigh - absoluteLow;
	double span = high - low;
	if ( constraints.minimumViewSpan > 0 )
		span = std::max( span, std::min( constraints.minimumViewSpan, available ) );
	if ( constraints.maximumViewSpan > 0 )
		span = std::min( span, constraints.maximumViewSpan );
	span = std::min( span, available );
	const double center = low + ( high - low ) * 0.5;
	low = center - span * 0.5;
	high = low + span;
	if ( low < absoluteLow ) {
		low = absoluteLow;
		high = low + span;
	}
	if ( high > absoluteHigh ) {
		high = absoluteHigh;
		low = high - span;
	}
	if ( low >= high )
		return;
	mRange = { axis.scale().inverse( low ), axis.scale().inverse( high ) };
	mManual = manual;
}

double AxisViewport::toPixel( double value, float pixelStart, float pixelLength, bool inverted,
							  const ChartAxis& axis ) const {
	const double low = axis.scale().forward( mRange.min );
	const double high = axis.scale().forward( mRange.max );
	const double fraction = ( axis.scale().forward( value ) - low ) / ( high - low );
	return pixelStart + ( inverted ? 1.0 - fraction : fraction ) * pixelLength;
}

double AxisViewport::fromPixel( float pixel, float pixelStart, float pixelLength, bool inverted,
								const ChartAxis& axis ) const {
	if ( pixelLength <= 0 )
		return mRange.min;
	double fraction = ( pixel - pixelStart ) / pixelLength;
	if ( inverted )
		fraction = 1.0 - fraction;
	const double low = axis.scale().forward( mRange.min );
	const double high = axis.scale().forward( mRange.max );
	return axis.scale().inverse( low + fraction * ( high - low ) );
}

SmallVector<double, 16> linearTicks( DataRange range, float pixelLength, float targetSpacing ) {
	SmallVector<double, 16> ticks;
	if ( pixelLength <= 0 || targetSpacing <= 0 || !std::isfinite( range.min ) ||
		 !std::isfinite( range.max ) || range.min >= range.max )
		return ticks;
	const double targetCount = std::max( 1.0, static_cast<double>( pixelLength / targetSpacing ) );
	const double step = niceStep( ( range.max - range.min ) / targetCount );
	if ( !std::isfinite( step ) || step <= 0 )
		return ticks;
	const double first = std::ceil( range.min / step );
	for ( size_t i = 0; i < 1024; ++i ) {
		const double value = ( first + static_cast<double>( i ) ) * step;
		if ( !std::isfinite( value ) || value > range.max )
			break;
		ticks.emplace_back( value == 0.0 ? 0.0 : value );
	}
	return ticks;
}

String formatNumericTick( double value, double step ) {
	char buffer[64];
	const double magnitude = std::max( std::abs( value ), std::abs( step ) );
	if ( magnitude >= 1e7 || ( magnitude > 0 && magnitude < 1e-4 ) )
		std::snprintf( buffer, sizeof( buffer ), "%.4g", value );
	else {
		const int decimals =
			step > 0 && step < 1
				? std::clamp( static_cast<int>( std::ceil( -std::log10( step ) ) ) + 1, 0, 12 )
				: 0;
		std::snprintf( buffer, sizeof( buffer ), "%.*f", decimals, value );
	}
	return buffer;
}

}}} // namespace EE::UI::Charts
