#include <algorithm>
#include <cmath>
#include <eepp/graphics/primitives.hpp>
#include <eepp/graphics/renderer/renderer.hpp>
#include <eepp/graphics/text.hpp>
#include <eepp/ui/charts/uichart.hpp>
#include <eepp/ui/uieventdispatcher.hpp>
#include <eepp/ui/uiscenenode.hpp>
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
		const bool x = series->xAxis() == axis;
		const bool y = series->yAxis() == axis;
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

float chartTextWidth( Font* font, const String& label ) {
	return font ? Text::getTextWidth( font, 12, label, 0, 4, 0.f, label.getTextHints() ) : 0.f;
}

} // namespace

LineSeries::LineSeries( String name, ChartAxis* xAxis, ChartAxis* yAxis,
						std::function<void()> changed ) :
	mName( std::move( name ) ), mChanged( std::move( changed ) ), mXAxis( xAxis ), mYAxis( yAxis ) {
	setDataSource( std::make_shared<OwnedXYDataSource>() );
}

void LineSeries::setXAxis( ChartAxis* axis ) {
	if ( axis && axis != mXAxis ) {
		mXAxis = axis;
		++mGeometryRevision;
		changed();
	}
}

void LineSeries::setYAxis( ChartAxis* axis ) {
	if ( axis && axis != mYAxis ) {
		mYAxis = axis;
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
	if ( mColor != color ) {
		mColor = color;
		++mGeometryRevision;
		changed();
	}
}

void LineSeries::setWidth( float width ) {
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

void LineSeries::setMiterLimit( float limit ) {
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

ChartAxis* ChartModel::addAxis( AxisPosition position ) {
	auto axis = std::make_unique<ChartAxis>( position, [this] { changed(); } );
	ChartAxis* result = axis.get();
	mAxes.emplace_back( std::move( axis ) );
	changed();
	return result;
}

LineSeries* ChartModel::addLineSeries( String name ) {
	auto series = std::unique_ptr<LineSeries>(
		new LineSeries( std::move( name ), xAxis(), yAxis(), [this] { changed(); } ) );
	LineSeries* result = series.get();
	mSeries.emplace_back( std::move( series ) );
	changed();
	return result;
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

LineSeries* UIChart::addLineSeries( String name ) {
	return mModel->addLineSeries( std::move( name ) );
}

AxisViewport& UIChart::viewport( const ChartAxis* axis ) {
	for ( auto& view : mAxes ) {
		if ( view.axis == axis )
			return view.viewport;
	}
	mAxes.emplace_back( ViewAxis{ const_cast<ChartAxis*>( axis ), {} } );
	return mAxes.back().viewport;
}

const AxisViewport& UIChart::viewport( const ChartAxis* axis ) const {
	for ( const auto& view : mAxes ) {
		if ( view.axis == axis )
			return view.viewport;
	}
	return mAxes.front().viewport;
}

void UIChart::synchronize() {
	for ( const auto& axis : mModel->axes() )
		if ( std::none_of( mAxes.begin(), mAxes.end(),
						   [&]( const ViewAxis& view ) { return view.axis == axis.get(); } ) ) {
			viewport( axis.get() );
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

DataRange UIChart::axisRange( const ChartAxis* axis ) {
	synchronize();
	return viewport( axis ).range();
}

void UIChart::setAxisRange( ChartAxis* axis, DataRange range ) {
	if ( !axis )
		return;
	synchronize();
	mFollowSpan = 0.0;
	viewport( axis ).setRange( range, *axis );
	++mViewRevision;
	++mLayoutRevision;
	invalidateDraw();
}

void UIChart::setFollowX( double span ) {
	mFollowSpan = std::isfinite( span ) && span > 0 ? span : 0.0;
	mFollowRevision = std::numeric_limits<Uint64>::max();
	invalidateDraw();
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
		if ( !series->visible() || !series->dataSource() || series->xAxis() != xAxis() )
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
	viewport( xAxis() ).setRange( { *latestX - mFollowSpan, *latestX }, *xAxis() );
	for ( auto& view : mAxes ) {
		const auto position = view.axis->position();
		if ( position != AxisPosition::Left && position != AxisPosition::Right )
			continue;
		std::optional<DataRange> extent;
		for ( const auto& series : mModel->series() ) {
			if ( !series->visible() || !series->dataSource() || series->yAxis() != view.axis )
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

void UIChart::updateLayout() {
	if ( mLayout.revision == mLayoutRevision )
		return;
	const float width = getPixelsSize().getWidth();
	const float height = getPixelsSize().getHeight();
	float leftMargin = 50.f;
	const float bottomMargin = xAxis()->label().empty() ? 34.f : 52.f;
	const float topMargin = yAxis()->label().empty() ? 12.f : 26.f;
	Font* font =
		getUISceneNode() ? getUISceneNode()->getUIThemeManager()->getDefaultFont() : nullptr;
	mLayout.xAxisLabelWidth = chartTextWidth( font, xAxis()->label() );
	float rightMargin = 14.f;
	mLayout.rightAxes.clear();
	for ( const auto& view : mAxes ) {
		if ( view.axis->position() != AxisPosition::Right )
			continue;
		Layout::RightAxis axisLayout;
		axisLayout.axis = view.axis;
		axisLayout.ticks = linearTicks( view.viewport.range(),
										std::max( 1.f, height - topMargin - bottomMargin ), 50.f );
		const double step =
			axisLayout.ticks.size() > 1 ? axisLayout.ticks[1] - axisLayout.ticks[0] : 1.0;
		float labelWidth = 0.f;
		for ( double tick : axisLayout.ticks ) {
			axisLayout.labels.emplace_back( axisLayout.axis->formatTick( tick, step ) );
			labelWidth = std::max( labelWidth, chartTextWidth( font, axisLayout.labels.back() ) );
		}
		axisLayout.width = std::max( 42.f, labelWidth + 16.f );
		rightMargin += axisLayout.width;
		mLayout.rightAxes.emplace_back( std::move( axisLayout ) );
	}
	for ( int pass = 0; pass < 2; ++pass ) {
		const float plotWidth = std::max( 1.f, width - leftMargin - rightMargin );
		const float plotHeight = std::max( 1.f, height - topMargin - bottomMargin );
		mLayout.plot =
			Rectf( leftMargin, topMargin, leftMargin + plotWidth, topMargin + plotHeight );
		mLayout.xTicks = linearTicks( viewport( xAxis() ).range(), plotWidth );
		mLayout.yTicks = linearTicks( viewport( yAxis() ).range(), plotHeight, 50.f );
		mLayout.xLabels.clear();
		mLayout.yLabels.clear();
		mLayout.xLabelWidths.clear();
		mLayout.yLabelWidths.clear();
		float maximumLabelWidth = 0.f;
		const double xStep =
			mLayout.xTicks.size() > 1 ? mLayout.xTicks[1] - mLayout.xTicks[0] : 1.0;
		const double yStep =
			mLayout.yTicks.size() > 1 ? mLayout.yTicks[1] - mLayout.yTicks[0] : 1.0;
		for ( double tick : mLayout.xTicks ) {
			mLayout.xLabels.emplace_back( xAxis()->formatTick( tick, xStep ) );
			mLayout.xLabelWidths.emplace_back( chartTextWidth( font, mLayout.xLabels.back() ) );
		}
		for ( double tick : mLayout.yTicks ) {
			mLayout.yLabels.emplace_back( yAxis()->formatTick( tick, yStep ) );
			const float labelWidth = chartTextWidth( font, mLayout.yLabels.back() );
			mLayout.yLabelWidths.emplace_back( labelWidth );
			maximumLabelWidth = std::max( maximumLabelWidth, labelWidth );
		}
		const float measuredMargin = std::max( 42.f, maximumLabelWidth + 12.f );
		if ( pass == 0 && std::abs( measuredMargin - leftMargin ) > 2.f )
			leftMargin = measuredMargin;
		else
			break;
	}
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
	const float halfWidth = cache.series->width() * 0.5f;
	bool previousValid = false;
	bool priorSegment = false;
	Vector2f previous;
	Vector2f priorDirection;
	Vector2f priorNormal;
	auto addTriangle = [&]( Vector2f a, Vector2f b, Vector2f c ) {
		for ( const auto& vertex : { a, b, c } ) {
			cache.geometry->addVertex( vertex );
			cache.geometry->addColor( cache.series->color() );
		}
	};
	auto addSquareCap = [&]( Vector2f center, Vector2f direction, Vector2f normal, bool start ) {
		if ( cache.series->cap() != LineCap::Square )
			return;
		const Vector2f outer = center + direction * ( start ? -halfWidth : halfWidth );
		addTriangle( outer + normal, outer - normal, center + normal );
		addTriangle( center + normal, outer - normal, center - normal );
	};
	for ( const auto& point : cache.reduced ) {
		if ( point.gap ) {
			if ( priorSegment )
				addSquareCap( previous, priorDirection, priorNormal, false );
			previousValid = false;
			priorSegment = false;
			continue;
		}
		const Vector2f current(
			static_cast<float>( xView.toPixel( point.x, mLayout.plot.Left, mLayout.plot.getWidth(),
											   false, *cache.series->xAxis() ) ),
			static_cast<float>( yView.toPixel( point.y, mLayout.plot.Top, mLayout.plot.getHeight(),
											   true, *cache.series->yAxis() ) ) );
		if ( previousValid ) {
			const float dx = current.x - previous.x;
			const float dy = current.y - previous.y;
			const float length = std::hypot( dx, dy );
			if ( length > 0.001f ) {
				const Vector2f direction( dx / length, dy / length );
				const Vector2f normal( -dy / length * halfWidth, dx / length * halfWidth );
				if ( !priorSegment )
					addSquareCap( previous, direction, normal, true );
				addTriangle( previous + normal, previous - normal, current + normal );
				addTriangle( current + normal, previous - normal, current - normal );
				if ( priorSegment ) {
					const float cross =
						priorDirection.x * direction.y - priorDirection.y * direction.x;
					if ( std::abs( cross ) > 0.0001f ) {
						const float side = cross > 0 ? -1.f : 1.f;
						const Vector2f outerA = priorNormal * side;
						const Vector2f outerB = normal * side;
						const Vector2f sum = outerA + outerB;
						const float sumLength = std::hypot( sum.x, sum.y );
						const float divisor = sumLength > 0
												  ? ( sum.x * outerB.x + sum.y * outerB.y ) /
														( sumLength * halfWidth )
												  : 0.f;
						if ( cache.series->join() == LineJoin::Miter &&
							 divisor > 1.f / cache.series->miterLimit() ) {
							const Vector2f miter =
								sum * ( 1.f / sumLength ) * ( halfWidth / divisor );
							addTriangle( previous + outerA, previous + miter, previous + outerB );
						} else {
							addTriangle( previous + outerA, previous, previous + outerB );
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
	}
	if ( priorSegment )
		addSquareCap( previous, priorDirection, priorNormal, false );
}

void UIChart::drawAxes() {
	Primitives primitive;
	primitive.setColor( Color( 125, 130, 138 ) );
	const auto& plot = mLayout.plot;
	const Vector2f origin( mScreenPos.x, mScreenPos.y );
	primitive.drawLine( Line2f( { origin.x + plot.Left, origin.y + plot.Bottom },
								{ origin.x + plot.Right, origin.y + plot.Bottom } ) );
	primitive.drawLine( Line2f( { origin.x + plot.Left, origin.y + plot.Top },
								{ origin.x + plot.Left, origin.y + plot.Bottom } ) );
	Font* font =
		getUISceneNode() ? getUISceneNode()->getUIThemeManager()->getDefaultFont() : nullptr;
	FontStyleConfig textStyle;
	textStyle.Font = font;
	textStyle.CharacterSize = 12;
	textStyle.FontColor = Color( 190, 194, 201 );
	for ( size_t i = 0; i < mLayout.xTicks.size(); ++i ) {
		const float x = static_cast<float>( viewport( xAxis() ).toPixel(
			mLayout.xTicks[i], plot.Left, plot.getWidth(), false, *xAxis() ) );
		primitive.drawLine( Line2f( { origin.x + x, origin.y + plot.Bottom },
									{ origin.x + x, origin.y + plot.Bottom + 4 } ) );
		if ( font ) {
			const float textWidth = mLayout.xLabelWidths[i];
			const String& label = mLayout.xLabels[i];
			Text::draw( label, { origin.x + x - textWidth * 0.5f, origin.y + plot.Bottom + 6 },
						textStyle, 4, label.getTextHints() );
		}
	}
	for ( size_t i = 0; i < mLayout.yTicks.size(); ++i ) {
		const float y = static_cast<float>( viewport( yAxis() ).toPixel(
			mLayout.yTicks[i], plot.Top, plot.getHeight(), true, *yAxis() ) );
		primitive.drawLine( Line2f( { origin.x + plot.Left - 4, origin.y + y },
									{ origin.x + plot.Left, origin.y + y } ) );
		if ( font ) {
			const float textWidth = mLayout.yLabelWidths[i];
			const String& label = mLayout.yLabels[i];
			Text::draw( label, { origin.x + plot.Left - 8 - textWidth, origin.y + y - 7 },
						textStyle, 4, label.getTextHints() );
		}
	}
	float rightOffset = 0.f;
	for ( const auto& axisLayout : mLayout.rightAxes ) {
		const float axisX = plot.Right + rightOffset;
		primitive.drawLine( Line2f( { origin.x + axisX, origin.y + plot.Top },
									{ origin.x + axisX, origin.y + plot.Bottom } ) );
		for ( size_t i = 0; i < axisLayout.ticks.size(); ++i ) {
			const float y =
				static_cast<float>( viewport( axisLayout.axis )
										.toPixel( axisLayout.ticks[i], plot.Top, plot.getHeight(),
												  true, *axisLayout.axis ) );
			primitive.drawLine( Line2f( { origin.x + axisX, origin.y + y },
										{ origin.x + axisX + 4, origin.y + y } ) );
			if ( font ) {
				const String& label = axisLayout.labels[i];
				Text::draw( label, { origin.x + axisX + 7, origin.y + y - 7 }, textStyle, 4,
							label.getTextHints() );
			}
		}
		rightOffset += axisLayout.width;
	}
	if ( font && !xAxis()->label().empty() ) {
		textStyle.FontColor = Color( 210, 214, 220 );
		const String& label = xAxis()->label();
		Text::draw( label,
					{ origin.x + plot.Left + ( plot.getWidth() - mLayout.xAxisLabelWidth ) * 0.5f,
					  origin.y + plot.Bottom + 25 },
					textStyle, 4, label.getTextHints() );
	}
	if ( font && !yAxis()->label().empty() ) {
		textStyle.FontColor = Color( 210, 214, 220 );
		const String& label = yAxis()->label();
		Text::draw( label, { origin.x + plot.Left, origin.y + 2 }, textStyle, 4,
					label.getTextHints() );
	}
}

void UIChart::draw() {
	UIWidget::draw();
	if ( !mVisible || getPixelsSize().getWidth() < 20 || getPixelsSize().getHeight() < 20 )
		return;
	synchronize();
	updateFollow();
	updateAutoRanges();
	Font* font =
		getUISceneNode() ? getUISceneNode()->getUIThemeManager()->getDefaultFont() : nullptr;
	if ( mLastXAxisLabel != xAxis()->label() || mLastYAxisLabel != yAxis()->label() ||
		 mLastFont != font ) {
		mLastXAxisLabel = xAxis()->label();
		mLastYAxisLabel = yAxis()->label();
		mLastFont = font;
		++mLayoutRevision;
		++mViewRevision;
	}
	updateLayout();
	drawAxes();
	const auto& plot = mLayout.plot;
	clipSmartEnable( static_cast<Int32>( mScreenPos.x + plot.Left ),
					 static_cast<Int32>( mScreenPos.y + plot.Top ),
					 static_cast<Uint32>( plot.getWidth() ),
					 static_cast<Uint32>( plot.getHeight() ) );
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
	if ( mHover ) {
		Primitives primitive;
		primitive.setColor( Color( 160, 165, 173, 140 ) );
		primitive.drawLine( Line2f( { mLastMouse.x, mScreenPos.y + plot.Top },
									{ mLastMouse.x, mScreenPos.y + plot.Bottom } ) );
	}
	clipSmartDisable();
}

void UIChart::updateHover( Vector2f screenPosition ) {
	mLastMouse = screenPosition;
	const Vector2f local = screenPosition - mScreenPos;
	if ( !mLayout.plot.contains( local ) ) {
		mHover = false;
		hideHoverTooltip();
		invalidateDraw();
		return;
	}
	mHover = true;
	float bestDistance = 10.f;
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
			const auto& xView = viewport( series->xAxis() );
			const auto& yView = viewport( series->yAxis() );
			const ReducedPoint* previous = nullptr;
			Vector2f previousPixel;
			for ( const auto& point : cache.reduced ) {
				if ( point.gap ) {
					previous = nullptr;
					continue;
				}
				const Vector2f pixel( static_cast<float>( xView.toPixel(
										  point.x, mLayout.plot.Left, mLayout.plot.getWidth(),
										  false, *series->xAxis() ) ),
									  static_cast<float>( yView.toPixel(
										  point.y, mLayout.plot.Top, mLayout.plot.getHeight(), true,
										  *series->yAxis() ) ) );
				if ( previous ) {
					const Vector2f segment = pixel - previousPixel;
					const float lengthSquared = segment.x * segment.x + segment.y * segment.y;
					const float t =
						lengthSquared > 0.0001f
							? std::clamp( ( ( local.x - previousPixel.x ) * segment.x +
											( local.y - previousPixel.y ) * segment.y ) /
											  lengthSquared,
										  0.f, 1.f )
							: 0.f;
					const float distance = std::hypot( previousPixel.x + t * segment.x - local.x,
													   previousPixel.y + t * segment.y - local.y );
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
										 *series->xAxis() );
		const size_t index = lowerBoundX( read, x );
		for ( size_t candidate : { index ? index - 1 : 0, index } ) {
			if ( candidate >= read.size() )
				continue;
			const ChartPoint point = pointAt( read, candidate );
			if ( !std::isfinite( point.x ) || !std::isfinite( point.y ) )
				continue;
			const float px = static_cast<float>( viewport( series->xAxis() )
													 .toPixel( point.x, mLayout.plot.Left,
															   mLayout.plot.getWidth(), false,
															   *series->xAxis() ) );
			const float py = static_cast<float>( viewport( series->yAxis() )
													 .toPixel( point.y, mLayout.plot.Top,
															   mLayout.plot.getHeight(), true,
															   *series->yAxis() ) );
			const float distance = std::hypot( px - local.x, py - local.y );
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
			tooltip += bestSeries->xAxis()->formatTick( bestPoint.x, 0 );
			tooltip += "\nY: ";
			tooltip += bestSeries->yAxis()->formatTick( bestPoint.y, 0 );
		}
		setTooltipText( tooltip );
		mHoverIndex = bestIndex;
		mHoverGeneration = bestGeneration;
		mHoverRevision = mModel->revision();
		mHoverSeries = bestSeries;
	} else if ( !bestSeries ) {
		hideHoverTooltip();
	}
	if ( bestSeries && isTooltipEnabled() ) {
		auto* tooltip = createTooltip();
		tooltip->setDontAutoHideOnMouseMove( true );
		tooltip->setPixelsPosition( getTooltipPosition() );
		if ( !tooltip->isVisible() )
			tooltip->show();
	}
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
		const float dx = mouse.x - mLastMouse.x;
		const float dy = mouse.y - mLastMouse.y;
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
