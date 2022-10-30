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
		for ( auto& file : colorSchemesFiles ) {
			auto colorSchemesInFile = TerminalColorScheme::loadFromFile( file );
			for ( auto& coloScheme : colorSchemesInFile )
				colorSchemes.emplace_back( coloScheme );
		}
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
		{ { KEY_E, KEYMOD_CTRL | KEYMOD_LALT | KEYMOD_SHIFT },
		  UITerminal::getExclusiveModeToggleCommandName() },
		{ { KEY_S, KEYMOD_LALT | KEYMOD_CTRL }, "terminal-rename" },
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

UIMenu* TerminalManager::createColorSchemeMenu() {
	mColorSchemeMenuesCreatedWithHeight = mApp->uiSceneNode()->getPixelsSize().getHeight();
	size_t maxItems = 19;
	auto cb = [&]( const Event* event ) {
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
												const std::string& workingDir ) {
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
	UITerminal* term = UITerminal::New(
		mApp->getTerminalFont() ? mApp->getTerminalFont() : mApp->getFontMono(),
		mApp->termConfig().fontSize.asPixels( 0, Sizef(), mApp->getDisplayDPI() ), Sizef( 16, 16 ),
		"", {},
		!workingDir.empty()
			? workingDir
			: ( !mApp->getCurrentProject().empty() ? mApp->getCurrentProject() : "" ),
		10000, nullptr, mUseFrameBuffer );
	term->getTerm()->getTerminal()->setAllowMemoryTrimnming( true );
	auto ret = mApp->getSplitter()->createWidgetInTabWidget(
		tabWidget, term, title.empty() ? mApp->i18n( "shell", "Shell" ).toUtf8() : title, true );
	mApp->getSplitter()->removeUnusedTab( tabWidget );
	ret.first->setIcon( mApp->findIcon( "filetype-bash" ) );
	term->setTitle( title );
	auto csIt = mTerminalColorSchemes.find( mTerminalCurrentColorScheme );
	term->setColorScheme( csIt != mTerminalColorSchemes.end()
							  ? mTerminalColorSchemes.at( mTerminalCurrentColorScheme )
							  : TerminalColorScheme::getDefault() );
	term->addEventListener( Event::OnTitleChange, [&]( const Event* event ) {
		if ( event->getNode() != mApp->getSplitter()->getCurWidget() )
			return;
		mApp->setAppTitle( event->getNode()->asType<UITerminal>()->getTitle() );
	} );
	term->addKeyBinds( mApp->getLocalKeybindings() );
	term->addKeyBinds( UICodeEditorSplitter::getLocalDefaultKeybindings() );
	term->addKeyBinds( getTerminalKeybindings() );
	// Remove the keybinds that are problematic for a terminal
	term->getKeyBindings().removeCommandsKeybind(
		{ "open-file", "download-file-web", "open-folder", "debug-draw-highlight-toggle",
		  "debug-draw-boxes-toggle", "debug-draw-debug-data", "debug-widget-tree-view",
		  "open-locatebar", "open-global-search", "menu-toggle", "console-toggle", "go-to-line" } );
	term->setCommand( "terminal-rename", [&, term] {
		UIMessageBox* msgBox = UIMessageBox::New(
			UIMessageBox::INPUT, mApp->i18n( "new_terminal_name", "New terminal name:" ) );
		msgBox->setTitle( "ecode" );
		msgBox->getTextInput()->setHint( mApp->i18n( "any_name", "Any name..." ) );
		msgBox->setCloseShortcut( { KEY_ESCAPE, KEYMOD_NONE } );
		msgBox->showWhenReady();
		msgBox->addEventListener( Event::MsgBoxConfirmClick, [&, msgBox, term]( const Event* ) {
			std::string title( msgBox->getTextInput()->getText().toUtf8() );
			term->setTitle( title );
			msgBox->close();
			term->setFocus();
		} );
	} );
	term->setCommand( "switch-to-previous-colorscheme", [&] {
		auto it = mTerminalColorSchemes.find( mTerminalCurrentColorScheme );
		auto prev = std::prev( it, 1 );
		if ( prev != mTerminalColorSchemes.end() ) {
			setTerminalColorScheme( prev->first );
		} else {
			setTerminalColorScheme( mTerminalColorSchemes.rbegin()->first );
		}
	} );
	term->setCommand( "switch-to-next-colorscheme", [&] {
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

} // namespace ecode
