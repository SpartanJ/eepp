#include <eepp/ee.hpp>

EE_MAIN_FUNC int main( int, char** ) {
	UIApplication app( { 1280, 720, "eepp - UI HTML Example" } );

	Log::instance()->setLogLevelThreshold( LogLevel::Debug );
	Log::instance()->setLogToStdOut( true );
	Log::instance()->setLiveWrite( true );

	auto win = app.getWindow();
	auto ui = app.getUI();

	ui->setColorSchemePreference( ColorSchemeExtPreference::Light );

	ui->loadLayoutFromString( R"xml(
	<vbox layout_width="match_parent" layout_height="match_parent">
		<hbox layout_width="match_parent" layout_height="wrap_content">
			<TextInput id="url_bar" layout_width="0" layout_weight="1"
				hint="@string(enter_address, Enter Address)" />
		</hbox>
		<ScrollView id="html_view" layout_width="match_parent" layout_height="0" layout_weight="1">
			<vbox layout_width="match_parent" layout_height="wrap_content" id="html_doc"></vbox>
		</ScrollView>
	</vbox>
	)xml" );

	auto urlBar = ui->find( "url_bar" )->asType<UITextInput>();
	auto mainContainer = ui->find( "html_doc" );

	const auto loadDocument = [&]( URI url ) {
		static String::HashType prevURL = 0;
		std::string data;
		if ( !url.getScheme().empty() ) {
			if ( url.getScheme() == "https" || url.getScheme() == "http" ) {
				auto response = Http::get( url, Seconds( 5 ) );
				data = response.getBody();
			} else if ( url.getScheme() == "file" ) {
				FileSystem::fileGet( url.getPath(), data );
			}
		} else if ( !url.getPath().empty() && url.getPath().front() == '/' ) {
			FileSystem::fileGet( url.getPath(), data );
		}

		if ( !data.empty() ) {
			if ( url.getPath().empty() || url.getPath().back() != '/' ) {
				if ( url.getScheme() == "file" &&
					 !FileSystem::fileExtension( url.getPath() ).empty() ) {
					url.setPath( FileSystem::fileRemoveFileName( url.getPath() ) );
				}
				url.setPath( url.getPath() + "/" );
			}
			mainContainer->closeAllChildren();
			if ( prevURL )
				ui->getStyleSheet().removeAllWithMarker( prevURL );
			ui->setURI( url );
			auto hash = String::hash( url.toString() );
			ui->loadLayoutFromString( data, mainContainer, hash );
			prevURL = hash;
		}
	};

	urlBar->on( Event::OnPressEnter,
				[&]( auto event ) { loadDocument( urlBar->getText().toUtf8() ); } );

	ui->setURLInterceptorCb( [&]( URI uri ) {
		loadDocument( ui->solveRelativePath( uri ) );
		return true;
	} );

	win->getInput()->pushCallback( [&loadDocument]( InputEvent* event ) {
		switch ( event->Type ) {
			case InputEvent::FileDropped: {
				std::string file( event->file.file );
				loadDocument( "file://" + file );
				break;
			}
			case InputEvent::TextDropped: {
				loadDocument( event->textdrop.text );
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
