#include <eepp/ee.hpp>

EE_MAIN_FUNC int main( int, char** ) {
	UIApplication app( { 1280, 720, "eepp - UI HTML Example" } );

	Log::instance()->setLogLevelThreshold( LogLevel::Debug );
	Log::instance()->setLogToStdOut( true );
	Log::instance()->setLiveWrite( true );

	Http::setDefaultUserAgent( "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like "
							   "Gecko) Chrome/146.0.0.0 Safari/537.36" );

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
			<PushButton id="backbtn" text="@string(back, Back)" />
			<PushButton id="fwdbtn" text="@string(forward, Forward)" />
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
	auto backBtn = ui->find( "backbtn" )->asType<UIPushButton>();
	auto fwdBtn = ui->find( "fwdbtn" )->asType<UIPushButton>();
	std::vector<URI> history;
	int historyIndex = -1;

	auto updateNavButtons = [&]() {
		backBtn->setEnabled( historyIndex > 0 );
		fwdBtn->setEnabled( historyIndex < static_cast<int>( history.size() ) - 1 );
	};

	const auto loadDocumentData = [ui, mainContainer, urlBar, &app]( URI url, std::string& data ) {
		if ( data.empty() )
			return;
		ui->ensureMainThread( [url, data, mainContainer, urlBar, ui, &app] {
			mainContainer->closeAllChildren();
			ui->getStyleSheet().removeAllWithoutMarker( app.getStyleSheetDefaultMarker() );
			ui->setURIFromURL( url );
			auto urlStr = url.toString();
			auto hash = String::hash( urlStr );
			ui->loadLayoutFromString( HTMLFormatter::HTMLtoXML( data ), mainContainer, hash );
			urlBar->setText( urlStr );
		} );
	};

	// We add a default `isHistoryNav` parameter to determine if we are pushing to history or just
	// navigating back/forth
	const auto loadDocument = [&]( URI url, bool isHistoryNav = false ) {
		if ( !isHistoryNav ) {
			// If we navigate to a new URL while in the middle of history, clear out the "forward"
			// history
			if ( historyIndex >= 0 && historyIndex < static_cast<int>( history.size() ) - 1 ) {
				history.resize( historyIndex + 1 );
			}

			// Don't add to history if we are just reloading the exact same current page manually
			if ( history.empty() || history.back().toString() != url.toString() ) {
				history.push_back( url );
				historyIndex = static_cast<int>( history.size() ) - 1;
			}
			updateNavButtons();
		}

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
		} else if ( !url.getPath().empty() ) {
			std::string data;
			FileSystem::fileGet( url.getPath(), data );
			loadDocumentData( url, data );
		}
	};

	backBtn->onClick( [&]( const MouseEvent* ) {
		if ( historyIndex > 0 ) {
			historyIndex--;
			updateNavButtons();
			loadDocument( history[historyIndex], true );
		}
	} );

	fwdBtn->onClick( [&]( const MouseEvent* ) {
		if ( historyIndex < static_cast<int>( history.size() ) - 1 ) {
			historyIndex++;
			updateNavButtons();
			loadDocument( history[historyIndex], true );
		}
	} );

	updateNavButtons();
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
