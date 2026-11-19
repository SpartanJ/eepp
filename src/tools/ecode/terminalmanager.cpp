#include "terminalmanager.hpp"
#include "ecode.hpp"
#include <eepp/system/filesystem.hpp>
#include <eepp/ui/uieventdispatcher.hpp>

#if EE_PLATFORM != EE_PLATFORM_WIN
#include <unistd.h>
#endif

using namespace std::literals;

namespace ecode {

TerminalManager::TerminalManager( App* app ) : mApp( app ) {}

UITerminal* TerminalManager::createTerminalInSplitter(
	const std::string& workingDir, std::string program, std::vector<std::string> args,
	const std::unordered_map<std::string, std::string>& env, bool fallback, bool keepAlive ) {
#if EE_PLATFORM == EE_PLATFORM_WIN
	std::string os = Sys::getOSName( true );
	if ( !LuaPattern::hasMatches( os, "Windows 1%d"sv ) &&
		 !LuaPattern::hasMatches( os, "Windows Server 201[69]"sv ) &&
		 !LuaPattern::hasMatches( os, "Windows Server 202%d"sv ) ) {
		if ( fallback )
			displayError( workingDir );
		return nullptr;
	}
#endif

	auto* splitter = mApp->getSplitter();
	const auto orientation = mApp->termConfig().newTerminalOrientation;
	// Resolve before show()/split(): both may change the active terminal or editor.
	const auto directory = workingDir.empty()
							   ? getWorkingDir( program.empty() ? getCurrentTerminal() : nullptr )
							   : workingDir;

	if ( orientation == NewTerminalOrientation::StatusBarPanel &&
		 mApp->getStatusTerminalController() ) {
		auto* controller = mApp->getStatusTerminalController();
		auto* terminal =
			controller->createTerminal( directory, std::move( program ), std::move( args ), env );
		controller->show();
		return terminal;
	}
	if ( !splitter )
		return nullptr;
	// Existing panes follow focus; the orientation preference only creates the first split.
	if ( !splitter->hasSplit() ) {
		if ( orientation == NewTerminalOrientation::Vertical ) {
			splitter->split( SplitDirection::Right, splitter->getCurWidget(), false );
		} else if ( orientation == NewTerminalOrientation::Horizontal ) {
			splitter->split( SplitDirection::Bottom, splitter->getCurWidget(), false );
		} else if ( orientation == NewTerminalOrientation::StatusBarPanel ) {
			return nullptr;
		}
	}
	return createNewTerminal( "", nullptr, directory, std::move( program ), std::move( args ), env,
							  fallback, keepAlive );
}

void TerminalManager::applyTerminalColorScheme( const TerminalColorScheme& colorScheme ) {
	forEachTerminal(
		[&colorScheme]( UITerminal* terminal ) { terminal->setColorScheme( colorScheme ); } );
}

void TerminalManager::forEachTerminal( const std::function<void( UITerminal* )>& fn ) const {
	mApp->getUISceneNode()->forEachNode( [&fn]( Node* node ) {
		if ( node->isType( UI_TYPE_TERMINAL ) )
			fn( node->asType<UITerminal>() );
	} );
}

void TerminalManager::connectTerminal( UITerminal* terminal, const char* defaultIcon ) {
	if ( !terminal )
		return;

	const auto& preferences = mApp->termConfig();
	mProgramStatus.watch( terminal, preferences.programStatus, defaultIcon );
	terminal->getTerm()->setKeepAlive( !preferences.closeTerminalTabOnExit );
	terminal->getTerm()->pushEventCallback(
		[&preferences, terminal]( const TerminalDisplay::Event& event ) {
			if ( event.type != TerminalDisplay::EventType::PROCESS_EXIT ||
				 !preferences.closeTerminalTabOnExit )
				return;
			if ( auto* tabs = UITabWidget::fromOwnedNode( terminal ) ) {
				if ( auto* tab = tabs->getTabFromOwnedWidget( terminal ) )
					tabs->removeTab( tab );
			}
		} );
}

UITerminal* TerminalManager::getCurrentTerminal() const {
	for ( auto* node = mApp->getUISceneNode()->getUIEventDispatcher()->getFocusNode(); node;
		  node = node->getParent() ) {
		if ( node->isType( UI_TYPE_TERMINAL ) )
			return node->asType<UITerminal>();
	}
	auto* current = mApp->getSplitter()->getCurWidget();
	return current && current->isType( UI_TYPE_TERMINAL ) ? current->asType<UITerminal>() : nullptr;
}

std::string TerminalManager::getWorkingDir( UITerminal* source ) const {
	if ( source && mApp->termConfig().inheritWorkingDirectory ) {
		const auto usable = []( const std::string& path ) {
			if ( !FileSystem::isDirectory( path ) )
				return false;
#if EE_PLATFORM == EE_PLATFORM_WIN
			return FileInfo( path ).isReadable();
#else
			return access( path.c_str(), X_OK ) == 0;
#endif
		};
		auto directory = source->getTerm()->getCurrentWorkingDirectory();
		if ( usable( directory ) )
			return directory;
		const auto& directoryAtLaunch = source->getTerm()->getWorkingDir();
		if ( usable( directoryAtLaunch ) )
			return directoryAtLaunch;
	}
	return getSelectedWorkingDir();
}

UITerminal* TerminalManager::createTerminalSplit( SplitDirection direction ) {
	auto directory = getWorkingDir( getCurrentTerminal() );
	auto* splitter = mApp->getSplitter();
	splitter->split( direction, splitter->getCurWidget(), false );
	return createNewTerminal( "", nullptr, directory );
}

void TerminalManager::setTerminalColorScheme( const std::string& name ) {
	if ( name != mTerminalCurrentColorScheme ) {
		mTerminalCurrentColorScheme = name;
		mApp->termConfig().colorScheme = name;
		auto csIt = mTerminalColorSchemes.find( mTerminalCurrentColorScheme );
		applyTerminalColorScheme( csIt != mTerminalColorSchemes.end()
									  ? mTerminalColorSchemes.at( mTerminalCurrentColorScheme )
									  : TerminalColorScheme::getDefault() );
		mApp->getNotificationCenter()->addNotification( String::format(
			mApp->i18n( "terminal_color_scheme_set", "Terminal color scheme: %s" ).toUtf8().c_str(),
			mTerminalCurrentColorScheme.c_str() ) );

		updateColorSchemeMenu();
	}
}

void TerminalManager::loadTerminalColorSchemes() {
	auto colorSchemes = TerminalColorScheme::loadFromFile(
		mApp->resPath() + "colorschemes/terminalcolorschemes.conf" );
	if ( colorSchemes.empty() )
		colorSchemes.emplace_back( TerminalColorScheme::getDefault() );
	if ( FileSystem::isDirectory( mTerminalColorSchemesPath ) ) {
		auto colorSchemesFiles = FileSystem::filesGetInPath( mTerminalColorSchemesPath );
		for ( const auto& file : colorSchemesFiles ) {
			auto colorSchemesInFile =
				TerminalColorScheme::loadFromFile( mTerminalColorSchemesPath + file );
			for ( const auto& coloScheme : colorSchemesInFile )
				colorSchemes.emplace_back( coloScheme );
		}
	} else {
		FileSystem::makeDir( mTerminalColorSchemesPath, true );
	}
	for ( const auto& colorScheme : colorSchemes )
		mTerminalColorSchemes.insert( { colorScheme.getName(), colorScheme } );
	mTerminalCurrentColorScheme = mApp->termConfig().colorScheme;
	if ( mTerminalColorSchemes.find( mTerminalCurrentColorScheme ) == mTerminalColorSchemes.end() )
		mTerminalCurrentColorScheme = mTerminalColorSchemes.begin()->first;
}

KeyBindings::ShortcutMap TerminalManager::getTerminalKeybindings() {
	return {
		{ { KEY_F, KeyMod::getDefaultModifier() | KEYMOD_SHIFT }, "terminal-find" },
		{ { KEY_G, KeyMod::getDefaultModifier() | KEYMOD_SHIFT }, "terminal-find-next" },
		{ { KEY_G, KeyMod::getDefaultModifier() | KEYMOD_SHIFT | KEYMOD_ALT },
		  "terminal-find-previous" },
		{ { KEY_T, KeyMod::getDefaultModifier() | KEYMOD_SHIFT }, "create-new-terminal" },
		{ { KEY_E,
			KeyMod::getDefaultModifier() | KeyMod::getDefaultSecondaryModifier() | KEYMOD_SHIFT },
		  UITerminal::getExclusiveModeToggleCommandName() },
		{ { KEY_S, KeyMod::getDefaultSecondaryModifier() | KeyMod::getDefaultModifier() },
		  "terminal-rename" },
	};
}

KeyBindings::ShortcutMap TerminalManager::getTerminalContextKeybindings() {
	auto defaults = getTerminalKeybindings();
	const auto clipboard = UITerminal::getDefaultKeybindings();
	defaults.insert( clipboard.begin(), clipboard.end() );
	return defaults;
}

const std::string& TerminalManager::getTerminalColorSchemesPath() const {
	return mTerminalColorSchemesPath;
}

void TerminalManager::setTerminalColorSchemesPath( const std::string& terminalColorSchemesPath ) {
	mTerminalColorSchemesPath = terminalColorSchemesPath;
}

void TerminalManager::updateColorSchemeMenu() {
	for ( UIPopUpMenu* menu : mColorSchemeMenus ) {
		for ( size_t i = 0; i < menu->getCount(); i++ ) {
			UIWidget* widget = menu->getItem( i );
			if ( widget->isType( UI_TYPE_MENURADIOBUTTON ) ) {
				auto* menuItem = widget->asType<UIMenuRadioButton>();
				menuItem->setActive( mTerminalCurrentColorScheme == menuItem->getText() );
			}
		}
	}
}

bool TerminalManager::getUseFrameBuffer() const {
	return mUseFrameBuffer || mApp->termConfig().useFrameBuffer;
}

void TerminalManager::setUseFrameBuffer( bool useFrameBuffer ) {
	mUseFrameBuffer = useFrameBuffer;
}

void TerminalManager::configureTerminalScrollback() {
	UIMessageBox* msgBox =
		UIMessageBox::New( UIMessageBox::INPUT, mApp->i18n( "configure_terminal_scrollback",
															"Configure terminal scrollback:" ) );
	msgBox->setTitle( mApp->getWindowTitle() );
	msgBox->setCloseShortcut( { KEY_ESCAPE, 0 } );
	msgBox->getTextInput()->setAllowOnlyNumbers( true, false );
	msgBox->getTextInput()->setText( String::toString( mApp->getConfig().term.scrollback ) );
	msgBox->center();
	msgBox->showWhenReady();
	msgBox->on( Event::OnConfirm, [this, msgBox]( const Event* ) {
		int val;
		if ( String::fromString( val, msgBox->getTextInput()->getText() ) && val >= 0 ) {
			mApp->getConfig().term.scrollback = val;
			msgBox->closeWindow();
		}
	} );
}

std::string TerminalManager::getSelectedWorkingDir() const {
	const auto& config = mApp->getConfig().term;
	switch ( config.workingDir ) {
		case TerminalWorkingDir::ProjectRoot:
			return !mApp->getCurrentProject().empty() ? mApp->getCurrentProject()
													  : mApp->getCurrentWorkingDir();
		case TerminalWorkingDir::CurrentFileDir:
			return mApp->getCurrentFileDir();
		case TerminalWorkingDir::UserHome:
			return Sys::getUserDirectory();
		case TerminalWorkingDir::Other:
			return config.workingDirOther;
		default:
			return mApp->getCurrentWorkingDir();
	}
}

UIMenu* TerminalManager::createColorSchemeMenu( bool emptyMenu ) {
	mColorSchemeMenusCreatedWithHeight =
		emptyMenu ? 0 : mApp->uiSceneNode()->getPixelsSize().getHeight();
	size_t maxItems = 19;
	auto cb = [this]( const Event* event ) {
		UIMenuItem* item = event->getNode()->asType<UIMenuItem>();
		const String& name = item->getText();
		setTerminalColorScheme( name );
	};

	UIPopUpMenu* menu = UIPopUpMenu::New();
	menu->on( Event::OnItemClicked, cb );
	mColorSchemeMenus.push_back( menu );
	size_t total = 0;
	const auto& colorSchemes = mTerminalColorSchemes;

	if ( emptyMenu )
		return mColorSchemeMenus[0];

	for ( auto& colorScheme : colorSchemes ) {
		menu->addRadioButton( colorScheme.first, mTerminalCurrentColorScheme == colorScheme.first );

		if ( mColorSchemeMenus.size() == 1 && menu->getCount() == 1 ) {
			menu->reloadStyle( true, true );
			Float height = menu->getPixelsSize().getHeight();
			Float tHeight = mApp->uiSceneNode()->getPixelsSize().getHeight();
			maxItems = (int)eeceil( tHeight / height ) - 2;
		}

		total++;

		if ( menu->getCount() == maxItems && colorSchemes.size() - total > 1 ) {
			UIPopUpMenu* newMenu = UIPopUpMenu::New();
			menu->addSubMenu( mApp->i18n( "more_ellipsis", "More..." ), nullptr, newMenu );
			newMenu->on( Event::OnItemClicked, cb );
			mColorSchemeMenus.push_back( newMenu );
			menu = newMenu;
		}
	}

	return mColorSchemeMenus[0];
}

void TerminalManager::updateMenuColorScheme( UIMenuSubMenu* colorSchemeMenu ) {
	if ( mColorSchemeMenusCreatedWithHeight != mApp->uiSceneNode()->getPixelsSize().getHeight() ) {
		for ( UIPopUpMenu* menu : mColorSchemeMenus )
			menu->close();
		mColorSchemeMenus.clear();
		auto* newMenu = createColorSchemeMenu();
		newMenu->reloadStyle( true, true );
		colorSchemeMenu->setSubMenu( newMenu );
	}
}

#if EE_PLATFORM == EE_PLATFORM_WIN
std::string quoteString( std::string str ) {
	std::string escapedStr = "";
	for ( char chr : str ) {
		if ( std::strchr( "()%!^\"<>&| ", chr ) )
			escapedStr += '^';
		escapedStr += chr;
	}
	return escapedStr;
}

static int openExternal( const std::string& defShell, const std::string& cmd,
						 const std::string& scriptsPath, const std::string& workingDir ) {
	// This is an utility bat script based in the Geany utility script called "geany-run-helper"
	static const std::string_view RUN_HELPER =
		R"shellscript(REM USAGE: ecode-run-helper DIRECTORY AUTOCLOSE COMMAND...

REM unnecessary, but we get the directory
cd %1
shift
REM save autoclose option and remove it
set autoclose=%1
shift

REM spawn the child
REM it's tricky because shift doesn't affect %*, so hack it out
REM https://en.wikibooks.org/wiki/Windows_Batch_Scripting#Command-line_arguments
set SPAWN=
:argloop
if -%1-==-- goto argloop_end
	set SPAWN=%SPAWN% %1
	shift
goto argloop
:argloop_end
%SPAWN%

REM show the result
echo:
echo:
echo:------------------
echo:(program exited with code: %ERRORLEVEL%)
echo:

REM and if wanted, wait on the user
if not %autoclose%==1 pause
	)shellscript";
	if ( !cmd.empty() && !scriptsPath.empty() ) {
		std::string runHelperPath = scriptsPath + "ecode-run-helper.bat";
		bool canContinue = true;
		if ( !FileSystem::fileExists( runHelperPath ) &&
			 !FileSystem::fileWrite( runHelperPath, RUN_HELPER ) ) {
			canContinue = false;
		}
		if ( canContinue ) {
			std::string cmdDir = String::trim( FileSystem::fileRemoveFileName( cmd ) );
			if ( cmdDir.empty() )
				cmdDir = workingDir;
			std::string cmdFile = String::trim( FileSystem::fileNameFromPath( cmd ) );
			auto fcmd = "cmd.exe /q /c " + quoteString( "\"" + runHelperPath + "\" \"" + cmdDir +
														"\" 0 \"" + cmdFile + "\"" );
			Log::info( "Running: %s", fcmd );
			return Sys::execute( fcmd, workingDir );
		} else {
			Log::info( "Couldn't write runHelperPath %s", runHelperPath );
		}
	}

	std::vector<std::string> options;
	options.reserve( 3 );
	if ( !defShell.empty() )
		options.push_back( defShell );
	options.push_back( "cmd" );
	options.push_back( "powershell" );
	for ( const auto& option : options ) {
		auto externalShell( Sys::which( option ) );
		if ( !externalShell.empty() ) {
			if ( !cmd.empty() ) {
				auto fcmd = externalShell + " /q /c " + quoteString( "\"" + cmd + "\"" );
				Log::info( "Running: %s", fcmd );
				return Sys::execute( fcmd, workingDir );
			} else {
				return Sys::execute( externalShell, workingDir );
			}
		}
	}
	return 0;
}
#elif EE_PLATFORM == EE_PLATFORM_MACOS
static int openExternal( const std::string&, const std::string& cmd, const std::string&,
						 const std::string& workingDir ) {

	std::vector<std::string> options = { "ghostty", "iTerm2", "eterm" };
	for ( const auto& option : options ) {
		auto externalShell( Sys::which( option ) );
		if ( !externalShell.empty() ) {
			if ( !cmd.empty() ) {
				auto fcmd = externalShell + " -e \"" + cmd + "\"";
				Log::info( "Running: %s", fcmd );
				return Sys::execute( fcmd, workingDir );
			} else {
				return Sys::execute( externalShell, workingDir );
			}
		}
	}

	static const std::string externalShell = "open -a terminal --args";
	if ( !cmd.empty() ) {
		std::string fcmd = externalShell + " \"" + cmd + "\"";
		Log::info( "Running: %s", fcmd );
		return Sys::execute( fcmd, workingDir );
	}
	return Sys::execute( externalShell, workingDir );
}
#else
static int openExternal( const std::string&, const std::string& cmd, const std::string&,
						 const std::string& workingDir ) {
	std::vector<std::string> options = { "gnome-terminal", "konsole", "eterm", "xterm", "st" };
	for ( const auto& option : options ) {
		auto externalShell( Sys::which( option ) );
		if ( !externalShell.empty() ) {
			if ( !cmd.empty() ) {
				auto fcmd = externalShell + " -e \"" + cmd + "\"";
				Log::info( "Running: %s", fcmd );
				return Sys::execute( fcmd, workingDir );
			} else {
				return Sys::execute( externalShell, workingDir );
			}
		}
	}
	return 0;
}
#endif

int TerminalManager::openInExternalTerminal( const std::string& cmd,
											 const std::string& workingDir ) {
	Log::info( "Trying to open in external terminal: %s %s", cmd, workingDir );
	return openExternal( mApp->termConfig().shell, cmd, mApp->getScriptsPath(), workingDir );
}

void TerminalManager::displayError( const std::string& workingDir ) {
	if ( mApp->getConfig().term.unsupportedOSWarnDisabled ) {
		openExternal( mApp->termConfig().shell, "", mApp->getScriptsPath(), workingDir );
	} else {
		UIMessageBox* msgBox = UIMessageBox::New(
			UIMessageBox::OK,
			mApp->i18n( "feature_not_supported_in_os",
						"This feature is not supported in this Operating System.\necode will try "
						"to open an external terminal." ) );

		UICheckBox* chkDoNotWarn = UICheckBox::New();
		chkDoNotWarn->setLayoutMargin( Rectf( 0, 8, 0, 0 ) )
			->setLayoutSizePolicy( SizePolicy::WrapContent, SizePolicy::WrapContent )
			->setLayoutGravity( UI_HALIGN_LEFT | UI_VALIGN_CENTER )
			->setClipType( ClipType::None )
			->setParent( msgBox->getLayoutCont()->getFirstChild() )
			->setId( "terminal-not-supported-chk" );
		chkDoNotWarn->setText( mApp->i18n( "terminal_not_supported_do_not_warn",
										   "Always open an external terminal (do not warn)" ) );
		chkDoNotWarn->toPosition( 1 );
		msgBox->on( Event::OnConfirm, [this, chkDoNotWarn, workingDir]( const Event* ) {
			if ( chkDoNotWarn->isChecked() )
				mApp->getConfig().term.unsupportedOSWarnDisabled = true;
			openExternal( mApp->termConfig().shell, "", mApp->getScriptsPath(), workingDir );
		} );
		msgBox->showWhenReady();
	}
}

UITerminal* TerminalManager::createNewTerminal(
	const std::string& title, UITabWidget* inTabWidget, const std::string& workingDir,
	std::string program, std::vector<std::string> args,
	const std::unordered_map<std::string, std::string>& env, bool fallback, bool keepAlive ) {
#if EE_PLATFORM == EE_PLATFORM_EMSCRIPTEN
	UIMessageBox* msgBox = UIMessageBox::New(
		UIMessageBox::OK,
		mApp->i18n( "feature_not_supported_in_emscripten",
					"This feature is only supported in the desktop version of ecode." ) );
	msgBox->showWhenReady();
	return nullptr;
#else
	const auto directory = workingDir.empty()
							   ? getWorkingDir( program.empty() ? getCurrentTerminal() : nullptr )
							   : workingDir;
	UITabWidget* tabWidget = nullptr;

	if ( !inTabWidget ) {
		UIWidget* curWidget = mApp->getSplitter()->getCurWidget();
		if ( !curWidget )
			return nullptr;
		tabWidget = mApp->getSplitter()->tabWidgetFromWidget( curWidget );
	} else {
		tabWidget = inTabWidget;
	}

	if ( !tabWidget ) {
		if ( !mApp->getSplitter()->getTabWidgets().empty() ) {
			tabWidget = mApp->getSplitter()->getTabWidgets()[0];
		} else {
			return nullptr;
		}
	}
	Sizef initialSize( 16, 16 );
	if ( tabWidget->getContainerNode() ) {
		initialSize = tabWidget->getContainerNode()->getPixelsSize();
		if ( Sizef::Zero == initialSize ) {
			// HACK: Force the Scene Node to update the styles and layouts.
			tabWidget->getUISceneNode()->updateDirtyStyles();
			tabWidget->getUISceneNode()->updateDirtyStyleStates();
			tabWidget->getUISceneNode()->updateDirtyLayouts();
			initialSize = tabWidget->getContainerNode()->getPixelsSize();
		}
	}

	if ( program.empty() && !mApp->termConfig().shell.empty() )
		program = mApp->termConfig().shell;

	if ( args.empty() && !mApp->termConfig().shellArgs.empty() )
		args = Process::parseArgs( mApp->termConfig().shellArgs );

	auto display = TerminalDisplay::create(
		mApp->getUISceneNode()->getWindow(),
		mApp->getTerminalFont() ? mApp->getTerminalFont() : mApp->getFontMono(),
		mApp->termConfig().fontSize.asPixels( 0, Sizef(), mApp->getDisplayDPI() ), initialSize,
		std::move( program ), std::move( args ), directory, mApp->termConfig().historySize(),
		nullptr, getUseFrameBuffer(), keepAlive, env, mApp->termConfig().sessionOptions() );
	UITerminal* term = display ? UITerminal::New( display ) : nullptr;

	if ( term == nullptr || term->getTerm() == nullptr ) {
		if ( fallback )
			displayError( workingDir );
		return nullptr;
	}

	mApp->termConfig().applyInitial( term );
	term->setExclusiveMode( mApp->termConfig().exclusiveMode );

	auto ret = mApp->getSplitter()->createWidgetInTabWidget(
		tabWidget, term, title.empty() ? mApp->i18n( "shell", "Shell" ).toUtf8() : title, true );
	ret.first->setIcon( mApp->findIcon( "filetype-bash" ) );
	mApp->getSplitter()->removeUnusedTab( tabWidget, true, false );

	term->removeEventsOfType( Event::OnFocus );
	term->on( Event::OnFocus, [this, term]( const Event* ) {
		mApp->getSplitter()->setCurrentWidget(
			mApp->getSplitter()->getBaseLayout()->inParentTreeOf( term ) ? term : nullptr );
	} );
	term->setTitle( title );
	auto csIt = mTerminalColorSchemes.find( mTerminalCurrentColorScheme );
	term->getTerm()->setAllowMemoryTrimming( true );
	connectTerminal( term, "filetype-bash" );
	term->setColorScheme( csIt != mTerminalColorSchemes.end()
							  ? mTerminalColorSchemes.at( mTerminalCurrentColorScheme )
							  : TerminalColorScheme::getDefault() );
	term->on( Event::OnTitleChange, [this]( const Event* event ) {
		if ( event->getNode() != mApp->getSplitter()->getCurWidget() )
			return;
		mApp->setAppTitle( event->getNode()->asType<UITerminal>()->getTitle() );
	} );
	setKeybindings( term );
	term->setCommand( "terminal-rename", [this, term] {
		UIMessageBox* msgBox = UIMessageBox::New(
			UIMessageBox::INPUT, mApp->i18n( "new_terminal_name", "New terminal name:" ) );
		msgBox->setTitle( mApp->getWindowTitle() );
		msgBox->getTextInput()->setHint( mApp->i18n( "any_name_ellipsis", "Any name..." ) );
		msgBox->setCloseShortcut( { KEY_ESCAPE, KEYMOD_NONE } );
		msgBox->showWhenReady();
		msgBox->on( Event::OnConfirm, [msgBox, term]( const Event* ) {
			std::string title( msgBox->getTextInput()->getText().toUtf8() );
			term->setTitle( title );
			msgBox->close();
			term->setFocus();
		} );
	} );
	term->setCommand( "switch-to-previous-colorscheme", [this] {
		auto it = mTerminalColorSchemes.find( mTerminalCurrentColorScheme );
		auto prev = std::prev( it, 1 );
		if ( prev != mTerminalColorSchemes.end() ) {
			setTerminalColorScheme( prev->first );
		} else {
			setTerminalColorScheme( mTerminalColorSchemes.rbegin()->first );
		}
	} );
	term->setCommand( "switch-to-next-colorscheme", [this] {
		auto it = mTerminalColorSchemes.find( mTerminalCurrentColorScheme );
		setTerminalColorScheme( ++it != mTerminalColorSchemes.end()
									? it->first
									: mTerminalColorSchemes.begin()->first );
	} );
	term->setCommand( UITerminal::getExclusiveModeToggleCommandName(),
					  [term] { term->setExclusiveMode( !term->getExclusiveMode() ); } );
	term->setCommand( "move-tab-to-start", [this] {
		auto widget = mApp->getSplitter()->getCurWidget();
		if ( widget == nullptr || widget->getData() == 0 )
			return;
		UITabWidget* tabWidget = mApp->getSplitter()->tabWidgetFromWidget( widget );
		if ( tabWidget ) {
			UITab* tab = (UITab*)widget->getData();
			tabWidget->moveTab( tab, 0 );
		}
	} );
	term->setCommand( "move-tab-to-end", [this] {
		auto widget = mApp->getSplitter()->getCurWidget();
		if ( widget == nullptr || widget->getData() == 0 )
			return;
		UITabWidget* tabWidget = mApp->getSplitter()->tabWidgetFromWidget( widget );
		if ( tabWidget ) {
			UITab* tab = (UITab*)widget->getData();
			tabWidget->moveTab( tab, tabWidget->getTabCount() );
		}
	} );
	mApp->registerUnlockedCommands( *term );
	term->getKeyBindings().removeCommandKeybind( "find-replace" );
	term->getKeyBindings().removeCommandKeybind( "open-locatebar" );
	term->setFocus();
	return term;
#endif
}

void TerminalManager::setKeybindings( UITerminal* term ) {
	term->getKeyBindings().reset();
	term->addKeyBinds( mApp->getRealLocalKeybindings() );
	term->addKeyBinds( mApp->getRealSplitterKeybindings() );
	// Remove the keybinds that are problematic for a terminal
	static constexpr std::string_view excluded[] = { "open-file",
													 "download-file-web",
													 "open-folder",
													 "debug-draw-highlight-toggle",
													 "debug-draw-boxes-toggle",
													 "debug-draw-debug-data",
													 "debug-widget-tree-view",
													 "open-locatebar",
													 "open-command-palette",
													 "open-global-search",
													 "menu-toggle",
													 "console-toggle",
													 "go-to-line",
													 "editor-go-back",
													 "editor-go-forward",
													 "project-run-executable",
													 "project-build-and-run",
													 "find-replace" };
	for ( const auto& shortcut :
		  KeyBindings::getOrderedShortcuts( term->getKeyBindings().getShortcutMap() ) ) {
		const auto command = term->getKeyBindings().getCommandFromKeyBind( shortcut );
		if ( std::find( std::begin( excluded ), std::end( excluded ), command ) !=
			 std::end( excluded ) )
			term->getKeyBindings().removeKeybind( shortcut );
	}
	// Terminal bindings must be installed last so they can intentionally reuse shortcuts removed
	// from the editor/global context (for example mod+shift+f).
	term->addKeyBinds( mApp->getRealTerminalKeybindings() );
	if ( auto* controller = mApp->getStatusTerminalController(); controller ) {
		if ( auto* tabs = controller->getTabWidget(); tabs && tabs->inParentTreeOf( term ) )
			term->getKeyBindings().addKeybindsStringUnordered( mApp->getStatusBarKeybindings() );
	}
	const auto& section = mApp->getTerminalKeybindings();
	// Rebuild both lookups after removing every inherited alias of a terminal-owned command.
	// Merely removing the Main shortcut leaves alternatives and stale context-menu labels.
	auto inherited = term->getKeyBindings().getShortcutMap();
	std::erase_if( inherited, [&]( const auto& entry ) {
		return section.isUnbound( entry.second ) ||
			   std::any_of(
				   section.bindings.begin(), section.bindings.end(),
				   [&]( const auto& binding ) { return binding.second.command == entry.second; } );
	} );
	term->getKeyBindings().reset();
	term->addKeyBinds( inherited );
	section.apply( term->getKeyBindings() );
}

} // namespace ecode
