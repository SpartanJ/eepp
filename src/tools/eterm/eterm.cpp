#include "eterm.hpp"
#include <iostream>

namespace eterm {

void App::TerminalSplitterClient::onTabCreated( UITab* tab, UIWidget* widget ) {
	if ( mApp.terminalIcon && widget && widget->isType( UI_TYPE_TERMINAL ) )
		tab->setIcon( mApp.terminalIcon->createDrawable( PixelDensity::dpToPxI( 12 ) ) );
	mApp.configureTab( tab );
}

void App::TerminalSplitterClient::onWidgetFocusChange( UIWidget* ) {
	mApp.updateWindowTitle();
}

std::string App::getResourcePath() const {
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

String App::i18n( const std::string& key, const String& defaultValue ) const {
	return scene ? scene->i18n( key, defaultValue ) : defaultValue;
}

void App::loadColorSchemes( const std::string& resPath ) {
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

UITerminal* App::terminalFromTab( UITab* tab ) {
	return tab && tab->getOwnedWidget() && tab->getOwnedWidget()->isType( UI_TYPE_TERMINAL )
			   ? tab->getOwnedWidget()->asType<UITerminal>()
			   : nullptr;
}

void App::savePreferences() {
	if ( config && !config->savePreferences() )
		Log::error( "Could not save eterm configuration to %s", config->getConfigPath() );
}

void App::saveWindowState() {
	if ( !config || !appWindow )
		return;
	config->captureWindowState( appWindow );
	if ( !config->saveWindowState() )
		Log::error( "Could not save eterm window state to %s", config->getConfigPath() );
}

void App::forEachTerminal( const std::function<void( UITerminal* )>& fn ) {
	if ( !tabSplitter )
		return;
	tabSplitter->forEachWidgetType(
		UI_TYPE_TERMINAL, [&fn]( UIWidget* widget ) { fn( widget->asType<UITerminal>() ); } );
}

void App::createNewTerminal() {
	auto* current = tabSplitter ? tabSplitter->getCurWidget() : nullptr;
	auto* terminal =
		current && current->isType( UI_TYPE_TERMINAL ) ? current->asType<UITerminal>() : nullptr;
	switch ( config->terminal.newTerminalBehavior ) {
		case NewTerminalBehavior::VerticalSplit:
			createTerminalSplit( SplitDirection::Right, terminal );
			break;
		case NewTerminalBehavior::HorizontalSplit:
			createTerminalSplit( SplitDirection::Bottom, terminal );
			break;
		default:
			createTerminal();
			break;
	}
}

void App::updateWindowTitle() {
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

bool App::hasTerminals() const {
	bool found = false;
	if ( tabSplitter )
		tabSplitter->forEachWidgetStoppable( [&found]( UIWidget* ) {
			found = true;
			return true;
		} );
	return found;
}

bool App::hasRunningChildren( UITab* tab ) {
	auto* terminal = terminalFromTab( tab );
	return terminal && terminal->getTerm() &&
		   Sys::processHasChildren( terminal->getTerm()->getProcessId() );
}

void App::closeTab( UITab* tab ) {
	if ( !tabSplitter || !tab || !tab->getOwnedWidget() )
		return;
	tabSplitter->closeTab( tab->getOwnedWidget()->asType<UIWidget>(),
						   UITabWidget::FocusTabBehavior::Default );
}

void App::queueExitCloseTab( UITab* tab ) {
	if ( tab && std::find( pendingExitCloseTabs.begin(), pendingExitCloseTabs.end(), tab ) ==
					pendingExitCloseTabs.end() ) {
		pendingExitCloseTabs.emplace_back( tab );
	}
}

void App::queueExitedTabs() {
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

void App::requestCloseTab( UITab* tab ) {
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

void App::renameSession( UITerminal* terminal ) {
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

void App::maximizeTabWidget( UITabWidget* tabWidget ) {
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
	win->setCheckEphemeralCloseFn( []( Node* focusNode ) {
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

void App::restoreMaximizedTabWidget() {
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

void App::configureTab( UITab* tab ) {
	tab->on( Event::OnCreateContextMenu, [this]( const Event* event ) {
		const auto* menuEvent = static_cast<const ContextMenuEvent*>( event );
		auto* menu = menuEvent->getMenu();
		auto* clickedTab = event->getNode()->asType<UITab>();
		auto* terminal = terminalFromTab( clickedTab );
		if ( !menu || !terminal || !clickedTab->getTabWidget() )
			return;

		const auto addItem = [this, menu, terminal]( const String& text, const std::string& icon,
													 const std::string& command ) {
			DrawablePtr drawable;
			if ( auto* menuIcon = scene->findIcon( icon ) )
				drawable = menuIcon->createDrawable( PixelDensity::dpToPxI( 12 ) );
			auto* item = menu->add( text, std::move( drawable ),
									terminal->getKeyBindings().getCommandKeybindString( command ) );
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
		addItem( i18n( "split_left", "Split Left" ), "split-horizontal", "split-left" );
		addItem( i18n( "split_right", "Split Right" ), "split-horizontal", "split-right" );
		addItem( i18n( "split_top", "Split Top" ), "split-vertical", "split-top" );
		addItem( i18n( "split_bottom", "Split Bottom" ), "split-vertical", "split-bottom" );

		menu->addSeparator();
		auto* exclusiveMode = menu->addCheckBox(
			i18n( "enable_exclusive_mode", "Enable Exclusive Mode" ), terminal->getExclusiveMode(),
			terminal->getKeyBindings().getCommandKeybindString(
				UITerminal::getExclusiveModeToggleCommandName() ) );
		exclusiveMode->setId( UITerminal::getExclusiveModeToggleCommandName() );
		addItem( i18n( "rename_session", "Rename Session" ), "", "terminal-rename" );
		addItem( i18n( "settings", "Settings" ), "settings", "open-settings" );

		menu->addSeparator();
		const bool canMove = clickedTab->getTabWidget()->getTabCount() > 1;
		const Uint32 tabIndex = clickedTab->getTabWidget()->getTabIndex( clickedTab );
		addItem( i18n( "move_tab_left", "Move Tab Left" ), "", "move-tab-left" )
			->setEnabled( canMove && tabIndex > 0 );
		addItem( i18n( "move_tab_right", "Move Tab Right" ), "", "move-tab-right" )
			->setEnabled( canMove && tabIndex + 1 < clickedTab->getTabWidget()->getTabCount() );
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
			} else if ( command == "open-settings" ) {
				settingsActions->showSettings();
			} else {
				terminal->execute( command );
			}

			if ( previous && tabSplitter->ownedWidgetExists( previous ) )
				tabSplitter->setCurrentWidget( previous );
		} );
	} );
}

UITerminal* App::createTerminal( UITabWidget* target ) {
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
	terminal->setExclusiveMode( config->terminal.exclusiveMode );
	terminal->setScrollViewType( config->terminal.scrollBarType );
	terminal->setVerticalScrollMode( config->terminal.scrollBarMode );
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
	terminal->on( Event::OnCreateContextMenu, [this, terminal]( const Event* event ) {
		auto menu = static_cast<const ContextMenuEvent*>( event )->getMenu();
		const auto addItem = [this, menu, terminal]( const String& text, const std::string& icon,
													 const std::string& command ) {
			DrawablePtr drawable;
			if ( auto* menuIcon = scene->findIcon( icon ) )
				drawable = menuIcon->createDrawable( PixelDensity::dpToPxI( 12 ) );
			auto* item = menu->add( text, std::move( drawable ),
									terminal->getKeyBindings().getCommandKeybindString( command ) );
			item->setId( command );
			return item;
		};
		menu->addSeparator();
		addItem( i18n( "new_terminal", "New Terminal" ), "terminal", "create-new-terminal" );
		menu->addSeparator();
		addItem( i18n( "settings", "Settings" ), "settings", "open-settings" );
		addItem( i18n( "key_bindings", "Keybindings" ), "keybindings", "open-keybindings" );
		menu->on( Event::OnItemClicked, [this, terminal]( const Event* itemEvent ) {
			if ( !itemEvent->getNode()->isType( UI_TYPE_MENUITEM ) )
				return;
			const auto& command = itemEvent->getNode()->getId();
			auto* previous = tabSplitter->getCurWidget();
			tabSplitter->setCurrentWidget( terminal );
			terminal->execute( command );
			if ( previous && tabSplitter->ownedWidgetExists( previous ) )
				tabSplitter->setCurrentWidget( previous );
		} );
	} );
	tabSplitter->setCurrentWidget( terminal );
	if ( !terminalConfig.executeInShell.empty() )
		terminal->executeFile( terminalConfig.executeInShell );
	terminal->setFocus();
	updateWindowTitle();
	return terminal;
}

UITerminal* App::createTerminalSplit( SplitDirection direction, UIWidget* widget ) {
	auto* source = widget ? tabSplitter->tabWidgetFromWidget( widget ) : nullptr;
	auto* target = source ? tabSplitter->splitTabWidget( direction, source ) : nullptr;
	return target ? createTerminal( target ) : nullptr;
}

void App::moveTab( UIWidget* widget, int offset ) {
	if ( !widget || offset == 0 )
		return;
	auto* tabs = tabSplitter->tabWidgetFromWidget( widget );
	auto* tab = tabs ? tabs->getTabFromOwnedWidget( widget ) : nullptr;
	if ( !tab )
		return;
	const Uint32 index = tabs->getTabIndex( tab );
	if ( offset < 0 && index > 0 )
		tabs->moveTab( tab, index - 1 );
	else if ( offset > 0 && index + 1 < tabs->getTabCount() )
		tabs->moveTab( tab, index + 1 );
}

void App::addTabKeyBindings( UITerminal* terminal ) {
	registerTabCommands( *terminal, terminal );
	terminal->setCommand( "terminal-rename", [this, terminal] { renameSession( terminal ); } );
	terminal->setCommand( UITerminal::getExclusiveModeToggleCommandName(), [terminal] {
		terminal->setExclusiveMode( !terminal->getExclusiveMode() );
	} );
	applyKeybindings( terminal );
}

bool App::closeWindow( EE::Window::Window* ) {
	if ( closeApproved || !warnBeforeClose ) {
		saveWindowState();
		return true;
	}
	bool running = false;
	tabSplitter->forEachTab( [&running]( UITab* tab ) { running |= hasRunningChildren( tab ); } );
	if ( !running ) {
		saveWindowState();
		return true;
	}
	if ( closeDialog )
		return false;
	closeDialog = UIMessageBox::New(
		UIMessageBox::OK_CANCEL,
		i18n( "close_window_running_process_confirm",
			  "Are you sure you want to close this window? It is still running a process." ) );
	closeDialog->setTitle( "eterm" );
	closeDialog->on( Event::OnConfirm, [this]( const Event* ) {
		closeApproved = true;
		saveWindowState();
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

int App::run( int argc, char* argv[] ) {
#ifdef EE_DEBUG
	Log::instance()->setLogToStdOut( !Runtime::isOffscreen() );
	Log::instance()->setLiveWrite( true );
#endif
	config = std::make_unique<eterm::AppConfig>( Sys::getConfigPath( "eterm" ) );
	config->load();
	args::ArgumentParser parser( "eterm" );
	args::HelpFlag help( parser, "help", "Display this help menu", { 'h', "help" } );
	args::ValueFlag<std::string> shell( parser, "shell", "Shell name or path", { 's', "shell" },
										config->terminal.shell );
	args::ValueFlag<std::string> shellArgs( parser, "shell-args", "Shell command line arguments",
											{ "shell-args" }, config->terminal.shellArguments );
	args::ValueFlag<size_t> historySize( parser, "scrollback", "Maximum history size (lines)",
										 { 'l', "scrollback" }, config->terminal.historySize );
	args::Flag fb( parser, "framebuffer", "Use frame buffer (more memory usage, less CPU usage)",
				   { "fb", "framebuffer" } );
	args::ValueFlag<std::string> fontPath( parser, "fontpath", "Font path", { 'f', "font" },
										   config->font.path );
	args::ValueFlag<std::string> fallbackFontPath( parser, "fallback-fontpath",
												   "Fallback Font path", { "fallback-font" },
												   config->font.fallbackPath );
	args::ValueFlag<Float> fontSize( parser, "fontsize", "Font size (in dp)", { "fontsize" },
									 config->font.size );
	const std::unordered_map<std::string, FontHinting> fontHintingMap{
		{ "none", FontHinting::None },
		{ "slight", FontHinting::Slight },
		{ "full", FontHinting::Full },
	};
	args::MapFlag<std::string, FontHinting> fontHinting(
		parser, "font-hinting", "Font hinting mode (accepted values: none, slight, full)",
		{ "font-hinting" }, fontHintingMap, config->font.hinting );
	const std::unordered_map<std::string, FontAntialiasing> fontAntialiasingMap{
		{ "none", FontAntialiasing::None },
		{ "grayscale", FontAntialiasing::Grayscale },
		{ "subpixel", FontAntialiasing::Subpixel },
	};
	args::MapFlag<std::string, FontAntialiasing> fontAntialiasing(
		parser, "font-antialiasing",
		"Font antialiasing mode (accepted values: none, grayscale, subpixel)",
		{ "font-antialiasing" }, fontAntialiasingMap, config->font.antialiasing );
	args::ValueFlag<Float> width( parser, "winwidth", "Window width (in dp)", { "width" },
								  config->windowState.size.getWidth() );
	args::ValueFlag<Float> height( parser, "winheight", "Window height (in dp)", { "height" },
								   config->windowState.size.getHeight() );
	args::ValueFlag<Float> pixelDensity( parser, "pixel-density",
										 "Set default application pixel density",
										 { 'd', "pixel-density" } );
	args::Positional<std::string> wd( parser, "wording-dir", "Working Directory / executable" );
	args::Flag closeOnExit( parser, "close-on-exit",
							"close the application when the executable exits", { 'c', "close" } );
	args::ValueFlag<std::string> executeInShell( parser, "execute-in-shell",
												 "execute program in shell", { 'e', "execute" },
												 config->terminal.executeInShell );
	args::Flag vsync( parser, "vsync", "Enable vsync", { "vsync" } );
	args::ValueFlag<std::string> colorScheme( parser, "color-scheme", "Load color scheme",
											  { "color-scheme" }, config->theme.colorScheme );
	args::Flag listColorSchemes( parser, "color-schemes", "Lists color schemes",
								 { "list-color-schemes" } );
	args::ValueFlag<Uint32> maxFPS( parser, "max-fps",
									"Maximum rendering frames per second of the terminal. Default "
									"value will be the refresh rate of the screen.",
									{ "max-fps" }, config->window.maxFPS );
	args::MapFlag<std::string, TerminalCursorMode> cursorStyle(
		parser, "cursor-style",
		"Sets the cursor-style (accepted values: blinking_block, steady_block, blink_underline, "
		"steady_underline, blink_bar, steady_bar)",
		{ "cursor-style" }, TerminalCursorHelper::getTerminalCursorModeMap(),
		config->terminal.cursorStyle );
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
										 { "tabs" }, config->terminal.initialTabs );

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
	config->terminal.shell = shell.Get();
	config->terminal.shellArguments = shellArgs.Get();
	config->terminal.historySize = historySize.Get();
	config->font.path = fontPath.Get();
	config->font.fallbackPath = fallbackFontPath.Get();
	config->font.size = fontSize.Get();
	config->font.hinting = fontHinting.Get();
	config->font.antialiasing = fontAntialiasing.Get();
	config->windowState.size = { static_cast<int>( width.Get() ),
								 static_cast<int>( height.Get() ) };
	if ( pixelDensity )
		config->window.pixelDensity = pixelDensity.Get();
	if ( wd )
		config->terminal.workingDirectory = wd.Get();
	config->terminal.executeInShell = executeInShell.Get();
	config->theme.colorScheme = colorScheme.Get();
	config->window.maxFPS = maxFPS.Get();
	config->terminal.cursorStyle = cursorStyle.Get();
	config->terminal.initialTabs = initialTabs.Get();
	config->terminal.useFrameBuffer |= fb.Get();
	config->terminal.closeOnExit |= closeOnExit.Get();
	config->window.vsync |= vsync.Get();
	config->window.benchmarkMode |= benchmarkModeFlag.Get();
	config->window.warnBeforeClose |= warnBeforeCloseFlag.Get();
	config->window.alwaysShowTabBar |= alwaysShowTabBar.Get();
	if ( !config->savePreferences() )
		Log::error( "Could not save eterm configuration to %s", config->getConfigPath() );

	const std::string initialWorkingDirectory = FileSystem::getCurrentWorkingDirectory();
	const std::string resPath = getResourcePath();
	loadColorSchemes( resPath );
	if ( listColorSchemes.Get() ) {
		std::cout << "Color schemes:\n";
		for ( const auto& colorSchemeEntry : terminalColorSchemes )
			std::cout << "\t" << colorSchemeEntry.first << "\n";
		return EXIT_SUCCESS;
	}
	if ( !config->theme.colorScheme.empty() ) {
		auto colorSchemeIt = terminalColorSchemes.find( config->theme.colorScheme );
		if ( colorSchemeIt != terminalColorSchemes.end() )
			selectedColorScheme = &colorSchemeIt->second;
	}

	DisplayManager* displayManager = Engine::instance()->getDisplayManager();
	Display* currentDisplay = displayManager->getDisplayIndex(
		config->windowState.displayIndex >= 0 &&
				config->windowState.displayIndex < displayManager->getDisplayCount()
			? config->windowState.displayIndex
			: 0 );
	if ( !currentDisplay ) {
		std::cerr << "Display not found, exiting" << std::endl;
		return EXIT_FAILURE;
	}

	Sizei windowSize( config->windowState.size );
	const auto displaySize = currentDisplay->getUsableBounds().getSize();
	if ( displaySize.getWidth() > 0 && windowSize.getWidth() >= displaySize.getWidth() )
		windowSize.setWidth( static_cast<int>( displaySize.getWidth() * 0.8f ) );
	if ( displaySize.getHeight() > 0 && windowSize.getHeight() >= displaySize.getHeight() )
		windowSize.setHeight( static_cast<int>( displaySize.getHeight() * 0.75f ) );

	FontTrueTypePtr uiFont;
	if ( !config->font.uiPath.empty() && FileSystem::fileExists( config->font.uiPath ) ) {
		uiFont = FontTrueType::New( "eterm-ui-font" );
		if ( !uiFont->loadFromFile( config->font.uiPath ) )
			uiFont.reset();
	}
	UIApplication::Settings appSettings;
	appSettings.basePath = FileSystem::removeLastFolderFromPath( resPath );
	appSettings.pixelDensity = config->window.pixelDensity > 0 ? config->window.pixelDensity
															   : currentDisplay->getPixelDensity();
	appSettings.fontHinting = config->font.hinting;
	appSettings.fontAntialiasing = config->font.antialiasing;
	appSettings.baseFont = uiFont.get();
	const Int32 frameRateLimit =
		config->window.benchmarkMode ? 0 : static_cast<Int32>( config->window.maxFPS );
	UIApplication app( WindowSettings( windowSize.getWidth(), windowSize.getHeight(), "eterm",
									   WindowStyle::Default, WindowBackend::Default, 32,
									   resPath + "icon/eterm.png" ),
					   appSettings,
					   ContextSettings( config->window.vsync, frameRateLimit,
										config->window.multisamples,
										config->window.rendererVersion ) );
	appWindow = app.getWindow();
	scene = app.getUI();
	if ( !appWindow || !appWindow->isOpen() || !scene )
		return EXIT_FAILURE;
	settingsActions = std::make_unique<SettingsActions>( this );
	keybindingsPath = config->getConfigPath() + "keybindings.cfg";
	loadKeybindings();
	fileWatcher = std::make_unique<efsw::FileWatcher>();
	fileWatcher->addWatch( config->getConfigPath(), this );
	fileWatcher->watch();
	scene->setColorSchemePreference( config->theme.uiColorScheme );
	scene->getUIThemeManager()->setDefaultFontSize( config->font.uiSize );
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
		Tab.tab_modified > Tab::close {
			foreground-image: url("data:image/svg,<svg viewBox='0 0 24 24' width='12' height='12' fill='#ffffff'><path d='M12 22C17.5228 22 22 17.5228 22 12C22 6.47715 17.5228 2 12 2C6.47715 2 2 6.47715 2 12C2 17.5228 6.47715 22 12 22Z'></path></svg>");
			foreground-tint: var(--primary);
			foreground-size: 6dp 6dp;
			foreground-position: center;
			opacity: 1;
		}
		Tab.tab_modified > Tab::close:hover {
			foreground-image: url("data:image/svg,<svg width='16' height='16' viewBox='0 0 16 16'><path fill='#ffffff' fill-rule='evenodd' d='M 2.3432061,13.657206 A 8.0002061,8.0002061 0 1 1 13.657206,2.3432061 8.0002061,8.0002061 0 0 1 2.3432061,13.657206 Z m 3.687,-8.6869999 a 0.75,0.75 0 0 0 -1.06,1.06 l 1.97,1.97 -1.97,1.97 a 0.75,0.75 0 1 0 1.06,1.0599999 l 1.97,-1.9699999 1.97,1.9699999 A 0.75,0.75 0 1 0 11.030206,9.9702061 l -1.9699999,-1.97 1.9699999,-1.97 a 0.75,0.75 0 1 0 -1.0599999,-1.06 l -1.97,1.97 z' /></svg>");
			foreground-tint: var(--tab-close-hover);
			foreground-size: 10dp 10dp;
			foreground-position: center;
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
	if ( !config->font.path.empty() && FileSystem::fileExists( config->font.path ) ) {
		terminalFont = FontTrueType::New( "eterm-monospace", resourceScope ).get();
		if ( terminalFont->loadFromFile( config->font.path ) )
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

	if ( !config->font.fallbackPath.empty() ) {
		if ( FileSystem::fileExists( config->font.fallbackPath ) ) {
			auto fallback = FontTrueType::New( "eterm-fallback-font", resourceScope );
			if ( fallback->loadFromFile( config->font.fallbackPath ) )
				resourceScope.getFontService().addFallbackFont( std::move( fallback ) );
		}
	} else if ( auto fallback = resourceScope.findFont( "DroidSansFallbackFull" ) ) {
		resourceScope.getFontService().addFallbackFont( std::move( fallback ) );
	}

	const std::string launchPath = config->terminal.workingDirectory.empty()
									   ? initialWorkingDirectory
									   : config->terminal.workingDirectory;
	FileInfo launchFile( launchPath );
	const bool launchExecutable = launchFile.isRegularFile() && launchFile.isExecutable();
	terminalConfig.program = launchExecutable ? launchFile.getFilepath() : config->terminal.shell;
	terminalConfig.arguments = config->terminal.shellArguments.empty()
								   ? std::vector<std::string>{}
								   : String::split( config->terminal.shellArguments );
	terminalConfig.workingDirectory = launchFile.getDirectoryPath();
	terminalConfig.executeInShell = config->terminal.executeInShell;
	terminalConfig.historySize = config->terminal.historySize;
	terminalConfig.cursorStyle = config->terminal.cursorStyle;
	terminalConfig.fontHinting = config->font.hinting;
	terminalConfig.fontAntialiasing = config->font.antialiasing;
	terminalConfig.useFrameBuffer = config->terminal.useFrameBuffer;
	terminalConfig.keepAlive = !launchExecutable && config->terminal.shell.empty();
	terminalConfig.closeOnExit = config->terminal.closeOnExit;
	warnBeforeClose = config->window.warnBeforeClose;
	benchmarkMode = config->window.benchmarkMode;
	terminalFontSize = PixelDensity::dpToPx( config->font.size );

	mainLayout = UILinearLayout::NewVertical();
	mainLayout->setParent( scene->getRoot() );
	mainLayout->setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::MatchParent );
	mainLayout->setPixelsSize( appWindow->getSize().asFloat() );

	tabSplitter = UITabWidgetSplitter::New( &splitterClient, scene );
	tabSplitter->setHideTabBarOnSingleTab( !config->window.alwaysShowTabBar );
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
			if ( !hasTerminals() ) {
				saveWindowState();
				appWindow->close();
			} else {
				updateWindowTitle();
			}
		} );
	} );
	auto* tabs = tabSplitter->createTabWidget( mainLayout );
	if ( !tabs ) {
		std::cerr << "Could not create terminal tab widget" << std::endl;
		return EXIT_FAILURE;
	}
	mainLayout->updateLayout();

	for ( size_t tab = 0; tab < eemax( static_cast<size_t>( 1 ), config->terminal.initialTabs );
		  ++tab ) {
		if ( !createTerminal( tabs ) ) {
			appWindow->showMessageBox(
				EE::Window::Window::MessageBoxType::Error, "eterm",
				i18n( "operating_system_not_supported", "Operating System not supported." ) );
			return EXIT_FAILURE;
		}
	}
	if ( config->windowState.position != Vector2i( -1, -1 ) &&
		 config->windowState.displayIndex < displayManager->getDisplayCount() ) {
		// 1 px offset to avoid a bug in SDL2 2.28 when maximizing windows
		appWindow->setPosition( config->windowState.position.x +
									( config->windowState.maximized ? -1 : 0 ),
								config->windowState.position.y );
	}
#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN
	if ( config->windowState.maximized ) {
#if EE_PLATFORM == EE_PLATFORM_LINUX
		scene->runOnMainThread( [this] { appWindow->maximize(); } );
#elif EE_PLATFORM != EE_PLATFORM_MACOS
		appWindow->maximize();
#endif
	}
#endif

	appWindow->setCloseRequestCallback(
		[this]( EE::Window::Window* window ) { return closeWindow( window ); } );
	appWindow->setQuitCallback( [this]( EE::Window::Window* window ) {
		if ( window->isOpen() && closeWindow( window ) )
			window->close();
	} );
	app.setShowMemoryManagerResult( true );
	appWindow->runMainLoop( [this] {
		appWindow->getInput()->update();
		SceneManager::instance()->update();
		if ( keybindingsChanged.exchange( false, std::memory_order_acq_rel ) )
			reloadKeybindings();
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
	fileWatcher.reset();
	return EXIT_SUCCESS;
}

} // namespace eterm

EE_MAIN_FUNC int main( int argc, char* argv[] ) {
	App app;
	return app.run( argc, argv );
}
