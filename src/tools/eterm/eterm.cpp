#include <args/args.hxx>
#include <eepp/core/small_vector.hpp>
#include <eepp/ee.hpp>
#include <eepp/ui/iconmanager.hpp>
#include <eepp/ui/tools/uitabwidgetsplitter.hpp>
#include <eepp/ui/tools/uiwidgetinspector.hpp>
#include <eepp/ui/uiapplication.hpp>
#include <eepp/ui/uilinearlayout.hpp>
#include <eepp/ui/uimessagebox.hpp>
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
using namespace EE::UI::Tools;
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

class EtermApp {
  public:
	int run( int argc, char* argv[] );

	class TerminalSplitterClient : public UITabWidgetSplitter::Client {
	  public:
		explicit TerminalSplitterClient( EtermApp& app ) : mApp( app ) {}

		void onTabCreated( UITab* tab, UIWidget* ) override;
		void onWidgetFocusChange( UIWidget* ) override;

	  private:
		EtermApp& mApp;
	};

  private:
	std::string getResourcePath() const;
	String i18n( const std::string& key, const String& defaultValue ) const;
	void loadColorSchemes( const std::string& resPath );
	static UITerminal* terminalFromTab( UITab* tab );
	void updateWindowTitle();
	bool hasTerminals() const;
	static bool hasRunningChildren( UITab* tab );
	void closeTab( UITab* tab );
	void queueExitCloseTab( UITab* tab );
	void queueExitedTabs();
	void requestCloseTab( UITab* tab );
	void renameSession( UITerminal* terminal );
	void maximizeTabWidget( UITabWidget* tabWidget );
	void restoreMaximizedTabWidget();
	void configureTab( UITab* tab );
	UITerminal* createTerminal( UITabWidget* target = nullptr );
	UITerminal* createTerminalSplit( SplitDirection direction, UITerminal* terminal );
	void addTabKeyBindings( UITerminal* terminal );
	bool closeWindow( EE::Window::Window* );

	EE::Window::Window* appWindow{ nullptr };
	UISceneNode* scene{ nullptr };
	UILinearLayout* mainLayout{ nullptr };
	UITabWidgetSplitter* tabSplitter{ nullptr };
	FontTrueType* terminalFont{ nullptr };
	UIIcon* terminalIcon{ nullptr };
	UIMessageBox* closeDialog{ nullptr };
	UIWidget* closeDialogWidget{ nullptr };
	UIWindow* maximizedTabWidgetWindow{ nullptr };
	UITabWidget* maximizedTabWidget{ nullptr };
	UINodeLink* maximizedTabWidgetLink{ nullptr };
	TerminalLaunchConfig terminalConfig;
	std::map<std::string, TerminalColorScheme> terminalColorSchemes;
	const TerminalColorScheme* selectedColorScheme{ nullptr };
	Float terminalFontSize{ 12 };
	bool warnBeforeClose{ false };
	bool closeApproved{ false };
	bool benchmarkMode{ false };
	Clock secondsCounter;
	SmallVector<UITab*, 8> pendingExitCloseTabs;
	TerminalSplitterClient splitterClient{ *this };
};

void EtermApp::TerminalSplitterClient::onTabCreated( UITab* tab, UIWidget* ) {
	if ( mApp.terminalIcon )
		tab->setIcon( mApp.terminalIcon->createDrawable( PixelDensity::dpToPxI( 12 ) ) );
	mApp.configureTab( tab );
}

void EtermApp::TerminalSplitterClient::onWidgetFocusChange( UIWidget* ) {
	mApp.updateWindowTitle();
}

std::string EtermApp::getResourcePath() const {
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

String EtermApp::i18n( const std::string& key, const String& defaultValue ) const {
	return scene ? scene->i18n( key, defaultValue ) : defaultValue;
}

void EtermApp::loadColorSchemes( const std::string& resPath ) {
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

UITerminal* EtermApp::terminalFromTab( UITab* tab ) {
	return tab && tab->getOwnedWidget() && tab->getOwnedWidget()->isType( UI_TYPE_TERMINAL )
			   ? tab->getOwnedWidget()->asType<UITerminal>()
			   : nullptr;
}

void EtermApp::updateWindowTitle() {
	if ( !appWindow )
		return;
	String title{ "eterm" };
	if ( tabSplitter ) {
		if ( auto* terminal = tabSplitter->getCurWidget() &&
									  tabSplitter->getCurWidget()->isType( UI_TYPE_TERMINAL )
								  ? tabSplitter->getCurWidget()->asType<UITerminal>()
								  : nullptr;
			 terminal && !terminal->getTitle().empty() ) {
			title += " - ";
			title += terminal->getTitle();
		}
	}
	if ( benchmarkMode ) {
		title += " - ";
		title += String::toString( appWindow->getFPS() );
		title += " " + i18n( "fps", "FPS" );
	}
	appWindow->setTitle( title );
}

bool EtermApp::hasTerminals() const {
	bool found = false;
	if ( tabSplitter )
		tabSplitter->forEachWidgetStoppable( [&found]( UIWidget* ) {
			found = true;
			return true;
		} );
	return found;
}

bool EtermApp::hasRunningChildren( UITab* tab ) {
	auto* terminal = terminalFromTab( tab );
	return terminal && terminal->getTerm() &&
		   Sys::processHasChildren( terminal->getTerm()->getProcessId() );
}

void EtermApp::closeTab( UITab* tab ) {
	if ( !tabSplitter || !tab || !tab->getOwnedWidget() )
		return;
	tabSplitter->closeTab( tab->getOwnedWidget()->asType<UIWidget>(),
						   UITabWidget::FocusTabBehavior::Default );
}

void EtermApp::queueExitCloseTab( UITab* tab ) {
	if ( tab && std::find( pendingExitCloseTabs.begin(), pendingExitCloseTabs.end(), tab ) ==
					pendingExitCloseTabs.end() ) {
		pendingExitCloseTabs.emplace_back( tab );
	}
}

void EtermApp::queueExitedTabs() {
	if ( !terminalConfig.closeOnExit )
		return;
	tabSplitter->forEachTab( [this]( UITab* tab ) {
		auto* terminal = terminalFromTab( tab );
		if ( !terminal || !terminal->getTerm() )
			return;
		const auto& session = terminal->getTerm()->getSession();
		auto snapshot = session ? session->snapshot() : nullptr;
		if ( snapshot && snapshot->processExited )
			queueExitCloseTab( tab );
	} );
}

void EtermApp::requestCloseTab( UITab* tab ) {
	if ( !warnBeforeClose || !hasRunningChildren( tab ) ) {
		closeTab( tab );
		return;
	}
	if ( closeDialog )
		return;
	closeDialog = UIMessageBox::New(
		UIMessageBox::OK_CANCEL,
		i18n( "close_terminal_running_process_confirm",
			  "Are you sure you want to close this terminal?\nIt is still running a process." ) );
	closeDialogWidget = tab->getOwnedWidget()->asType<UIWidget>();
	closeDialog->setTitle( "eterm" );
	closeDialog->on( Event::OnConfirm, [this]( const Event* ) {
		if ( closeDialogWidget && tabSplitter->ownedWidgetExists( closeDialogWidget ) )
			tabSplitter->closeTab( closeDialogWidget, UITabWidget::FocusTabBehavior::Default );
	} );
	closeDialog->on( Event::OnClose, [this]( const Event* ) {
		closeDialog = nullptr;
		closeDialogWidget = nullptr;
	} );
	closeDialog->center();
	closeDialog->showWhenReady();
}

void EtermApp::renameSession( UITerminal* terminal ) {
	if ( !terminal )
		return;
	auto* msgBox =
		UIMessageBox::New( UIMessageBox::INPUT, i18n( "new_terminal_name", "New terminal name:" ) );
	msgBox->setTitle( "eterm" );
	msgBox->getTextInput()->setHint( i18n( "any_name_ellipsis", "Any name..." ) );
	msgBox->setCloseShortcut( { KEY_ESCAPE, KEYMOD_NONE } );
	msgBox->on( Event::OnConfirm, [msgBox, terminal]( const Event* ) {
		terminal->setTitle( msgBox->getTextInput()->getText().toUtf8() );
		msgBox->close();
		terminal->setFocus();
	} );
	msgBox->showWhenReady();
}

void EtermApp::maximizeTabWidget( UITabWidget* tabWidget ) {
	if ( !tabWidget || scene->getRoot()->hasChild( "detached_tab_widget_win" ) )
		return;

	UIWindow::StyleConfig winCfg;
	winCfg.WinFlags = UI_WIN_SHADOW | UI_WIN_MODAL | UI_WIN_EPHEMERAL | UI_WIN_NO_DECORATION;
	auto* win = UIWindow::NewOpt( UIWindow::SIMPLE_LAYOUT, winCfg );
	maximizedTabWidgetWindow = win;
	maximizedTabWidget = tabWidget;
	win->setPixelsSize( scene->getPixelsSize() - PixelDensity::dpToPx( 64 ) );
	win->setId( "detached_tab_widget_win" );
	win->getModalWidget()->onClick( [this]( auto ) { restoreMaximizedTabWidget(); } );
	win->setAnchors( UI_ANCHOR_TOP | UI_ANCHOR_LEFT | UI_ANCHOR_BOTTOM | UI_ANCHOR_RIGHT );
	win->toFront();
	win->center();
	win->setKeyBindingCommand( "close-maximized-tab-widget",
							   [this] { restoreMaximizedTabWidget(); } );
	win->getKeyBindings().addKeybind( { KEY_ESCAPE }, "close-maximized-tab-widget" );
	win->setCheckEphemeralCloseFn( [this]( Node* focusNode ) {
		if ( focusNode->isType( UI_TYPE_POPUPMENU ) )
			return false;
		if ( focusNode->getSceneNode()->isUISceneNode() ) {
			auto* sceneNode = static_cast<UISceneNode*>( focusNode->getSceneNode() );
			auto* widgetTreeView = sceneNode->getRoot()->hasChild( "widget-tree-view" );
			if ( widgetTreeView &&
				 ( focusNode == widgetTreeView || widgetTreeView->inParentTreeOf( focusNode ) ) )
				return false;
		}
		return true;
	} );
	auto* tabWidgetParent = tabWidget->getParent();
	const bool wasFirstSplit = tabWidgetParent->isType( UI_TYPE_SPLITTER ) &&
							   tabWidgetParent->asType<UISplitter>()->getFirstWidget() == tabWidget;
	auto* nodeLink = UINodeLink::NewLink( tabWidget );
	maximizedTabWidgetLink = nodeLink;
	if ( wasFirstSplit )
		nodeLink->setClass( "was_first_split" );
	tabWidget->setParent( win );
	tabWidget->setId( "detached_tab_widget" );
	tabWidget->setPixelsPosition( Vector2f::Zero );
	tabWidget->setPixelsSize( win->getContainer()->getPixelsSize() );
	tabWidget->setAnchors( UI_ANCHOR_TOP | UI_ANCHOR_LEFT | UI_ANCHOR_BOTTOM | UI_ANCHOR_RIGHT );
	if ( !tabWidget->inParentTreeOf( scene->getEventDispatcher()->getFocusNode() ) )
		tabWidget->getTabSelected()->getOwnedWidget()->setFocus();
	win->on( Event::OnWindowClose, [this]( auto ) { restoreMaximizedTabWidget(); } );
	nodeLink->setParent( tabWidgetParent );
	nodeLink->setId( "nodelink_tab_widget" );
	if ( wasFirstSplit && tabWidgetParent->isType( UI_TYPE_SPLITTER ) )
		tabWidgetParent->asType<UISplitter>()->swap();

	auto* fullscreenImg = UIImage::New();
	fullscreenImg->unsetFlags( UI_AUTO_SIZE );
	fullscreenImg->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	fullscreenImg->setParent( win );
	fullscreenImg->setSize( { 24, tabWidget->getTabBar()->getSize().getHeight() } );
	fullscreenImg->setPosition( { tabWidget->getSize().getWidth() - 24, 0 } );
	fullscreenImg->setAnchors( UI_ANCHOR_TOP | UI_ANCHOR_RIGHT );
	if ( auto* icon = scene->findIcon( "fullscreen" ) )
		fullscreenImg->setDrawable( icon->createDrawable( PixelDensity::dpToPxI( 16 ) ) );
	fullscreenImg->setVerticalAlign( UI_VALIGN_CENTER );
	fullscreenImg->setHorizontalAlign( UI_HALIGN_CENTER );
	fullscreenImg->addClass( "pseudo_anchor" );
	fullscreenImg->toFront();
	fullscreenImg->onClick( [this]( const Event* ) {
		restoreMaximizedTabWidget();
		appWindow->getCursorManager()->set( Cursor::SysType::SysArrow );
	} );
}

void EtermApp::restoreMaximizedTabWidget() {
	if ( !scene || !SceneManager::isActive() )
		return;
	if ( !maximizedTabWidgetLink || !maximizedTabWidget )
		return;
	auto* nodeLink = maximizedTabWidgetLink;
	auto* curTabWidget = maximizedTabWidget;
	auto* win = maximizedTabWidgetWindow;
	auto* splitterParent = nodeLink->getParent();
	nodeLink->setParent( scene );
	curTabWidget->setParent( splitterParent );
	curTabWidget->setAnchors( 0 );
	curTabWidget->setId( "" );
	if ( nodeLink->hasClass( "was_first_split" ) && splitterParent->isType( UI_TYPE_SPLITTER ) ) {
		splitterParent->asType<UISplitter>()->swap();
	}
	nodeLink->close();
	maximizedTabWidgetLink = nullptr;
	maximizedTabWidget = nullptr;
	maximizedTabWidgetWindow = nullptr;
	if ( win && !win->isClosing() )
		win->close();
}

void EtermApp::configureTab( UITab* tab ) {
	tab->on( Event::OnCreateContextMenu, [this]( const Event* event ) {
		const auto* menuEvent = static_cast<const ContextMenuEvent*>( event );
		auto* menu = menuEvent->getMenu();
		auto* clickedTab = event->getNode()->asType<UITab>();
		auto* terminal = terminalFromTab( clickedTab );
		if ( !menu || !terminal || !clickedTab->getTabWidget() )
			return;

		const auto addItem = [this, menu]( const String& text, const std::string& icon,
										   const std::string& command ) {
			DrawablePtr drawable;
			if ( auto* menuIcon = scene->findIcon( icon ) )
				drawable = menuIcon->createDrawable( PixelDensity::dpToPxI( 12 ) );
			auto* item = menu->add( text, std::move( drawable ) );
			item->setId( command );
			return item;
		};
		addItem( i18n( "close_tab", "Close Tab" ), "document-close", "close-tab" );
		addItem( i18n( "close_other_tabs", "Close Other Tabs" ), "", "close-other-tabs" );
		addItem( i18n( "close_clean_tabs", "Close Clean Tabs" ), "", "close-clean-tabs" );
		addItem( i18n( "close_all_tabs", "Close All Tabs" ), "", "close-all-tabs" );
		addItem( i18n( "close_tabs_to_the_left", "Close Tabs To The Left" ), "",
				 "close-tabs-to-the-left" );
		addItem( i18n( "close_tabs_to_the_right", "Close Tabs To The Right" ), "",
				 "close-tabs-to-the-right" );

		menu->addSeparator();
		auto* exclusiveMode =
			menu->addCheckBox( i18n( "enable_exclusive_mode", "Enable Exclusive Mode" ),
							   terminal->getExclusiveMode() );
		exclusiveMode->setId( UITerminal::getExclusiveModeToggleCommandName() );
		addItem( i18n( "rename_session", "Rename Session" ), "", "terminal-rename" );

		menu->addSeparator();
		const bool canMove = clickedTab->getTabWidget()->getTabCount() > 1;
		addItem( i18n( "move_tab_to_start", "Move Tab To Start" ), "window", "move-tab-to-start" )
			->setEnabled( canMove );
		addItem( i18n( "move_tab_to_end", "Move Tab To End" ), "window", "move-tab-to-end" )
			->setEnabled( canMove );

		menu->addSeparator();
		addItem( scene->getRoot()->hasChild( "detached_tab_widget_win" )
					 ? i18n( "restore_maximized_tab_widget", "Restore Maximized Tab Widget" )
					 : i18n( "maximize_tab_widget", "Maximize Tab Widget" ),
				 "fullscreen",
				 scene->getRoot()->hasChild( "detached_tab_widget_win" )
					 ? "restore-maximized-tab-widget"
					 : "maximize-tab-widget" );

		menu->on( Event::OnItemClicked, [this, clickedTab, terminal]( const Event* itemEvent ) {
			if ( !itemEvent->getNode()->isType( UI_TYPE_MENUITEM ) )
				return;
			const auto& command = itemEvent->getNode()->getId();
			auto* previous = tabSplitter->getCurWidget();
			tabSplitter->setCurrentWidget( terminal );

			if ( command == "close-clean-tabs" ) {
				tabSplitter->tryCloseAllTabs( terminal, UITabWidget::FocusTabBehavior::Default );
			} else if ( command == "move-tab-to-start" ) {
				clickedTab->getTabWidget()->moveTab( clickedTab, 0 );
			} else if ( command == "move-tab-to-end" ) {
				clickedTab->getTabWidget()->moveTab( clickedTab,
													 clickedTab->getTabWidget()->getTabCount() );
			} else if ( command == "maximize-tab-widget" ) {
				maximizeTabWidget( clickedTab->getTabWidget() );
			} else if ( command == "restore-maximized-tab-widget" ) {
				restoreMaximizedTabWidget();
			} else {
				terminal->execute( command );
			}

			if ( previous && tabSplitter->ownedWidgetExists( previous ) )
				tabSplitter->setCurrentWidget( previous );
		} );
	} );
}

UITerminal* EtermApp::createTerminal( UITabWidget* target ) {
	if ( !target && tabSplitter ) {
		auto* current = tabSplitter->getCurWidget();
		target = current ? tabSplitter->tabWidgetFromWidget( current )
						 : tabSplitter->getFirstTabWidget();
	}
	if ( !target )
		return nullptr;
	Sizef initialSize{ 16, 16 };
	if ( target->getContainerNode() &&
		 target->getContainerNode()->getPixelsSize() != Sizef::Zero ) {
		initialSize = target->getContainerNode()->getPixelsSize();
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

	auto* tab =
		tabSplitter->createWidgetInTabWidget( target, terminal, i18n( "terminal", "Terminal" ) )
			.first;
	addTabKeyBindings( terminal );
	terminal->on( Event::OnTitleChange, [this, tab, terminal]( const Event* ) {
		tab->setText( terminal->getTitle().empty() ? i18n( "terminal", "Terminal" )
												   : String( terminal->getTitle() ) );
		if ( tabSplitter->getCurWidget() == terminal )
			updateWindowTitle();
	} );
	terminal->getTerm()->pushEventCallback( [this, tab]( const TerminalDisplay::Event& event ) {
		if ( terminalConfig.closeOnExit && event.type == TerminalDisplay::EventType::PROCESS_EXIT )
			queueExitCloseTab( tab );
	} );
	tabSplitter->setCurrentWidget( terminal );
	if ( !terminalConfig.executeInShell.empty() )
		terminal->executeFile( terminalConfig.executeInShell );
	terminal->setFocus();
	updateWindowTitle();
	return terminal;
}

UITerminal* EtermApp::createTerminalSplit( SplitDirection direction, UITerminal* terminal ) {
	auto* source = terminal ? tabSplitter->tabWidgetFromWidget( terminal ) : nullptr;
	auto* target = source ? tabSplitter->splitTabWidget( direction, source ) : nullptr;
	return target ? createTerminal( target ) : nullptr;
}

void EtermApp::addTabKeyBindings( UITerminal* terminal ) {
	tabSplitter->registerSplitterCommands( *terminal );
	terminal->setCommand( "create-new-terminal", [this] { createTerminal(); } );
	terminal->setCommand( "debug-widget-tree-view",
						  [this] { UIWidgetInspector::create( scene ); } );
	terminal->setCommand( "terminal-rename", [this, terminal] { renameSession( terminal ); } );
	terminal->setCommand( UITerminal::getExclusiveModeToggleCommandName(), [terminal] {
		terminal->setExclusiveMode( !terminal->getExclusiveMode() );
	} );
	terminal->setCommand( "next-tab", [this, terminal] {
		if ( auto* tabs = tabSplitter->tabWidgetFromWidget( terminal ) )
			tabs->focusNextTab();
	} );
	terminal->setCommand( "previous-tab", [this, terminal] {
		if ( auto* tabs = tabSplitter->tabWidgetFromWidget( terminal ) )
			tabs->focusPreviousTab();
	} );
	terminal->setCommand( "split-right", [this, terminal] {
		createTerminalSplit( SplitDirection::Right, terminal );
	} );
	terminal->setCommand( "split-bottom", [this, terminal] {
		createTerminalSplit( SplitDirection::Bottom, terminal );
	} );
	terminal->setCommand(
		"split-left", [this, terminal] { createTerminalSplit( SplitDirection::Left, terminal ); } );
	terminal->setCommand(
		"split-top", [this, terminal] { createTerminalSplit( SplitDirection::Top, terminal ); } );
	terminal->setCommand( "switch-to-previous-split",
						  [this, terminal] { tabSplitter->switchPreviousSplit( terminal ); } );
	terminal->setCommand( "switch-to-next-split",
						  [this, terminal] { tabSplitter->switchNextSplit( terminal ); } );
	terminal->addKeyBinding( { KEY_T, KeyMod::getDefaultModifier() | KEYMOD_SHIFT },
							 "create-new-terminal" );
	terminal->addKeyBinding( { KEY_W, KeyMod::getDefaultModifier() | KEYMOD_SHIFT }, "close-tab" );
	terminal->addKeyBinding( { KEY_F11, KeyMod::getDefaultModifier() | KEYMOD_SHIFT },
							 "debug-widget-tree-view" );
	terminal->addKeyBinding( { KEY_PAGEDOWN, KEYMOD_CTRL }, "next-tab" );
	terminal->addKeyBinding( { KEY_PAGEUP, KEYMOD_CTRL }, "previous-tab" );
	terminal->addKeyBinding( { KEY_TAB, KEYMOD_CTRL }, "next-tab" );
	terminal->addKeyBinding( { KEY_TAB, KEYMOD_CTRL | KEYMOD_SHIFT }, "previous-tab" );
	terminal->addKeyBinding( { KEY_L, KeyMod::getDefaultSecondaryModifier() | KEYMOD_SHIFT },
							 "split-right" );
	terminal->addKeyBinding( { KEY_K, KeyMod::getDefaultSecondaryModifier() | KEYMOD_SHIFT },
							 "split-bottom" );
	terminal->addKeyBinding( { KEY_J, KeyMod::getDefaultSecondaryModifier() | KEYMOD_SHIFT },
							 "split-left" );
	terminal->addKeyBinding( { KEY_I, KeyMod::getDefaultSecondaryModifier() | KEYMOD_SHIFT },
							 "split-top" );
	terminal->addKeyBinding(
		{ KEY_J, KeyMod::getDefaultModifier() | KeyMod::getDefaultSecondaryModifier() },
		"switch-to-previous-split" );
	terminal->addKeyBinding(
		{ KEY_L, KeyMod::getDefaultModifier() | KeyMod::getDefaultSecondaryModifier() },
		"switch-to-next-split" );
}

bool EtermApp::closeWindow( EE::Window::Window* ) {
	if ( closeApproved || !warnBeforeClose )
		return true;
	bool running = false;
	tabSplitter->forEachTab( [&running]( UITab* tab ) { running |= hasRunningChildren( tab ); } );
	if ( !running )
		return true;
	if ( closeDialog )
		return false;
	closeDialog = UIMessageBox::New(
		UIMessageBox::OK_CANCEL,
		i18n( "close_window_running_process_confirm",
			  "Are you sure you want to close this window? It is still running a process." ) );
	closeDialog->setTitle( "eterm" );
	closeDialog->on( Event::OnConfirm, [this]( const Event* ) {
		closeApproved = true;
		appWindow->close();
	} );
	closeDialog->on( Event::OnClose, [this]( const Event* ) {
		closeDialog = nullptr;
		closeDialogWidget = nullptr;
	} );
	closeDialog->center();
	closeDialog->showWhenReady();
	return false;
}

int EtermApp::run( int argc, char* argv[] ) {
#ifdef EE_DEBUG
	Log::instance()->setLogToStdOut( !Runtime::isOffscreen() );
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
	args::Flag alwaysShowTabBar( parser, "always-show-tab-bar",
								 "Always show the tab bar, even with a single tab.",
								 { "always-show-tab-bar" } );
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
	scene->getUIThemeManager()->setDefaultEffectsEnabled( false );
	scene->combineStyleSheet( R"css(
		TabWidget {
			max-tab-width: 200dp;
		}
		Tab > Tab::Text {
			text-overflow: ellipsis;
		}
		.pseudo_anchor {
			tint: var(--floating-icon);
			cursor: arrow;
		}
		.pseudo_anchor:hover {
			tint: var(--primary);
			cursor: hand;
		}
	)css" );

	auto& resourceScope = *scene->getResourceScope();
	auto remixIconFont = FontTrueType::New( "eterm-remixicon", resourceScope );
	auto noniconsFont = FontTrueType::New( "eterm-nonicons", resourceScope );
	auto codIconFont = FontTrueType::New( "eterm-codicon", resourceScope );
	if ( remixIconFont->loadFromFile( resPath + "fonts/remixicon.ttf" ) &&
		 noniconsFont->loadFromFile( resPath + "fonts/nonicons.ttf" ) &&
		 codIconFont->loadFromFile( resPath + "fonts/codicon.ttf" ) ) {
		scene->getUIIconThemeManager()->setCurrentTheme( IconManager::init(
			"eterm", remixIconFont.get(), noniconsFont.get(), codIconFont.get() ) );
		terminalIcon = scene->findIcon( "terminal" );
	}
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

	tabSplitter = UITabWidgetSplitter::New( &splitterClient, scene );
	tabSplitter->setHideTabBarOnSingleTab( !alwaysShowTabBar.Get() );
	tabSplitter->setCanCreateSplitFn( [this]( SplitDirection, UIWidget* ) {
		restoreMaximizedTabWidget();
		return true;
	} );
	tabSplitter->setTabTryCloseCallback( [this]( UIWidget* widget, UITabWidget::FocusTabBehavior,
												 std::function<void()> ) {
		if ( auto* detachedTabWidget = scene->getRoot()->find<UIWidget>( "detached_tab_widget" );
			 widget && detachedTabWidget && detachedTabWidget->inParentTreeOf( widget ) ) {
			restoreMaximizedTabWidget();
		}
		if ( warnBeforeClose ) {
			auto* tab = tabSplitter->getTabFromWidget( widget );
			if ( hasRunningChildren( tab ) ) {
				requestCloseTab( tab );
				return false;
			}
		}
		return true;
	} );
	tabSplitter->setOnTabWidgetCreateCb( [this]( UITabWidget* tabs ) {
		tabs->on( Event::OnTabSelected, [this]( const Event* ) { updateWindowTitle(); } );
		tabs->on( Event::OnTabClosed, [this]( const Event* event ) {
			auto* closedTab = static_cast<const TabEvent*>( event )->getTab();
			pendingExitCloseTabs.erase(
				std::remove( pendingExitCloseTabs.begin(), pendingExitCloseTabs.end(), closedTab ),
				pendingExitCloseTabs.end() );
			if ( closeDialogWidget == closedTab->getOwnedWidget() )
				closeDialogWidget = nullptr;
			if ( !hasTerminals() )
				appWindow->close();
			else
				updateWindowTitle();
		} );
	} );
	auto* tabs = tabSplitter->createTabWidget( mainLayout );
	if ( !tabs ) {
		std::cerr << "Could not create terminal tab widget" << std::endl;
		return EXIT_FAILURE;
	}
	mainLayout->updateLayout();

	for ( size_t tab = 0; tab < eemax( static_cast<size_t>( 1 ), initialTabs.Get() ); ++tab ) {
		if ( !createTerminal( tabs ) ) {
			appWindow->showMessageBox(
				EE::Window::Window::MessageBoxType::Error, "eterm",
				i18n( "operating_system_not_supported", "Operating System not supported." ) );
			return EXIT_FAILURE;
		}
	}

	appWindow->setCloseRequestCallback(
		[this]( EE::Window::Window* window ) { return closeWindow( window ); } );
	app.setShowMemoryManagerResult( true );
	appWindow->runMainLoop( [this] {
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

} // namespace

EE_MAIN_FUNC int main( int argc, char* argv[] ) {
	EtermApp app;
	return app.run( argc, argv );
}
