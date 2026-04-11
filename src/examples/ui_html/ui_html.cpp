#include <eepp/ee.hpp>

#include <args/args.hxx>
#include <iostream>

EE_MAIN_FUNC int main( int argc, char** argv ) {
	args::ArgumentParser parser( "eepp HTML Example" );
	args::HelpFlag help( parser, "help", "Display this help menu", { 'h', "help" } );

	args::Positional<std::string> url( parser, "URL", "The URL to request" );
	args::ValueFlag<std::string> prefersColorScheme(
		parser, "prefers-color-scheme",
		"Set the preferred color scheme (\"light\", \"dark\" or \"system\")",
		{ 'c', "prefers-color-scheme" } );
	args::Flag hnDark( parser, "hn-dark",
					   "Force a custom CSS style for Hacker News site to be dark.", { "hn-dark" } );
	args::Flag benchmarkMode( parser, "benchmark-mode",
							  "Render as much as possible to measure the rendering performance.",
							  { "benchmark-mode" } );
	args::ValueFlag<Float> pixelDensityConf( parser, "pixel-density",
											 "Set default application pixel density",
											 { 'd', "pixel-density" } );

	try {
		parser.ParseCLI( Sys::parseArguments( argc, argv ) );
	} catch ( const args::Help& ) {
		std::cout << parser;
		return EXIT_SUCCESS;
	} catch ( const args::ParseError& e ) {
		std::cerr << e.what() << std::endl;
		std::cerr << parser;
		return EXIT_FAILURE;
	} catch ( args::ValidationError& e ) {
		std::cerr << e.what() << std::endl;
		std::cerr << parser;
		return EXIT_FAILURE;
	}

	UIApplication app(
		WindowSettings{ 1280, 720, "eepp - UI HTML Example" },
		UIApplication::Settings( {}, pixelDensityConf ? pixelDensityConf.Get() : 0.f ),
		ContextSettings(
			false, benchmarkMode.Get() ? 0 : ContextSettings::FrameRateLimitScreenRefreshRate ) );

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

	ui->setColorSchemePreference(
		!prefersColorScheme.Get().empty()
			? ColorSchemePreferences::fromStringExt( prefersColorScheme.Get() )
			: ColorSchemeExtPreference::Light );

	bool useHNDark = hnDark.Get();

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
	auto scrollView = ui->find( "html_view" )->asType<UIScrollView>();
	std::vector<URI> history;
	int historyIndex = -1;

	auto updateNavButtons = [&]() {
		backBtn->setEnabled( historyIndex > 0 );
		fwdBtn->setEnabled( historyIndex < static_cast<int>( history.size() ) - 1 );
	};

	const auto loadDocumentData = [ui, mainContainer, urlBar, &app, scrollView,
								   useHNDark]( URI url, std::string& data ) {
		if ( data.empty() )
			return;
		ui->ensureMainThread( [url, data, mainContainer, urlBar, ui, &app, scrollView, useHNDark] {
			mainContainer->closeAllChildren();
			ui->getStyleSheet().removeAllWithoutMarker( app.getStyleSheetDefaultMarker() );
			ui->setURIFromURL( url );
			auto urlStr = url.toString();
			auto hash = String::hash( urlStr );
			scrollView->getVerticalScrollBar()->setValue( 0 );
			ui->loadLayoutFromString( HTMLFormatter::HTMLtoXML( data ), mainContainer, hash );
			urlBar->setText( urlStr );

			if ( useHNDark && url.getAuthority() == "news.ycombinator.com" ) {
				static const std::string_view HN_DARK = R"css(
				  body * {
				    color: #dcdccc !important;
				  }
				  body,
				  #hnmain,
				  .pagetop {
				    background-color: #404040 !important;
				  }
				  body > center > table > tbody > tr:first-child * {
				    background-color: #505050 !important;
				  }
				  body > center > table > tbody > tr:first-child * a:hover {
				    background: #404040 !important;
				  }
				  body code, body pre, body input, body textarea {
				    background: #505050 !important;
				  }
				  body a {
				    color: #7F9F7F !important;
				  }
				  body .subtext a {
				    color: #dcdccc !important;
				  }
				  body a:visited, body a:visited span {
				    color: #CC9393 !important;
				  }
				  body a:hover, body a:hover span {
				    background: #505050 !important;
				  }
				)css";

				StyleSheetParser parser;
				if ( parser.loadFromString( HN_DARK ) )
					ui->getStyleSheet().combineStyleSheet( parser.getStyleSheet() );
			}
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
	loadDocument( !url.Get().empty() ? url.Get() : "https://news.ycombinator.com" );

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

	if ( benchmarkMode.Get() ) {
		app.getWindow()->runMainLoop( [&app]() {
			app.getWindow()->getInput()->update();
			SceneManager::instance()->update();
			app.getWindow()->clear();
			SceneManager::instance()->draw();
			auto tm = app.getUI()->getUIThemeManager();
			String fps( String::format( "FPS: %d", app.getWindow()->getFPS() ) );
			Text::draw( fps, Vector2f::Zero, tm->getDefaultFont(), tm->getDefaultFontSize(),
						Color::magenta, 0, 0, Color::Black, Color::Black, Vector2f{ 1, 1 }, 4,
						TextHints::AllAscii );
			app.getWindow()->display();
		} );
		return EXIT_SUCCESS;
	}
	return app.run();
}
