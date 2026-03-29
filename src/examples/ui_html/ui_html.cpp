#include <eepp/ee.hpp>

EE_MAIN_FUNC int main( int, char** ) {
	UIApplication app( { 1280, 720, "eepp - UI HTML Example" } );

	Log::instance()->setLogLevelThreshold( LogLevel::Debug );
	Log::instance()->setLogToStdOut( true );
	Log::instance()->setLiveWrite( true );

	auto win = app.getWindow();
	auto ui = app.getUI();

	FontTrueType* remixIconFont = FontTrueType::New( "icon", "assets/fonts/remixicon.ttf" );
	FontTrueType* noniconsFont = FontTrueType::New( "nonicons", "assets/fonts/nonicons.ttf" );
	FontTrueType* codIconFont = FontTrueType::New( "codicon", "assets/fonts/codicon.ttf" );
	ui->getUIIconThemeManager()->setCurrentTheme(
		IconManager::init( "icons", remixIconFont, noniconsFont, codIconFont ) );

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

	const auto loadDocumentData = [ui, mainContainer, urlBar]( URI url, std::string& data ) {
		if ( data.empty() )
			return;
		static String::HashType prevURL = 0;
		ui->ensureMainThread( [=] {
			mainContainer->closeAllChildren();
			if ( prevURL )
				ui->getStyleSheet().removeAllWithMarker( prevURL );
			ui->setURIFromURL( url );
			auto urlStr = url.toString();
			auto hash = String::hash( urlStr );
			ui->loadLayoutFromString( HTMLFormatter::HTMLtoXML( data ), mainContainer, hash );
			prevURL = hash;
			urlBar->setText( urlStr );
		} );
	};

	const auto loadDocument = [&]( URI url ) {
		if ( !url.getScheme().empty() ) {
			if ( url.getScheme() == "https" || url.getScheme() == "http" ) {
				Http::getAsync(
					[=]( const Http&, Http::Request&, Http::Response& response ) {
						std::string data = response.getBody();
						loadDocumentData( url, data );
					},
					url, Seconds( 5 ) );
			} else if ( url.getScheme() == "file" ) {
				std::string data;
				FileSystem::fileGet( url.getPath(), data );
				loadDocumentData( url, data );
			}
		} else if ( !url.getPath().empty() && url.getPath().front() == '/' ) {
			std::string data;
			FileSystem::fileGet( url.getPath(), data );
			loadDocumentData( url, data );
		}
	};

	loadDocument( "https://news.ycombinator.com" );

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
