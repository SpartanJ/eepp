#include <eepp/ee.hpp>

EE_MAIN_FUNC int main( int, char** ) {
	UIApplication app( { 1024, 768, "eepp - UIApplication Hello World" } );
	auto* ui = app.getUI();
	ui->loadLayoutFromString( R"xml(
			<LinearLayout layout_width="match_parent"
						  layout_height="match_parent"
						  orientation="vertical">
				<TextView id="text_view"
						  layout_width="match_parent"
						  layout_height="wrap_content"
						  text="Hello, I am a TextView" />
				<PushButton id="button_view"
						layout_width="match_parent"
						layout_height="wrap_content"
						text="Hello, I am a PushButton" />
			</LinearLayout>
	)xml" );
	ui->on( Event::KeyUp, [ui]( const Event* event ) {
		if ( event->asKeyEvent()->getKeyCode() == KEY_F11 ) {
			UIWidgetInspector::create( ui );
		}
	} );

	return app.run();
}
