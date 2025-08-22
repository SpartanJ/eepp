#include <args/args.hxx>
#include <eepp/ee.hpp>
#include <eterm/terminal/terminaldisplay.hpp>
#include <iostream>

EE::Window::Window* win = NULL;
std::shared_ptr<TerminalDisplay> terminal = nullptr;
Clock lastRender;
Clock secondsCounter;
Time frameTime{ Time::Zero };
bool benchmarkMode{ false };
std::string windowStringData;
std::map<std::string, TerminalColorScheme> terminalColorSchemes;

void loadColorSchemes( const std::string& resPath ) {
	auto configPath = Sys::getConfigPath( "eterm" );
	auto colorSchemes =
		TerminalColorScheme::loadFromFile( resPath + "colorschemes/terminalcolorschemes.conf" );
	auto colorSchemesPath = configPath + FileSystem::getOSSlash() + "colorschemes";
	if ( FileSystem::isDirectory( colorSchemesPath ) ) {
		auto colorSchemesFiles = FileSystem::filesGetInPath( colorSchemesPath );
		for ( auto& file : colorSchemesFiles ) {
			auto colorSchemesInFile = TerminalColorScheme::loadFromFile( file );
			std::copy( colorSchemesInFile.begin(), colorSchemesInFile.end(),
					   std::back_inserter( colorSchemes ) );
		}
	}
	for ( auto colorScheme : colorSchemes )
		terminalColorSchemes.insert( { colorScheme.getName(), colorScheme } );
}

void inputCallback( InputEvent* event ) {
	if ( !terminal || event->Type == InputEvent::EventsSent )
		return;

	switch ( event->Type ) {
		case InputEvent::MouseMotion: {
			terminal->onMouseMove( win->getInput()->getMousePos(),
								   win->getInput()->getPressTrigger() );
			break;
		}
		case InputEvent::MouseButtonDown: {
			terminal->onMouseDown( win->getInput()->getMousePos(),
								   win->getInput()->getPressTrigger() );
#if EE_PLATFORM == EE_PLATFORM_ANDROID
			win->startTextInput();
#endif
			break;
		}
		case InputEvent::MouseButtonUp: {
			terminal->onMouseUp( win->getInput()->getMousePos(),
								 win->getInput()->getReleaseTrigger() );

			if ( win->getInput()->getDoubleClickTrigger() ) {
				terminal->onMouseDoubleClick( win->getInput()->getMousePos(),
											  win->getInput()->getDoubleClickTrigger() );
			}

			break;
		}
		case InputEvent::Window: {
			switch ( event->window.type ) {
				case InputEvent::WindowKeyboardFocusLost:
				case InputEvent::WindowKeyboardFocusGain: {
					terminal->setFocus( win->hasFocus() );
					break;
				}
			}
			break;
		}
		case InputEvent::KeyUp: {
			break;
		}
		case InputEvent::KeyDown: {
			terminal->onKeyDown( event->key.keysym.sym, event->key.keysym.unicode,
								 event->key.keysym.mod, event->key.keysym.scancode );
#if EE_PLATFORM == EE_PLATFORM_ANDROID
			if ( event->key.keysym.sym == KEY_RETURN ||
				 event->key.keysym.scancode == SCANCODE_RETURN ) {
				win->startTextInput();
			}
#endif
			break;
		}
		case InputEvent::TextInput: {
			terminal->onTextInput( event->text.text );
			break;
		}
		case InputEvent::TextEditing: {
			terminal->onTextEditing( event->textediting.text, event->textediting.start,
									 event->textediting.length );

			break;
		}
		case InputEvent::VideoExpose:
			terminal->setFocus( win->hasFocus() );
			terminal->invalidate();
			break;
		case InputEvent::VideoResize: {
			terminal->setPosition( { 0, 0 } );
			terminal->setSize( win->getSize().asFloat() );
			break;
		}
	}
}

EE_MAIN_FUNC int main( int argc, char* argv[] ) {
#ifdef EE_DEBUG
	Log::instance()->setLogToStdOut( true );
	Log::instance()->setLiveWrite( true );
#endif
	args::ArgumentParser parser( "eterm" );
	args::HelpFlag help( parser, "help", "Display this help menu", { 'h', "help" } );
	args::ValueFlag<std::string> shell( parser, "shell", "Shell name or path", { 's', "shell" },
										"" );
	args::ValueFlag<std::string> shellArgs( parser, "shell-args", "Shell command line arguments",
											{ "shell-args" }, "" );
	args::ValueFlag<size_t> historySize( parser, "scrollback", "Maximum history size (lines)",
										 { 'l', "scrollback" }, 10000 );
	args::Flag fb( parser, "framebuffer", "Use frame buffer (more memory usage, less CPU usage)",
				   { "fb", "framebuffer" } );
	args::ValueFlag<std::string> fontPath( parser, "fontpath", "Font path", { 'f', "font" } );
	args::ValueFlag<std::string> fallbackFontPathF( parser, "fallback-fontpath",
													"Fallback Font path", { "fallback-font" } );
	args::ValueFlag<Float> fontSize( parser, "fontsize", "Font size (in dp)", { "fontsize" }, 11 );
	args::ValueFlag<Float> width( parser, "winwidth", "Window width (in dp)", { "width" }, 1280 );
	args::ValueFlag<Float> height( parser, "winheight", "Window height (in dp)", { "height" },
								   720 );
	args::ValueFlag<Float> pixelDensityConf( parser, "pixel-density",
											 "Set default application pixel density",
											 { 'd', "pixel-density" } );
	args::Positional<std::string> wd( parser, "wording-dir", "Working Directory / executable" );
	args::Flag closeOnExit( parser, "close-on-exit",
							"close the application when the executable exits", { 'c', "close" } );
	args::ValueFlag<std::string> executeInShell(
		parser, "execute-in-shell", "execute program in shell", { 'e', "execute" }, "" );
	args::Flag vsync( parser, "vsync", "Enable vsync", { "vsync" } );
	args::ValueFlag<std::string> colorScheme( parser, "color-scheme", "Load color scheme",
											  { "color-scheme" }, "" );
	args::Flag listColorSchemes( parser, "color-schemes", "Lists color schemes",
								 { "list-color-schemes" } );
	args::ValueFlag<Uint32> maxFPS( parser, "max-fps",
									"Maximum rendering frames per second of the terminal. Default "
									"value will be the refresh rate of the screen.",
									{ "max-fps" }, 0 );
	args::Flag benchmarkModeFlag(
		parser, "benchmark-mode",
		"Render as much as possible to measure the rendering performance.", { "benchmark-mode" } );

	try {
		parser.ParseCLI( argc, argv );
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

	DisplayManager* displayManager = Engine::instance()->getDisplayManager();
	Display* currentDisplay = displayManager->getDisplayIndex( 0 );

	std::string resPath = Sys::getProcessPath();
#if EE_PLATFORM == EE_PLATFORM_MACOS
	if ( String::contains( resPath, "ecode.app" ) ) {
		resPath = FileSystem::getCurrentWorkingDirectory();
		FileSystem::dirAddSlashAtEnd( resPath );
	}
#elif EE_PLATFORM == EE_PLATFORM_LINUX
	if ( String::contains( resPath, ".mount_" ) ) {
		resPath = FileSystem::getCurrentWorkingDirectory();
		FileSystem::dirAddSlashAtEnd( resPath );
	}
#elif EE_PLATFORM == EE_PLATFORM_EMSCRIPTEN
	resPath += "eterm/";
#endif
	resPath += "assets";
	FileSystem::dirAddSlashAtEnd( resPath );

	if ( listColorSchemes.Get() || colorScheme )
		loadColorSchemes( resPath );

	if ( listColorSchemes.Get() ) {
		std::cout << "Color schemes:\n";
		for ( const auto& tcs : terminalColorSchemes )
			std::cout << "\t" << tcs.first << "\n";
		return EXIT_SUCCESS;
	}

	displayManager->enableScreenSaver();
	displayManager->enableMouseFocusClickThrough();
	displayManager->disableBypassCompositor();

	Sizei winSize( width.Get(), height.Get() );
	win = Engine::instance()->createWindow(
		WindowSettings( winSize.getWidth(), winSize.getHeight(), "eterm", WindowStyle::Default,
						WindowBackend::Default, 32, resPath + "icon/eterm.png",
						pixelDensityConf ? pixelDensityConf.Get()
										 : currentDisplay->getPixelDensity() ),
		ContextSettings( vsync.Get() ) );

	if ( win->isOpen() ) {
		win->setClearColor( RGB( 0, 0, 0 ) );

		benchmarkMode = benchmarkModeFlag.Get();

		FontTrueType* fontMono = nullptr;
		if ( fontPath && FileSystem::fileExists( fontPath.Get() ) ) {
			FileInfo file( fontPath.Get() );
			fontMono = FontTrueType::New( "monospace" );
			if ( fontMono->loadFromFile( file.getFilepath() ) ) {
				FontFamily::loadFromRegular( fontMono );
			} else {
				fontMono = nullptr;
			}
		}
		if ( fontMono == nullptr ) {
			fontMono = FontTrueType::New( "monospace" );
			fontMono->loadFromFile( resPath + "fonts/DejaVuSansMonoNerdFontComplete.ttf" );
			FontFamily::loadFromRegular( fontMono, "DejaVuSansMono" );
		}

		if ( FileSystem::fileExists( resPath + "fonts/NotoColorEmoji.ttf" ) ) {
			FontTrueType::New( "emoji-color" )
				->loadFromFile( resPath + "fonts/NotoColorEmoji.ttf" );
		} else if ( FileSystem::fileExists( resPath + "fonts/NotoEmoji-Regular.ttf" ) ) {
			FontTrueType::New( "emoji-font" )
				->loadFromFile( resPath + "fonts/NotoEmoji-Regular.ttf" );
		}

		std::string fallbackFontPath( fallbackFontPathF
										  ? fallbackFontPathF.Get()
										  : resPath + "fonts/DroidSansFallbackFull.ttf" );
		if ( FileSystem::fileExists( fallbackFontPath ) ) {
			FontTrueType* fallbackFont = FontTrueType::New( "fallback-font" );
			if ( fallbackFont->loadFromFile( fallbackFontPath ) )
				FontManager::instance()->addFallbackFont( fallbackFont );
		}

		Float realMaxFPS = maxFPS.Get() ? maxFPS.Get() : currentDisplay->getRefreshRate();
		frameTime = benchmarkMode ? Time::Zero : Milliseconds( 1000.f / realMaxFPS );

		FileInfo file( wd ? wd.Get() : FileSystem::getCurrentWorkingDirectory() );
		terminal = TerminalDisplay::create(
			win, fontMono, PixelDensity::dpToPx( fontSize.Get() ), win->getSize().asFloat(),
			file.isRegularFile() && file.isExecutable() ? file.getFilepath() : shell.Get(),
			shellArgs ? String::split( shellArgs.Get() ) : std::vector<std::string>(),
			file.getDirectoryPath(), historySize.Get(), nullptr, fb.Get(),
			!( file.isRegularFile() && file.isExecutable() ) );

		if ( terminal == nullptr ) {
			win->close();
			win->showMessageBox( EE::Window::Window::MessageBoxType::Error, "eterm",
								 "Operating System not supported." );
			terminal.reset();
			Engine::destroySingleton();
			MemoryManager::showResults();
			return EXIT_FAILURE;
		}

		terminal->getTerminal()->setAllowMemoryTrimnming( true );
		terminal->pushEventCallback( [&closeOnExit]( const TerminalDisplay::Event& event ) {
			if ( event.type == TerminalDisplay::EventType::TITLE ) {
				windowStringData = event.eventData;
				win->setTitle( "eterm - " + windowStringData );
			} else if ( event.type == TerminalDisplay::EventType::PROCESS_EXIT &&
						closeOnExit.Get() ) {
				win->close();
			}
		} );
		if ( shell )
			terminal->setKeepAlive( false );

		if ( colorScheme ) {
			auto selColorScheme = terminalColorSchemes.find( colorScheme.Get() );
			if ( selColorScheme != terminalColorSchemes.end() )
				terminal->setColorScheme( selColorScheme->second );
		}

		if ( !executeInShell.Get().empty() )
			terminal->executeFile( executeInShell.Get() );

		win->startTextInput();

		win->getInput()->pushCallback( &inputCallback );

		win->runMainLoop( [] {
			bool termNeedsUpdate = false;
			win->getInput()->update();

			if ( terminal )
				termNeedsUpdate = !terminal->update();

			if ( terminal && ( benchmarkMode || terminal->isDirty() ) &&
				 ( !termNeedsUpdate || lastRender.getElapsedTime() >= frameTime ) ) {
				lastRender.restart();
				win->clear();
				terminal->draw();
				win->display();
			} else if ( !benchmarkMode && !termNeedsUpdate ) {
				win->getInput()->waitEvent( Milliseconds( win->hasFocus() ? 16 : 100 ) );
			}

			if ( benchmarkMode && secondsCounter.getElapsedTime() >= Seconds( 1 ) ) {
				win->setTitle( "eterm - " + windowStringData + " - " +
							   String::toString( win->getFPS() ) + " FPS" );
				secondsCounter.restart();
			}
		} );
	}

	terminal.reset();

	Engine::destroySingleton();

	MemoryManager::showResults();

	return EXIT_SUCCESS;
}
