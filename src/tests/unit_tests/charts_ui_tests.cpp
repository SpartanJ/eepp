#include "utest.h"
#include <eepp/system/filesystem.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/ui/charts/uichart.hpp>
#include <eepp/ui/uiapplication.hpp>
#include <eepp/ui/uiroot.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uitooltip.hpp>

using namespace EE;
using namespace EE::Window;
using namespace EE::UI;
using namespace EE::UI::Charts;

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
	UIApplication app( WindowSettings{ 480, 320, "Chart Test" },
					   UIApplication::Settings( System::Sys::getProcessPath() + ".." +
													System::FileSystem::getOSSlash(),
												1.f ) );
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
	second->setYAxis( right );
	second->setPoints( std::vector<ChartPoint>{ { 0, 2 }, { 1, 1 }, { 2, 3 } } );
	chart->fit();
	EXPECT_EQ( chart->model()->series().size(), 2u );
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
	UIApplication app( WindowSettings{ 480, 320, "Chart Hover Test" },
					   UIApplication::Settings( System::Sys::getProcessPath() + ".." +
													System::FileSystem::getOSSlash(),
												1.f ) );
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
