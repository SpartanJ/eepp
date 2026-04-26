#include <eepp/ee.hpp>

EE_MAIN_FUNC int main( int, char** ) {
	UIApplication app( { 800, 600, "eepp - UIRichText Example" } );

	app.getUI()->loadLayoutFromString( R"xml(
	<ScrollView layout_width="match_parent" layout_height="match_parent">
		<vbox id="main_container" layout_width="match_parent" layout_height="wrap_content" padding="8dp">
			<RichText text-selection="true" font-size="12dp"
				color="white">Welcome to the <span color="#FFD700" font-style="bold">UIRichText</span> example!
				This component supports <span color="#00FF00" font-style="italic">styled text</span>,
				<span color="#00BFFF" font-style="shadow">shadows</span>,
				and <span color="#FF4500" text-stroke-width="1dp" text-stroke-color="black">outlines</span> using <span font-family="monospace" color="#A9A9A9">HTML-like tags</span>.
			</RichText>
			<Image src="file://assets/icon/ee.png" margin="4dp" layout-gravity="center_horizontal" />
			<RichText font-size="12dp"
			color="#fefefe">We can also mix <span color="#FFD700" font-style="bold">contents</span> with more <span color="#00FF00" font-style="italic">text</span>!
			</RichText>
		</vbox>
	</ScrollView>
	)xml" );

	auto mainContainer = app.getUI()->find<UILinearLayout>( "main_container" );

	app.getWindow()->getInput()->pushCallback( [mainContainer, &app]( InputEvent* event ) {
		switch ( event->Type ) {
			case InputEvent::FileDropped: {
				std::string file( event->file.file );
				std::string data;
				FileSystem::fileGet( file, data );
				mainContainer->closeAllChildren();
				std::string uri( "file://" + FileSystem::fileRemoveFileName( file ) );
				FileSystem::dirAddSlashAtEnd( uri );
				app.getUI()->setURI( URI( uri ) );
				app.getUI()->loadLayoutFromString( data, mainContainer );
				break;
			}
			case InputEvent::TextDropped: {
				mainContainer->closeAllChildren();
				app.getUI()->loadLayoutFromString( event->textdrop.text, mainContainer );
				break;
			}
			default:
				break;
		}
	} );

	app.getUI()->on( Event::KeyUp, [&app]( const Event* event ) {
		if ( event->asKeyEvent()->getKeyCode() == KEY_F11 ) {
			UIWidgetInspector::create( app.getUI() );
		}
	} );

	return app.run();
}
