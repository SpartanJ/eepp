#include <cmath>
#include <eepp/ee.hpp>

using namespace EE::UI::Charts;

namespace {

std::vector<ChartPoint> wave( size_t count, double phase ) {
	std::vector<ChartPoint> points;
	points.reserve( count );
	for ( size_t i = 0; i < count; ++i ) {
		const double x = static_cast<double>( i ) * 0.04;
		points.push_back( { x, std::sin( x + phase ) } );
	}
	return points;
}

} // namespace

EE_MAIN_FUNC int main( int, char** ) {
	UIApplication app(
		{ 1280, 850, "eepp - Charts Example" }, UIApplication::Settings{},
		ContextSettings( true, ContextSettings::FrameRateLimitScreenRefreshRate, 4 ) );
	auto* ui = app.getUI();
	ui->getRoot()->setBackgroundColor( Color( 22, 27, 37 ) );
	auto* layout = ui->loadLayoutFromString( R"xml(
	<style>
		.chart-title { text-color: #e1e6ee; font-size: 16dp; }
		.chart-view { background-color: #202733; }
	</style>
	<vbox lw="mp" lh="mp" padding="8dp">
		<hbox lw="mp" lh="0" lw8="0.5" margin-bottom="8dp">
			<vbox lw="0" lw8="0.5" lh="mp" margin-right="8dp">
				<TextView class="chart-title" text="Waves: hover, drag, wheel; C toggles cosine" lh="wc" />
				<Chart id="waves" class="chart-view" lw="mp" lh="0" lw8="1" />
			</vbox>
			<vbox lw="0" lw8="0.5" lh="mp">
				<TextView class="chart-title" text="Two Y axes: detailed hover; T toggles temperature" lh="wc" />
				<Chart id="dual" class="chart-view" lw="mp" lh="0" lw8="1" />
			</vbox>
		</hbox>
		<hbox lw="mp" lh="0" lw8="0.5">
			<vbox lw="0" lw8="0.5" lh="mp" margin-right="8dp">
				<TextView class="chart-title" text="Dense: 250k points, hover spikes; R fits all" lh="wc" />
				<Chart id="dense" class="chart-view" lw="mp" lh="0" lw8="1" />
			</vbox>
			<vbox lw="0" lw8="0.5" lh="mp">
				<TextView class="chart-title" text="Live ring: hover; Space adds 40 samples" lh="wc" />
				<Chart id="live" class="chart-view" lw="mp" lh="0" lw8="1" />
			</vbox>
		</hbox>
	</vbox>
	)xml" );

	auto* waves = layout->find<UIChart>( "waves" );
	waves->xAxis()->setLabel( "Time (s)" );
	waves->yAxis()->setLabel( "Amplitude" );
	auto* sine = waves->addLineSeries( "Sine" );
	sine->setColor( Color( 83, 179, 255 ) );
	sine->setWidth( 3.f );
	sine->setPoints( wave( 500, 0 ) );
	sine->setTooltipProvider(
		[]( const PointTooltipContext& point ) -> std::optional<TooltipPayload> {
			return TooltipText{
				String::format( "Sine at %.2f s\nAmplitude: %.3f", point.x, point.y ) };
		} );
	auto* cosine = waves->addLineSeries( "Cosine" );
	cosine->setColor( Color( 255, 177, 77 ) );
	cosine->setCap( LineCap::Square );
	cosine->setPoints( wave( 500, 1.5707963267948966 ) );
	waves->fit();

	auto* dual = layout->find<UIChart>( "dual" );
	dual->xAxis()->setLabel( "Minute" );
	dual->yAxis()->setLabel( "Load (%)" );
	dual->yAxis()->setFormatter(
		[]( double value, double ) { return String::format( "%.0f%%", value ); } );
	auto* temperatureAxis = dual->model()->addAxis( AxisPosition::Right );
	temperatureAxis->setLabel( "Temperature (C)" );
	temperatureAxis->setFormatter(
		[]( double value, double ) { return String::format( "%.1f C", value ); } );
	std::vector<ChartPoint> load;
	std::vector<double> temperatureX;
	std::vector<Float> temperatureY;
	load.reserve( 120 );
	temperatureX.reserve( 120 );
	temperatureY.reserve( 120 );
	for ( size_t i = 0; i < 120; ++i ) {
		const double x = static_cast<double>( i );
		load.push_back( { x, 50.0 + 18.0 * std::sin( x * 0.14 ) + 5.0 * std::sin( x * 0.9 ) } );
		temperatureX.push_back( x );
		temperatureY.push_back(
			static_cast<Float>( 37.0 + 0.08 * x + 2.0 * std::sin( x * 0.12 ) ) );
	}
	auto* loadSeries = dual->addLineSeries( "Load" );
	loadSeries->setColor( Color( 129, 211, 142 ) );
	loadSeries->setPoints( std::move( load ) );
	loadSeries->setTooltipProvider(
		[]( const PointTooltipContext& point ) -> std::optional<TooltipPayload> {
			TooltipData data;
			data.title = "Load sample";
			data.fields.emplace_back(
				TooltipField{ "Index", String::toString( static_cast<Uint64>( point.index ) ) } );
			data.fields.emplace_back( TooltipField{ "Minute", String::format( "%.0f", point.x ) } );
			data.fields.emplace_back( TooltipField{ "Load", String::format( "%.1f%%", point.y ) } );
			return data;
		} );
	auto* temperatureSeries = dual->addLineSeries( "Temperature" );
	temperatureSeries->setYAxis( temperatureAxis );
	temperatureSeries->setColor( Color( 255, 113, 132 ) );
	temperatureSeries->setDataSource( makeArrayXYDataSource(
		std::make_shared<const std::vector<double>>( std::move( temperatureX ) ),
		std::make_shared<const std::vector<float>>( std::move( temperatureY ) ) ) );
	temperatureSeries->setTooltipProvider(
		[]( const PointTooltipContext& point ) -> std::optional<TooltipPayload> {
			TooltipData data;
			data.title = "Temperature sample";
			data.fields.emplace_back( TooltipField{ "Minute", String::format( "%.0f", point.x ) } );
			data.fields.emplace_back(
				TooltipField{ "Temperature", String::format( "%.1f C", point.y ) } );
			data.description = "Uses the right Y axis";
			return data;
		} );
	dual->fit();

	auto* dense = layout->find<UIChart>( "dense" );
	ChartStyle denseStyle = dense->chartStyle();
	denseStyle.hoverColor = Color( 182, 146, 255, 150 );
	denseStyle.tickLabelColor = Color( 202, 193, 227 );
	dense->setChartStyle( denseStyle );
	dense->xAxis()->setLabel( "Sample" );
	dense->yAxis()->setLabel( "Signal" );
	std::vector<ChartPoint> samples;
	samples.reserve( 250000 );
	for ( size_t i = 0; i < 250000; ++i ) {
		const double x = static_cast<double>( i );
		const double spike = i % 18713 == 0 ? 3.0 : 0.0;
		samples.push_back(
			{ x, 0.3 * std::sin( x * 0.004 ) + 0.12 * std::sin( x * 0.059 ) + spike } );
	}
	auto* signal = dense->addLineSeries( "Signal" );
	signal->setColor( Color( 182, 146, 255 ) );
	signal->setJoin( LineJoin::Bevel );
	signal->setPoints( std::move( samples ) );
	dense->fit();

	auto* live = layout->find<UIChart>( "live" );
	live->xAxis()->setLabel( "Sample" );
	live->yAxis()->setLabel( "Value" );
	auto ring = std::make_shared<RingXYDataSource>( 1000 );
	for ( size_t i = 0; i < 1000; ++i ) {
		const double x = static_cast<double>( i );
		ring->append( { x, std::sin( x * 0.03 ) + 0.2 * std::sin( x * 0.17 ) } );
	}
	auto* rolling = live->addLineSeries( "Rolling value" );
	rolling->setColor( Color( 255, 207, 95 ) );
	rolling->setDataSource( ring );
	rolling->setTooltipProvider(
		[]( const PointTooltipContext& point ) -> std::optional<TooltipPayload> {
			return TooltipText{
				String::format( "Live sample %.0f\nValue: %.3f", point.x, point.y ) };
		} );
	live->setFollowX( 250.0 );

	auto next = std::make_shared<double>( 1000.0 );
	const auto appendSamples = [ring, next]( int count ) {
		for ( int i = 0; i < count; ++i, ++*next ) {
			const double x = *next;
			ring->append( { x, std::sin( x * 0.03 ) + 0.2 * std::sin( x * 0.17 ) } );
		}
	};
	ui->setInterval( [appendSamples] { appendSamples( 1 ); }, Milliseconds( 100 ),
					 String::hash( "chart-live-ring" ) );
	ui->on( Event::KeyDown, [appendSamples, waves, dual, dense, live, cosine,
							 temperatureSeries]( const Event* event ) {
		if ( event->asKeyEvent()->getKeyCode() == KEY_R ) {
			for ( auto* chart : { waves, dual, dense, live } )
				chart->fit();
			live->setFollowX( 250.0 );
		} else if ( event->asKeyEvent()->getKeyCode() == KEY_SPACE ) {
			appendSamples( 40 );
		} else if ( event->asKeyEvent()->getKeyCode() == KEY_C ) {
			cosine->setVisible( !cosine->visible() );
		} else if ( event->asKeyEvent()->getKeyCode() == KEY_T ) {
			temperatureSeries->setVisible( !temperatureSeries->visible() );
		}
	} );

	return app.run();
}
