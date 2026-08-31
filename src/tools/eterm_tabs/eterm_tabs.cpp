#include <args/args.hxx>
#include <eepp/core/small_vector.hpp>
#include <eepp/ee.hpp>
#include <eepp/ui/uiapplication.hpp>
#include <eepp/ui/uilinearlayout.hpp>
#include <eepp/ui/uimessagebox.hpp>
#include <eepp/ui/uitabwidget.hpp>
#include <eterm/ui/uiterminal.hpp>

#include <algorithm>
#include <iostream>
#include <iterator>
#include <map>
#include <unordered_map>

using namespace EE;
using namespace EE::Graphics;
using namespace EE::Scene;
using namespace EE::System;
using namespace EE::UI;
using namespace EE::Window;
using namespace eterm::Terminal;
using namespace eterm::UI;

namespace {

struct TerminalLaunchConfig {
	std::string program;
	std::vector<std::string> arguments;
	std::string workingDirectory;
	std::string executeInShell;
	size_t historySize{ 10000 };
	TerminalCursorMode cursorStyle{ TerminalCursorMode::SteadyUnderline };
	FontHinting fontHinting{ FontHinting::Full };
	FontAntialiasing fontAntialiasing{ FontAntialiasing::Grayscale };
	bool useFrameBuffer{ false };
	bool keepAlive{ true };
	bool closeOnExit{ false };
};

EE::Window::Window* appWindow{ nullptr };
UISceneNode* scene{ nullptr };
UILinearLayout* mainLayout{ nullptr };
UITabWidget* tabs{ nullptr };
FontTrueType* terminalFont{ nullptr };
UIMessageBox* closeDialog{ nullptr };
UITab* closeDialogTab{ nullptr };
TerminalLaunchConfig terminalConfig;
std::map<std::string, TerminalColorScheme> terminalColorSchemes;
const TerminalColorScheme* selectedColorScheme{ nullptr };
Float terminalFontSize{ 12 };
bool warnBeforeClose{ false };
bool closeApproved{ false };
bool benchmarkMode{ false };
Clock secondsCounter;
SmallVector<UITab*, 8> pendingExitCloseTabs;

std::string getResourcePath() {
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
	return resPath;
}

void loadColorSchemes( const std::string& resPath ) {
	auto colorSchemes =
		TerminalColorScheme::loadFromFile( resPath + "colorschemes/terminalcolorschemes.conf" );
	const std::string configColorSchemesPath =
		Sys::getConfigPath( "eterm" ) + FileSystem::getOSSlash() + "colorschemes";
	if ( FileSystem::isDirectory( configColorSchemesPath ) ) {
		for ( const auto& file : FileSystem::filesGetInPath( configColorSchemesPath ) ) {
			auto fileColorSchemes = TerminalColorScheme::loadFromFile( file );
			colorSchemes.insert( colorSchemes.end(),
								 std::make_move_iterator( fileColorSchemes.begin() ),
								 std::make_move_iterator( fileColorSchemes.end() ) );
		}
	}
	for ( auto& colorScheme : colorSchemes ) {
		std::string name = colorScheme.getName();
		terminalColorSchemes.emplace( std::move( name ), std::move( colorScheme ) );
	}
}

UITerminal* terminalFromTab( UITab* tab ) {
	return tab && tab->getOwnedWidget() && tab->getOwnedWidget()->isType( UI_TYPE_TERMINAL )
			   ? tab->getOwnedWidget()->asType<UITerminal>()
			   : nullptr;
}

void updateWindowTitle() {
	if ( !appWindow )
		return;
	std::string title{ "eterm" };
	if ( tabs ) {
		if ( auto* terminal = terminalFromTab( tabs->getTabSelected() );
			 terminal && !terminal->getTitle().empty() ) {
			title += " - ";
			title += terminal->getTitle();
		}
	}
	if ( benchmarkMode ) {
		title += " - ";
		title += String::toString( appWindow->getFPS() );
		title += " FPS";
	}
	appWindow->setTitle( title );
}

bool hasRunningChildren( UITab* tab ) {
	auto* terminal = terminalFromTab( tab );
	return terminal && terminal->getTerm() &&
		   Sys::processHasChildren( terminal->getTerm()->getProcessId() );
}

void closeTab( UITab* tab ) {
	if ( !tabs || !tab || tabs->getTabIndex( tab ) == eeINDEX_NOT_FOUND )
		return;
	tabs->removeTab( tab );
}

void queueExitCloseTab( UITab* tab ) {
	if ( tab && std::find( pendingExitCloseTabs.begin(), pendingExitCloseTabs.end(), tab ) ==
					pendingExitCloseTabs.end() ) {
		pendingExitCloseTabs.emplace_back( tab );
	}
}

void queueExitedTabs() {
	if ( !terminalConfig.closeOnExit )
		return;
	tabs->forEachTab( []( UITab* tab ) {
		auto* terminal = terminalFromTab( tab );
		if ( !terminal || !terminal->getTerm() )
			return;
		const auto& session = terminal->getTerm()->getSession();
		auto snapshot = session ? session->snapshot() : nullptr;
		if ( snapshot && snapshot->processExited )
			queueExitCloseTab( tab );
	} );
}

void requestCloseTab( UITab* tab ) {
	if ( !warnBeforeClose || !hasRunningChildren( tab ) ) {
		closeTab( tab );
		return;
	}
	if ( closeDialog )
		return;
	closeDialog = UIMessageBox::New(
		UIMessageBox::OK_CANCEL,
		"Are you sure you want to close this terminal?\nIt is still running a process." );
	closeDialogTab = tab;
	closeDialog->setTitle( "eterm" );
	closeDialog->on( Event::OnConfirm, []( const Event* ) { closeTab( closeDialogTab ); } );
	closeDialog->on( Event::OnClose, []( const Event* ) {
		closeDialog = nullptr;
		closeDialogTab = nullptr;
	} );
	closeDialog->center();
	closeDialog->showWhenReady();
}

void addTabKeyBindings( UITerminal* terminal, UITab* tab );

UITerminal* createTerminal() {
	Sizef initialSize{ 16, 16 };
	if ( tabs && tabs->getContainerNode() &&
		 tabs->getContainerNode()->getPixelsSize() != Sizef::Zero ) {
		initialSize = tabs->getContainerNode()->getPixelsSize();
	}

	auto* terminal = UITerminal::New(
		terminalFont, terminalFontSize, initialSize, terminalConfig.program,
		terminalConfig.arguments, {}, terminalConfig.workingDirectory, terminalConfig.historySize,
		nullptr, terminalConfig.useFrameBuffer, terminalConfig.keepAlive );
	if ( !terminal || !terminal->getTerm() ) {
		eeSAFE_DELETE( terminal );
		return nullptr;
	}

	terminal->getTerm()->setAllowMemoryTrimming( true );
	terminal->getTerm()->setCursorMode( terminalConfig.cursorStyle );
	terminal->getTerm()->setFontHinting( terminalConfig.fontHinting );
	terminal->getTerm()->setFontAntialiasing( terminalConfig.fontAntialiasing );
	if ( selectedColorScheme )
		terminal->setColorScheme( *selectedColorScheme );

	auto* tab = tabs->add( "Terminal", terminal );
	addTabKeyBindings( terminal, tab );
	terminal->on( Event::OnTitleChange, [tab, terminal]( const Event* ) {
		tab->setText( terminal->getTitle().empty() ? "Terminal" : terminal->getTitle() );
		if ( tabs->getTabSelected() == tab )
			updateWindowTitle();
	} );
	terminal->getTerm()->pushEventCallback( [tab]( const TerminalDisplay::Event& event ) {
		if ( terminalConfig.closeOnExit && event.type == TerminalDisplay::EventType::PROCESS_EXIT )
			queueExitCloseTab( tab );
	} );
	tabs->setTabSelected( tab );
	if ( !terminalConfig.executeInShell.empty() )
		terminal->executeFile( terminalConfig.executeInShell );
	terminal->setFocus();
	updateWindowTitle();
	return terminal;
}

void addTabKeyBindings( UITerminal* terminal, UITab* tab ) {
	terminal->setCommand( "create-new-terminal", [] { createTerminal(); } );
	terminal->setCommand( "close-tab", [tab] { requestCloseTab( tab ); } );
	terminal->setCommand( "next-tab", [] { tabs->focusNextTab(); } );
	terminal->setCommand( "previous-tab", [] { tabs->focusPreviousTab(); } );
	terminal->addKeyBinding( { KEY_T, KeyMod::getDefaultModifier() | KEYMOD_SHIFT },
							 "create-new-terminal" );
	terminal->addKeyBinding( { KEY_W, KeyMod::getDefaultModifier() | KEYMOD_SHIFT }, "close-tab" );
	terminal->addKeyBinding( { KEY_PAGEDOWN, KEYMOD_CTRL }, "next-tab" );
	terminal->addKeyBinding( { KEY_PAGEUP, KEYMOD_CTRL }, "previous-tab" );
	terminal->addKeyBinding( { KEY_TAB, KEYMOD_CTRL }, "next-tab" );
	terminal->addKeyBinding( { KEY_TAB, KEYMOD_CTRL | KEYMOD_SHIFT }, "previous-tab" );
}

bool closeWindow( EE::Window::Window* ) {
	if ( closeApproved || !warnBeforeClose )
		return true;
	bool running = false;
	tabs->forEachTab( [&running]( UITab* tab ) { running |= hasRunningChildren( tab ); } );
	if ( !running )
		return true;
	if ( closeDialog )
		return false;
	closeDialog = UIMessageBox::New(
		UIMessageBox::OK_CANCEL,
		"Are you sure you want to close this window? It is still running a process." );
	closeDialog->setTitle( "eterm" );
	closeDialog->on( Event::OnConfirm, []( const Event* ) {
		closeApproved = true;
		appWindow->close();
	} );
	closeDialog->on( Event::OnClose, []( const Event* ) {
		closeDialog = nullptr;
		closeDialogTab = nullptr;
	} );
	closeDialog->center();
	closeDialog->showWhenReady();
	return false;
}

} // namespace

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
	args::ValueFlag<std::string> fallbackFontPath( parser, "fallback-fontpath",
												   "Fallback Font path", { "fallback-font" } );
	args::ValueFlag<Float> fontSize( parser, "fontsize", "Font size (in dp)", { "fontsize" }, 11 );
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
	args::ValueFlag<Float> width( parser, "winwidth", "Window width (in dp)", { "width" }, 1280 );
	args::ValueFlag<Float> height( parser, "winheight", "Window height (in dp)", { "height" },
								   720 );
	args::ValueFlag<Float> pixelDensity( parser, "pixel-density",
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
	args::MapFlag<std::string, TerminalCursorMode> cursorStyle(
		parser, "cursor-style",
		"Sets the cursor-style (accepted values: blinking_block, steady_block, blink_underline, "
		"steady_underline, blink_bar, steady_bar)",
		{ "cursor-style" }, TerminalCursorHelper::getTerminalCursorModeMap(),
		TerminalCursorMode::SteadyUnderline );
	args::Flag benchmarkModeFlag(
		parser, "benchmark-mode",
		"Render as much as possible to measure the rendering performance.", { "benchmark-mode" } );
	args::Flag warnBeforeCloseFlag(
		parser, "warn-before-closing",
		"Prompts for confirmation if a program is still running when closing the terminal.",
		{ "warn-before-closing" } );
	args::ValueFlag<size_t> initialTabs( parser, "tabs", "Number of initial terminal tabs",
										 { "tabs" }, 1 );

	try {
		parser.ParseCLI( argc, argv );
	} catch ( const args::Help& ) {
		std::cout << parser;
		return EXIT_SUCCESS;
	} catch ( const args::ParseError& error ) {
		std::cerr << error.what() << std::endl;
		std::cerr << parser;
		return EXIT_FAILURE;
	} catch ( args::ValidationError& error ) {
		std::cerr << error.what() << std::endl;
		std::cerr << parser;
		return EXIT_FAILURE;
	}

	const std::string initialWorkingDirectory = FileSystem::getCurrentWorkingDirectory();
	const std::string resPath = getResourcePath();
	if ( listColorSchemes.Get() || colorScheme )
		loadColorSchemes( resPath );
	if ( listColorSchemes.Get() ) {
		std::cout << "Color schemes:\n";
		for ( const auto& colorSchemeEntry : terminalColorSchemes )
			std::cout << "\t" << colorSchemeEntry.first << "\n";
		return EXIT_SUCCESS;
	}
	if ( colorScheme ) {
		auto colorSchemeIt = terminalColorSchemes.find( colorScheme.Get() );
		if ( colorSchemeIt != terminalColorSchemes.end() )
			selectedColorScheme = &colorSchemeIt->second;
	}

	DisplayManager* displayManager = Engine::instance()->getDisplayManager();
	Display* currentDisplay = displayManager->getDisplayIndex( 0 );
	if ( !currentDisplay ) {
		std::cerr << "Display not found, exiting" << std::endl;
		return EXIT_FAILURE;
	}

	Sizei windowSize( width.Get(), height.Get() );
	const auto displaySize = currentDisplay->getUsableBounds().getSize();
	if ( displaySize.getWidth() > 0 && windowSize.getWidth() >= displaySize.getWidth() )
		windowSize.setWidth( static_cast<int>( displaySize.getWidth() * 0.8f ) );
	if ( displaySize.getHeight() > 0 && windowSize.getHeight() >= displaySize.getHeight() )
		windowSize.setHeight( static_cast<int>( displaySize.getHeight() * 0.75f ) );

	UIApplication::Settings appSettings;
	appSettings.basePath = FileSystem::removeLastFolderFromPath( resPath );
	appSettings.pixelDensity =
		pixelDensity ? pixelDensity.Get() : currentDisplay->getPixelDensity();
	appSettings.fontHinting = fontHinting.Get();
	appSettings.fontAntialiasing = fontAntialiasing.Get();
	const Int32 frameRateLimit =
		benchmarkModeFlag.Get()
			? 0
			: static_cast<Int32>( maxFPS.Get() ? maxFPS.Get() : currentDisplay->getRefreshRate() );
	UIApplication app( WindowSettings( windowSize.getWidth(), windowSize.getHeight(), "eterm",
									   WindowStyle::Default, WindowBackend::Default, 32,
									   resPath + "icon/eterm.png",
									   appSettings.pixelDensity.value() ),
					   appSettings, ContextSettings( vsync.Get(), frameRateLimit ) );
	appWindow = app.getWindow();
	scene = app.getUI();
	if ( !appWindow || !appWindow->isOpen() || !scene )
		return EXIT_FAILURE;
	FileSystem::changeWorkingDirectory( initialWorkingDirectory );
	appWindow->setClearColor( RGB( 0, 0, 0 ) );

	auto& resourceScope = *scene->getResourceScope();
	if ( fontPath && FileSystem::fileExists( fontPath.Get() ) ) {
		terminalFont = FontTrueType::New( "eterm-monospace", resourceScope ).get();
		if ( terminalFont->loadFromFile( fontPath.Get() ) )
			FontFamily::loadFromRegular( terminalFont );
		else
			terminalFont = nullptr;
	}
	if ( !terminalFont ) {
		terminalFont = FontTrueType::New( "eterm-monospace", resourceScope ).get();
		if ( !terminalFont->loadFromFile( resPath + "fonts/DejaVuSansMonoNerdFontComplete.ttf" ) ) {
			std::cerr << "Could not load terminal font" << std::endl;
			return EXIT_FAILURE;
		}
		FontFamily::loadFromRegular( terminalFont, "DejaVuSansMono" );
	}

	if ( fallbackFontPath ) {
		if ( FileSystem::fileExists( fallbackFontPath.Get() ) ) {
			auto fallback = FontTrueType::New( "eterm-fallback-font", resourceScope );
			if ( fallback->loadFromFile( fallbackFontPath.Get() ) )
				resourceScope.getFontService().addFallbackFont( std::move( fallback ) );
		}
	} else if ( auto fallback = resourceScope.findFont( "DroidSansFallbackFull" ) ) {
		resourceScope.getFontService().addFallbackFont( std::move( fallback ) );
	}

	const std::string launchPath = wd ? wd.Get() : initialWorkingDirectory;
	FileInfo launchFile( launchPath );
	const bool launchExecutable = launchFile.isRegularFile() && launchFile.isExecutable();
	terminalConfig.program = launchExecutable ? launchFile.getFilepath() : shell.Get();
	terminalConfig.arguments =
		shellArgs ? String::split( shellArgs.Get() ) : std::vector<std::string>{};
	terminalConfig.workingDirectory = launchFile.getDirectoryPath();
	terminalConfig.executeInShell = executeInShell.Get();
	terminalConfig.historySize = historySize.Get();
	terminalConfig.cursorStyle = cursorStyle.Get();
	terminalConfig.fontHinting = fontHinting.Get();
	terminalConfig.fontAntialiasing = fontAntialiasing.Get();
	terminalConfig.useFrameBuffer = fb.Get();
	terminalConfig.keepAlive = !launchExecutable && !shell;
	terminalConfig.closeOnExit = closeOnExit.Get();
	warnBeforeClose = warnBeforeCloseFlag.Get();
	benchmarkMode = benchmarkModeFlag.Get();
	terminalFontSize = PixelDensity::dpToPx( fontSize.Get() );

	mainLayout = UILinearLayout::NewVertical();
	mainLayout->setParent( scene->getRoot() );
	mainLayout->setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::MatchParent );
	mainLayout->setPixelsSize( appWindow->getSize().asFloat() );

	tabs = UITabWidget::New();
	tabs->setParent( mainLayout );
	tabs->setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::MatchParent );
	tabs->setTabsClosable( true );
	tabs->setAllowRearrangeTabs( true );
	tabs->setHideTabBarOnSingleTab( true );
	tabs->setTabTryCloseCallback( []( UITab* tab, UITabWidget::FocusTabBehavior ) {
		if ( warnBeforeClose && hasRunningChildren( tab ) ) {
			requestCloseTab( tab );
			return false;
		}
		return true;
	} );
	tabs->on( Event::OnTabSelected, []( const Event* ) { updateWindowTitle(); } );
	tabs->on( Event::OnTabClosed, []( const Event* event ) {
		auto* closedTab = static_cast<const TabEvent*>( event )->getTab();
		pendingExitCloseTabs.erase(
			std::remove( pendingExitCloseTabs.begin(), pendingExitCloseTabs.end(), closedTab ),
			pendingExitCloseTabs.end() );
		if ( closeDialogTab == closedTab )
			closeDialogTab = nullptr;
		if ( tabs->getTabCount() == 0 )
			appWindow->close();
		else
			updateWindowTitle();
	} );
	mainLayout->updateLayout();

	for ( size_t tab = 0; tab < eemax( static_cast<size_t>( 1 ), initialTabs.Get() ); ++tab ) {
		if ( !createTerminal() ) {
			appWindow->showMessageBox( EE::Window::Window::MessageBoxType::Error, "eterm",
									   "Operating System not supported." );
			return EXIT_FAILURE;
		}
	}

	appWindow->setCloseRequestCallback( &closeWindow );
	app.setShowMemoryManagerResult( true );
	appWindow->runMainLoop( [] {
		appWindow->getInput()->update();
		SceneManager::instance()->update();
		queueExitedTabs();
		// Process-exit events are drained from UITerminal scheduled updates. Removing a tab from
		// that callback would mutate the scheduled-widget set while it is being traversed.
		while ( !pendingExitCloseTabs.empty() ) {
			auto* tab = pendingExitCloseTabs.back();
			pendingExitCloseTabs.pop_back();
			closeTab( tab );
		}
		if ( benchmarkMode || scene->invalidated() ) {
			appWindow->clear();
			SceneManager::instance()->draw();
			appWindow->display();
		} else {
#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN
			appWindow->getInput()->waitEvent( Milliseconds( appWindow->hasFocus() ? 16 : 100 ) );
#endif
		}
		if ( benchmarkMode && secondsCounter.getElapsedTime() >= Seconds( 1 ) ) {
			updateWindowTitle();
			secondsCounter.restart();
		}
	} );
	return EXIT_SUCCESS;
}
