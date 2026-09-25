#include "utest.h"
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
	auto* first = chart->addLineSeries( "First" );
	first->setPoints( std::vector<ChartPoint>{ { 0, 0 }, { 1, 2 }, { 2, 1 } } );
	first->setTooltipProvider(
		[]( const PointTooltipContext& context ) -> std::optional<TooltipPayload> {
			TooltipData payload;
			payload.title = "Sample";
			payload.fields.emplace_back(
				TooltipField{ "Index", String::toString( static_cast<Uint64>( context.index ) ) } );
			return payload;
		} );
	auto* second = chart->addLineSeries( "Second" );
	second->setCap( LineCap::Square );
	second->setJoin( LineJoin::Bevel );
	auto* right = chart->model()->addAxis( AxisPosition::Right );
	EXPECT_TRUE( chart->model()->axes().is_small() );
	second->setYAxis( right );
	second->setPoints( std::vector<ChartPoint>{ { 0, 2 }, { 1, 1 }, { 2, 3 } } );
	chart->fit();
	EXPECT_EQ( chart->model()->series().size(), 2u );
	EXPECT_TRUE( chart->model()->series().is_small() );
	auto* secondView = UIChart::New();
	secondView->setModel( chart->sharedModel() );
	secondView->setPixelsSize( 200, 140 );
	secondView->setPixelsPosition( 240, 0 );
	secondView->setParent( app.getUI()->getRoot() );
	secondView->fit();
	EXPECT_EQ( secondView->model(), chart->model() );
	app.getUI()->flushDirtyStyleAndLayout();
	app.getUI()->draw();
	EXPECT_EQ( chart->axisRange( chart->xAxis() ).max, 2 );
	std::static_pointer_cast<OwnedXYDataSource>( first->dataSource() )->append( { 3, 4 } );
	app.getUI()->draw();
	EXPECT_EQ( chart->axisRange( chart->xAxis() ).max, 3 );
	chart->setAxisRange( chart->xAxis(), { 0, 1 } );
	EXPECT_EQ( chart->axisRange( chart->xAxis() ).max, 1 );
	chart->setFollowX( 1.0 );
	std::static_pointer_cast<OwnedXYDataSource>( first->dataSource() )->append( { 4, 5 } );
	app.getUI()->draw();
}

UTEST( UIChart, hoverShowsUpdatesAndHidesTooltip ) {
	UIApplication app(
		WindowSettings{ 480, 320, "Chart Hover Test" },
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1.f ) );
	auto* chart = eeNew( ChartProbe, () );
	chart->setPixelsSize( 400, 240 );
	chart->setParent( app.getUI()->getRoot() );
	auto* series = chart->addLineSeries( "Flat" );
	series->setPoints( std::vector<ChartPoint>{ { 0, 0 }, { 1, 0 } } );
	series->setTooltipProvider( []( const PointTooltipContext& ) -> std::optional<TooltipPayload> {
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
	chart->onMouseLeave( position, 0 );
	EXPECT_FALSE( chart->getTooltip()->isVisible() );
	EXPECT_TRUE( chart->getTooltipText().empty() );
	EXPECT_TRUE( chart->getTooltip()->getText().empty() );
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
	chart->addLineSeries( "Dense" )->setPoints( std::vector<ChartPoint>{ { 0, 0 }, { 1, 1 } } );
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
	chart->xAxis()->setFormatter( []( double, double ) { return String(); } );
	chart->yAxis()->setFormatter( []( double, double ) { return String(); } );
	chart->setAxisRange( chart->xAxis(), { 0, 100 } );
	chart->setAxisRange( chart->yAxis(), { 0, 100 } );
	auto* line = chart->addLineSeries( "Corner" );
	line->setColor( Color( 0, 255, 0 ) );
	line->setWidth( 12.f );
	line->setPoints( std::vector<ChartPoint>{ { 20, 50 }, { 60, 50 }, { 60, 90 } } );
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
