#include <eepp/ee.hpp>

EE_MAIN_FUNC int main( int, char** ) {
	UIApplication app( { 640, 480, "eepp - UIApplication Hello World" } );
	app.getUI()->loadLayoutFromString( R"xml(
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
	return app.run();
}
