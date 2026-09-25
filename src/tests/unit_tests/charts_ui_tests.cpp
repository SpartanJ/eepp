#include "utest.h"
#include <eepp/graphics/globalbatchrenderer.hpp>
#include <eepp/graphics/image.hpp>
#include <eepp/graphics/pixeldensity.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/ui/charts/uichart.hpp>
#include <eepp/ui/uiapplication.hpp>
#include <eepp/ui/uiroot.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uistyle.hpp>
#include <eepp/ui/uitooltip.hpp>

using namespace EE;
using namespace EE::Window;
using namespace EE::UI;
using namespace EE::UI::Charts;
using namespace EE::UI::CSS;
using namespace EE::Graphics;
using namespace EE::System;

namespace {

class ChartProbe : public UIChart {
  public:
	ChartProbe() : UIChart() {}

	using UIChart::onMouseDown;
	using UIChart::onMouseLeave;
	using UIChart::onMouseMove;
	using UIChart::onMouseOver;
	using UIChart::onMouseUp;
};

} // namespace

UTEST( UIChart, buildsAndDrawsMultipleSeries ) {
	UIApplication app(
		WindowSettings{ 480, 320, "Chart Test" },
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1.f ) );
	auto* declarative = app.getUI()->loadLayoutFromString(
		"<Chart id=\"declarative_chart\" layout_width=\"20dp\" layout_height=\"20dp\" />" );
	ASSERT_TRUE( declarative != nullptr );
	EXPECT_TRUE( declarative->getElementTag() == "chart" );
	auto* chart = UIChart::New();
	chart->setPixelsSize( 440, 280 );
	chart->setParent( app.getUI()->getRoot() );
	auto& first = chart->addLineSeries( "First" );
	first.setPoints( std::vector<ChartPoint>{ { 0, 0 }, { 1, 2 }, { 2, 1 } } );
	first.setTooltipProvider(
		[]( const PointTooltipContext& context ) -> std::optional<TooltipPayload> {
			TooltipData payload;
			payload.title = "Sample";
			payload.fields.emplace_back(
				TooltipField{ "Index", String::toString( static_cast<Uint64>( context.index ) ) } );
			return payload;
		} );
	auto& second = chart->addLineSeries( "Second" );
	second.setCap( LineCap::Square );
	second.setJoin( LineJoin::Bevel );
	auto& right = chart->model().addAxis( AxisPosition::Right );
	EXPECT_TRUE( chart->model().axes().is_small() );
	second.setYAxis( right );
	second.setPoints( std::vector<ChartPoint>{ { 0, 2 }, { 1, 1 }, { 2, 3 } } );
	chart->fit();
	EXPECT_EQ( chart->model().series().size(), 2u );
	EXPECT_TRUE( chart->model().series().is_small() );
	auto* secondView = UIChart::New();
	secondView->setModel( chart->sharedModel() );
	secondView->setPixelsSize( 200, 140 );
	secondView->setPixelsPosition( 240, 0 );
	secondView->setParent( app.getUI()->getRoot() );
	secondView->fit();
	EXPECT_EQ( &secondView->model(), &chart->model() );
	app.getUI()->flushDirtyStyleAndLayout();
	app.getUI()->draw();
	EXPECT_EQ( chart->axisRange( chart->xAxis() ).max, 2 );
	std::static_pointer_cast<OwnedXYDataSource>( first.dataSource() )->append( { 3, 4 } );
	app.getUI()->draw();
	EXPECT_EQ( chart->axisRange( chart->xAxis() ).max, 3 );
	chart->setAxisRange( chart->xAxis(), { 0, 1 } );
	EXPECT_EQ( chart->axisRange( chart->xAxis() ).max, 1 );
	chart->setFollowX( 1.0 );
	std::static_pointer_cast<OwnedXYDataSource>( first.dataSource() )->append( { 4, 5 } );
	app.getUI()->draw();
}

UTEST( UIChart, hoverShowsUpdatesAndHidesTooltip ) {
	UIApplication app(
		WindowSettings{ 480, 320, "Chart Hover Test" },
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1.f ) );
	auto* chart = eeNew( ChartProbe, () );
	chart->setPixelsSize( 400, 240 );
	chart->setParent( app.getUI()->getRoot() );
	auto& series = chart->addLineSeries( "Flat" );
	series.setPoints( std::vector<ChartPoint>{ { 0, 0 }, { 1, 0 } } );
	series.setTooltipProvider( []( const PointTooltipContext& ) -> std::optional<TooltipPayload> {
		return TooltipText{ "Flat sample" };
	} );
	chart->setAxisRange( chart->xAxis(), { 0, 1 } );
	chart->setAxisRange( chart->yAxis(), { -1, 1 } );
	app.getUI()->flushDirtyStyleAndLayout();
	app.getUI()->draw();
	const Vector2i position = ( chart->getScreenPos() + Vector2f( 200, 109 ) ).asInt();
	chart->onMouseOver( position, 0 );
	ASSERT_TRUE( chart->getTooltip() != nullptr );
	EXPECT_TRUE( chart->getTooltip()->isVisible() );
	EXPECT_TRUE( chart->getTooltipText() == "Flat sample" );
	chart->onMouseMove( position + Vector2i( 5, 0 ), 0 );
	EXPECT_TRUE( chart->getTooltip()->isVisible() );
	chart->onMouseDown( position, EE_BUTTON_LMASK );
	EXPECT_FALSE( chart->getTooltip()->isVisible() );
	EXPECT_TRUE( chart->getTooltipText().empty() );
	EXPECT_TRUE( chart->getTooltip()->getText().empty() );
	chart->onMouseUp( position, EE_BUTTON_LMASK );
	chart->onMouseMove( position, 0 );
	EXPECT_TRUE( chart->getTooltip()->isVisible() );
	EXPECT_TRUE( chart->getTooltipText() == "Flat sample" );
	EXPECT_TRUE( chart->getTooltip()->getText() == "Flat sample" );
	series.setVisible( false );
	app.getUI()->draw();
	EXPECT_FALSE( chart->getTooltip()->isVisible() );
	series.setVisible( true );
	app.getUI()->draw();
	EXPECT_TRUE( chart->getTooltip()->isVisible() );
	chart->onMouseLeave( position, 0 );
	EXPECT_FALSE( chart->getTooltip()->isVisible() );
	EXPECT_TRUE( chart->getTooltipText().empty() );
	EXPECT_TRUE( chart->getTooltip()->getText().empty() );
	series.setTooltipProvider( []( const PointTooltipContext& ) -> std::optional<TooltipPayload> {
		return TooltipText{ "" };
	} );
	chart->onMouseOver( position, 0 );
	EXPECT_FALSE( chart->getTooltip()->isVisible() );
}

UTEST( UIChart, stationaryHoverTracksLiveRingSamples ) {
	UIApplication app(
		WindowSettings{ 480, 320, "Chart Live Hover Test" },
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1.f ) );
	auto* chart = eeNew( ChartProbe, () );
	chart->setPixelsSize( 400, 240 );
	chart->setParent( app.getUI()->getRoot() );
	auto ring = std::make_shared<RingXYDataSource>( 16 );
	for ( int x = 0; x < 10; ++x )
		ring->append( { static_cast<double>( x ), 0.0 } );
	auto& series = chart->addLineSeries( "Live" );
	series.setDataSource( ring );
	int hoveredSample = -1;
	series.setTooltipProvider(
		[&hoveredSample]( const PointTooltipContext& point ) -> std::optional<TooltipPayload> {
			hoveredSample = static_cast<int>( point.x );
			return TooltipText{ String::format( "Sample %d", hoveredSample ) };
		} );
	chart->setFollowX( 8.0 );
	app.getUI()->flushDirtyStyleAndLayout();
	app.getUI()->draw();
	const Vector2i position = ( chart->getScreenPos() + Vector2f( 214, 109 ) ).asInt();
	chart->onMouseOver( position, 0 );
	ASSERT_TRUE( chart->getTooltip() != nullptr );
	ASSERT_TRUE( chart->getTooltip()->isVisible() );
	const int initialSample = hoveredSample;
	ASSERT_TRUE( initialSample >= 0 );
	ring->append( { 10.0, 0.0 } );
	app.getUI()->draw();
	EXPECT_EQ( hoveredSample, initialSample + 1 );
	EXPECT_TRUE( chart->getTooltip()->isVisible() );
}

UTEST( UITooltip, emptyTextNeverBecomesVisible ) {
	UIApplication app(
		WindowSettings{ 320, 240, "Empty Tooltip Test" },
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1.f ) );
	auto* host = UIWidget::New();
	host->setParent( app.getUI()->getRoot() );
	auto* tooltip = host->createTooltip();
	tooltip->show();
	EXPECT_FALSE( tooltip->isVisible() );
	tooltip->setText( "Visible" );
	tooltip->show();
	EXPECT_TRUE( tooltip->isVisible() );
	tooltip->setText( "" );
	EXPECT_FALSE( tooltip->isVisible() );
	tooltip->setText( " \n\t" );
	tooltip->show();
	EXPECT_FALSE( tooltip->isVisible() );
	host->setTooltipText( "Restored" );
	tooltip->show();
	EXPECT_TRUE( tooltip->isVisible() );
	host->setTooltipText( "" );
	EXPECT_FALSE( tooltip->isVisible() );
}

UTEST( UIChart, styleUsesDeviceIndependentMeasurements ) {
	const Float previousDensity = PixelDensity::getPixelDensity();
	UIApplication app(
		WindowSettings{ 480, 320, "Chart Density Test" },
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 2.f ) );
	auto* chart = UIChart::New();
	chart->setPixelsSize( 400, 240 );
	chart->setParent( app.getUI()->getRoot() );
	ChartStyle style = chart->chartStyle();
	style.fontSize = 15.f;
	style.axisColor = Color( 12, 34, 56 );
	chart->setChartStyle( style );
	chart->getUIStyle()->setStyleSheetVariable( StyleSheetVariable( "--chart-test", "#123456" ) );
	EXPECT_TRUE( chart->themeColor( "--chart-test", Color::Black ) == Color( 18, 52, 86 ) );
	EXPECT_TRUE( chart->themeColor( "--chart-absent", Color::White ) == Color::White );
	chart->addLineSeries( "Dense" ).setPoints( std::vector<ChartPoint>{ { 0, 0 }, { 1, 1 } } );
	app.getUI()->flushDirtyStyleAndLayout();
	app.getUI()->draw();
	EXPECT_EQ( PixelDensity::getPixelDensity(), 2.f );
	EXPECT_EQ( chart->chartStyle().fontSize, 15.f );
	EXPECT_TRUE( chart->chartStyle().axisColor == Color( 12, 34, 56 ) );
	EXPECT_EQ( app.getWindow()->getWindowInfo()->WindowConfig.Width, 480u );
	PixelDensity::setPixelDensity( previousDensity );
}

UTEST( UIChart, miterJoinFillsInnerWedgeAtHighDensity ) {
	const Float previousDensity = PixelDensity::getPixelDensity();
	UIApplication app(
		WindowSettings{ 220, 220, "Chart Join Test" },
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 3.f ) );
	auto* chart = UIChart::New();
	chart->setPixelsSize( 200, 200 );
	chart->setParent( app.getUI()->getRoot() );
	ChartStyle style;
	style.leftMargin = style.rightMargin = style.topMargin = style.bottomMargin = 0.f;
	style.minimumAxisMargin = style.axisLabelPadding = 0.f;
	chart->setChartStyle( style );
	chart->xAxis().setFormatter( []( double, double ) { return String(); } );
	chart->yAxis().setFormatter( []( double, double ) { return String(); } );
	chart->setAxisRange( chart->xAxis(), { 0, 100 } );
	chart->setAxisRange( chart->yAxis(), { 0, 100 } );
	auto& line = chart->addLineSeries( "Corner" );
	line.setColor( Color( 0, 255, 0 ) );
	line.setWidth( 12.f );
	line.setPoints( std::vector<ChartPoint>{ { 20, 50 }, { 60, 50 }, { 60, 90 } } );
	app.getUI()->flushDirtyStyleAndLayout();
	app.getWindow()->setClearColor( Color( 20, 20, 20 ) );
	app.getWindow()->clear();
	app.getUI()->draw();
	Image image = app.getWindow()->getFrontBufferImage();
	ASSERT_TRUE( image.getWidth() > 125 && image.getHeight() > 105 );
	const Color wedge = image.getPixel( 125, 105 );
	EXPECT_TRUE_MSG( wedge.g > 180 && wedge.r < 100,
					 "The inside wedge between adjacent stroke quads must be filled" );
	PixelDensity::setPixelDensity( previousDensity );
}

UTEST( UIChart, smoothLineBendsWithoutOvershootingAndRotatedLabelsFit ) {
	const Float previousDensity = PixelDensity::getPixelDensity();
	UIApplication app(
		WindowSettings{ 260, 220, "Chart Interpolation Test" },
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1.f ) );
	auto* chart = eeNew( ChartProbe, () );
	chart->setPixelsSize( 200, 200 );
	chart->setParent( app.getUI()->getRoot() );
	ChartStyle style;
	style.leftMargin = style.rightMargin = style.topMargin = style.bottomMargin = 0.f;
	style.minimumAxisMargin = style.axisLabelPadding = 0.f;
	chart->setChartStyle( style );
	chart->xAxis().setFormatter( []( double, double ) { return String(); } );
	chart->yAxis().setFormatter( []( double, double ) { return String(); } );
	chart->setAxisRange( chart->xAxis(), { 0, 3 } );
	chart->setAxisRange( chart->yAxis(), { 0, 1 } );
	auto& line = chart->addLineSeries( "Hill" );
	line.setColor( Color( 255, 0, 0 ) );
	line.setWidth( 2.f );
	line.setTooltipProvider( []( const PointTooltipContext& ) -> std::optional<TooltipPayload> {
		return TooltipText{ "Smooth point" };
	} );
	line.setPoints( std::vector<ChartPoint>{ { 0, 0 }, { 1, 1 }, { 2, 1 }, { 3, 0 } } );
	app.getUI()->flushDirtyStyleAndLayout();
	app.getWindow()->setClearColor( Color( 20, 20, 20 ) );
	const auto draw = [&]() {
		app.getWindow()->clear();
		app.getUI()->draw();
		GlobalBatchRenderer::instance()->draw();
		return app.getWindow()->getFrontBufferImage();
	};
	Image linear = draw();
	line.setInterpolation( LineInterpolation::MonotoneCubic );
	Image smooth = draw();
	EXPECT_TRUE( linear.getPixel( 50, 22 ).r < 100 );
	EXPECT_TRUE( smooth.getPixel( 50, 22 ).r > 180 );
	EXPECT_EQ( line.interpolation(), LineInterpolation::MonotoneCubic );
	chart->onMouseOver( ( chart->getScreenPos() + Vector2f( 50, 22 ) ).asInt(), 0 );
	EXPECT_TRUE( chart->getTooltipText() == "Smooth point" );
	chart->onMouseLeave( Vector2i( 50, 22 ), 0 );
	style.xTickLabelAngle = -90.f;
	style.xTickLabelAnchor = ChartTickLabelAnchor::End;
	chart->setChartStyle( style );
	chart->xAxis().setFormatter( []( double, double ) { return String( "September 2026" ); } );
	Image labels = draw();
	EXPECT_EQ( chart->chartStyle().xTickLabelAngle, -90.f );
	EXPECT_TRUE( labels.getPixel( 50, 22 ).r < 100 );
	bool drewRotatedLabel = false;
	for ( unsigned int y = 110; y < 185 && !drewRotatedLabel; ++y ) {
		for ( unsigned int x = 1; x < 199; ++x ) {
			const Color pixel = labels.getPixel( x, y );
			if ( pixel.r > 100 && pixel.g > 100 && pixel.b > 100 ) {
				drewRotatedLabel = true;
				break;
			}
		}
	}
	EXPECT_TRUE( drewRotatedLabel );
	PixelDensity::setPixelDensity( previousDensity );
}

UTEST( UIChart, screenGridStaysFixedWhileDataGridPans ) {
	const Float previousDensity = PixelDensity::getPixelDensity();
	UIApplication app(
		WindowSettings{ 220, 220, "Chart Grid Test" },
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1.f ) );
	auto* chart = UIChart::New();
	chart->setPixelsSize( 200, 200 );
	chart->setParent( app.getUI()->getRoot() );
	ChartStyle style;
	style.leftMargin = style.rightMargin = style.topMargin = style.bottomMargin = 0.f;
	style.verticalGrid.mode = ChartGridMode::ScreenInterval;
	style.verticalGrid.spacing = 50.0;
	style.verticalGrid.width = 3.f;
	style.verticalGrid.color = Color( 255, 0, 0 );
	style.horizontalGrid.mode = ChartGridMode::DataInterval;
	style.horizontalGrid.spacing = 50.0;
	style.horizontalGrid.width = 3.f;
	style.horizontalGrid.color = Color( 0, 255, 0 );
	chart->setChartStyle( style );
	chart->xAxis().setFormatter( []( double, double ) { return String(); } );
	chart->yAxis().setFormatter( []( double, double ) { return String(); } );
	chart->setAxisRange( chart->xAxis(), { 0, 100 } );
	chart->setAxisRange( chart->yAxis(), { 0, 100 } );
	app.getUI()->flushDirtyStyleAndLayout();
	app.getWindow()->setClearColor( Color( 20, 20, 20 ) );
	const auto draw = [&]() {
		app.getWindow()->clear();
		app.getUI()->draw();
		GlobalBatchRenderer::instance()->draw();
		return app.getWindow()->getFrontBufferImage();
	};
	Image before = draw();
	const Color fixedBefore = before.getPixel( 50, 70 );
	const Color dataBefore = before.getPixel( 80, 100 );
	EXPECT_TRUE( fixedBefore.r > 180 && fixedBefore.g < 100 );
	EXPECT_TRUE( dataBefore.g > 180 && dataBefore.r < 100 );
	chart->setAxisRange( chart->yAxis(), { 25, 125 } );
	Image after = draw();
	const Color fixedAfter = after.getPixel( 50, 70 );
	const Color dataAfter = after.getPixel( 80, 150 );
	const Color formerDataPixel = after.getPixel( 80, 100 );
	EXPECT_TRUE( fixedAfter.r > 180 && fixedAfter.g < 100 );
	EXPECT_TRUE( dataAfter.g > 180 && dataAfter.r < 100 );
	EXPECT_TRUE( formerDataPixel.g < 100 );
	style.horizontalGrid.mode = ChartGridMode::AxisTicks;
	chart->setChartStyle( style );
	Image ticks = draw();
	EXPECT_TRUE( ticks.getPixel( 80, 100 ).g > 180 );
	auto& right = chart->model().addAxis( AxisPosition::Right );
	chart->setAxisRange( right, { 0, 10 } );
	style.horizontalGrid.mode = ChartGridMode::DataInterval;
	style.horizontalGrid.spacing = 5.0;
	style.horizontalGrid.axis = &right;
	chart->setChartStyle( style );
	Image secondary = draw();
	EXPECT_TRUE( secondary.getPixel( 80, 100 ).g > 180 );
	PixelDensity::setPixelDensity( previousDensity );
}
