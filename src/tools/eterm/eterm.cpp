#include <args/args.hxx>
#include <eepp/ee.hpp>
#include <eterm/terminal/terminaldisplay.hpp>

EE::Window::Window* win = NULL;
std::shared_ptr<TerminalDisplay> terminal = nullptr;

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

void mainLoop() {
	win->getInput()->update();

	if ( terminal )
		terminal->update();

	if ( terminal && terminal->isDirty() ) {
		win->clear();
		terminal->draw();
		win->display();
	} else {
		win->getInput()->waitEvent( Milliseconds( win->hasFocus() ? 16 : 100 ) );
	}
}

EE_MAIN_FUNC int main( int argc, char* argv[] ) {
#ifdef EE_DEBUG
	Log::instance()->setConsoleOutput( true );
	Log::instance()->setLiveWrite( true );
#endif
	args::ArgumentParser parser( "eterm" );
	args::HelpFlag help( parser, "help", "Display this help menu", { 'h', "help" } );
	args::ValueFlag<std::string> shell( parser, "shell", "Shell name or path", { 's', "shell" },
										"" );
	args::ValueFlag<size_t> historySize( parser, "scrollback", "Maximum history size (lines)",
										 { 'l', "scrollback" }, 10000 );
	args::Flag fb( parser, "framebuffer", "Use frame buffer (more memory usage, less CPU usage)",
				   { "fb", "framebuffer" } );
	args::ValueFlag<std::string> fontPath( parser, "fontpath", "Font path", { 'f', "font" } );
	args::ValueFlag<Float> fontSize( parser, "fontsize", "Font size (in dp)", { 's', "fontsize" },
									 11 );
	args::ValueFlag<Float> width( parser, "winwidth", "Window width (in dp)", { "width" }, 1280 );
	args::ValueFlag<Float> height( parser, "winheight", "Window height (in dp)", { "height" },
								   720 );
	args::ValueFlag<Float> pixelDenstiyConf( parser, "pixel-density",
											 "Set default application pixel density",
											 { 'd', "pixel-density" } );
	args::Positional<std::string> wd( parser, "wording-dir", "Working Directory" );

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
#if EE_PLATFORM == EE_PLATFORM_MACOSX
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

	displayManager->enableScreenSaver();
	displayManager->enableMouseFocusClickThrough();
	displayManager->disableBypassCompositor();

	Sizei winSize( width.Get(), height.Get() );
	win = Engine::instance()->createWindow(
		WindowSettings( winSize.getWidth(), winSize.getHeight(), "eterm", WindowStyle::Default,
						WindowBackend::Default, 32, resPath + "icon/ee.png",
						pixelDenstiyConf ? pixelDenstiyConf.Get()
										 : currentDisplay->getPixelDensity() ),
		ContextSettings( true ) );

	if ( win->isOpen() ) {
		win->setClearColor( RGB( 0, 0, 0 ) );

		FontTrueType* fontMono = nullptr;
		if ( fontPath && FileSystem::fileExists( fontPath.Get() ) ) {
			FileInfo file( fontPath.Get() );
			fontMono = FontTrueType::New( "monospace" );
			if ( !fontMono->loadFromFile( file.getFilepath() ) )
				fontMono = nullptr;
		}
		if ( fontMono == nullptr ) {
			fontMono = FontTrueType::New( "monospace" );
			fontMono->loadFromFile( resPath + "fonts/DejaVuSansMonoNerdFontComplete.ttf" );
			fontMono->setEnableEmojiFallback( false );
		}

		if ( FileSystem::fileExists( resPath + "fonts/NotoColorEmoji.ttf" ) ) {
			FontTrueType::New( "emoji-color" )
				->loadFromFile( resPath + "fonts/NotoColorEmoji.ttf" );
		} else if ( FileSystem::fileExists( resPath + "fonts/NotoEmoji-Regular.ttf" ) ) {
			FontTrueType::New( "emoji-font" )
				->loadFromFile( resPath + "fonts/NotoEmoji-Regular.ttf" );
		}

		if ( !terminal || terminal->hasTerminated() ) {
			terminal = TerminalDisplay::create(
				win, fontMono, PixelDensity::dpToPx( fontSize.Get() ), win->getSize().asFloat(),
				shell.Get(), {}, wd ? wd.Get() : FileSystem::getCurrentWorkingDirectory(),
				historySize.Get(), nullptr, fb.Get() );
			terminal->pushEventCallback( [&]( const TerminalDisplay::Event& event ) {
				if ( event.type == TerminalDisplay::EventType::TITLE )
					win->setTitle( "eterm - " + event.eventData );
			} );

			win->startTextInput();
		}

		win->getInput()->pushCallback( &inputCallback );

		win->runMainLoop( &mainLoop );
	}

	terminal.reset();

	Engine::destroySingleton();

	MemoryManager::showResults();

	return EXIT_SUCCESS;
}
