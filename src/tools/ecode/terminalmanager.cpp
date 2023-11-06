#include "terminalmanager.hpp"
#include "ecode.hpp"
#include <eepp/system/filesystem.hpp>

namespace ecode {

TerminalManager::TerminalManager( App* app ) : mApp( app ) {}

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
	auto setShellFn = []( App* app, UIWindow* window, UIComboBox* shellCombo ) {
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
	ok->onClick(
		[&, window, shellCombo]( const MouseEvent* ) { setShellFn( mApp, window, shellCombo ); },
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
	UIMessageBox* msgBox = UIMessageBox::New(
		UIMessageBox::INPUT,
		mApp->i18n( "configure_terminal_scrollback", "Configure terminal scrollback:" ) );
	msgBox->setTitle( mApp->getWindowTitle() );
	msgBox->setCloseShortcut( { KEY_ESCAPE, 0 } );
	msgBox->getTextInput()->setAllowOnlyNumbers( true, false );
	msgBox->getTextInput()->setText( String::toString( mApp->getConfig().term.scrollback ) );
	msgBox->center();
	msgBox->showWhenReady();
	msgBox->on( Event::OnConfirm, [&, msgBox]( const Event* ) {
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
			menu->addSubMenu( mApp->i18n( "more...", "More..." ), nullptr, newMenu );
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

UITerminal* TerminalManager::createNewTerminal( const std::string& title, UITabWidget* inTabWidget,
												const std::string& workingDir, std::string program,
												const std::vector<std::string>& args ) {
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
	if ( term->getTerm() == nullptr ) {
		UIMessageBox* msgBox = UIMessageBox::New(
			UIMessageBox::OK,
			mApp->i18n( "feature_not_supported_in_os",
						"This feature is not supported in this Operating System" ) );
		msgBox->showWhenReady();
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
	term->setCommand( "terminal-rename", [&, term] {
		UIMessageBox* msgBox = UIMessageBox::New(
			UIMessageBox::INPUT, mApp->i18n( "new_terminal_name", "New terminal name:" ) );
		msgBox->setTitle( mApp->getWindowTitle() );
		msgBox->getTextInput()->setHint( mApp->i18n( "any_name", "Any name..." ) );
		msgBox->setCloseShortcut( { KEY_ESCAPE, KEYMOD_NONE } );
		msgBox->showWhenReady();
		msgBox->addEventListener( Event::OnConfirm, [&, msgBox, term]( const Event* ) {
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
		  "console-toggle", "go-to-line", "editor-go-back", "editor-go-forward" } );
}

} // namespace ecode
