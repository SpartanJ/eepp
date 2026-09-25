#include <algorithm>
#include <cmath>
#include <eepp/graphics/globalbatchrenderer.hpp>
#include <eepp/graphics/pixeldensity.hpp>
#include <eepp/graphics/primitives.hpp>
#include <eepp/graphics/renderer/renderer.hpp>
#include <eepp/graphics/text.hpp>
#include <eepp/ui/charts/uichart.hpp>
#include <eepp/ui/uieventdispatcher.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uistyle.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <eepp/ui/uitooltip.hpp>
#include <limits>

using namespace EE::Graphics;

namespace EE { namespace UI { namespace Charts {

namespace {

Uint64 dataRevision( const ChartModel& model ) {
	Uint64 revision = model.revision();
	for ( const auto& series : model.series() ) {
		if ( series->dataSource() ) {
			const auto read = series->dataSource()->acquireRead();
			revision ^=
				read.generation() + 0x9e3779b97f4a7c15ULL + ( revision << 6 ) + ( revision >> 2 );
		}
	}
	return revision;
}

std::optional<DataRange> axisExtent( const ChartModel& model, const ChartAxis* axis ) {
	std::optional<DataRange> extent;
	for ( const auto& series : model.series() ) {
		if ( !series->visible() || !series->dataSource() )
			continue;
		const bool x = &series->xAxis() == axis;
		const bool y = &series->yAxis() == axis;
		if ( !x && !y )
			continue;
		const auto read = series->dataSource()->acquireRead();
		const auto next = x ? xExtent( read ) : yExtent( read );
		if ( next ) {
			if ( !extent )
				extent = next;
			else {
				extent->min = std::min( extent->min, next->min );
				extent->max = std::max( extent->max, next->max );
			}
		}
	}
	return extent;
}

Float chartTextWidth( Font* font, const String& label, Uint32 fontSize ) {
	return font ? Text::getTextWidth( font, fontSize, label, 0, 4, 0.f, label.getTextHints() )
				: 0.f;
}

bool validStrokePoint( const std::vector<Vector2f>& points, size_t index ) {
	return index < points.size() && std::isfinite( points[index].x ) &&
		   std::isfinite( points[index].y );
}

Float monotoneTangent( const std::vector<Vector2f>& points, size_t index ) {
	const Vector2f point = points[index];
	const bool before =
		index > 0 && validStrokePoint( points, index - 1 ) && points[index - 1].x < point.x;
	const bool after = validStrokePoint( points, index + 1 ) && points[index + 1].x > point.x;
	if ( !before && !after )
		return 0.f;
	const Float previous =
		before ? ( point.y - points[index - 1].y ) / ( point.x - points[index - 1].x ) : 0.f;
	const Float next =
		after ? ( points[index + 1].y - point.y ) / ( points[index + 1].x - point.x ) : 0.f;
	if ( !before )
		return next;
	if ( !after )
		return previous;
	if ( previous * next <= 0.f )
		return 0.f;
	const Float leftWidth = point.x - points[index - 1].x;
	const Float rightWidth = points[index + 1].x - point.x;
	const Float firstWeight = 2.f * rightWidth + leftWidth;
	const Float secondWeight = rightWidth + 2.f * leftWidth;
	return ( firstWeight + secondWeight ) / ( firstWeight / previous + secondWeight / next );
}

Float monotoneCubicY( Float start, Float end, Float startTangent, Float endTangent, Float t ) {
	const Float t2 = t * t;
	const Float t3 = t2 * t;
	return ( 2.f * t3 - 3.f * t2 + 1.f ) * start + ( t3 - 2.f * t2 + t ) * startTangent +
		   ( -2.f * t3 + 3.f * t2 ) * end + ( t3 - t2 ) * endTangent;
}

} // namespace

LineSeries::LineSeries( String name, ChartAxis* xAxis, ChartAxis* yAxis,
						std::function<void()> changed ) :
	mName( std::move( name ) ), mChanged( std::move( changed ) ), mXAxis( xAxis ), mYAxis( yAxis ) {
	setDataSource( std::make_shared<OwnedXYDataSource>() );
}

void LineSeries::setXAxis( ChartAxis& axis ) {
	if ( &axis != mXAxis ) {
		mXAxis = &axis;
		++mGeometryRevision;
		changed();
	}
}

void LineSeries::setYAxis( ChartAxis& axis ) {
	if ( &axis != mYAxis ) {
		mYAxis = &axis;
		++mGeometryRevision;
		changed();
	}
}

void LineSeries::setVisible( bool visible ) {
	if ( mVisible != visible ) {
		mVisible = visible;
		++mGeometryRevision;
		changed();
	}
}

void LineSeries::setColor( Color color ) {
	if ( mColor != color || !mColorExplicit ) {
		mColor = color;
		mColorExplicit = true;
		++mGeometryRevision;
		changed();
	}
}

void LineSeries::setWidth( Float width ) {
	width = std::max( 0.f, width );
	if ( mWidth != width ) {
		mWidth = width;
		++mGeometryRevision;
		changed();
	}
}

void LineSeries::setJoin( LineJoin join ) {
	if ( mJoin != join ) {
		mJoin = join;
		++mGeometryRevision;
		changed();
	}
}

void LineSeries::setCap( LineCap cap ) {
	if ( mCap != cap ) {
		mCap = cap;
		++mGeometryRevision;
		changed();
	}
}

void LineSeries::setInterpolation( LineInterpolation interpolation ) {
	if ( mInterpolation != interpolation ) {
		mInterpolation = interpolation;
		++mGeometryRevision;
		changed();
	}
}

void LineSeries::setMiterLimit( Float limit ) {
	limit = std::isfinite( limit ) ? std::max( 1.f, limit ) : 4.f;
	if ( mMiterLimit != limit ) {
		mMiterLimit = limit;
		++mGeometryRevision;
		changed();
	}
}

void LineSeries::setPoints( std::vector<ChartPoint> points ) {
	auto owned = std::dynamic_pointer_cast<OwnedXYDataSource>( mSource );
	if ( !owned ) {
		owned = std::make_shared<OwnedXYDataSource>();
		setDataSource( owned );
	}
	owned->setPoints( std::move( points ) );
	changed();
}

void LineSeries::setPoints( std::span<const double> xs, std::span<const double> ys ) {
	const size_t count = std::min( xs.size(), ys.size() );
	std::vector<ChartPoint> points;
	points.reserve( count );
	for ( size_t i = 0; i < count; ++i )
		points.emplace_back( ChartPoint{ xs[i], ys[i] } );
	setPoints( std::move( points ) );
}

void LineSeries::setValues( std::span<const double> ys, double start, double step ) {
	std::vector<ChartPoint> points;
	points.reserve( ys.size() );
	for ( size_t i = 0; i < ys.size(); ++i )
		points.emplace_back( ChartPoint{ start + step * static_cast<double>( i ), ys[i] } );
	setPoints( std::move( points ) );
}

void LineSeries::setDataSource( std::shared_ptr<XYDataSource> source ) {
	mSource = std::move( source );
	++mGeometryRevision;
	changed();
}

ChartModel::ChartModel() {
	mAxes.emplace_back(
		std::make_unique<ChartAxis>( AxisPosition::Bottom, [this] { changed(); } ) );
	mAxes.emplace_back( std::make_unique<ChartAxis>( AxisPosition::Left, [this] { changed(); } ) );
}

ChartAxis& ChartModel::addAxis( AxisPosition position ) {
	auto axis = std::make_unique<ChartAxis>( position, [this] { changed(); } );
	ChartAxis* result = axis.get();
	mAxes.emplace_back( std::move( axis ) );
	changed();
	return *result;
}

LineSeries& ChartModel::addLineSeries( String name ) {
	auto series = std::unique_ptr<LineSeries>(
		new LineSeries( std::move( name ), &xAxis(), &yAxis(), [this] { changed(); } ) );
	LineSeries* result = series.get();
	mSeries.emplace_back( std::move( series ) );
	changed();
	return *result;
}

UIChart* UIChart::New() {
	return eeNew( UIChart, () );
}

UIChart::UIChart() : UIWidget( "chart" ) {
	setModel( std::make_shared<ChartModel>() );
	subscribeScheduledUpdate();
	setSize( 400, 240 );
}

void UIChart::setModel( std::shared_ptr<ChartModel> model ) {
	mModelConnection.disconnect();
	mAxes.clear();
	mCaches.clear();
	mHover = false;
	mHoverSeries = nullptr;
	mModel = model ? std::move( model ) : std::make_shared<ChartModel>();
	mAutoRevision = std::numeric_limits<Uint64>::max();
	mModelConnection = mModel->mRevision.observe( [this]( const Uint64& ) { invalidateDraw(); } );
	++mViewRevision;
	++mLayoutRevision;
	invalidateDraw();
}

void UIChart::scheduledUpdate( const Time& time ) {
	UIWidget::scheduledUpdate( time );
	for ( const auto& cache : mCaches ) {
		if ( !cache.series->dataSource() )
			continue;
		const auto read = cache.series->dataSource()->acquireRead();
		if ( !cache.geometry || cache.generation != read.generation() ) {
			invalidateDraw();
			break;
		}
	}
}

LineSeries& UIChart::addLineSeries( String name ) {
	return mModel->addLineSeries( std::move( name ) );
}

AxisViewport& UIChart::viewport( const ChartAxis& axis ) {
	for ( auto& view : mAxes ) {
		if ( view.axis == &axis )
			return view.viewport;
	}
	mAxes.emplace_back( ViewAxis{ const_cast<ChartAxis*>( &axis ), {} } );
	return mAxes.back().viewport;
}

const AxisViewport& UIChart::viewport( const ChartAxis& axis ) const {
	for ( const auto& view : mAxes ) {
		if ( view.axis == &axis )
			return view.viewport;
	}
	return mAxes.front().viewport;
}

void UIChart::synchronize() {
	for ( const auto& axis : mModel->axes() )
		if ( std::none_of( mAxes.begin(), mAxes.end(),
						   [&]( const ViewAxis& view ) { return view.axis == axis.get(); } ) ) {
			viewport( *axis );
			++mLayoutRevision;
			++mViewRevision;
		}
	for ( auto& view : mAxes ) {
		if ( view.axisRevision != view.axis->revision() ) {
			view.axisRevision = view.axis->revision();
			++mLayoutRevision;
			++mViewRevision;
		}
	}
	for ( const auto& series : mModel->series() ) {
		const auto found =
			std::find_if( mCaches.begin(), mCaches.end(), [&]( const SeriesCache& cache ) {
				return cache.series == series.get();
			} );
		if ( found == mCaches.end() ) {
			mCaches.emplace_back();
			mCaches.back().series = series.get();
		}
	}
}

void UIChart::fit() {
	mFollowSpan = 0.0;
	synchronize();
	for ( auto& view : mAxes )
		view.viewport.fit( axisExtent( *mModel, view.axis ), *view.axis );
	++mViewRevision;
	++mLayoutRevision;
	invalidateDraw();
}

DataRange UIChart::axisRange( const ChartAxis& axis ) {
	synchronize();
	return viewport( axis ).range();
}

void UIChart::setAxisRange( ChartAxis& axis, DataRange range ) {
	synchronize();
	mFollowSpan = 0.0;
	viewport( axis ).setRange( range, axis );
	++mViewRevision;
	++mLayoutRevision;
	invalidateDraw();
}

void UIChart::setFollowX( double span ) {
	mFollowSpan = std::isfinite( span ) && span > 0 ? span : 0.0;
	mFollowRevision = std::numeric_limits<Uint64>::max();
	invalidateDraw();
}

void UIChart::setChartStyle( ChartStyle style ) {
	mChartStyle = std::move( style );
	++mLayoutRevision;
	++mViewRevision;
	invalidateDraw();
}

void UIChart::updateThemeColors() {
	mAxisColor = mChartStyle.axisColor ? *mChartStyle.axisColor
									   : themeColor( "--tab-line", { 125, 130, 138 } );
	mTickColor = mChartStyle.tickColor.value_or( mAxisColor );
	mTickLabelColor = mChartStyle.tickLabelColor ? *mChartStyle.tickLabelColor
												 : themeColor( "--font-hint", { 190, 194, 201 } );
	mAxisLabelColor = mChartStyle.axisLabelColor ? *mChartStyle.axisLabelColor
												 : themeColor( "--font", { 210, 214, 220 } );
	if ( mChartStyle.hoverColor ) {
		mHoverColor = *mChartStyle.hoverColor;
	} else {
		mHoverColor = themeColor( "--primary", { 160, 165, 173 } );
		mHoverColor.a = 140;
	}
	const Color series = mChartStyle.seriesColor ? *mChartStyle.seriesColor
												 : themeColor( "--primary", { 48, 130, 220 } );
	if ( mSeriesColor != series ) {
		mSeriesColor = series;
		++mViewRevision;
	}
}

void UIChart::updateFollow() {
	if ( mFollowSpan <= 0 )
		return;
	const Uint64 revision = dataRevision( *mModel );
	if ( mFollowRevision == revision )
		return;
	mFollowRevision = revision;
	std::optional<double> latestX;
	for ( const auto& series : mModel->series() ) {
		if ( !series->visible() || !series->dataSource() || &series->xAxis() != &xAxis() )
			continue;
		auto read = series->dataSource()->acquireRead();
		if ( read.size() == 0 )
			continue;
		const auto extent =
			read.order() == DataOrder::AscendingX ? std::optional<DataRange>{} : xExtent( read );
		const double candidate = read.order() == DataOrder::AscendingX
									 ? pointAt( read, read.size() - 1 ).x
								 : extent ? extent->max
										  : std::numeric_limits<double>::quiet_NaN();
		if ( std::isfinite( candidate ) )
			latestX = latestX ? std::max( *latestX, candidate ) : candidate;
	}
	if ( !latestX )
		return;
	viewport( xAxis() ).setRange( { *latestX - mFollowSpan, *latestX }, xAxis() );
	for ( auto& view : mAxes ) {
		const auto position = view.axis->position();
		if ( position != AxisPosition::Left && position != AxisPosition::Right )
			continue;
		std::optional<DataRange> extent;
		for ( const auto& series : mModel->series() ) {
			if ( !series->visible() || !series->dataSource() || &series->yAxis() != view.axis )
				continue;
			auto read = series->dataSource()->acquireRead();
			auto next = visibleYExtent( read, viewport( series->xAxis() ).range() );
			if ( next ) {
				if ( !extent )
					extent = next;
				else {
					extent->min = std::min( extent->min, next->min );
					extent->max = std::max( extent->max, next->max );
				}
			}
		}
		view.viewport.fit( extent, *view.axis );
	}
	++mViewRevision;
	++mLayoutRevision;
}

void UIChart::updateAutoRanges() {
	if ( mFollowSpan > 0 )
		return;
	const Uint64 revision = dataRevision( *mModel );
	if ( mAutoRevision == revision )
		return;
	mAutoRevision = revision;
	bool changed = false;
	for ( auto& view : mAxes ) {
		if ( view.viewport.manual() )
			continue;
		view.viewport.fit( axisExtent( *mModel, view.axis ), *view.axis );
		changed = true;
	}
	if ( changed ) {
		++mViewRevision;
		++mLayoutRevision;
	}
}

void UIChart::onSizeChange() {
	UIWidget::onSizeChange();
	++mLayoutRevision;
	++mViewRevision;
}

const ChartAxis& UIChart::gridAxis( const ChartGridStyle& grid, bool vertical ) const {
	const ChartAxis& primary = vertical ? xAxis() : yAxis();
	if ( !grid.axis )
		return primary;
	for ( const auto& axis : mModel->axes() ) {
		if ( axis.get() == grid.axis ) {
			const AxisPosition position = axis->position();
			return ( vertical ? position == AxisPosition::Bottom || position == AxisPosition::Top
							  : position == AxisPosition::Left || position == AxisPosition::Right )
					   ? *axis
					   : primary;
		}
	}
	return primary;
}

void UIChart::updateLayout() {
	if ( mLayout.revision == mLayoutRevision )
		return;
	const auto dp = []( Float value ) { return PixelDensity::dpToPx( value ); };
	const Uint32 fontSize =
		std::max( 1u, static_cast<Uint32>( std::round( dp( mChartStyle.fontSize ) ) ) );
	const Float width = getPixelsSize().getWidth();
	const Float height = getPixelsSize().getHeight();
	Float leftMargin = dp( mChartStyle.leftMargin );
	Float bottomMargin = dp( xAxis().label().empty() ? mChartStyle.bottomMargin
													 : mChartStyle.bottomMarginWithLabel );
	const Float topMargin =
		dp( yAxis().label().empty() ? mChartStyle.topMargin : mChartStyle.topMarginWithLabel );
	Font* font = mChartStyle.font	? mChartStyle.font
				 : getUISceneNode() ? getUISceneNode()->getUIThemeManager()->getDefaultFont()
									: nullptr;
	mLayout.xAxisLabelWidth = chartTextWidth( font, xAxis().label(), fontSize );
	const Float angle = std::isfinite( mChartStyle.xTickLabelAngle )
							? std::clamp( mChartStyle.xTickLabelAngle, -90.f, 90.f )
							: 0.f;
	const Float radians = angle * 0.017453292519943295f;
	const Float labelHeight =
		font ? font->getLineSpacing( fontSize ) : static_cast<Float>( fontSize );
	Float rightMargin = dp( mChartStyle.rightMargin );
	mLayout.rightAxes.clear();
	for ( const auto& view : mAxes ) {
		if ( view.axis->position() != AxisPosition::Right )
			continue;
		Layout::RightAxis axisLayout;
		axisLayout.axis = view.axis;
		axisLayout.ticks =
			linearTicks( view.viewport.range(), std::max( 1.f, height - topMargin - bottomMargin ),
						 dp( mChartStyle.yTickSpacing ) );
		const double step =
			axisLayout.ticks.size() > 1 ? axisLayout.ticks[1] - axisLayout.ticks[0] : 1.0;
		Float labelWidth = 0.f;
		for ( double tick : axisLayout.ticks ) {
			axisLayout.labels.emplace_back( axisLayout.axis->formatTick( tick, step ) );
			labelWidth =
				std::max( labelWidth, chartTextWidth( font, axisLayout.labels.back(), fontSize ) );
		}
		axisLayout.width = labelWidth > 0.f
							   ? std::max( dp( mChartStyle.minimumAxisMargin ),
										   labelWidth + dp( mChartStyle.rightAxisLabelPadding ) )
							   : 0.f;
		rightMargin += axisLayout.width;
		mLayout.rightAxes.emplace_back( std::move( axisLayout ) );
	}
	for ( int pass = 0; pass < 2; ++pass ) {
		const Float plotWidth = std::max( 1.f, width - leftMargin - rightMargin );
		const Float plotHeight = std::max( 1.f, height - topMargin - bottomMargin );
		mLayout.plot =
			Rectf( leftMargin, topMargin, leftMargin + plotWidth, topMargin + plotHeight );
		mLayout.xTicks =
			linearTicks( viewport( xAxis() ).range(), plotWidth, dp( mChartStyle.xTickSpacing ) );
		mLayout.yTicks =
			linearTicks( viewport( yAxis() ).range(), plotHeight, dp( mChartStyle.yTickSpacing ) );
		mLayout.xLabels.clear();
		mLayout.yLabels.clear();
		mLayout.xLabelWidths.clear();
		mLayout.yLabelWidths.clear();
		Float maximumLabelWidth = 0.f;
		const double xStep =
			mLayout.xTicks.size() > 1 ? mLayout.xTicks[1] - mLayout.xTicks[0] : 1.0;
		const double yStep =
			mLayout.yTicks.size() > 1 ? mLayout.yTicks[1] - mLayout.yTicks[0] : 1.0;
		for ( double tick : mLayout.xTicks ) {
			mLayout.xLabels.emplace_back( xAxis().formatTick( tick, xStep ) );
			mLayout.xLabelWidths.emplace_back(
				chartTextWidth( font, mLayout.xLabels.back(), fontSize ) );
		}
		Float rotatedHeight = 0.f;
		for ( Float labelWidth : mLayout.xLabelWidths ) {
			if ( labelWidth > 0.f ) {
				rotatedHeight =
					std::max( rotatedHeight, std::abs( std::sin( radians ) ) * labelWidth +
												 std::abs( std::cos( radians ) ) * labelHeight );
			}
		}
		mLayout.xTickLabelHeight = rotatedHeight;
		const Float requiredBottom =
			rotatedHeight + dp( mChartStyle.tickLabelGap ) +
			( xAxis().label().empty() ? dp( 5.f ) : dp( 8.f ) + labelHeight );
		if ( pass == 0 && rotatedHeight > 0.f ) {
			bottomMargin = std::max( bottomMargin, requiredBottom );
		}
		for ( double tick : mLayout.yTicks ) {
			mLayout.yLabels.emplace_back( yAxis().formatTick( tick, yStep ) );
			const Float labelWidth = chartTextWidth( font, mLayout.yLabels.back(), fontSize );
			mLayout.yLabelWidths.emplace_back( labelWidth );
			maximumLabelWidth = std::max( maximumLabelWidth, labelWidth );
		}
		const Float measuredMargin =
			maximumLabelWidth > 0.f
				? std::max( dp( mChartStyle.minimumAxisMargin ),
							maximumLabelWidth + dp( mChartStyle.axisLabelPadding ) )
				: dp( mChartStyle.leftMargin );
		if ( pass == 0 ) {
			leftMargin = measuredMargin;
		} else {
			break;
		}
	}
	const auto updateGridTicks = [this]( const ChartGridStyle& grid, bool vertical,
										 SmallVector<double, 16>& ticks ) {
		ticks.clear();
		if ( grid.mode != ChartGridMode::AxisTicks || !std::isfinite( grid.spacing ) ||
			 grid.spacing <= 0 )
			return;
		const Float length = vertical ? mLayout.plot.getWidth() : mLayout.plot.getHeight();
		const Float spacing = PixelDensity::dpToPx( static_cast<Float>( grid.spacing ) );
		if ( !std::isfinite( spacing ) || spacing <= 0 )
			return;
		ticks = linearTicks( viewport( gridAxis( grid, vertical ) ).range(), length,
							 std::max( spacing, length / 256.f ) );
	};
	updateGridTicks( mChartStyle.verticalGrid, true, mLayout.verticalGridTicks );
	updateGridTicks( mChartStyle.horizontalGrid, false, mLayout.horizontalGridTicks );
	mLayout.revision = mLayoutRevision;
}

void UIChart::updateGeometry( SeriesCache& cache, const XYDataRead& read ) {
	cache.reduced.clear();
	if ( cache.geometry )
		cache.geometry->clear();
	else
		cache.geometry = VertexBuffer::New( VERTEX_FLAGS_PRIMITIVE, PRIMITIVE_TRIANGLES );
	if ( !cache.series->visible() || read.size() < 2 || cache.series->width() <= 0 )
		return;
	const auto& xView = viewport( cache.series->xAxis() );
	const auto& yView = viewport( cache.series->yAxis() );
	IndexRange visible{ 0, read.size() };
	if ( read.order() == DataOrder::AscendingX ) {
		visible = visibleLogicalRange( read, xView.range().min, xView.range().max );
		visible.begin = visible.begin ? visible.begin - 1 : 0;
		visible.end = std::min( read.size(), visible.end + 1 );
		reducePixelBuckets( read, visible, xView.range(),
							static_cast<size_t>( mLayout.plot.getWidth() ), cache.reduced );
	} else {
		cache.reduced.reserve( read.size() );
		for ( const auto& chunk : read.chunks() ) {
			std::visit(
				[&]( const auto& xs, const auto& ys ) {
					for ( size_t i = 0; i < xs.size(); ++i ) {
						const double x = xs[i];
						const double y = ys[i];
						cache.reduced.emplace_back(
							ReducedPoint{ x, y, chunk.logicalBegin + i,
										  !std::isfinite( x ) || !std::isfinite( y ) } );
					}
				},
				chunk.xs, chunk.ys );
		}
	}
	const Float halfWidth = PixelDensity::dpToPx( cache.series->width() ) * 0.5f;
	bool previousValid = false;
	bool priorSegment = false;
	Vector2f previous;
	Vector2f priorDirection;
	Vector2f priorNormal;
	const Color seriesColor = cache.series->hasCustomColor() ? cache.series->color() : mSeriesColor;
	auto addTriangle = [&]( Vector2f a, Vector2f b, Vector2f c ) {
		for ( const auto& vertex : { a, b, c } ) {
			cache.geometry->addVertex( vertex );
			cache.geometry->addColor( seriesColor );
		}
	};
	auto addSquareCap = [&]( Vector2f center, Vector2f direction, Vector2f normal, bool start ) {
		if ( cache.series->cap() != LineCap::Square )
			return;
		const Vector2f outer = center + direction * ( start ? -halfWidth : halfWidth );
		addTriangle( outer + normal, outer - normal, center + normal );
		addTriangle( center + normal, outer - normal, center - normal );
	};
	cache.strokePoints.clear();
	cache.strokePoints.reserve( cache.reduced.size() );
	for ( const auto& point : cache.reduced ) {
		if ( point.gap ) {
			cache.strokePoints.emplace_back( std::numeric_limits<Float>::quiet_NaN(), 0.f );
			continue;
		}
		cache.strokePoints.emplace_back(
			static_cast<Float>( xView.toPixel( point.x, mLayout.plot.Left, mLayout.plot.getWidth(),
											   false, cache.series->xAxis() ) ),
			static_cast<Float>( yView.toPixel( point.y, mLayout.plot.Top, mLayout.plot.getHeight(),
											   true, cache.series->yAxis() ) ) );
	}
	auto emitPoint = [&]( Vector2f current ) {
		if ( !std::isfinite( current.x ) || !std::isfinite( current.y ) ) {
			if ( priorSegment )
				addSquareCap( previous, priorDirection, priorNormal, false );
			previousValid = false;
			priorSegment = false;
			return;
		}
		if ( previousValid ) {
			const Float dx = current.x - previous.x;
			const Float dy = current.y - previous.y;
			const Float length = std::hypot( dx, dy );
			if ( length > 0.001f ) {
				const Vector2f direction( dx / length, dy / length );
				const Vector2f normal( -dy / length * halfWidth, dx / length * halfWidth );
				if ( !priorSegment )
					addSquareCap( previous, direction, normal, true );
				addTriangle( previous + normal, previous - normal, current + normal );
				addTriangle( current + normal, previous - normal, current - normal );
				if ( priorSegment ) {
					const Float cross =
						priorDirection.x * direction.y - priorDirection.y * direction.x;
					if ( std::abs( cross ) > 0.0001f ) {
						const Float side = cross > 0 ? -1.f : 1.f;
						const Vector2f outerA = priorNormal * side;
						const Vector2f outerB = normal * side;
						// The two segment quads leave this wedge empty at every turn.
						addTriangle( previous + outerA, previous, previous + outerB );
						const Vector2f sum = outerA + outerB;
						const Float sumLength = std::hypot( sum.x, sum.y );
						const Float divisor = sumLength > 0
												  ? ( sum.x * outerB.x + sum.y * outerB.y ) /
														( sumLength * halfWidth )
												  : 0.f;
						if ( cache.series->join() == LineJoin::Miter &&
							 divisor > 1.f / cache.series->miterLimit() ) {
							const Vector2f miter =
								sum * ( 1.f / sumLength ) * ( halfWidth / divisor );
							addTriangle( previous + outerA, previous + miter, previous + outerB );
						}
					}
				}
				priorDirection = direction;
				priorNormal = normal;
				priorSegment = true;
			}
		}
		previous = current;
		previousValid = true;
	};
	for ( size_t i = 0; i < cache.strokePoints.size(); ++i ) {
		const Vector2f current = cache.strokePoints[i];
		if ( cache.series->interpolation() != LineInterpolation::MonotoneCubic || i == 0 ||
			 !validStrokePoint( cache.strokePoints, i ) ||
			 !validStrokePoint( cache.strokePoints, i - 1 ) ||
			 current.x <= cache.strokePoints[i - 1].x ) {
			emitPoint( current );
			continue;
		}
		const Vector2f start = cache.strokePoints[i - 1];
		const Float dx = current.x - start.x;
		const Float dy = current.y - start.y;
		const Float startTangent = monotoneTangent( cache.strokePoints, i - 1 ) * dx;
		const Float endTangent = monotoneTangent( cache.strokePoints, i ) * dx;
		const int steps = std::clamp(
			static_cast<int>( std::ceil( std::max( dx, std::abs( dy ) ) / 4.f ) ), 1, 96 );
		for ( int step = 1; step <= steps; ++step ) {
			const Float t = static_cast<Float>( step ) / static_cast<Float>( steps );
			const Float y = monotoneCubicY( start.y, current.y, startTangent, endTangent, t );
			emitPoint( { start.x + dx * t, y } );
		}
	}
	if ( priorSegment )
		addSquareCap( previous, priorDirection, priorNormal, false );
}

void UIChart::drawAxes() {
	const auto dp = []( Float value ) { return PixelDensity::dpToPx( value ); };
	const Float tickLength = dp( mChartStyle.tickLength );
	const Float tickLabelGap = dp( mChartStyle.tickLabelGap );
	Primitives primitive;
	primitive.setLineWidth( dp( mChartStyle.axisWidth ) );
	primitive.setColor( mAxisColor );
	const auto& plot = mLayout.plot;
	const Vector2f origin( mScreenPos.x, mScreenPos.y );
	primitive.drawLine( Line2f( { origin.x + plot.Left, origin.y + plot.Bottom },
								{ origin.x + plot.Right, origin.y + plot.Bottom } ) );
	primitive.drawLine( Line2f( { origin.x + plot.Left, origin.y + plot.Top },
								{ origin.x + plot.Left, origin.y + plot.Bottom } ) );
	Font* font = mChartStyle.font	? mChartStyle.font
				 : getUISceneNode() ? getUISceneNode()->getUIThemeManager()->getDefaultFont()
									: nullptr;
	FontStyleConfig textStyle;
	textStyle.Font = font;
	textStyle.CharacterSize =
		std::max( 1u, static_cast<Uint32>( std::round( dp( mChartStyle.fontSize ) ) ) );
	textStyle.FontColor = mTickLabelColor;
	auto drawText = [&]( const String& label, Vector2f position ) {
		Text::draw( label, { std::round( position.x ), std::round( position.y ) }, textStyle, 4,
					label.getTextHints() );
	};
	const Float angle = std::isfinite( mChartStyle.xTickLabelAngle )
							? std::clamp( mChartStyle.xTickLabelAngle, -90.f, 90.f )
							: 0.f;
	const Float radians = angle * 0.017453292519943295f;
	const Float sine = std::sin( radians );
	const Float cosine = std::cos( radians );
	const Float labelHeight = font ? font->getLineSpacing( textStyle.CharacterSize ) : 0.f;
	const Float anchor = mChartStyle.xTickLabelAnchor == ChartTickLabelAnchor::Start ? 0.f
						 : mChartStyle.xTickLabelAnchor == ChartTickLabelAnchor::End ? 1.f
																					 : 0.5f;
	primitive.setColor( mTickColor );
	for ( size_t i = 0; i < mLayout.xTicks.size(); ++i ) {
		const Float x = static_cast<Float>( viewport( xAxis() ).toPixel(
			mLayout.xTicks[i], plot.Left, plot.getWidth(), false, xAxis() ) );
		primitive.drawLine( Line2f( { origin.x + x, origin.y + plot.Bottom },
									{ origin.x + x, origin.y + plot.Bottom + tickLength } ) );
		if ( font ) {
			const Float textWidth = mLayout.xLabelWidths[i];
			const String& label = mLayout.xLabels[i];
			if ( angle == 0.f ) {
				drawText( label, { origin.x + x - textWidth * anchor,
								   origin.y + plot.Bottom + tickLabelGap } );
			} else if ( !label.empty() ) {
				const Float firstX = -textWidth * anchor;
				const Float lastX = firstX + textWidth;
				const Float minimumY =
					std::min( sine * firstX, sine * lastX ) + std::min( 0.f, cosine * labelHeight );
				const Float pivotX = std::round( origin.x + x );
				const Float pivotY = std::round( origin.y + plot.Bottom + tickLabelGap - minimumY );
				GlobalBatchRenderer::instance()->draw();
				GLi->pushMatrix();
				GLi->translatef( pivotX, pivotY, 0.f );
				GLi->rotatef( angle, 0.f, 0.f, 1.f );
				GLi->translatef( -pivotX, -pivotY, 0.f );
				Text::draw( label, { std::round( pivotX + firstX ), pivotY }, textStyle, 4,
							label.getTextHints() );
				GlobalBatchRenderer::instance()->draw();
				GLi->popMatrix();
			}
		}
	}
	for ( size_t i = 0; i < mLayout.yTicks.size(); ++i ) {
		const Float y = static_cast<Float>( viewport( yAxis() ).toPixel(
			mLayout.yTicks[i], plot.Top, plot.getHeight(), true, yAxis() ) );
		primitive.drawLine( Line2f( { origin.x + plot.Left - tickLength, origin.y + y },
									{ origin.x + plot.Left, origin.y + y } ) );
		if ( font ) {
			const Float textWidth = mLayout.yLabelWidths[i];
			const String& label = mLayout.yLabels[i];
			drawText( label, { origin.x + plot.Left - tickLength - tickLabelGap - textWidth,
							   origin.y + y - textStyle.CharacterSize * 0.6f } );
		}
	}
	Float rightOffset = 0.f;
	for ( const auto& axisLayout : mLayout.rightAxes ) {
		const Float axisX = plot.Right + rightOffset;
		primitive.setColor( mAxisColor );
		primitive.drawLine( Line2f( { origin.x + axisX, origin.y + plot.Top },
									{ origin.x + axisX, origin.y + plot.Bottom } ) );
		primitive.setColor( mTickColor );
		for ( size_t i = 0; i < axisLayout.ticks.size(); ++i ) {
			const Float y =
				static_cast<Float>( viewport( *axisLayout.axis )
										.toPixel( axisLayout.ticks[i], plot.Top, plot.getHeight(),
												  true, *axisLayout.axis ) );
			primitive.drawLine( Line2f( { origin.x + axisX, origin.y + y },
										{ origin.x + axisX + tickLength, origin.y + y } ) );
			if ( font ) {
				const String& label = axisLayout.labels[i];
				drawText( label, { origin.x + axisX + tickLength + tickLabelGap,
								   origin.y + y - textStyle.CharacterSize * 0.6f } );
			}
		}
		rightOffset += axisLayout.width;
	}
	if ( font && !xAxis().label().empty() ) {
		textStyle.FontColor = mAxisLabelColor;
		const String& label = xAxis().label();
		drawText( label,
				  { origin.x + plot.Left + ( plot.getWidth() - mLayout.xAxisLabelWidth ) * 0.5f,
					origin.y + plot.Bottom +
						std::max( dp( mChartStyle.axisLabelGap ),
								  tickLabelGap + mLayout.xTickLabelHeight + dp( 6.f ) ) } );
	}
	if ( font && !yAxis().label().empty() ) {
		textStyle.FontColor = mAxisLabelColor;
		const String& label = yAxis().label();
		drawText( label, { origin.x + plot.Left, origin.y + dp( mChartStyle.axisLabelInset ) } );
	}
}

void UIChart::drawGrid() {
	if ( mChartStyle.verticalGrid.mode == ChartGridMode::Disabled &&
		 mChartStyle.horizontalGrid.mode == ChartGridMode::Disabled )
		return;
	const auto& plot = mLayout.plot;
	const Vector2f origin( mScreenPos.x, mScreenPos.y );
	Primitives primitive;
	Color defaultColor = mAxisColor;
	defaultColor.a = std::min<Uint8>( defaultColor.a, 72 );
	const auto drawDirection = [&]( const ChartGridStyle& grid, bool vertical,
									const SmallVector<double, 16>& ticks ) {
		if ( grid.mode == ChartGridMode::Disabled || !std::isfinite( grid.width ) ||
			 grid.width <= 0 || !std::isfinite( grid.spacing ) || grid.spacing <= 0 ||
			 !std::isfinite( grid.offset ) )
			return;
		primitive.setColor( grid.color.value_or( defaultColor ) );
		primitive.setLineWidth( PixelDensity::dpToPx( grid.width ) );
		const Float start = vertical ? plot.Left : plot.Top;
		const Float end = vertical ? plot.Right : plot.Bottom;
		const Float length = end - start;
		Float previousPixel = std::numeric_limits<Float>::quiet_NaN();
		const auto drawAt = [&]( double position ) {
			if ( !std::isfinite( position ) || position <= start + 0.5 || position >= end - 0.5 )
				return;
			const Float pixel =
				std::round( static_cast<Float>( position ) + ( vertical ? origin.x : origin.y ) );
			if ( pixel == previousPixel )
				return;
			previousPixel = pixel;
			if ( vertical ) {
				primitive.drawLine(
					Line2f( { pixel, origin.y + plot.Top }, { pixel, origin.y + plot.Bottom } ) );
			} else {
				primitive.drawLine(
					Line2f( { origin.x + plot.Left, pixel }, { origin.x + plot.Right, pixel } ) );
			}
		};
		if ( grid.mode == ChartGridMode::ScreenInterval ) {
			const Float spacing = PixelDensity::dpToPx( static_cast<Float>( grid.spacing ) );
			const Float offset = PixelDensity::dpToPx( static_cast<Float>( grid.offset ) );
			if ( !std::isfinite( spacing ) || spacing <= 0 || !std::isfinite( offset ) )
				return;
			const Float first = std::fmod( std::fmod( offset, spacing ) + spacing, spacing );
			const double count = std::floor( ( length - first ) / spacing ) + 1.0;
			if ( !std::isfinite( count ) || count <= 0 )
				return;
			const double stride = std::max( 1.0, std::ceil( count / 256.0 ) );
			for ( size_t i = 0; i < 256; ++i ) {
				const double index = static_cast<double>( i ) * stride;
				if ( index >= count )
					break;
				drawAt( start + first + index * spacing );
			}
			return;
		}
		const ChartAxis& axis = gridAxis( grid, vertical );
		const AxisViewport& view = viewport( axis );
		const auto drawValue = [&]( double value ) {
			drawAt( view.toPixel( value, start, length, !vertical, axis ) );
		};
		if ( grid.mode == ChartGridMode::AxisTicks ) {
			for ( double value : ticks )
				drawValue( value );
			return;
		}
		if ( grid.mode != ChartGridMode::DataInterval )
			return;
		const DataRange range = view.range();
		const double first = std::ceil( ( range.min - grid.offset ) / grid.spacing );
		const double last = std::floor( ( range.max - grid.offset ) / grid.spacing );
		const double count = last - first + 1.0;
		if ( !std::isfinite( count ) || count <= 0 )
			return;
		const double stride = std::max( 1.0, std::ceil( count / 256.0 ) );
		for ( size_t i = 0; i < 256; ++i ) {
			const double index = first + static_cast<double>( i ) * stride;
			if ( index > last || !std::isfinite( index ) )
				break;
			drawValue( grid.offset + index * grid.spacing );
		}
	};
	drawDirection( mChartStyle.verticalGrid, true, mLayout.verticalGridTicks );
	drawDirection( mChartStyle.horizontalGrid, false, mLayout.horizontalGridTicks );
}

void UIChart::draw() {
	UIWidget::draw();
	if ( !mVisible || getPixelsSize().getWidth() < PixelDensity::dpToPx( 20.f ) ||
		 getPixelsSize().getHeight() < PixelDensity::dpToPx( 20.f ) )
		return;
	const Float density = PixelDensity::getPixelDensity();
	if ( mLastDensity != density ) {
		mLastDensity = density;
		++mLayoutRevision;
		++mViewRevision;
	}
	updateThemeColors();
	synchronize();
	updateFollow();
	updateAutoRanges();
	Font* font = mChartStyle.font	? mChartStyle.font
				 : getUISceneNode() ? getUISceneNode()->getUIThemeManager()->getDefaultFont()
									: nullptr;
	if ( mLastXAxisLabel != xAxis().label() || mLastYAxisLabel != yAxis().label() ||
		 mLastFont != font ) {
		mLastXAxisLabel = xAxis().label();
		mLastYAxisLabel = yAxis().label();
		mLastFont = font;
		++mLayoutRevision;
		++mViewRevision;
	}
	const bool layoutChanged = mLayout.revision != mLayoutRevision;
	updateLayout();
	drawAxes();
	const auto& plot = mLayout.plot;
	clipSmartEnable( static_cast<Int32>( mScreenPos.x + plot.Left ),
					 static_cast<Int32>( mScreenPos.y + plot.Top ),
					 static_cast<Uint32>( plot.getWidth() ),
					 static_cast<Uint32>( plot.getHeight() ) );
	drawGrid();
	bool geometryChanged = false;
	for ( auto& cache : mCaches ) {
		if ( !cache.series->visible() || !cache.series->dataSource() )
			continue;
		bool rebuilt = false;
		{
			auto read = cache.series->dataSource()->acquireRead();
			const auto& xView = viewport( cache.series->xAxis() );
			const auto& yView = viewport( cache.series->yAxis() );
			const auto& delta = read.delta();
			if ( cache.geometry && cache.generation != read.generation() &&
				 cache.viewRevision == mViewRevision &&
				 cache.seriesRevision == cache.series->geometryRevision() && xView.manual() &&
				 yView.manual() && read.order() == DataOrder::AscendingX && delta &&
				 !delta->reset && delta->fromGeneration == cache.generation &&
				 delta->removedFront == 0 && delta->modified.empty() &&
				 delta->appended.begin < delta->appended.end && !cache.reduced.empty() &&
				 cache.reduced.back().x > xView.range().max &&
				 pointAt( read, delta->appended.begin ).x > xView.range().max )
				cache.generation = read.generation();
			if ( !cache.geometry || cache.generation != read.generation() ||
				 cache.viewRevision != mViewRevision ||
				 cache.seriesRevision != cache.series->geometryRevision() ) {
				updateGeometry( cache, read );
				cache.generation = read.generation();
				cache.viewRevision = mViewRevision;
				cache.seriesRevision = cache.series->geometryRevision();
				rebuilt = true;
				geometryChanged = true;
			}
		}
		if ( rebuilt && cache.geometry && cache.geometry->getVertexCount() )
			cache.geometry->compile();
		if ( cache.geometry && cache.geometry->getVertexCount() ) {
			cache.geometry->bind();
			GLi->translatef( mScreenPos.x, mScreenPos.y, 0 );
			cache.geometry->draw();
			GLi->translatef( -mScreenPos.x, -mScreenPos.y, 0 );
			cache.geometry->unbind();
		}
	}
	if ( mHover && !mPanning &&
		 ( geometryChanged || layoutChanged || mHoverRevision != mModel->revision() ) ) {
		// Geometry and view ranges now match the new data, so a stationary mouse can hit a new
		// sample.
		updateHover( mLastMouse, false );
	}
	if ( mHover ) {
		Primitives primitive;
		primitive.setColor( mHoverColor );
		primitive.setLineWidth( PixelDensity::dpToPx( mChartStyle.axisWidth ) );
		primitive.drawLine( Line2f( { mLastMouse.x, mScreenPos.y + plot.Top },
									{ mLastMouse.x, mScreenPos.y + plot.Bottom } ) );
	}
	clipSmartDisable();
}

void UIChart::updateHover( Vector2f screenPosition, bool invalidate ) {
	mLastMouse = screenPosition;
	const Vector2f local = screenPosition - mScreenPos;
	if ( !mLayout.plot.contains( local ) ) {
		mHover = false;
		hideHoverTooltip();
		if ( invalidate )
			invalidateDraw();
		return;
	}
	mHover = true;
	Float bestDistance = PixelDensity::dpToPx( mChartStyle.hoverDistance );
	LineSeries* bestSeries = nullptr;
	ChartPoint bestPoint;
	size_t bestIndex = 0;
	Uint64 bestGeneration = 0;
	for ( const auto& cache : mCaches ) {
		auto* series = cache.series;
		if ( !series->visible() || !series->dataSource() )
			continue;
		auto read = series->dataSource()->acquireRead();
		if ( cache.generation == read.generation() && cache.viewRevision == mViewRevision &&
			 cache.seriesRevision == series->geometryRevision() ) {
			const ReducedPoint* previous = nullptr;
			Vector2f previousPixel;
			for ( size_t i = 0; i < cache.reduced.size(); ++i ) {
				const auto& point = cache.reduced[i];
				if ( point.gap || !validStrokePoint( cache.strokePoints, i ) ) {
					previous = nullptr;
					continue;
				}
				const Vector2f pixel = cache.strokePoints[i];
				if ( previous ) {
					if ( std::max( previousPixel.x, pixel.x ) < local.x - bestDistance ||
						 std::min( previousPixel.x, pixel.x ) > local.x + bestDistance ) {
						previous = &point;
						previousPixel = pixel;
						continue;
					}
					Float t;
					Float distance;
					if ( series->interpolation() == LineInterpolation::MonotoneCubic &&
						 pixel.x > previousPixel.x ) {
						const Float dx = pixel.x - previousPixel.x;
						t = std::clamp( ( local.x - previousPixel.x ) / dx, 0.f, 1.f );
						const Float y =
							monotoneCubicY( previousPixel.y, pixel.y,
											monotoneTangent( cache.strokePoints, i - 1 ) * dx,
											monotoneTangent( cache.strokePoints, i ) * dx, t );
						distance = std::hypot( previousPixel.x + t * dx - local.x, y - local.y );
					} else {
						const Vector2f segment = pixel - previousPixel;
						const Float lengthSquared = segment.x * segment.x + segment.y * segment.y;
						t = lengthSquared > 0.0001f
								? std::clamp( ( ( local.x - previousPixel.x ) * segment.x +
												( local.y - previousPixel.y ) * segment.y ) /
												  lengthSquared,
											  0.f, 1.f )
								: 0.f;
						distance = std::hypot( previousPixel.x + t * segment.x - local.x,
											   previousPixel.y + t * segment.y - local.y );
					}
					if ( distance < bestDistance ) {
						const size_t candidate =
							t < 0.5f ? previous->sourceIndex : point.sourceIndex;
						if ( candidate < read.size() ) {
							bestDistance = distance;
							bestSeries = series;
							bestPoint = pointAt( read, candidate );
							bestIndex = candidate;
							bestGeneration = read.generation();
						}
					}
				}
				previous = &point;
				previousPixel = pixel;
			}
		}
		if ( read.order() != DataOrder::AscendingX || read.size() == 0 )
			continue;
		const double x = viewport( series->xAxis() )
							 .fromPixel( local.x, mLayout.plot.Left, mLayout.plot.getWidth(), false,
										 series->xAxis() );
		const size_t index = lowerBoundX( read, x );
		for ( size_t candidate : { index ? index - 1 : 0, index } ) {
			if ( candidate >= read.size() )
				continue;
			const ChartPoint point = pointAt( read, candidate );
			if ( !std::isfinite( point.x ) || !std::isfinite( point.y ) )
				continue;
			const Float px = static_cast<Float>( viewport( series->xAxis() )
													 .toPixel( point.x, mLayout.plot.Left,
															   mLayout.plot.getWidth(), false,
															   series->xAxis() ) );
			const Float py = static_cast<Float>( viewport( series->yAxis() )
													 .toPixel( point.y, mLayout.plot.Top,
															   mLayout.plot.getHeight(), true,
															   series->yAxis() ) );
			const Float distance = std::hypot( px - local.x, py - local.y );
			if ( distance < bestDistance ) {
				bestDistance = distance;
				bestSeries = series;
				bestPoint = point;
				bestIndex = candidate;
				bestGeneration = read.generation();
			}
		}
	}
	if ( bestSeries && ( bestIndex != mHoverIndex || bestGeneration != mHoverGeneration ||
						 mHoverSeries != bestSeries || mHoverRevision != mModel->revision() ) ) {
		String tooltip;
		bool hasCustomTooltip = false;
		if ( bestSeries->tooltipProvider() ) {
			auto provided =
				bestSeries->tooltipProvider()( { bestIndex, bestPoint.x, bestPoint.y } );
			if ( provided ) {
				hasCustomTooltip = true;
				std::visit(
					[&]( auto& payload ) {
						using Payload = std::decay_t<decltype( payload )>;
						if constexpr ( std::is_same_v<Payload, TooltipText> ) {
							tooltip = std::move( payload.text );
						} else {
							tooltip = std::move( payload.title );
							for ( const auto& field : payload.fields ) {
								tooltip += "\n";
								tooltip += field.label;
								tooltip += ": ";
								tooltip += field.value;
							}
							if ( !payload.description.empty() ) {
								tooltip += "\n";
								tooltip += payload.description;
							}
						}
					},
					*provided );
			}
		}
		if ( !hasCustomTooltip ) {
			tooltip = bestSeries->name();
			tooltip += "\nX: ";
			tooltip += bestSeries->xAxis().formatTick( bestPoint.x, 0 );
			tooltip += "\nY: ";
			tooltip += bestSeries->yAxis().formatTick( bestPoint.y, 0 );
		}
		setTooltipText( tooltip );
		mHoverIndex = bestIndex;
		mHoverGeneration = bestGeneration;
		mHoverRevision = mModel->revision();
		mHoverSeries = bestSeries;
	} else if ( !bestSeries ) {
		hideHoverTooltip();
		mHoverRevision = mModel->revision();
	}
	if ( bestSeries && isTooltipEnabled() && !mTooltipText.empty() ) {
		auto* tooltip = createTooltip();
		tooltip->setDontAutoHideOnMouseMove( true );
		tooltip->setPixelsPosition( getTooltipPosition() );
		if ( !tooltip->isVisible() )
			tooltip->show();
	}
	if ( invalidate )
		invalidateDraw();
}

void UIChart::hideHoverTooltip() {
	mHoverSeries = nullptr;
	if ( !mTooltipText.empty() )
		setTooltipText( "" );
	if ( getTooltip() )
		getTooltip()->setVisible( false );
}

Uint32 UIChart::onMouseDown( const Vector2i& position, const Uint32& flags ) {
	if ( flags & EE_BUTTON_LMASK ) {
		mPanning = true;
		mLastMouse = position.asFloat();
		hideHoverTooltip();
	}
	return UIWidget::onMouseDown( position, flags );
}

Uint32 UIChart::onMouseUp( const Vector2i& position, const Uint32& flags ) {
	if ( flags & EE_BUTTON_LMASK )
		mPanning = false;
	return UIWidget::onMouseUp( position, flags );
}

Uint32 UIChart::onMouseMove( const Vector2i& position, const Uint32& flags ) {
	const Vector2f mouse = position.asFloat();
	if ( mPanning && ( flags & EE_BUTTON_LMASK ) && mLayout.plot.getWidth() > 0 &&
		 mLayout.plot.getHeight() > 0 ) {
		mFollowSpan = 0.0;
		const Float dx = mouse.x - mLastMouse.x;
		const Float dy = mouse.y - mLastMouse.y;
		for ( auto& view : mAxes ) {
			const bool horizontal = view.axis->position() == AxisPosition::Bottom ||
									view.axis->position() == AxisPosition::Top;
			const DataRange range = view.viewport.range();
			const double span =
				view.axis->scale().forward( range.max ) - view.axis->scale().forward( range.min );
			view.viewport.pan( horizontal ? -dx / mLayout.plot.getWidth() * span
										  : dy / mLayout.plot.getHeight() * span,
							   *view.axis );
		}
		++mViewRevision;
		++mLayoutRevision;
		invalidateDraw();
	} else {
		updateHover( mouse );
	}
	mLastMouse = mouse;
	return UIWidget::onMouseMove( position, flags );
}

Uint32 UIChart::onMouseOver( const Vector2i& position, const Uint32& flags ) {
	const Uint32 result = UIWidget::onMouseOver( position, flags );
	updateHover( position.asFloat() );
	return result;
}

Uint32 UIChart::onMouseLeave( const Vector2i& position, const Uint32& flags ) {
	mHover = false;
	hideHoverTooltip();
	invalidateDraw();
	return UIWidget::onMouseLeave( position, flags );
}

Uint32 UIChart::onMouseWheel( const Vector2f& offset, bool flipped ) {
	if ( offset.y == 0 || !getEventDispatcher() )
		return 0;
	const Vector2f local = getEventDispatcher()->getMousePosf() - mScreenPos;
	if ( !mLayout.plot.contains( local ) )
		return 0;
	const double factor = std::pow( 1.15, flipped ? -offset.y : offset.y );
	mFollowSpan = 0.0;
	for ( auto& view : mAxes ) {
		const bool horizontal = view.axis->position() == AxisPosition::Bottom ||
								view.axis->position() == AxisPosition::Top;
		const double anchor =
			horizontal ? view.viewport.fromPixel( local.x, mLayout.plot.Left,
												  mLayout.plot.getWidth(), false, *view.axis )
					   : view.viewport.fromPixel( local.y, mLayout.plot.Top,
												  mLayout.plot.getHeight(), true, *view.axis );
		view.viewport.zoom( anchor, factor, *view.axis );
	}
	++mViewRevision;
	++mLayoutRevision;
	invalidateDraw();
	return 1;
}

}}} // namespace EE::UI::Charts
