#include "terminalmanager.hpp"
#include "ecode.hpp"
#include <eepp/system/filesystem.hpp>

using namespace std::literals;

namespace ecode {

TerminalManager::TerminalManager( App* app ) : mApp( app ) {}

UITerminal* TerminalManager::createTerminalInSplitter( const std::string& workingDir,
													   bool fallback ) {
#if EE_PLATFORM == EE_PLATFORM_WIN
	std::string os = Sys::getOSName( true );
	if ( !LuaPattern::matches( os, "Windows 1%d"sv ) &&
		 !LuaPattern::matches( os, "Windows Server 201[69]"sv ) &&
		 !LuaPattern::matches( os, "Windows Server 202%d"sv ) )
		return nullptr;
#endif

	UITerminal* term = nullptr;
	auto splitter = mApp->getSplitter();
	auto& config = mApp->getConfig();
	if ( splitter && splitter->hasSplit() ) {
		if ( splitter->getTabWidgets().size() == 2 ) {
			UIOrientation orientation = splitter->getMainSplitOrientation();
			if ( config.term.newTerminalOrientation == NewTerminalOrientation::Vertical &&
				 orientation == UIOrientation::Horizontal ) {
				term = createNewTerminal( "", splitter->getTabWidgets()[1], workingDir, "", {},
										  fallback );
			} else if ( config.term.newTerminalOrientation == NewTerminalOrientation::Horizontal &&
						orientation == UIOrientation::Vertical ) {
				term = createNewTerminal( "", splitter->getTabWidgets()[1], workingDir, "", {},
										  fallback );
			} else {
				term = createNewTerminal( "", nullptr, workingDir );
			}
		} else {
			term = createNewTerminal();
		}
	} else if ( splitter ) {
		switch ( config.term.newTerminalOrientation ) {
			case NewTerminalOrientation::Vertical: {
				auto cwd = workingDir.empty() ? mApp->getCurrentWorkingDir() : workingDir;
				splitter->split( UICodeEditorSplitter::SplitDirection::Right,
								 splitter->getCurWidget(), false );
				term = createNewTerminal( "", nullptr, cwd, "", {}, fallback );
				break;
			}
			case NewTerminalOrientation::Horizontal: {
				auto cwd = workingDir.empty() ? mApp->getCurrentWorkingDir() : workingDir;
				splitter->split( UICodeEditorSplitter::SplitDirection::Bottom,
								 splitter->getCurWidget(), false );
				term = createNewTerminal( "", nullptr, cwd, "", {}, fallback );
				break;
			}
			case NewTerminalOrientation::Same: {
				term = createNewTerminal( "", nullptr, "", "", {}, fallback );
				break;
			}
		}
	}
	return term;
}

void TerminalManager::applyTerminalColorScheme( const TerminalColorScheme& colorScheme ) {
	mApp->getSplitter()->forEachWidget( [colorScheme]( UIWidget* widget ) {
		if ( widget->isType( UI_TYPE_TERMINAL ) )
			widget->asType<UITerminal>()->setColorScheme( colorScheme );
	} );

	if ( mApp->getStatusTerminalController() &&
		 mApp->getStatusTerminalController()->getUITerminal() ) {
		mApp->getStatusTerminalController()->getUITerminal()->setColorScheme( colorScheme );
	}
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

std::map<KeyBindings::Shortcut, std::string> TerminalManager::getTerminalKeybindings() {
	return {
		{ { KEY_T, KeyMod::getDefaultModifier() | KEYMOD_SHIFT }, "create-new-terminal" },
		{ { KEY_E, KeyMod::getDefaultModifier() | KEYMOD_LALT | KEYMOD_SHIFT },
		  UITerminal::getExclusiveModeToggleCommandName() },
		{ { KEY_S, KEYMOD_LALT | KeyMod::getDefaultModifier() }, "terminal-rename" },
	};
}

const std::string& TerminalManager::getTerminalColorSchemesPath() const {
	return mTerminalColorSchemesPath;
}

void TerminalManager::setTerminalColorSchemesPath( const std::string& terminalColorSchemesPath ) {
	mTerminalColorSchemesPath = terminalColorSchemesPath;
}

void TerminalManager::updateColorSchemeMenu() {
	for ( UIPopUpMenu* menu : mColorSchemeMenues ) {
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
	return mUseFrameBuffer;
}

void TerminalManager::setUseFrameBuffer( bool useFrameBuffer ) {
	mUseFrameBuffer = useFrameBuffer;
}

void TerminalManager::configureTerminalShell() {
	static const auto layout( R"xml(
		<window layout_width="300dp" layout_height="150dp" window-flags="default|shadow" window-title='@string(shell_configuration, "Shell Configuration")'>
		<vbox lw="mp" lh="mp" padding="4dp">
			<vbox lw="mp" lh="0" lw8="1">
			<textview text='@string(configure_default_shell, "Configure default shell")' font-size="14dp" margin-bottom="8dp" />
			<ComboBox id="shell_combo" layout_width="match_parent" layout_height="wrap_content" selectedIndex="0" popup-to-root="true"></ComboBox>
			</vbox>
			<hbox layout_gravity="right">
				<pushbutton id="ok" text="@string(msg_box_ok, Ok)" icon="ok" />
				<pushbutton id="cancel" text="@string(msg_box_cancel, Cancel)" margin-left="8dp" icon="cancel" />
			</hbox>
		</vbox>
		</window>
	)xml" );
	UIWindow* window = mApp->getUISceneNode()->loadLayoutFromString( layout )->asType<UIWindow>();
	UIComboBox* shellCombo = window->find<UIComboBox>( "shell_combo" );
	UIPushButton* ok = window->find<UIPushButton>( "ok" );
	UIPushButton* cancel = window->find<UIPushButton>( "cancel" );
	const std::vector<std::string> list{
		"bash", "sh", "zsh", "fish", "nu", "csh", "tcsh", "ksh", "dash", "cmd", "powershell",
	};
	std::vector<String> found;
	for ( const auto& i : list ) {
		std::string f( Sys::which( i ) );
		if ( !f.empty() )
			found.emplace_back( std::move( f ) );
	}
	shellCombo->getListBox()->addListBoxItems( found );
	const char* shellEnv = std::getenv( "SHELL" );
	if ( !mApp->termConfig().shell.empty() ) {
		shellCombo->getListBox()->setSelected( mApp->termConfig().shell );
		shellCombo->setText( mApp->termConfig().shell );
	} else if ( shellEnv ) {
		std::string shellEnvStr( FileSystem::fileExists( shellEnv ) ? shellEnv
																	: Sys::which( shellEnv ) );
		if ( !shellEnvStr.empty() )
			shellCombo->getListBox()->setSelected( shellEnvStr );
	} else if ( Sys::getPlatform() == "Windows" ) {
		std::string shellEnvStr( Sys::which( "powershell" ) );
		if ( !shellEnvStr.empty() )
			shellCombo->getListBox()->setSelected( shellEnvStr );
	}
	const auto setShellFn = []( App* app, UIWindow* window, UIComboBox* shellCombo ) {
		std::string shell( shellCombo->getText().toUtf8() );
		if ( !Sys::which( shell ).empty() || FileSystem::fileExists( shell ) ) {
			app->getConfig().term.shell = shell;
			window->closeWindow();
		} else {
			app->errorMsgBox( app->i18n(
				"shell_not_found", "The shell selected was not found in the file system.\nMake "
								   "sure that the shell is visible in your PATH" ) );
		}
	};
	if ( shellCombo->getListBox()->getVerticalScrollBar() &&
		 found.size() > shellCombo->getDropDownList()->getMaxNumVisibleItems() )
		shellCombo->getListBox()->getVerticalScrollBar()->setClickStep(
			shellCombo->getDropDownList()->getMaxNumVisibleItems() / (Float)found.size() );
	ok->setFocus();
	ok->onClick( [this, setShellFn, window,
				  shellCombo]( const MouseEvent* ) { setShellFn( mApp, window, shellCombo ); },
				 MouseButton::EE_BUTTON_LEFT );
	cancel->onClick( [window]( const MouseEvent* ) { window->closeWindow(); }, EE_BUTTON_LEFT );
	window->on( Event::KeyDown, [window]( const Event* event ) {
		if ( event->asKeyEvent()->getKeyCode() == KEY_ESCAPE )
			window->closeWindow();
	} );
	window->center();
	window->on( Event::OnWindowReady, [ok]( const Event* ) { ok->setFocus(); } );
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

UIMenu* TerminalManager::createColorSchemeMenu() {
	mColorSchemeMenuesCreatedWithHeight = mApp->uiSceneNode()->getPixelsSize().getHeight();
	size_t maxItems = 19;
	auto cb = [this]( const Event* event ) {
		UIMenuItem* item = event->getNode()->asType<UIMenuItem>();
		const String& name = item->getText();
		setTerminalColorScheme( name );
	};

	UIPopUpMenu* menu = UIPopUpMenu::New();
	menu->addEventListener( Event::OnItemClicked, cb );
	mColorSchemeMenues.push_back( menu );
	size_t total = 0;
	const auto& colorSchemes = mTerminalColorSchemes;

	for ( auto& colorScheme : colorSchemes ) {
		menu->addRadioButton( colorScheme.first, mTerminalCurrentColorScheme == colorScheme.first );

		if ( mColorSchemeMenues.size() == 1 && menu->getCount() == 1 ) {
			menu->reloadStyle( true, true );
			Float height = menu->getPixelsSize().getHeight();
			Float tHeight = mApp->uiSceneNode()->getPixelsSize().getHeight();
			maxItems = (int)eeceil( tHeight / height ) - 2;
		}

		total++;

		if ( menu->getCount() == maxItems && colorSchemes.size() - total > 1 ) {
			UIPopUpMenu* newMenu = UIPopUpMenu::New();
			menu->addSubMenu( mApp->i18n( "more_ellipsis", "More..." ), nullptr, newMenu );
			newMenu->addEventListener( Event::OnItemClicked, cb );
			mColorSchemeMenues.push_back( newMenu );
			menu = newMenu;
		}
	}

	return mColorSchemeMenues[0];
}

void TerminalManager::updateMenuColorScheme( UIMenuSubMenu* colorSchemeMenu ) {
	if ( mColorSchemeMenuesCreatedWithHeight != mApp->uiSceneNode()->getPixelsSize().getHeight() ) {
		for ( UIPopUpMenu* menu : mColorSchemeMenues )
			menu->close();
		mColorSchemeMenues.clear();
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

static void openExternal( const std::string& defShell, const std::string& cmd,
						  const std::string& scriptsPath, const std::string& workingDir ) {
	// This is an utility bat script based in the Geany utility script called "geany-run-helper"
	static const std::string RUN_HELPER =
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
		if ( !FileSystem::fileExists( runHelperPath ) )
			FileSystem::fileWrite( runHelperPath, RUN_HELPER );
		std::string cmdDir = String::trim( FileSystem::fileRemoveFileName( cmd ) );
		if ( cmdDir.empty() )
			cmdDir = workingDir;
		std::string cmdFile = String::trim( FileSystem::fileNameFromPath( cmd ) );
		auto fcmd = "cmd.exe /q /c " + quoteString( "\"" + runHelperPath + "\" \"" + cmdDir +
													"\" 0 \"" + cmdFile + "\"" );
		Log::info( "Running: %s", fcmd );
		Sys::execute( fcmd, workingDir );
		return;
	}

	std::vector<std::string> options;
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
				Sys::execute( fcmd, workingDir );
				return;
			} else {
				Sys::execute( externalShell, workingDir );
				return;
			}
		}
	}
}
#elif EE_PLATFORM == EE_PLATFORM_MACOS
static void openExternal( const std::string&, const std::string& cmd, const std::string&,
						  const std::string& workingDir ) {
	static const std::string externalShell = "open -a terminal";
	if ( !cmd.empty() )
		std::string fcmd = externalShell + " \"" + cmd + "\"";
	Log::info( "Running: %s", fcmd );
	Sys::execute( fcmd, workingDir );
	else Sys::execute( externalShell, workingDir );
}
#else
static void openExternal( const std::string&, const std::string& cmd, const std::string&,
						  const std::string& workingDir ) {
	std::vector<std::string> options = { "gnome-terminal", "konsole", "xterm", "st" };
	for ( const auto& option : options ) {
		auto externalShell( Sys::which( option ) );
		if ( !externalShell.empty() ) {
			if ( !cmd.empty() ) {
				auto fcmd = externalShell + " -e \"" + cmd + "\"";
				Log::info( "Running: %s", fcmd );
				Sys::execute( fcmd, workingDir );
				return;
			} else {
				Sys::execute( externalShell, workingDir );
				return;
			}
		}
	}
}
#endif

void TerminalManager::openInExternalTerminal( const std::string& cmd,
											  const std::string& workingDir ) {
	openExternal( mApp->termConfig().shell, cmd, mApp->getScriptsPath(), workingDir );
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

UITerminal* TerminalManager::createNewTerminal( const std::string& title, UITabWidget* inTabWidget,
												const std::string& workingDir, std::string program,
												const std::vector<std::string>& args,
												bool fallback ) {
#if EE_PLATFORM == EE_PLATFORM_EMSCRIPTEN
	UIMessageBox* msgBox = UIMessageBox::New(
		UIMessageBox::OK,
		mApp->i18n( "feature_not_supported_in_emscripten",
					"This feature is only supported in the desktop version of ecode." ) );
	msgBox->showWhenReady();
	return nullptr;
#else
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
			// Minor hack. Force the Scene Node to update the styles and layouts.
			tabWidget->getUISceneNode()->update( Time::Zero );
			initialSize = tabWidget->getContainerNode()->getPixelsSize();
		}
	}

	if ( program.empty() && !mApp->termConfig().shell.empty() )
		program = mApp->termConfig().shell;

	UITerminal* term = UITerminal::New(
		mApp->getTerminalFont() ? mApp->getTerminalFont() : mApp->getFontMono(),
		mApp->termConfig().fontSize.asPixels( 0, Sizef(), mApp->getDisplayDPI() ), initialSize,
		program, args, !workingDir.empty() ? workingDir : mApp->getCurrentWorkingDir(),
		mApp->termConfig().scrollback, nullptr, mUseFrameBuffer );

	if ( term == nullptr || term->getTerm() == nullptr ) {
		if ( fallback )
			displayError( workingDir );
		return nullptr;
	}

	auto ret = mApp->getSplitter()->createWidgetInTabWidget(
		tabWidget, term, title.empty() ? mApp->i18n( "shell", "Shell" ).toUtf8() : title, true );
	ret.first->setIcon( mApp->findIcon( "filetype-bash" ) );
	mApp->getSplitter()->removeUnusedTab( tabWidget, true, false );

	term->setTitle( title );
	auto csIt = mTerminalColorSchemes.find( mTerminalCurrentColorScheme );
	term->getTerm()->getTerminal()->setAllowMemoryTrimnming( true );
	term->setColorScheme( csIt != mTerminalColorSchemes.end()
							  ? mTerminalColorSchemes.at( mTerminalCurrentColorScheme )
							  : TerminalColorScheme::getDefault() );
	term->addEventListener( Event::OnTitleChange, [this]( const Event* event ) {
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
		msgBox->addEventListener( Event::OnConfirm, [msgBox, term]( const Event* ) {
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
	term->setCommand( UITerminal::getExclusiveModeToggleCommandName(), [term, this] {
		term->setExclusiveMode( !term->getExclusiveMode() );
		mApp->updateTerminalMenu();
	} );
	mApp->registerUnlockedCommands( *term );
	mApp->getSplitter()->registerSplitterCommands( *term );
	term->setFocus();
	return term;
#endif
}

void TerminalManager::setKeybindings( UITerminal* term ) {
	term->getKeyBindings().reset();
	term->addKeyBinds( mApp->getRealLocalKeybindings() );
	term->addKeyBinds( mApp->getRealSplitterKeybindings() );
	term->addKeyBinds( mApp->getRealTerminalKeybindings() );
	// Remove the keybinds that are problematic for a terminal
	term->getKeyBindings().removeCommandsKeybind(
		{ "open-file", "download-file-web", "open-folder", "debug-draw-highlight-toggle",
		  "debug-draw-boxes-toggle", "debug-draw-debug-data", "debug-widget-tree-view",
		  "open-locatebar", "open-command-palette", "open-global-search", "menu-toggle",
		  "console-toggle", "go-to-line", "editor-go-back", "editor-go-forward",
		  "project-run-executable" } );
}

} // namespace ecode
