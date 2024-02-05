#include <eepp/ee.hpp>

// Reference https://eugenkiss.github.io/7guis/tasks#timer
EE_MAIN_FUNC int main( int, char** ) {
	UIApplication app( { 380, 160, "eepp - 7GUIs - Timer" } );
	UIWidget* vbox = app.getUI()->loadLayoutFromString( R"xml(
	<vbox layout_width="match_parent" layout_height="match_parent" padding="8dp">
		<hbox layout_width="match_parent" layout_height="wrap_content" marginBottom="8dp">
			<TextView text="Elapsed Time: " marginRight="8dp" />
			<ProgressBar id="progressbar" layout_width="fixed" layout_weight="1" layout_height="match_parent" displayPercent="true" />
		</hbox>
		<TextView id="elapsed_time" text="0" marginBottom="8dp" />
		<hbox layout_width="match_parent" layout_height="wrap_content" marginBottom="8dp">
			<TextView text="Duration: " marginRight="8dp" />
			<Slider id="duration_slider" orientation="horizontal" layout_width="fixed" layout_weight="1"
					layout_height="wrap_content" value="1" />
		</hbox>
		<PushButton id="reset_button" text="Reset" layout_width="match_parent" />
	</vbox>
	)xml" );
	Clock clock;
	auto progressBar = vbox->find<UIProgressBar>( "progressbar" );
	auto durationSlider = vbox->find<UISlider>( "duration_slider" );
	auto elapsedTimeView = vbox->find<UITextView>( "elapsed_time" );
	auto intervalId = String::hash( "unique_interval_id" );
	const auto update = [&]() {
		double totalDurationInSeconds = static_cast<double>( durationSlider->getValue() * 100. );
		elapsedTimeView->setText( clock.getElapsedTime().asSeconds() < totalDurationInSeconds
									  ? clock.getElapsedTime().toString()
									  : Seconds( totalDurationInSeconds ).toString() );
		progressBar->setProgress(
			std::min( clock.getElapsedTime().asSeconds(), totalDurationInSeconds ) /
			totalDurationInSeconds * 100.f );
		if ( clock.getElapsedTime().asSeconds() >= totalDurationInSeconds )
			app.getUI()->removeActionsByTag( intervalId );
	};
	vbox->find<UIPushButton>( "reset_button" )->onClick( [&]( auto ) {
		clock.restart();
		app.getUI()->removeActionsByTag( intervalId );
		app.getUI()->setInterval( [&update] { update(); }, Milliseconds( 100 ), intervalId );
	} );
	return app.run();
}
