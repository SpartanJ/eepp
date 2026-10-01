#include <eepp/ee.hpp>
#include <eepp/graphics/texturedrawable.hpp>

#include <args/args.hxx>
#include <iostream>
#include <unordered_map>

using namespace EE::UI::Tools;

struct BrowserTabs : UITabWidgetSplitter::Client {
	UITabWidgetSplitter* splitter{ nullptr };
	UITextInput* urlBar{ nullptr };
	UIPushButton* backBtn{ nullptr };
	UIPushButton* fwdBtn{ nullptr };

	UIWebView* current() const {
		return splitter && splitter->getCurWidget() ? splitter->getCurWidget()->asType<UIWebView>()
													: nullptr;
	}

	UITabWidget* currentTabWidget() const {
		return splitter ? splitter->tabWidgetFromWidget( current() ) : nullptr;
	}

	void cycleTab( bool next ) const {
		auto* tabs = currentTabWidget();
		if ( !tabs || tabs->getTabCount() == 0 )
			return;
		const auto count = tabs->getTabCount();
		const auto index = tabs->getTabSelectedIndex();
		tabs->setTabSelected( next ? ( index + 1 ) % count : ( index + count - 1 ) % count );
	}

	void updateNavigation() const {
		UIWebView* view = current();
		backBtn->setEnabled( view && view->canGoBack() );
		fwdBtn->setEnabled( view && view->canGoForward() );
	}

	void onTabCreated( UITab*, UIWidget* ) override {}

	void onWidgetFocusChange( UIWidget* widget ) override {
		auto* view = widget->asType<UIWebView>();
		urlBar->setText( view->getCurrentURI().toString() );
		updateNavigation();
	}
};

EE_MAIN_FUNC int main( int argc, char** argv ) {
	std::shared_ptr<ThreadPool> threadPool(
		ThreadPool::createShared( eemax<int>( 4, Sys::getCPUCount() ) ) );
	Http::setThreadPool( threadPool );

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
	const std::unordered_map<std::string, FontHinting> fontHintingMap{
		{ "none", FontHinting::None },
		{ "slight", FontHinting::Slight },
		{ "full", FontHinting::Full },
	};
	args::MapFlag<std::string, FontHinting> fontHinting(
		parser, "font-hinting", "Font hinting mode (accepted values: none, slight, full)",
		{ "font-hinting" }, fontHintingMap, FontHinting::Full );
	const std::unordered_map<std::string, FontAntialiasing> fontAntialiasingMap{
		{ "none", FontAntialiasing::None },
		{ "grayscale", FontAntialiasing::Grayscale },
		{ "subpixel", FontAntialiasing::Subpixel },
	};
	args::MapFlag<std::string, FontAntialiasing> fontAntialiasing(
		parser, "font-antialiasing",
		"Font antialiasing mode (accepted values: none, grayscale, subpixel)",
		{ "font-antialiasing" }, fontAntialiasingMap, FontAntialiasing::Grayscale );

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

	UIApplication::Settings appSettings( {}, pixelDensityConf ? pixelDensityConf.Get() : 0.f );
	appSettings.fontHinting = fontHinting.Get();
	appSettings.fontAntialiasing = fontAntialiasing.Get();

	UIApplication app(
		WindowSettings{ 1280, 720, "eepp - UI HTML Example", WindowStyle::Default,
						WindowBackend::Default, 32, Sys::getProcessPath() + "assets/icon/ee.png" },
		appSettings,
		ContextSettings( false,
						 benchmarkMode.Get() ? 0 : ContextSettings::FrameRateLimitScreenRefreshRate,
						 4 ) );

	Log::instance()->setLogLevelThreshold( LogLevel::Debug );
	Log::instance()->setLogToStdOut( !Runtime::isOffscreen() );
	Log::instance()->setLiveWrite( true );

	Http::setDefaultUserAgent( "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like "
							   "Gecko) Chrome/148.0.0.0 Safari/537.36" );

	auto win = app.getWindow();
	if ( !win->isOpen() )
		return EXIT_FAILURE;

	auto ui = app.getUI();
	ui->setThreadPool( threadPool );

	ResourceScope& resourceScope = *ui->getResourceScope();
	FontTrueTypePtr remixIconFont =
		FontTrueType::New( "icon", "assets/fonts/remixicon.ttf", resourceScope );
	FontTrueTypePtr noniconsFont =
		FontTrueType::New( "nonicons", "assets/fonts/nonicons.ttf", resourceScope );
	FontTrueTypePtr codIconFont =
		FontTrueType::New( "codicon", "assets/fonts/codicon.ttf", resourceScope );
	ui->getUIIconThemeManager()->setCurrentTheme(
		IconManager::init( "icons", remixIconFont.get(), noniconsFont.get(), codIconFont.get() ) );

	ui->setColorSchemePreference(
		!prefersColorScheme.Get().empty()
			? ColorSchemePreferences::fromStringExt( prefersColorScheme.Get() )
			: ColorSchemeExtPreference::System );

	bool useHNDark = hnDark.Get();

	auto vbox = ui->loadLayoutFromString( R"xml(
	<style>
		#tabs_host TabWidget { max-tab-width: 200dp; }
		#tabs_host Tab > Tab::Text { text-overflow: ellipsis; }
		PushButton.webview_ui {
			border-top-color: transparent;
			border-right-color: transparent;
			border-bottom-color: transparent;
			border-left-color: transparent;
		}
		PushButton.webview_ui:hover {
			border-top-color: var(--primary);
			border-right-color: var(--primary);
			border-bottom-color: var(--primary);
			border-left-color: var(--primary);
		}
	</style>
	<vbox layout_width="match_parent" layout_height="match_parent">
		<hbox layout_width="match_parent" layout_height="wrap_content" padding-bottom="1dp">
			<PushButton lw="26dp" id="backbtn" class="webview_ui" text="@string(back, Back)"
				icon="icon(arrow-left-s, 22dp)"
				text-as-fallback="true" />
			<PushButton lw="26dp" id="fwdbtn"  class="webview_ui" text="@string(forward, Forward)"
				icon="icon(arrow-right-s, 22dp)"
				text-as-fallback="true" />
			<PushButton lw="26dp" id="refreshbtn"  class="webview_ui" text="@string(refresh, Refresh)"
				icon="icon(refresh, 18dp)"
				text-as-fallback="true" />
			<PushButton lw="26dp" id="newtabbtn" class="webview_ui" text="New Tab"
				icon="icon(add, 18dp)" text-as-fallback="true" />
			<TextInput id="url_bar" layout_width="0" layout_weight="1"
				hint="@string(enter_address, Enter Address)" />
		</hbox>
		<vbox id="tabs_host" layout_width="match_parent" layout_height="0" layout_weight="1" />
	</vbox>
	)xml",
										  nullptr, app.getStyleSheetDefaultMarker() );

	auto urlBar = ui->find( "url_bar" )->asType<UITextInput>();
	auto backBtn = ui->find( "backbtn" )->asType<UIPushButton>();
	auto fwdBtn = ui->find( "fwdbtn" )->asType<UIPushButton>();
	auto refreshBtn = ui->find( "refreshbtn" )->asType<UIPushButton>();
	auto newTabBtn = ui->find( "newtabbtn" )->asType<UIPushButton>();

	const std::string configPath = Sys::getConfigPath( "eepp-ui-html" );
	const std::string cookiePath =
		configPath.empty() ? std::string() : configPath + FileSystem::getOSSlash() + "cookies.txt";
	auto cookies = std::make_shared<CookieManager>();
	if ( !cookiePath.empty() )
		cookies->loadFromFile( cookiePath );
	auto saveCookies = [&]() {
		if ( !cookiePath.empty() &&
			 ( FileSystem::isDirectory( configPath ) ||
			   FileSystem::makeDir( configPath, true, 0700 ) ) &&
			 !cookies->saveToFile( cookiePath ) )
			Log::error( "Could not save UI HTML cookies to %s", cookiePath.c_str() );
	};
	auto cache = WebResourceCache::New();
	const auto cachePartition = cache->createPartition();
	BrowserTabs browser;
	browser.urlBar = urlBar;
	browser.backBtn = backBtn;
	browser.fwdBtn = fwdBtn;
	std::unique_ptr<UITabWidgetSplitter> splitter( UITabWidgetSplitter::New( &browser, ui ) );
	splitter->setShowTabBarWhenSplit( true );
	browser.splitter = splitter.get();
	splitter->setOnTabWidgetCreateCb( [&browser, &splitter]( UITabWidget* tabWidget ) {
		tabWidget->on( Event::OnTabClosed, [&browser, &splitter]( const Event* ) {
			if ( !browser.current() )
				splitter->setCurrentWidget( splitter->getSomeWidget() );
			if ( !browser.current() ) {
				browser.urlBar->setText( "" );
				browser.updateNavigation();
			}
		} );
	} );
	splitter->createTabWidget( vbox->find( "tabs_host" ) );

	std::function<void( UIWebView* )> configureNavigation;
	auto newTab = [&]( bool focus = true ) {
		auto* view = UIWebView::New();
		view->setStyleSheetDefaultMarker( app.getStyleSheetDefaultMarker() );
		view->setCookieManager( cookies );
		view->setWebResourceCache( cache, cachePartition );
		view->setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::MatchParent );
		auto* target = browser.current() ? splitter->tabWidgetFromWidget( browser.current() )
										 : splitter->getFirstTabWidget();
		splitter->createWidgetInTabWidget( target, view, "New Tab", focus );
		view->onNavigationStarted( [&browser, view]( const URI& uri ) {
			if ( browser.current() == view )
				browser.urlBar->setText( uri.toString() );
		} );
		view->on( Event::OnCreateContextMenu, [view, cookies, cache, ui,
											   &saveCookies]( const Event* event ) {
			auto* context = static_cast<const ContextMenuEvent*>( event );
			auto* menu = context->getMenu();
			std::string linkHref;
			for ( const Node* node = context->getTarget(); node; node = node->getParent() ) {
				if ( node->isType( UI_TYPE_TEXTSPAN ) ) {
					auto* span = node->asConstType<UITextSpan>();
					if ( span->getElementTag() == "a" ) {
						auto* link = static_cast<const UIAnchorSpan*>( span );
						if ( !link->getHref().empty() )
							linkHref = view->getDocumentSceneNode()
										   ->solveRelativePath( URI( link->getHref() ) )
										   .toString();
						break;
					}
				}
				if ( node == view->getDocumentContainer() )
					break;
			}
			if ( !linkHref.empty() )
				menu->add( "Open Link in New Tab" )->setId( "open-link-new-tab" );
			menu->addSeparator();
			auto* clearDomain = menu->add( "Clear Domain Cookies" );
			clearDomain->setId( "clear-domain-cookies" );
			clearDomain->setEnabled( !view->getCurrentURI().getAuthority().empty() );
			menu->add( "Clear All Cookies" )->setId( "clear-all-cookies" );
			menu->add( "Inspect" )->setId( "inspect" );
			menu->on( Event::OnItemClicked,
					  [view, cookies, cache, ui, &saveCookies,
					   linkHref = std::move( linkHref )]( const Event* itemEvent ) {
						  const auto& id = itemEvent->getNode()->getId();
						  if ( id == "open-link-new-tab" ) {
							  NavigationRequest request{ URI( linkHref ) };
							  request.target = NavigationRequest::Target::NewTab;
							  view->getDocumentSceneNode()->navigate( request );
						  } else if ( id == "clear-domain-cookies" ) {
							  cookies->clearDomain( view->getCurrentURI().getAuthority() );
							  cache->clear();
							  saveCookies();
							  view->refresh();
						  } else if ( id == "clear-all-cookies" ) {
							  cookies->clear();
							  cache->clear();
							  saveCookies();
							  view->refresh();
						  } else if ( id == "inspect" ) {
							  UIWidgetInspector::create( ui );
						  }
					  } );
		} );
		configureNavigation( view );
		return view;
	};

	configureNavigation = [useHNDark, &browser, &splitter, &newTab]( UIWebView* webView ) {
		webView->on( Event::OnLinkOpenRequested, [&newTab]( const Event* event ) {
			auto* request = static_cast<const UIWebView::LinkOpenEvent*>( event );
			auto* view = newTab( false );
			view->loadURI( request->uri );
			request->accept();
		} );
		webView->on( Event::OnFaviconChanged, [webView, &splitter]( const Event* event ) {
			if ( auto* tab = splitter->getTabFromWidget( webView ) ) {
				const auto& icon = static_cast<const UIWebView::FaviconEvent*>( event )->icon;
				tab->setIcon( icon ? TextureDrawable::New( icon ) : DrawablePtr{} );
				if ( icon )
					tab->getIcon()->setSize( 16, 16 );
			}
		} );
		webView->on( Event::OnTitleChanged, [webView, &splitter]( const Event* event ) {
			const auto& title = static_cast<const UIWebView::TitleEvent*>( event )->title;
			if ( auto* tab = splitter->getTabFromWidget( webView ) ) {
				const URI& uri = webView->getCurrentURI();
				tab->setText( title.empty() ? ( uri.getAuthority().empty() ? uri.toString()
																		   : uri.getAuthority() )
											: title );
			}
		} );
		webView->onNavigationCompleted( [webView, &browser, &splitter,
										 useHNDark]( const URI& uri ) {
			if ( auto* tab = splitter->getTabFromWidget( webView ) )
				tab->setText( uri.getAuthority().empty() ? uri.toString() : uri.getAuthority() );
			if ( browser.current() == webView ) {
				browser.updateNavigation();
				browser.urlBar->setText( uri.toString() );
			}

			if ( useHNDark && uri.getAuthority() == "news.ycombinator.com" ) {
				static const std::string_view HN_DARK = R"css(
			  body * {
			    color: #dcdccc !important;
			  }
			  body,
			  #hnmain {
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
					webView->getDocumentSceneNode()->combineStyleSheet( parser.getStyleSheet() );
			}
		} );
	};

	ui->setKeyBindingCommand( "go-back", [&browser] {
		if ( auto* view = browser.current() ) {
			view->goHistoryBack();
			browser.updateNavigation();
		}
	} );

	ui->setKeyBindingCommand( "go-forward", [&browser] {
		if ( auto* view = browser.current() ) {
			view->goHistoryForward();
			browser.updateNavigation();
		}
	} );

	ui->setKeyBindingCommand( "reload", [&browser] {
		if ( auto* view = browser.current() )
			view->refresh();
	} );

	ui->setKeyBindingCommand( "focus-address-bar", [urlBar] {
		urlBar->setFocus();
		urlBar->getDocument().selectAll();
	} );
	ui->setKeyBindingCommand( "new-tab", [&newTab, ui] {
		newTab();
		ui->executeKeyBindingCommand( "focus-address-bar" );
	} );
	ui->setKeyBindingCommand( "close-tab", [&browser, &splitter] {
		if ( auto* view = browser.current() ) {
			if ( splitter->tryTabClose( view, UITabWidget::FocusTabBehavior::Default ) )
				splitter->closeTab( view, UITabWidget::FocusTabBehavior::Default );
		}
	} );
	ui->setKeyBindingCommand( "next-tab", [&browser] { browser.cycleTab( true ); } );
	ui->setKeyBindingCommand( "previous-tab", [&browser] { browser.cycleTab( false ); } );
	ui->setKeyBindingCommand( "switch-to-last-tab", [&browser] {
		if ( auto* tabs = browser.currentTabWidget(); tabs && tabs->getTabCount() )
			tabs->setTabSelected( tabs->getTabCount() - 1 );
	} );

	for ( Uint32 i = 1; i <= 9; ++i ) {
		const auto command = String::format( "switch-to-tab-%u", i );
		ui->setKeyBindingCommand( command, [&browser, i] {
			if ( auto* tabs = browser.currentTabWidget() )
				tabs->setTabSelected( i - 1 );
		} );
		ui->getKeyBindings().addKeybindString( String::format( "mod+%u", i ), command );
	}
	ui->getKeyBindings().addKeybindsString( {
		{ "mod+0", "switch-to-last-tab" },
		{ "mod+t", "new-tab" },
		{ "mod+w", "close-tab" },
		{ "mod+tab", "next-tab" },
		{ "mod+shift+tab", "previous-tab" },
		{ "ctrl+pagedown", "next-tab" },
		{ "ctrl+pageup", "previous-tab" },
		{ "mod+l", "focus-address-bar" },
		{ "alt+d", "focus-address-bar" },
		{ "f6", "focus-address-bar" },
		{ "mod+r", "reload" },
		{ "f5", "reload" },
		{ "alt+left", "go-back" },
		{ "alt+right", "go-forward" },
	} );

	backBtn->onClick( [ui]( const MouseEvent* ) { ui->executeKeyBindingCommand( "go-back" ); } );
	fwdBtn->onClick( [ui]( const MouseEvent* ) { ui->executeKeyBindingCommand( "go-forward" ); } );
	refreshBtn->onClick( [ui]( const MouseEvent* ) { ui->executeKeyBindingCommand( "reload" ); } );
	newTabBtn->onClick( [ui]( const MouseEvent* ) { ui->executeKeyBindingCommand( "new-tab" ); } );

	urlBar->on( Event::OnPressEnter, [&browser, urlBar]( auto ) {
		if ( auto* view = browser.current() )
			view->loadURI( urlBar->getText().toUtf8() );
	} );

	auto* firstView = newTab();
	firstView->loadURI( !url.Get().empty() ? url.Get() : "https://news.ycombinator.com" );
	browser.updateNavigation();

	win->getInput()->pushCallback( [&browser]( InputEvent* event ) {
		UIWebView* view = browser.current();
		if ( !view )
			return;
		switch ( event->Type ) {
			case InputEvent::FileDropped: {
				std::string file( event->file.file );
				view->loadURI( "file://" + file );
				break;
			}
			case InputEvent::TextDropped: {
				view->loadURI( event->textdrop.text );
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
		saveCookies();
		return EXIT_SUCCESS;
	}
	int result = app.run();
	saveCookies();
	return result;
}
