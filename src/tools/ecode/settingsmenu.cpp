#include "settingsmenu.hpp"
#include "uitreeviewfs.hpp"

namespace ecode {

String SettingsMenu::i18n( const std::string& key, const String& def ) {
	return mApp->i18n( key, def );
}

std::string SettingsMenu::getKeybind( const std::string& command ) {
	return mApp->getKeybind( command );
}

DrawablePtr SettingsMenu::findIcon( const std::string& name ) {
	return mApp->findIcon( name );
}

void SettingsMenu::runCommand( const std::string& command ) {
	mApp->runCommand( command );
}

void SettingsMenu::createSettingsMenu( App* app, UIMenuBar* menuBar ) {
	Clock clock;
	mApp = app;
	mUISceneNode = app->getUISceneNode();
	mSplitter = app->getSplitter();
	mMenuBar = menuBar;

	mSettingsMenu = UIPopUpMenu::New();
	mSettingsMenu->setId( "settings_menu" );
	mSettingsMenu
		->add( i18n( "new_file", "New File" ), findIcon( "document-new" ),
			   getKeybind( "create-new" ) )
		->setId( "create-new" );
	mSettingsMenu
		->add( i18n( "new_terminal", "New Terminal" ), findIcon( "terminal" ),
			   getKeybind( "create-new-terminal" ) )
		->setId( "create-new-terminal" );
	mSettingsMenu
		->add( i18n( "new_window", "New Window" ), findIcon( "window" ),
			   getKeybind( "create-new-window" ) )
		->setId( "create-new-window" );
	mSettingsMenu
		->add( i18n( "open_file_ellipsis", "Open File..." ), findIcon( "document-open" ),
			   getKeybind( "open-file" ) )
		->setId( "open-file" );
	mSettingsMenu
		->add( i18n( "open_folder_ellipsis", "Open Folder..." ), findIcon( "document-open" ),
			   getKeybind( "open-folder" ) )
		->setId( "open-folder" );
	mSettingsMenu
		->add( i18n( "open_file_from_web_ellipsis", "Open File from Web..." ),
			   findIcon( "download-cloud" ), getKeybind( "download-file-web" ) )
		->setId( "download-file-web" );
	mSettingsMenu
		->addSubMenu( i18n( "recent_files", "Recent Files" ), findIcon( "document-recent" ),
					  ( mRecentFilesMenu = UIPopUpMenu::New() ) )
		->setId( "menu-recent-files" );
	mSettingsMenu
		->addSubMenu( i18n( "recent_folders", "Recent Folders" ), findIcon( "document-recent" ),
					  UIPopUpMenu::New() )
		->setId( "recent-folders" );
	mSettingsMenu->addSeparator();
	mSettingsMenu
		->add( i18n( "save", "Save" ), findIcon( "document-save" ), getKeybind( "save-doc" ) )
		->setId( "save-doc" );
	mSettingsMenu
		->add( i18n( "save_as_ellipsis", "Save as..." ), findIcon( "document-save-as" ),
			   getKeybind( "save-as-doc" ) )
		->setId( "save-as-doc" );
	mSettingsMenu
		->add( i18n( "save_all", "Save All" ), findIcon( "document-save-as" ),
			   getKeybind( "save-all" ) )
		->setId( "save-all" );
	mSettingsMenu->addSeparator()->setId( "settings-submenus-sep" );
	mSettingsMenu
		->add( i18n( "open_settings_ellipsis", "Open Settings..." ), findIcon( "settings" ),
			   getKeybind( "open-settings" ) )
		->setId( "open-settings" );
	mSettingsMenu
		->add( i18n( "open_project_settings_ellipsis", "Open Project Settings..." ),
			   findIcon( "folder-settings" ), getKeybind( "open-project-settings" ) )
		->setId( "open-project-settings" )
		->setEnabled( mApp->projectIsOpen() );
	mSettingsMenu->addSeparator();

	mProjectMenu = UIPopUpMenu::New();
	mDocMenu = UIPopUpMenu::New();
	const auto buildDocMenu = [this] {
		if ( mDocMenu->getCount() == 0 ) {
			createDocumentMenu();
			mDocMenu->reloadStyle( true, true );
		}
	};
	const auto lazyBuildDocMenu = [this, buildDocMenu]( const Event* ) {
		buildDocMenu();
		updateDocumentMenu();
	};
	const auto lazyBuildProjectMenu = [this]( const Event* ) {
		if ( mProjectMenu->getCount() == 0 ) {
			createProjectMenu();
			mProjectMenu->reloadStyle( true, true );
		}
		updateProjectSettingsMenu();
	};
	auto projectMenuButton =
		mSettingsMenu
			->addSubMenu( i18n( "project", "Project" ), findIcon( "folder-user" ), mProjectMenu )
			->setId( "project_settings" )
			->asType<UIWidget>();
	mProjectMenu->on( Event::OnMenuShow, lazyBuildProjectMenu );
	projectMenuButton->on( Event::OnMenuShow, lazyBuildProjectMenu );

	auto docMenuButton =
		mSettingsMenu->addSubMenu( i18n( "document", "Document" ), findIcon( "file" ), mDocMenu )
			->setId( "doc-menu" )
			->asType<UIWidget>();
	mDocMenu->on( Event::OnMenuShow, lazyBuildDocMenu );
	docMenuButton->on( Event::OnMenuShow, lazyBuildDocMenu );

	mTerminalMenu = UIPopUpMenu::New();
	const auto lazyBuildTerminalMenu = [this]( const Event* ) {
		if ( mTerminalMenu->getCount() == 0 ) {
			createTerminalMenu();
			mTerminalMenu->reloadStyle( true, true );
		}
		updateTerminalMenu();
	};
	auto terminalMenuButton =
		mSettingsMenu
			->addSubMenu( i18n( "terminal", "Terminal" ), findIcon( "terminal" ), mTerminalMenu )
			->setId( "term-menu" )
			->asType<UIWidget>();
	mTerminalMenu->on( Event::OnMenuShow, lazyBuildTerminalMenu );
	terminalMenuButton->on( Event::OnMenuShow, lazyBuildTerminalMenu );

	mEditMenu = UIPopUpMenu::New();
	const auto lazyBuildEditMenu = [this]( const Event* ) {
		if ( mEditMenu->getCount() == 0 ) {
			createEditMenu();
			mEditMenu->reloadStyle( true, true );
		}
		updateEditMenu();
	};
	auto editMenuButton = mSettingsMenu->addSubMenu( i18n( "edit", "Edit" ), nullptr, mEditMenu )
							  ->setId( "edit-menu" )
							  ->asType<UIWidget>();
	mEditMenu->on( Event::OnMenuShow, lazyBuildEditMenu );
	editMenuButton->on( Event::OnMenuShow, lazyBuildEditMenu );

	mViewMenu = UIPopUpMenu::New();
	const auto lazyBuildViewMenu = [this]( const Event* ) {
		if ( mViewMenu->getCount() == 0 ) {
			createViewMenu();
			mViewMenu->reloadStyle( true, true );
		}
		updateViewMenu();
	};
	auto viewMenuButton = mSettingsMenu->addSubMenu( i18n( "view", "View" ), nullptr, mViewMenu )
							  ->setId( "view-menu" )
							  ->asType<UIWidget>();
	mViewMenu->on( Event::OnMenuShow, lazyBuildViewMenu );
	viewMenuButton->on( Event::OnMenuShow, lazyBuildViewMenu );

	mToolsMenu = UIPopUpMenu::New();
	const auto lazyBuildToolsMenu = [this]( const Event* ) {
		if ( mToolsMenu->getCount() == 0 ) {
			createToolsMenu();
			mToolsMenu->reloadStyle( true, true );
		}
	};
	auto toolsMenuButton =
		mSettingsMenu->addSubMenu( i18n( "tools", "Tools" ), findIcon( "tools" ), mToolsMenu )
			->setId( "tools-menu" )
			->asType<UIWidget>();
	mToolsMenu->on( Event::OnMenuShow, lazyBuildToolsMenu );
	toolsMenuButton->on( Event::OnMenuShow, lazyBuildToolsMenu );

	mWindowMenu = UIPopUpMenu::New();
	const auto lazyBuildWindowMenu = [this]( const Event* ) {
		if ( mWindowMenu->getCount() == 0 ) {
			createWindowMenu();
			mWindowMenu->reloadStyle( true, true );
		}
	};
	auto windowMenuButton =
		mSettingsMenu
			->addSubMenu( i18n( "window", "Window" ), findIcon( "window-opt" ), mWindowMenu )
			->setId( "window-menu" )
			->asType<UIWidget>();
	mWindowMenu->on( Event::OnMenuShow, lazyBuildWindowMenu );
	windowMenuButton->on( Event::OnMenuShow, lazyBuildWindowMenu );

	mHelpMenu = UIPopUpMenu::New();
	const auto lazyBuildHelpMenu = [this]( const Event* ) {
		if ( mHelpMenu->getCount() == 0 ) {
			createHelpMenu();
			mHelpMenu->reloadStyle( true, true );
		}
	};
	auto helpMenuButton =
		mSettingsMenu->addSubMenu( i18n( "help", "Help" ), findIcon( "help" ), mHelpMenu )
			->setId( "help-menu" )
			->asType<UIWidget>();
	mHelpMenu->on( Event::OnMenuShow, lazyBuildHelpMenu );
	helpMenuButton->on( Event::OnMenuShow, lazyBuildHelpMenu );

	mSettingsMenu->addSeparator();
	mSettingsMenu
		->add( i18n( "close", "Close" ), findIcon( "document-close" ), getKeybind( "close-tab" ) )
		->setId( "close-tab" );
	mSettingsMenu
		->add( i18n( "close_folder", "Close Folder" ), findIcon( "document-close" ),
			   getKeybind( "close-folder" ) )
		->setId( "close-folder" );
	mSettingsMenu->addSeparator();
	mSettingsMenu->add( i18n( "quit", "Quit" ), findIcon( "quit" ), getKeybind( "close-app" ) )
		->setId( "close-app" );
	mSettingsButton = mUISceneNode->find<UIWidget>( "settings" );
	mSettingsButton->on( Event::MouseClick, [this]( const Event* ) { toggleSettingsMenu(); } );
	mSettingsMenu->on( Event::OnItemClicked, [this]( const Event* event ) {
		if ( !event->getNode()->isType( UI_TYPE_MENUITEM ) )
			return;
		const String& id = event->getNode()->asType<UIMenuItem>()->getId();
		runCommand( id );
	} );
	mApp->updateRecentFiles();
	mApp->updateRecentFolders();

	mMenuBar->setPopUpMenu( 0, mSettingsMenu );
	mMenuBar->setPopUpMenu( 1, getEditMenu() );
	mMenuBar->setPopUpMenu( 2, getViewMenu() );
	mMenuBar->setPopUpMenu( 3, getDocMenu() );
	mMenuBar->setPopUpMenu( 4, getTerminalMenu() );
	mMenuBar->setPopUpMenu( 5, getProjectMenu() );
	mMenuBar->setPopUpMenu( 6, getToolsMenu() );
	mMenuBar->setPopUpMenu( 7, getWindowMenu() );
	mMenuBar->setPopUpMenu( 8, getHelpMenu() );

	auto* menuHint = mUISceneNode->find( "menu_hint" );

	const auto onMenuShowEvent = [this, menuHint]( UIPopUpMenu* menu, UIWidget* menuButton,
												   Uint32 menuBarIndex ) {
		menu->on( Event::OnMenuShow, [this, menuButton, menuBarIndex, menu, menuHint]( auto ) {
			if ( menuBarIndex == 0 && !mApp->isAnyStatusBarSectionVisible() )
				menuHint->setVisible( true );
			menu->setOwnerNode( mApp->getConfig().ui.showMenuBar
									? mMenuBar->getButton( menuBarIndex )->asType<UIWidget>()
									: menuButton );
		} );

		if ( menuBarIndex == 0 ) {
			menu->on( Event::OnMenuHide, [menuHint, this]( auto ) {
				menuHint->setVisible( false );

				if ( mSplitter->getUISceneNode()->getEventDispatcher()->getFocusNode() ==
						 mSettingsMenu &&
					 mSplitter->getCurWidget() ) {
					mSplitter->getCurWidget()->setFocus();
				}
			} );
		}

		menu->on( Event::OnVisibleChange, [this, menuBarIndex]( const Event* event ) {
			if ( mApp->getConfig().ui.showMenuBar ) {
				auto button = mMenuBar->getButton( menuBarIndex );
				if ( event->getNode()->isVisible() ) {
					button->select();
					mMenuBar->setCurrentMenu( event->getNode()->asType<UIPopUpMenu>() );
				} else if ( button->isSelected() ) {
					button->unselect();
					mMenuBar->setCurrentMenu( nullptr );
				}
			}
		} );

		menu->on( Event::OnItemClicked, [this]( const Event* ) {
			if ( mApp->getConfig().ui.showMenuBar )
				mMenuBar->setCurrentMenu( nullptr );
		} );
	};

	onMenuShowEvent( mSettingsMenu, mSettingsButton, 0 );
	onMenuShowEvent( mEditMenu, editMenuButton, 1 );
	onMenuShowEvent( mViewMenu, viewMenuButton, 2 );
	onMenuShowEvent( mDocMenu, docMenuButton, 3 );
	onMenuShowEvent( mTerminalMenu, terminalMenuButton, 4 );
	onMenuShowEvent( mProjectMenu, projectMenuButton, 5 );
	onMenuShowEvent( mToolsMenu, toolsMenuButton, 6 );
	onMenuShowEvent( mWindowMenu, windowMenuButton, 7 );
	onMenuShowEvent( mHelpMenu, helpMenuButton, 8 );

	updateMenu();

	Log::info( "Settings Menu took: %s", clock.getElapsedTime().toString() );
}

UIMenu* SettingsMenu::createFileTypeMenu( bool emptyMenu ) {
	mFileTypeMenusCreatedWithHeight = emptyMenu ? 0 : mUISceneNode->getPixelsSize().getHeight();
	size_t maxItems = 19;
	auto* dM = SyntaxDefinitionManager::instance();
	auto names = dM->getLanguageNames();
	auto cb = [this, dM]( const Event* event ) {
		UIMenuItem* item = event->getNode()->asType<UIMenuItem>();
		const String& name = item->getText();
		if ( mSplitter->curEditorExistsAndFocused() ) {
			mSplitter->getCurEditor()->setSyntaxDefinition(
				dM->getByLanguageName( name.toUtf8() ) );
			updateCurrentFileType();
		}
	};

	UIPopUpMenu* menu = UIPopUpMenu::New();
	menu->on( Event::OnItemClicked, cb );
	mFileTypeMenus.push_back( menu );
	size_t total = 0;

	if ( emptyMenu )
		return mFileTypeMenus[0];

	for ( const auto& name : names ) {
		menu->addRadioButton(
			name, mSplitter->curEditorExistsAndFocused() &&
					  mSplitter->getCurEditor()->getSyntaxDefinition().getLanguageName() == name );

		if ( mFileTypeMenus.size() == 1 && menu->getCount() == 1 ) {
			auto menuBar = mUISceneNode->findByType( UI_TYPE_MENUBAR );
			menu->reloadStyle( true, true );
			Float height = menu->getPixelsSize().getHeight();
			Float tHeight = mUISceneNode->getPixelsSize().getHeight() -
							( menuBar ? menuBar->getPixelsSize().getHeight() : 0 );
			maxItems = (int)eeceil( tHeight / height ) - 2;
		}

		total++;

		if ( menu->getCount() == maxItems && names.size() - total > 1 ) {
			UIPopUpMenu* newMenu = UIPopUpMenu::New();
			menu->addSubMenu( i18n( "more_ellipsis", "More..." ), nullptr, newMenu );
			newMenu->on( Event::OnItemClicked, cb );
			mFileTypeMenus.push_back( newMenu );
			menu = newMenu;
		}
	}

	return mFileTypeMenus[0];
}

UIMenu* SettingsMenu::createColorSchemeMenu( bool emptyMenu ) {
	mColorSchemeMenusCreatedWithHeight = emptyMenu ? 0 : mUISceneNode->getPixelsSize().getHeight();
	size_t maxItems = 19;
	auto cb = [this]( const Event* event ) {
		UIMenuItem* item = event->getNode()->asType<UIMenuItem>();
		const String& name = item->getText();
		mSplitter->setColorScheme( name );
	};

	UIPopUpMenu* menu = UIPopUpMenu::New();
	menu->on( Event::OnItemClicked, cb );
	mColorSchemeMenus.push_back( menu );
	size_t total = 0;
	const auto& colorSchemes = mSplitter->getColorSchemes();

	if ( emptyMenu )
		return mColorSchemeMenus[0];

	for ( auto& colorScheme : colorSchemes ) {
		menu->addRadioButton( colorScheme.first,
							  mSplitter->getCurrentColorSchemeName() == colorScheme.first );

		if ( mColorSchemeMenus.size() == 1 && menu->getCount() == 1 ) {
			auto menuBar = mUISceneNode->findByType( UI_TYPE_MENUBAR );
			menu->reloadStyle( true, true );
			Float height = menu->getPixelsSize().getHeight();
			Float tHeight = mUISceneNode->getPixelsSize().getHeight() -
							( menuBar ? menuBar->getPixelsSize().getHeight() : 0 );
			maxItems = (int)eeceil( tHeight / height ) - 2;
		}

		total++;

		if ( menu->getCount() == maxItems && colorSchemes.size() - total > 1 ) {
			UIPopUpMenu* newMenu = UIPopUpMenu::New();
			menu->addSubMenu( i18n( "more_ellipsis", "More..." ), nullptr, newMenu );
			newMenu->on( Event::OnItemClicked, cb );
			mColorSchemeMenus.push_back( newMenu );
			menu = newMenu;
		}
	}

	return mColorSchemeMenus[0];
}

UITerminal* SettingsMenu::getCurrentTerminal() const {
	UITerminal* splitterTerm =
		mSplitter->getCurWidget() && mSplitter->getCurWidget()->isType( UI_TYPE_TERMINAL )
			? mSplitter->getCurWidget()->asType<UITerminal>()
			: nullptr;
	/* TODO: Think in a better way to detect if the "Current Terminal" is the tab widget terminal */
	if ( splitterTerm == nullptr ) {
		auto stc = mApp->getStatusTerminalController();
		if ( stc && stc->getTabWidget() && stc->getTabWidget()->getTabSelected() &&
			 stc->getTabWidget()->getTabSelected()->getOwnedWidget() &&
			 stc->getTabWidget()->getTabSelected()->getOwnedWidget()->isType( UI_TYPE_TERMINAL ) &&
			 stc->getTabWidget()->getTabSelected()->getOwnedWidget()->hasVisibility() ) {
			return stc->getTabWidget()->getTabSelected()->getOwnedWidget()->asType<UITerminal>();
		}
	}
	return splitterTerm;
}

UIMenu* SettingsMenu::createDocumentMenu() {
	auto setupAutoIndentMenu = [this](
								   UIPopUpMenu* parentMenu, const std::string& menuId,
								   std::function<TextDocument::AutoIndentConfig()> getConfig,
								   std::function<void( TextDocument::AutoIndentConfig )> onClick ) {
		UIPopUpMenu* autoIndentMenu = UIPopUpMenu::New();
		auto subMenu =
			parentMenu->addSubMenu( i18n( "auto_indent", "Auto-Indent" ), nullptr, autoIndentMenu );
		subMenu
			->setTooltipText(
				i18n( "auto_indent_tooltip",
					  "Configures the automatic indentation behavior when pressing Enter.\n"
					  "None: No automatic indentation.\n"
					  "Preserve: Preserves the indentation level of the previous line.\n"
					  "Smart: Preserves indentation and automatically indents between auto-closed "
					  "brackets." ) )
			->setId( menuId );
		subMenu->on( Event::OnMenuShow, [this, autoIndentMenu, getConfig, onClick]( const Event* ) {
			if ( autoIndentMenu->getCount() == 0 ) {
				autoIndentMenu->addRadioButton( i18n( "auto_indent_none", "None" ) )
					->setId( "auto_indent_none" );
				autoIndentMenu->addRadioButton( i18n( "auto_indent_preserve", "Preserve" ) )
					->setId( "auto_indent_preserve" );
				autoIndentMenu->addRadioButton( i18n( "auto_indent_smart", "Smart" ) )
					->setId( "auto_indent_smart" );
				autoIndentMenu->on( Event::OnItemClicked, [onClick]( const Event* event ) {
					const String& text = event->getNode()->asType<UIMenuRadioButton>()->getId();
					TextDocument::AutoIndentConfig autoIndent =
						text == "auto_indent_none"		 ? TextDocument::AutoIndentConfig::None
						: text == "auto_indent_preserve" ? TextDocument::AutoIndentConfig::Preserve
														 : TextDocument::AutoIndentConfig::Smart;
					onClick( autoIndent );
				} );
			}
			TextDocument::AutoIndentConfig currentConfig = getConfig();
			autoIndentMenu->getItemId( "auto_indent_none" )
				->asType<UIMenuRadioButton>()
				->setActive( currentConfig == TextDocument::AutoIndentConfig::None );
			autoIndentMenu->getItemId( "auto_indent_preserve" )
				->asType<UIMenuRadioButton>()
				->setActive( currentConfig == TextDocument::AutoIndentConfig::Preserve );
			autoIndentMenu->getItemId( "auto_indent_smart" )
				->asType<UIMenuRadioButton>()
				->setActive( currentConfig == TextDocument::AutoIndentConfig::Smart );
		} );
		return subMenu;
	};

	// **** CURRENT DOCUMENT ****
	mDocMenu->add( i18n( "current_document", "Current Document" ) )
		->setTextAlign( UI_HALIGN_CENTER );

	mDocMenu
		->addCheckBox(
			i18n( "auto_detect_indent_type_and_width", "Auto Detect Indent Type & Width" ),
			mApp->getConfig().doc.autoDetectIndentType )
		->setId( "auto_indent_cur" );

	setupAutoIndentMenu(
		mDocMenu, "auto_indent_menu_cur",
		[this]() {
			return mSplitter->curEditorExistsAndFocused()
					   ? mSplitter->getCurEditor()->getDocument().getAutoIndent()
					   : TextDocument::AutoIndentConfig::Smart;
		},
		[this]( TextDocument::AutoIndentConfig autoIndent ) {
			if ( mSplitter->curEditorExistsAndFocused() )
				mSplitter->getCurEditor()->getDocument().setAutoIndent( autoIndent );
		} );

	UIMenuSubMenu* fileTypeMenu = mDocMenu->addSubMenu(
		i18n( "file_type", "File Type" ), findIcon( "file-code" ), createFileTypeMenu( true ) );

	fileTypeMenu->on( Event::OnMenuShow, [this, fileTypeMenu]( const Event* ) {
		if ( mFileTypeMenusCreatedWithHeight != mUISceneNode->getPixelsSize().getHeight() ) {
			for ( UIPopUpMenu* menu : mFileTypeMenus )
				menu->close();
			mFileTypeMenus.clear();
			auto* newMenu = createFileTypeMenu();
			newMenu->reloadStyle( true, true );
			fileTypeMenu->setSubMenu( newMenu );
		}
	} );

	UIPopUpMenu* fileEncoding = UIPopUpMenu::New();
	auto encodings = TextFormat::encodings();
	for ( const auto& enc : encodings )
		fileEncoding->addRadioButton( enc.second )->setId( enc.second );
	mDocMenu->addSubMenu( i18n( "file_encoding", "File Encoding" ), nullptr, fileEncoding )
		->setId( "file_encoding" );
	fileEncoding->on( Event::OnItemClicked, [this]( const Event* event ) {
		const String& text = event->getNode()->asType<UIMenuRadioButton>()->getId();
		if ( mSplitter->curEditorExistsAndFocused() ) {
			auto enc = TextFormat::encodingFromString( text.toUtf8() );
			if ( enc == mSplitter->getCurEditor()->getDocument().getEncoding() )
				return;
			mSplitter->getCurEditor()->getDocument().setEncoding( enc );
			mApp->updateDocInfo( mSplitter->getCurEditor()->getDocument() );
			if ( !mSplitter->getCurEditor()->getDocument().hasFilepath() )
				return;
			auto msgBox = UIMessageBox::New(
				UIMessageBox::YES_NO, i18n( "confirm_new_file_encoding",
											"To confirm the new file encoding it's required to "
											"save the file. Do you want to save it now?" ) );
			msgBox->on( Event::OnConfirm, [this]( auto ) {
				if ( mSplitter->curEditorExistsAndFocused() )
					mSplitter->getCurEditor()->getDocument().save();
			} );
			msgBox->showWhenReady();
		}
	} );

	UIPopUpMenu* tabTypeMenu = UIPopUpMenu::New();
	tabTypeMenu->addRadioButton( i18n( "tabs", "Tabs" ) )->setId( "tabs" );
	tabTypeMenu->addRadioButton( i18n( "spaces", "Spaces" ) )->setId( "spaces" );
	mDocMenu->addSubMenu( i18n( "indentation_type", "Indentation Type" ), nullptr, tabTypeMenu )
		->setId( "indent_type_cur" );
	tabTypeMenu->on( Event::OnItemClicked, [this]( const Event* event ) {
		const String& text = event->getNode()->asType<UIMenuRadioButton>()->getId();
		if ( mSplitter->curEditorExistsAndFocused() ) {
			TextDocument::IndentType indentType = text == "tabs"
													  ? TextDocument::IndentType::IndentTabs
													  : TextDocument::IndentType::IndentSpaces;
			mSplitter->getCurEditor()->getDocument().setIndentType( indentType );
		}
	} );

	UIPopUpMenu* indentWidthMenu = UIPopUpMenu::New();
	for ( size_t w = 2; w <= 12; w++ )
		indentWidthMenu
			->addRadioButton( String::toString( (Uint64)w ),
							  mSplitter->curEditorExistsAndFocused() &&
								  mSplitter->getCurEditor()->getDocument().getIndentWidth() == w )
			->setId( String::format( "indent_width_%zu", w ) )
			->setData( w );
	mDocMenu->addSubMenu( i18n( "indent_width", "Indent Width" ), nullptr, indentWidthMenu )
		->setId( "indent_width_cur" );
	indentWidthMenu->on( Event::OnItemClicked, [this]( const Event* event ) {
		if ( mSplitter->curEditorExistsAndFocused() ) {
			int width = event->getNode()->getData();
			mSplitter->getCurEditor()->getDocument().setIndentWidth( width );
		}
	} );

	UIPopUpMenu* tabWidthMenu = UIPopUpMenu::New();
	for ( size_t w = 2; w <= 12; w++ )
		tabWidthMenu
			->addRadioButton( String::toString( (Uint64)w ),
							  mSplitter->curEditorExistsAndFocused() &&
								  mSplitter->getCurEditor()->getTabWidth() == w )
			->setId( String::format( "tab_width_%zu", w ) )
			->setData( w );
	mDocMenu->addSubMenu( i18n( "tab_width", "Tab Width" ), nullptr, tabWidthMenu )
		->setId( "tab_width_cur" );
	tabWidthMenu->on( Event::OnItemClicked, [this]( const Event* event ) {
		if ( mSplitter->curEditorExistsAndFocused() ) {
			int width = event->getNode()->getData();
			mSplitter->getCurEditor()->setTabWidth( width );
		}
	} );

	UIPopUpMenu* lineEndingsMenu = UIPopUpMenu::New();
	lineEndingsMenu
		->addRadioButton( "Windows/DOS (CR/LF)",
						  mApp->getConfig().doc.lineEndings == TextFormat::LineEnding::CRLF )
		->setId( "CRLF" );
	lineEndingsMenu
		->addRadioButton( "Unix (LF)",
						  mApp->getConfig().doc.lineEndings == TextFormat::LineEnding::LF )
		->setId( "LF" );
	lineEndingsMenu
		->addRadioButton( "Macintosh (CR)",
						  mApp->getConfig().doc.lineEndings == TextFormat::LineEnding::CR )
		->setId( "CR" );
	mDocMenu->addSubMenu( i18n( "line_endings", "Line Endings" ), nullptr, lineEndingsMenu )
		->setId( "line_endings_cur" );
	lineEndingsMenu->on( Event::OnItemClicked, [this]( const Event* event ) {
		auto le =
			TextFormat::stringToLineEnding( event->getNode()->asType<UIRadioButton>()->getId() );
		if ( mSplitter->curEditorExistsAndFocused() ) {
			TextDocument& doc = mSplitter->getCurEditor()->getDocument();
			doc.setLineEnding( le );
			doc.setDirtyUntilSave();
			mApp->updateDocInfo( doc );
		}
	} );

	mDocMenu->addCheckBox( i18n( "read_only", "Read Only" ), false, getKeybind( "lock-toggle" ) )
		->setId( "read_only" );

	mDocMenu
		->addCheckBox( i18n( "trim_trailing_whitespaces", "Trim Trailing Whitespaces" ),
					   mApp->getConfig().doc.trimTrailingWhitespaces )
		->setId( "trim_whitespaces_cur" );

	mDocMenu
		->addCheckBox( i18n( "force_new_line_at_end_of_file", "Force New Line at End of File" ),
					   mApp->getConfig().doc.forceNewLineAtEndOfFile )
		->setId( "force_nl_cur" );

	mDocMenu
		->addCheckBox( i18n( "write_unicode_bom", "Write Unicode BOM" ),
					   mApp->getConfig().doc.writeUnicodeBOM )
		->setId( "write_bom_cur" );

	mDocMenu->on( Event::OnItemClicked, [this]( const Event* event ) {
		if ( event->getNode()->getId() == "open-document-settings" ) {
			runCommand( "open-document-settings" );
			return;
		}
		if ( !mSplitter->curEditorExistsAndFocused() ||
			 event->getNode()->isType( UI_TYPE_MENU_SEPARATOR ) ||
			 event->getNode()->isType( UI_TYPE_MENUSUBMENU ) )
			return;
		const String& id = event->getNode()->getId();
		TextDocument& doc = mSplitter->getCurEditor()->getDocument();

		if ( event->getNode()->isType( UI_TYPE_MENUCHECKBOX ) ) {
			UIMenuCheckBox* item = event->getNode()->asType<UIMenuCheckBox>();
			if ( "auto_indent_cur" == id ) {
				doc.setAutoDetectIndentType( item->isActive() );
			} else if ( "trim_whitespaces_cur" == id ) {
				doc.setTrimTrailingWhitespaces( item->isActive() );
			} else if ( "force_nl_cur" == id ) {
				doc.setForceNewLineAtEndOfFile( item->isActive() );
			} else if ( "write_bom_cur" == id ) {
				doc.setBOM( item->isActive() );
			} else if ( "read_only" == id ) {
				mSplitter->getCurEditor()->setLocked( item->isActive() );
			}
		}
	} );

	// Syntax color scheme remains a quick current-document/editor operation.
	mDocMenu->addSeparator()->setId( "end_current_document" );

	UIMenuSubMenu* colorSchemeMenu =
		mDocMenu->addSubMenu( i18n( "syntax_color_scheme", "Syntax Color Scheme" ),
							  findIcon( "palette" ), createColorSchemeMenu( true ) );
	colorSchemeMenu->on( Event::OnMenuShow, [this, colorSchemeMenu]( const Event* ) {
		if ( mColorSchemeMenusCreatedWithHeight != mUISceneNode->getPixelsSize().getHeight() ) {
			for ( UIPopUpMenu* menu : mColorSchemeMenus )
				menu->close();
			mColorSchemeMenus.clear();
			auto* newMenu = createColorSchemeMenu();
			newMenu->reloadStyle( true, true );
			colorSchemeMenu->setSubMenu( newMenu );
		}
	} );
	mDocMenu
		->add( i18n( "open_document_settings", "Open Document Settings" ), findIcon( "settings" ) )
		->setId( "open-document-settings" );

	return mDocMenu;
}

UIMenu* SettingsMenu::createTerminalMenu() {
	mTerminalMenu->add( i18n( "current_terminal", "Current Terminal" ) )
		->setTextAlign( UI_HALIGN_CENTER );

	mTerminalMenu
		->addCheckBox( i18n( "exclusive_mode", "Exclusive Mode" ), false,
					   getKeybind( UITerminal::getExclusiveModeToggleCommandName() ) )
		->setTooltipText( i18n(
			"exclusive_mode_tooltip",
			"Global Keybindings are disabled when exclusive mode is enabled.\nThis is to "
			"avoid keyboard shortcut overlapping between the terminal and the application." ) )
		->setId( "exclusive-mode" );
	mTerminalMenu
		->add( i18n( "rename_session", "Rename Session" ), nullptr,
			   getKeybind( "terminal-rename" ) )
		->setId( "terminal-rename" );

	mTerminalMenu->addSeparator()->setId( "end_current_terminal" );

#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN
	UIMenuSubMenu* termColorSchemeMenu = mTerminalMenu->addSubMenu(
		i18n( "terminal_color_scheme", "Terminal Color Scheme" ), findIcon( "palette" ),
		mApp->getTerminalManager()->createColorSchemeMenu( true ) );
	termColorSchemeMenu->on( Event::OnMenuShow, [this, termColorSchemeMenu]( const Event* ) {
		mApp->getTerminalManager()->updateMenuColorScheme( termColorSchemeMenu );
	} );
#endif

	mTerminalMenu
		->add( i18n( "open_terminal_settings", "Open Terminal Settings" ), findIcon( "settings" ) )
		->setId( "open-terminal-settings" );

	mTerminalMenu->on( Event::OnItemClicked, [this]( const Event* event ) {
		if ( !event->getNode()->isType( UI_TYPE_MENUITEM ) )
			return;
		const std::string& id( event->getNode()->getId() );
		if ( id == "open-terminal-settings" ) {
			runCommand( id );
		} else if ( UITerminal* terminal = getCurrentTerminal() ) {
			if ( id == "exclusive-mode" )
				terminal->setExclusiveMode(
					event->getNode()->asType<UIMenuCheckBox>()->isActive() );
			else
				terminal->execute( id );
		}
	} );

	return mTerminalMenu;
}

UIMenu* SettingsMenu::createEditMenu() {
	mEditMenu->setId( "edit_menu" );
	mEditMenu->add( i18n( "undo", "Undo" ), findIcon( "undo" ), getKeybind( "undo" ) )
		->setId( "undo" );
	mEditMenu->add( i18n( "redo", "Redo" ), findIcon( "redo" ), getKeybind( "redo" ) )
		->setId( "redo" );
	mEditMenu->addSeparator();
	mEditMenu->add( i18n( "cut", "Cut" ), findIcon( "cut" ), getKeybind( "cut" ) )->setId( "cut" );
	mEditMenu->add( i18n( "copy", "Copy" ), findIcon( "copy" ), getKeybind( "copy" ) )
		->setId( "copy" );
	mEditMenu->add( i18n( "paste", "Paste" ), findIcon( "paste" ), getKeybind( "paste" ) )
		->setId( "paste" );
	mEditMenu
		->add( i18n( "delete", "Delete" ), findIcon( "delete-text" ),
			   getKeybind( "delete-to-next-char" ) )
		->setId( "delete-to-next-char" );
	mEditMenu->addSeparator();
	mDateMenu = UIPopUpMenu::New();
	auto* dateMenuItem = mEditMenu->addSubMenu( i18n( "insert_date", "Insert Date" ),
												findIcon( "calendar-2" ), mDateMenu );
	dateMenuItem->setId( "insert_date_menu" );
	dateMenuItem->on( Event::OnMenuShow, [this, dateMenuItem]( const Event* ) {
		mDateMenu->setOwnerNode( dateMenuItem );
		updateDateMenu();
	} );
	mEditMenu->addSeparator();
	mEditMenu
		->add( i18n( "select_all", "Select All" ), findIcon( "select-all" ),
			   getKeybind( "select-all" ) )
		->setId( "select-all" );
	mEditMenu->addSeparator();
	mEditMenu
		->add( i18n( "find_replace", "Find/Replace" ), findIcon( "find-replace" ),
			   getKeybind( "find-replace" ) )
		->setId( "find-replace" );

	mEditMenu->addSeparator();

	mEditMenu
		->add( i18n( "open_containing_folder_in_fm", "Open Containing Folder in File Manager" ),
			   findIcon( "folder-open" ), getKeybind( "open-containing-folder" ) )
		->setId( "open-containing-folder" );
	mEditMenu
		->add( i18n( "copy_containing_folder_path", "Copy Containing Folder Path" ),
			   findIcon( "copy" ), getKeybind( "copy-containing-folder-path" ) )
		->setId( "copy-containing-folder-path" );
	mEditMenu
		->add( i18n( "copy_file_path", "Copy File Path" ), findIcon( "copy" ),
			   getKeybind( "copy-file-path" ) )
		->setId( "copy-file-path" );

	mEditMenu->addSeparator()->setId( "edit-move-sep" );

	mEditMenu
		->add( i18n( "open_in_new_window", "Open in New Window" ), findIcon( "window" ),
			   getKeybind( "open-in-new-window" ) )
		->setId( "open-in-new-window" );

	mEditMenu
		->add( i18n( "move_to_new_window", "Move to New Window" ), findIcon( "window" ),
			   getKeybind( "move-to-new-window" ) )
		->setId( "move-to-new-window" );

	mEditMenu->addSeparator()->setId( "edit-file-sep" );
	mEditMenu
		->add( i18n( "key_bindings", "Key Bindings" ), findIcon( "keybindings" ),
			   getKeybind( "keybindings" ) )
		->setId( "keybindings" );

	mEditMenu->on( Event::OnItemClicked, [this]( const Event* event ) {
		if ( !event->getNode()->isType( UI_TYPE_MENUITEM ) )
			return;
		runCommand( event->getNode()->getId() );
	} );

	return mEditMenu;
}

void SettingsMenu::updateEditMenu() {
	if ( mEditMenu->getCount() == 0 )
		return;

	UIMenuSeparator* moveSep = mEditMenu->find( "edit-move-sep" )->asType<UIMenuSeparator>();
	UIMenuSeparator* fileSep = mEditMenu->find( "edit-file-sep" )->asType<UIMenuSeparator>();

	if ( !mSplitter->curEditorExistsAndFocused() ) {
		mEditMenu->getItemId( "undo" )->setEnabled( false );
		mEditMenu->getItemId( "redo" )->setEnabled( false );
		mEditMenu->getItemId( "copy" )->setEnabled( false );
		mEditMenu->getItemId( "cut" )->setEnabled( false );
		mEditMenu->getItemId( "insert_date_menu" )->setEnabled( false );
		mEditMenu->getItemId( "open-containing-folder" )->setVisible( false );
		mEditMenu->getItemId( "copy-containing-folder-path" )->setVisible( false );
		moveSep->setEnabled( false )->setVisible( false );
		mEditMenu->getItemId( "open-in-new-window" )->setVisible( false );
		mEditMenu->getItemId( "move-to-new-window" )->setVisible( false );
		fileSep->setEnabled( false )->setVisible( false );
		mEditMenu->getItemId( "copy-file-path" )->setVisible( false );
		return;
	}
	auto doc = mSplitter->getCurEditor()->getDocumentRef();
	mEditMenu->getItemId( "undo" )->setEnabled( doc->hasUndo() );
	mEditMenu->getItemId( "redo" )->setEnabled( doc->hasRedo() );
	mEditMenu->getItemId( "copy" )->setEnabled( doc->hasSelection() );
	mEditMenu->getItemId( "cut" )->setEnabled( doc->hasSelection() );
	mEditMenu->getItemId( "insert_date_menu" )->setEnabled( true );
	mEditMenu->getItemId( "open-containing-folder" )->setVisible( doc->hasFilepath() );
	mEditMenu->getItemId( "copy-containing-folder-path" )->setVisible( doc->hasFilepath() );
	moveSep->setEnabled( true )->setVisible( true );
	mEditMenu->getItemId( "open-in-new-window" )->setVisible( doc->hasFilepath() );
	mEditMenu->getItemId( "move-to-new-window" )->setVisible( doc->hasFilepath() );
	fileSep->setEnabled( doc->hasFilepath() )->setVisible( doc->hasFilepath() );
	mEditMenu->getItemId( "copy-file-path" )->setVisible( doc->hasFilepath() );
}

void SettingsMenu::updateDateMenu() {
	if ( mDateMenu->getCount() != 0 )
		return;
	mDateMenu->removeAll();
	mDateMenu->removeEventsOfType( Event::OnItemClicked );

	struct DateFormatCommand {
		const char* command;
		const char* i18nKey;
		const char* label;
	};

	static constexpr DateFormatCommand DATE_COMMANDS[] = {
		{ "insert-date-dd-mm-yyyy", "insert_date_dd_mm_yyyy", "dd.mm.yyyy" },
		{ "insert-date-mm-dd-yyyy", "insert_date_mm_dd_yyyy", "mm.dd.yyyy" },
		{ "insert-date-yyyy-mm-dd", "insert_date_yyyy_mm_dd", "yyyy/mm/dd" },
		{ "insert-date-time-dd-mm-yyyy", "insert_date_time_dd_mm_yyyy", "dd.mm.yyyy hh:mm:ss" },
		{ "insert-date-time-mm-dd-yyyy", "insert_date_time_mm_dd_yyyy", "mm.dd.yyyy hh:mm:ss" },
		{ "insert-date-time-yyyy-mm-dd", "insert_date_time_yyyy_mm_dd", "yyyy/mm/dd hh:mm:ss" },
	};

	for ( const auto& cmd : DATE_COMMANDS ) {
		mDateMenu->add( i18n( cmd.i18nKey, cmd.label ), nullptr, getKeybind( cmd.command ) )
			->setId( cmd.command );
	}

	mDateMenu->addSeparator();
	mDateMenu
		->add( i18n( "use_custom_date_format", "Use Custom Date Format" ), nullptr,
			   getKeybind( "insert-date-custom" ) )
		->setId( "insert-date-custom" );
	mDateMenu
		->add( i18n( "set_custom_date_format", "Set Custom Date Format" ), nullptr,
			   getKeybind( "set-custom-date-format" ) )
		->setId( "set-custom-date-format" );

	mDateMenu->on( Event::OnItemClicked, [this]( const Event* event ) {
		if ( event->getNode()->isType( UI_TYPE_MENUITEM ) )
			runCommand( event->getNode()->getId() );
	} );
}

UIMenu* SettingsMenu::createWindowMenu() {
	UIPopUpMenu* splitMenu = UIPopUpMenu::New();

	for ( const auto& split :
		  { std::pair{ "split-left", "split_left" }, std::pair{ "split-right", "split_right" },
			std::pair{ "split-top", "split_top" }, std::pair{ "split-bottom", "split_bottom" } } ) {
		const bool horizontal = split.first == std::string_view{ "split-left" } ||
								split.first == std::string_view{ "split-right" };
		splitMenu
			->add( i18n( split.second,
						 split.first == std::string_view{ "split-left" }	? "Split Left"
						 : split.first == std::string_view{ "split-right" } ? "Split Right"
						 : split.first == std::string_view{ "split-top" }	? "Split Top"
																			: "Split Bottom" ),
				   findIcon( horizontal ? "split-horizontal" : "split-vertical" ),
				   getKeybind( split.first ) )
			->setId( split.first );
	}
	splitMenu->addSeparator();
	for ( const auto& split : { std::pair{ "terminal-split-left", "terminal_split_left" },
								std::pair{ "terminal-split-right", "terminal_split_right" },
								std::pair{ "terminal-split-top", "terminal_split_top" },
								std::pair{ "terminal-split-bottom", "terminal_split_bottom" } } ) {
		const bool horizontal = split.first == std::string_view{ "terminal-split-left" } ||
								split.first == std::string_view{ "terminal-split-right" };
		splitMenu
			->add( i18n( split.second, split.first == std::string_view{ "terminal-split-left" }
										   ? "Split Terminal Left"
									   : split.first == std::string_view{ "terminal-split-right" }
										   ? "Split Terminal Right"
									   : split.first == std::string_view{ "terminal-split-top" }
										   ? "Split Terminal Top"
										   : "Split Terminal Bottom" ),
				   findIcon( horizontal ? "split-horizontal" : "split-vertical" ),
				   getKeybind( split.first ) )
			->setId( split.first );
	}
	splitMenu->on( Event::OnItemClicked, [this]( const Event* event ) {
		if ( event->getNode()->isType( UI_TYPE_MENUITEM ) )
			runCommand( event->getNode()->getId() );
	} );
	mWindowMenu->addSubMenu( i18n( "split", "Split" ), findIcon( "split-horizontal" ), splitMenu );

	mWindowMenu->addSeparator();
	mWindowMenu
		->add( i18n( "zoom_in", "Zoom In" ), findIcon( "zoom-in" ), getKeybind( "font-size-grow" ) )
		->setId( "zoom-in" );
	mWindowMenu
		->add( i18n( "zoom_out", "Zoom Out" ), findIcon( "zoom-out" ),
			   getKeybind( "font-size-shrink" ) )
		->setId( "zoom-out" );
	mWindowMenu
		->add( i18n( "zoom_reset", "Zoom Reset" ), findIcon( "zoom-reset" ),
			   getKeybind( "font-size-reset" ) )
		->setId( "zoom-reset" );

	mWindowMenu->addSeparator();
	mWindowMenu->add( i18n( "reset_panel_layout", "Reset Panel Layout" ) )
		->setTooltipText( i18n( "reset_panel_layout_tooltip",
								"Restores all panels to their default sizes "
								"(e.g. sidebar, statusbar)." ) )
		->setId( "reset-panel-layout" );
	mWindowMenu->add( i18n( "reset_global_file_associations", "Reset Global File Associations" ) )
		->setTooltipText( i18n( "reset_global_file_associations_tooltip",
								"Clears your saved language choices for ambiguous file extensions\n"
								"(e.g. choosing C++ for .h files). This only affects files opened\n"
								"outside of project folders. After resetting, you'll be prompted\n"
								"to choose a language again when opening these files." ) )
		->setId( "reset-global-file-associations" );

	mWindowMenu->addSeparator();
	mWindowMenu
		->add( i18n( "take_screenshot", "Take Screenshot" ), findIcon( "image" ),
			   getKeybind( "take-screenshot" ) )
		->setId( "take-screenshot" );

	mWindowMenu->on( Event::OnItemClicked, [this]( const Event* event ) {
		if ( !event->getNode()->isType( UI_TYPE_MENUITEM ) )
			return;
		const String& id = event->getNode()->getId();
		if ( id == "zoom-in" )
			mSplitter->zoomIn();
		else if ( id == "zoom-out" )
			mSplitter->zoomOut();
		else if ( id == "zoom-reset" )
			mSplitter->zoomReset();
		else
			runCommand( id );
	} );
	return mWindowMenu;
}

UIMenu* SettingsMenu::createViewMenu() {
	mViewMenu
		->addCheckBox( i18n( "fullscreen_mode", "Full Screen Mode" ), false,
					   getKeybind( "fullscreen-toggle" ) )
		->setId( "fullscreen-toggle" );
	mViewMenu
		->addCheckBox( i18n( "show_side_panel", "Show Side Panel" ),
					   mApp->getConfig().ui.showSidePanel, getKeybind( "switch-side-panel" ) )
		->setId( "show-side-panel" );
	mViewMenu
		->addCheckBox( i18n( "show_status_bar", "Show Status Bar" ),
					   mApp->getConfig().ui.showStatusBar, getKeybind( "toggle-status-bar" ) )
		->setId( "toggle-status-bar" );
	mViewMenu
		->addCheckBox( i18n( "show_menu_bar", "Show Menu Bar" ), mApp->getConfig().ui.showMenuBar,
					   getKeybind( "toggle-menu-bar" ) )
		->setId( "toggle-menu-bar" );
	mViewMenu
		->add( i18n( "move_panel_to_left", "Move Panel To Left" ), findIcon( "layout-left" ),
			   getKeybind( "layout-left" ) )
		->setId( "move-panel-left" );
	mViewMenu
		->add( i18n( "move_panel_to_right", "Move Panel To Right" ), findIcon( "layout-right" ),
			   getKeybind( "layout-right" ) )
		->setId( "move-panel-right" );

	mViewMenu->on( Event::OnItemClicked, [this]( const Event* event ) {
		if ( event->getNode()->isType( UI_TYPE_MENUITEM ) )
			runCommand( event->getNode()->getId() );
	} );
	return mViewMenu;
}

UIPopUpMenu* SettingsMenu::createToolsMenu() {
	mToolsMenu->add( i18n( "plugin_manager", "Plugins Manager" ), findIcon( "extensions" ) )
		->setId( "plugin-manager-open" );

	mToolsMenu->addSeparator();

	mToolsMenu
		->add( i18n( "locate_ellipsis", "Locate..." ), findIcon( "search" ),
			   getKeybind( "open-locatebar" ) )
		->setId( "open-locatebar" );
	mToolsMenu
		->add( i18n( "command_palette_ellipsis", "Command Palette..." ), findIcon( "search" ),
			   getKeybind( "open-command-palette" ) )
		->setId( "open-command-palette" );
	mToolsMenu
		->add( i18n( "project_find_ellipsis", "Project Find..." ), findIcon( "search" ),
			   getKeybind( "open-global-search" ) )
		->setId( "open-global-search" );
	mToolsMenu
		->add( i18n( "show_open_documents_ellipsis", "Show Open Documents..." ),
			   findIcon( "search" ), getKeybind( "show-open-documents" ) )
		->setId( "show-open-documents" );
	mToolsMenu
		->add( i18n( "workspace_symbol_find_ellipsis", "Search Workspace Symbol..." ),
			   findIcon( "search" ), getKeybind( "open-workspace-symbol-search" ) )
		->setId( "open-workspace-symbol-search" );
	mToolsMenu
		->add( i18n( "document_symbol_find_ellipsis", "Search Document Symbol..." ),
			   findIcon( "search" ), getKeybind( "open-document-symbol-search" ) )
		->setId( "open-document-symbol-search" );
	mToolsMenu
		->add( i18n( "go_to_line_ellipsis", "Go to line..." ), findIcon( "go-to-line" ),
			   getKeybind( "go-to-line" ) )
		->setId( "go-to-line" );

	mToolsMenu->addSeparator();

	mToolsMenu
		->add( i18n( "check_languages_health", "Check Languages Health" ),
			   findIcon( "hearth-pulse" ), getKeybind( "check-languages-health" ) )
		->setId( "check-languages-health" );

	mToolsMenu->addSeparator();

	mToolsMenu
		->add( i18n( "load_cur_dir_as_folder", "Load current document directory as folder" ),
			   findIcon( "folder" ), getKeybind( "load-current-dir" ) )
		->setId( "load-current-dir" );

	mToolsMenu->addSeparator();

	mToolsMenu
		->add( i18n( "show_console", "Show Console" ), findIcon( "terminal" ),
			   getKeybind( "console-toggle" ) )
		->setId( "console-toggle" );
	mToolsMenu
		->add( i18n( "inspect_widgets", "Inspect Widgets" ), findIcon( "package" ),
			   getKeybind( "debug-widget-tree-view" ) )
		->setId( "debug-widget-tree-view" );

	mToolsMenu->on( Event::OnItemClicked, [this]( const Event* event ) {
		if ( !event->getNode()->isType( UI_TYPE_MENUITEM ) )
			return;
		runCommand( event->getNode()->getId() );
	} );
	return mToolsMenu;
}

UIMenu* SettingsMenu::createHelpMenu() {
	mHelpMenu->add( i18n( "ecode_source", "ecode Source Code" ), findIcon( "github" ) )
		->setId( "ecode-source" );
	mHelpMenu->add( i18n( "check_for_updates", "Check for Updates" ), findIcon( "refresh" ) )
		->setId( "check-for-updates" );
	mHelpMenu->add( i18n( "about_ecode", "About ecode" ), findIcon( "ecode" ) )
		->setId( "about-ecode" );
	mHelpMenu->on( Event::OnItemClicked,
				   [this]( const Event* event ) { runCommand( event->getNode()->getId() ); } );
	return mHelpMenu;
}

void SettingsMenu::toggleSettingsMenu() {
	if ( mApp->getConfig().ui.showMenuBar ) {
		mMenuBar->showMenu( 0 );
	} else {
		if ( ( !mSettingsMenu->isVisible() || mSettingsMenu->isHiding() ) &&
			 mSettingsMenu->getInactiveTime().getElapsedTime().asMilliseconds() > 1 ) {
			Vector2f pos( mSettingsButton->getPixelsPosition() );
			mSettingsButton->nodeToWorldTranslation( pos );
			UIMenu::findBestMenuPos( pos, mSettingsMenu );
			mSettingsMenu->setPixelsPosition( pos );
			mSettingsMenu->show();
		} else {
			mSettingsMenu->hide();
		}
	}
}

void SettingsMenu::updateProjectSettingsMenu() {
	mSettingsMenu->getItemId( "open-project-settings" )->setEnabled( mApp->projectIsOpen() );
	mSettingsMenu->getItemId( "project_settings" )->setEnabled( mApp->projectIsOpen() );

	if ( !mProjectMenu || mProjectMenu->getCount() == 0 )
		return;

	mProjectMenu->getItemId( "open-project-settings" )->setEnabled( mApp->projectIsOpen() );
	mProjectMenu->getItemId( "reset-project-file-associations" )
		->setEnabled( mApp->projectIsOpen() );
}

void SettingsMenu::updateTerminalMenu() {
	bool enabled = getCurrentTerminal() != nullptr;

	Node* child = mTerminalMenu->getFirstChild();
	while ( child && child->getId() != "end_current_terminal" ) {
		child->setEnabled( enabled );
		child = child->getNextNode();
	}

	if ( !enabled )
		return;

	mTerminalMenu->getItemId( "exclusive-mode" )
		->asType<UIMenuCheckBox>()
		->setActive( getCurrentTerminal()->getExclusiveMode() );
}

void SettingsMenu::updateDocumentMenu() {
	if ( !mDocMenu || mDocMenu->getCount() == 0 )
		return;

	bool enabled =
		mSplitter->getCurWidget() && mSplitter->getCurWidget()->isType( UI_TYPE_CODEEDITOR );

	Node* child = mDocMenu->getFirstChild();
	while ( child && child->getId() != "end_current_document" ) {
		child->setEnabled( enabled );
		child = child->getNextNode();
	}

	if ( !enabled )
		return;

	const TextDocument& doc = mSplitter->getCurEditor()->getDocument();

	mDocMenu->find( "auto_indent_cur" )
		->asType<UIMenuCheckBox>()
		->setActive( doc.getAutoDetectIndentType() );

	auto* curEncoding = mDocMenu->find( "file_encoding" )
							->asType<UIMenuSubMenu>()
							->getSubMenu()
							->find( TextFormat::encodingToString( doc.getEncoding() ) );
	if ( curEncoding )
		curEncoding->asType<UIMenuRadioButton>()->setActive( true );

	auto* curIndent = mDocMenu->find( "indent_width_cur" )
						  ->asType<UIMenuSubMenu>()
						  ->getSubMenu()
						  ->find( String::format( "indent_width_%d", doc.getIndentWidth() ) );
	if ( curIndent )
		curIndent->asType<UIMenuRadioButton>()->setActive( true );

	mDocMenu->find( "indent_type_cur" )
		->asType<UIMenuSubMenu>()
		->getSubMenu()
		->find( doc.getIndentType() == TextDocument::IndentType::IndentTabs ? "tabs" : "spaces" )
		->asType<UIMenuRadioButton>()
		->setActive( true );

	mDocMenu->find( "tab_width_cur" )
		->asType<UIMenuSubMenu>()
		->getSubMenu()
		->find( String::format( "tab_width_%d", mSplitter->getCurEditor()->getTabWidth() ) )
		->asType<UIMenuRadioButton>()
		->setActive( true );

	mDocMenu->find( "trim_whitespaces_cur" )
		->asType<UIMenuCheckBox>()
		->setActive( doc.getTrimTrailingWhitespaces() );

	mDocMenu->find( "force_nl_cur" )
		->asType<UIMenuCheckBox>()
		->setActive( doc.getForceNewLineAtEndOfFile() );

	mDocMenu->find( "write_bom_cur" )->asType<UIMenuCheckBox>()->setActive( doc.isBOM() );

	mDocMenu->find( "line_endings_cur" )
		->asType<UIMenuSubMenu>()
		->getSubMenu()
		->find( TextFormat::lineEndingToString( doc.getLineEnding() ) )
		->asType<UIMenuRadioButton>()
		->setActive( true );

	mDocMenu->find( "read_only" )
		->asType<UIMenuCheckBox>()
		->setActive( mSplitter->getCurEditor()->isLocked() );
}

void SettingsMenu::updateViewMenu() {
	if ( !mViewMenu || mViewMenu->getCount() == 0 )
		return;

	mViewMenu->getItemId( "fullscreen-toggle" )
		->asType<UIMenuCheckBox>()
		->setActive( !mApp->getWindow()->isWindowed() );

	mViewMenu->getItemId( "toggle-status-bar" )
		->asType<UIMenuCheckBox>()
		->setActive( mApp->getConfig().ui.showStatusBar );

	mViewMenu->getItemId( "show-side-panel" )
		->asType<UIMenuCheckBox>()
		->setActive( mApp->getConfig().ui.showSidePanel );

	mViewMenu->getItemId( "move-panel-left" )
		->setVisible( mApp->getConfig().ui.panelPosition == PanelPosition::Right );

	mViewMenu->getItemId( "move-panel-right" )
		->setVisible( mApp->getConfig().ui.panelPosition == PanelPosition::Left );
}

void SettingsMenu::showProjectTreeMenu() {
	mProjectTreeMenu->showOverMouseCursor();
}

void SettingsMenu::createProjectTreeMenu() {
	if ( mProjectTreeMenu && mProjectTreeMenu->isVisible() )
		mProjectTreeMenu->close();

	mProjectTreeMenu = UIPopUpMenu::New();

	if ( !mApp->getCurrentProject().empty() ) {
		mProjectTreeMenu->add( i18n( "new_file_ellipsis", "New File..." ), findIcon( "file-add" ) )
			->setId( "new_file" );
		mProjectTreeMenu
			->add( i18n( "new_folder_ellipsis", "New Folder..." ), findIcon( "folder-add" ) )
			->setId( "new_folder" );
		mProjectTreeMenu->addSeparator();
		mProjectTreeMenu
			->add( i18n( "open_folder_in_fm", "Open Folder in File Manager" ),
				   findIcon( "folder-open" ) )
			->setId( "open_folder" );
		mProjectTreeMenu
			->add( i18n( "open_folder_in_new_window", "Open Folder in New ecode Window" ),
				   findIcon( "folder-open" ) )
			->setId( "open-folder-in-new-window" );
		mProjectTreeMenu
			->add( i18n( "execute_dir_in_terminal", "Open directory in terminal" ),
				   findIcon( "filetype-bash" ) )
			->setId( "execute_dir_in_terminal" );
		mProjectTreeMenu->addSeparator();
		mProjectTreeMenu->add( i18n( "collapse_all", "Collapse All" ), findIcon( "collapse-all" ) )
			->setId( "collapse-all" );
		mProjectTreeMenu->add( i18n( "expand_all", "Expand All" ), findIcon( "expand-all" ) )
			->setId( "expand-all" );
		mProjectTreeMenu->addSeparator();
		mProjectTreeMenu
			->addCheckBox( i18n( "show_hidden_files", "Show hidden files" ),
						   !mApp->getFileSystemModel()->getDisplayConfig().ignoreHidden )
			->setId( "show-hidden-files" );
		mProjectTreeMenu->addSeparator();
		mProjectTreeMenu->add( i18n( "refresh_view", "Refresh View" ), findIcon( "refresh" ) )
			->setId( "refresh-view" );
		mProjectTreeMenu->addSeparator();
		mProjectTreeMenu
			->add( i18n( "configure_ignore_files_ellipsis", "Configure Ignore Files..." ) )
			->setId( "configure-ignore-files" );
	} else if ( !mApp->getFileSystemModel() ) {
		mProjectTreeMenu
			->add( i18n( "open_folder_in_fm", "Open Folder in File Manager" ),
				   findIcon( "folder-open" ) )
			->setId( "open-folder" );
		mProjectTreeMenu
			->add( i18n( "open_folder_in_new_window", "Open Folder in New ecode Window" ),
				   findIcon( "folder-open" ) )
			->setId( "open_folder_in_new_window" );
	}

	mProjectTreeMenu->on( Event::OnItemClicked, [this]( const Event* event ) {
		if ( !event->getNode()->isType( UI_TYPE_MENUITEM ) )
			return;
		UIMenuItem* item = event->getNode()->asType<UIMenuItem>();
		std::string id( item->getId() );
		if ( id.empty() )
			return;
		if ( "new_file" == id || "new_file_in_place" == id ) {
			mApp->newFile( FileInfo( mApp->getCurrentProject() ) );
		} else if ( "new_folder" == id ) {
			mApp->newFolder( FileInfo( mApp->getCurrentProject() ) );
		} else if ( "open_folder" == id ) {
			Engine::instance()->openURI( mApp->getCurrentProject() );
		} else if ( "open_folder_in_new_window" == id ) {
			mApp->loadFolder( mApp->getCurrentProject(), true );
		} else if ( "execute_dir_in_terminal" == id ) {
			mApp->getTerminalManager()->createNewTerminal( "", nullptr, mApp->getCurrentProject() );
		} else if ( "show-hidden-files" == id ) {
			mApp->toggleHiddenFiles();
		} else if ( "collapse-all" == id ) {
			mApp->getProjectTreeView()->collapseAll();
		} else if ( "expand-all" == id ) {
			mApp->getProjectTreeView()->expandAll();
		} else if ( "open-folder" == id ) {
			mApp->openFolderDialog();
		} else if ( "refresh-view" == id ) {
			mApp->refreshFolderView();
		} else if ( "configure-ignore-files" == id ) {
			mApp->treeViewConfigureIgnoreFiles();
		}
	} );

	showProjectTreeMenu();
}

void SettingsMenu::createProjectTreeMenu( const std::vector<FileInfo>& files ) {
	if ( mProjectTreeMenu && mProjectTreeMenu->isVisible() )
		mProjectTreeMenu->close();
	mProjectTreeMenu = UIPopUpMenu::New();

	bool allFiles = true;
	for ( const auto& file : files ) {
		if ( file.isDirectory() ) {
			allFiles = false;
			break;
		}
	}

	if ( files.size() == 1 && files[0].isDirectory() ) {
		mProjectTreeMenu->add( i18n( "new_file_ellipsis", "New File..." ), findIcon( "file-add" ) )
			->setId( "new_file" );
		mProjectTreeMenu
			->add( i18n( "new_folder_ellipsis", "New Folder..." ), findIcon( "folder-add" ) )
			->setId( "new_folder" );
		mProjectTreeMenu->addSeparator();
		mProjectTreeMenu
			->add( i18n( "open_folder_in_fm", "Open Folder in File Manager" ),
				   findIcon( "folder-open" ) )
			->setId( "open_folder" );
		mProjectTreeMenu
			->add( i18n( "open_folder_in_new_window", "Open Folder in New ecode Window" ),
				   findIcon( "folder-open" ) )
			->setId( "open_folder_in_new_window" );
		mProjectTreeMenu
			->add( i18n( "open_all_files_in_folder", "Open All Files in Folder" ),
				   findIcon( "folder-open" ) )
			->setId( "open_all_files_in_folder" );
		mProjectTreeMenu->addSeparator();
		mProjectTreeMenu
			->add( i18n( "find_in_folder_ellipsis", "Find in Folder..." ),
				   findIcon( "file-search" ) )
			->setId( "find_in_folder" );
	} else if ( files.size() == 1 ) {
		mProjectTreeMenu->add( i18n( "open_file", "Open File" ), findIcon( "document-open" ) )
			->setId( "open_file" );
		mProjectTreeMenu
			->add( i18n( "open_containing_folder_in_fm", "Open Containing Folder in File Manager" ),
				   findIcon( "folder-open" ) )
			->setId( "open_containing_folder_in_fm" );
		mProjectTreeMenu
			->add( i18n( "new_file_in_directory_ellipsis", "New File in directory..." ),
				   findIcon( "file-add" ) )
			->setId( "new_file_in_place" );
		mProjectTreeMenu
			->add( i18n( "new_folder_in_directory_ellipsis", "New Folder in directory..." ),
				   findIcon( "folder-add" ) )
			->setId( "new_folder_in_place" );
		mProjectTreeMenu
			->add( i18n( "duplicate_file_ellipsis", "Duplicate File..." ), findIcon( "file-copy" ) )
			->setId( "duplicate_file" );
	} else if ( allFiles && files.size() > 1 ) {
		mProjectTreeMenu->add( i18n( "open_files", "Open Files" ), findIcon( "document-open" ) )
			->setId( "open_files" );
	}

	if ( files.size() == 1 ) {
		mProjectTreeMenu->add( i18n( "rename", "Rename" ), findIcon( "edit" ), "F2" )
			->setId( "rename" );
	}

	mProjectTreeMenu
		->add( i18n( "remove_ellipsis", "Remove..." ), findIcon( "delete-bin" ), "Delete" )
		->setId( "remove" );

	if ( files.size() == 1 ) {
		auto& file = files[0];

		if ( file.isDirectory() || file.isExecutable() ) {
			mProjectTreeMenu->addSeparator();

			if ( file.isDirectory() ) {
				mProjectTreeMenu
					->add( i18n( "execute_dir_in_terminal", "Open directory in terminal" ),
						   findIcon( "filetype-bash" ) )
					->setId( "execute_dir_in_terminal" );
			} else if ( file.isExecutable() ) {
				mProjectTreeMenu
					->add( i18n( "execute_in_terminal", "Execute in terminal" ),
						   findIcon( "filetype-bash" ) )
					->setId( "execute_in_terminal" );
			}
		}
	}

	mProjectTreeMenu->addSeparator();
	mProjectTreeMenu
		->addCheckBox( i18n( "show_hidden_files", "Show hidden files" ),
					   !mApp->getFileSystemModel()->getDisplayConfig().ignoreHidden )
		->setId( "show-hidden-files" );

	mProjectTreeMenu->addSeparator();
	mProjectTreeMenu->add( i18n( "collapse_all", "Collapse All" ), findIcon( "collapse-all" ) )
		->setId( "collapse-all" );
	mProjectTreeMenu->add( i18n( "expand_all", "Expand All" ), findIcon( "expand-all" ) )
		->setId( "expand-all" );
	mProjectTreeMenu->addSeparator();
	mProjectTreeMenu->add( i18n( "refresh_view", "Refresh View" ), findIcon( "refresh" ) )
		->setId( "refresh-view" );

	if ( !mApp->getCurrentProject().empty() ) {
		mProjectTreeMenu->addSeparator();
		mProjectTreeMenu
			->add( i18n( "configure_ignore_files_ellipsis", "Configure Ignore Files..." ) )
			->setId( "configure-ignore-files" );
	}

	mProjectTreeMenu->on( Event::OnItemClicked, [this, files]( const Event* event ) {
		if ( !event->getNode()->isType( UI_TYPE_MENUITEM ) || files.empty() )
			return;
		UIMenuItem* item = event->getNode()->asType<UIMenuItem>();
		std::string id( item->getId() );
		auto file = files[0];

		if ( "new_file" == id || "new_file_in_place" == id ) {
			mApp->newFile( file );
		} else if ( "new_folder" == id || "new_folder_in_place" == id ) {
			mApp->newFolder( file );
		} else if ( "open_file" == id ) {
			mApp->openFileFromPath( file.getFilepath() );
		} else if ( "open_files" == id ) {
			for ( const auto& file : files )
				mApp->openFileFromPath( file.getFilepath() );
		} else if ( "remove" == id ) {
			mApp->getProjectTreeView()->deleteSelectedFiles();
		} else if ( "duplicate_file" == id ) {
			UIMessageBox* msgBox = mApp->newInputMsgBox(
				String::format( "%s \"%s\"",
								i18n( "duplicate_file", "Duplicate file" ).toUtf8().c_str(),
								file.getFileName().c_str() ),
				i18n( "enter_duplicate_file_name", "Enter duplicate file name:" ) );
			std::string newFileName( file.getFileName() );
			std::string ext( file.getExtension( false ) );
			newFileName = FileSystem::fileRemoveExtension( newFileName );
			newFileName += " (" + i18n( "filename_copy", "Copy" ) + ")";
			if ( !ext.empty() )
				newFileName += "." + ext;
			msgBox->getTextInput()->setText( newFileName );
			msgBox->on( Event::OnConfirm, [this, file, msgBox]( const Event* ) {
				auto newFilePath( mApp->getNewFilePath( file, msgBox ) );
				if ( !FileSystem::fileExists( newFilePath ) ) {
					if ( !FileSystem::fileCopy( file.getFilepath(), newFilePath ) )
						mApp->errorMsgBox( i18n( "error_copying_file", "Error copying file." ) );
					msgBox->closeWindow();
				} else {
					mApp->fileAlreadyExistsMsgBox();
				}
			} );
		} else if ( "rename" == id ) {
			mApp->renameFile( file );
		} else if ( "open_containing_folder_in_fm" == id ) {
			Engine::instance()->openURI( file.getDirectoryPath() );
		} else if ( "open_folder" == id ) {
			Engine::instance()->openURI( file.getFilepath() );
		} else if ( "open_folder_in_new_window" == id ) {
			mApp->loadFolder( file.getFilepath(), true );
		} else if ( "open_all_files_in_folder" == id ) {
			mApp->openAllFilesInFolder( file );
		} else if ( "find_in_folder" == id ) {
			std::string folder = file.getFilepath();
			FileSystem::filePathRemoveBasePath( mApp->getCurrentProject(), folder );
			FileSystem::dirAddSlashAtEnd( folder );
			folder += "*";
			mApp->showGlobalSearch( false, folder );
		} else if ( "show-hidden-files" == id ) {
			mApp->toggleHiddenFiles();
		} else if ( "execute_in_terminal" == id ) {
			UITerminal* term = mApp->getTerminalManager()->createNewTerminal(
				"", nullptr, file.getDirectoryPath() );
			if ( !term )
				return;
			term->executeFile( file.getFilepath() );
		} else if ( "execute_dir_in_terminal" == id ) {
			mApp->getTerminalManager()->createNewTerminal( "", nullptr, file.getDirectoryPath() );
		} else if ( "collapse-all" == id ) {
			mApp->getProjectTreeView()->collapseAll();
		} else if ( "expand-all" == id ) {
			mApp->getProjectTreeView()->expandAll();
		} else if ( "refresh-view" == id ) {
			mApp->refreshFolderView();
		} else if ( "configure-ignore-files" == id ) {
			mApp->treeViewConfigureIgnoreFiles();
		}
	} );

	showProjectTreeMenu();
}

void SettingsMenu::updateColorSchemeMenu() {
	for ( UIPopUpMenu* menu : mColorSchemeMenus ) {
		for ( size_t i = 0; i < menu->getCount(); i++ ) {
			UIWidget* widget = menu->getItem( i );
			if ( widget->isType( UI_TYPE_MENURADIOBUTTON ) ) {
				auto* menuItem = widget->asType<UIMenuRadioButton>();
				menuItem->setActive( mSplitter->getCurrentColorSchemeName() ==
									 menuItem->getText() );
			}
		}
	}
}

void SettingsMenu::updateCurrentFileType() {
	if ( !mSplitter->curEditorExistsAndFocused() )
		return;
	std::string curLang( mSplitter->getCurEditor()->getSyntaxDefinition().getLanguageName() );
	for ( UIPopUpMenu* menu : mFileTypeMenus ) {
		for ( size_t i = 0; i < menu->getCount(); i++ ) {
			if ( menu->getItem( i )->isType( UI_TYPE_MENURADIOBUTTON ) ) {
				UIMenuRadioButton* menuItem = menu->getItem( i )->asType<UIMenuRadioButton>();
				menuItem->setActive( curLang == menuItem->getText() );
			}
		}
	}
}

void SettingsMenu::updatedReopenClosedFileState() {
	if ( mRecentFilesMenu ) {
		auto* reopenBtn = mRecentFilesMenu->find( "reopen-closed-tab" );
		if ( reopenBtn )
			reopenBtn->setEnabled( !mApp->getRecentClosedFiles().empty() );
	}
}

UIPopUpMenu* SettingsMenu::getViewMenu() const {
	return mViewMenu;
}

UIPopUpMenu* SettingsMenu::getWindowMenu() const {
	return mWindowMenu;
}

UIPopUpMenu* SettingsMenu::getSettingsMenu() const {
	return mSettingsMenu;
}

UIPopUpMenu* SettingsMenu::getToolsMenu() const {
	return mToolsMenu;
}

UIPopUpMenu* SettingsMenu::getProjectMenu() const {
	return mProjectMenu;
}

UIPopUpMenu* SettingsMenu::getTerminalMenu() const {
	return mTerminalMenu;
}

UIPopUpMenu* SettingsMenu::getDocMenu() const {
	return mDocMenu;
}

UIPopUpMenu* SettingsMenu::getEditMenu() const {
	return mEditMenu;
}

UIPopUpMenu* SettingsMenu::getHelpMenu() const {
	return mHelpMenu;
}

void SettingsMenu::deleteFileDialog( const FileInfo& file ) {
	UIMessageBox* msgBox = UIMessageBox::New(
		UIMessageBox::OK_CANCEL,
		String::format(
			i18n( "confirm_remove_file", "Do you really want to remove \"%s\"?" ).toUtf8().c_str(),
			file.getFileName().c_str() ) );
	msgBox->on( Event::OnConfirm, [this, file, msgBox]( const Event* ) {
		auto errFn = [this, file] {
			mApp->errorMsgBox( String::format(
				std::string( i18n( "couldnt_remove", "Couldn't remove" ).toUtf8() + "%s." ).c_str(),
				file.isDirectory() ? i18n( "directory", "directory" ).toUtf8().c_str()
								   : i18n( "file", "file" ).toUtf8().c_str() ) );
		};

		if ( file.isDirectory() ) {
			if ( !FileSystem::dirRemoveAll( file.getFilepath() ) )
				errFn();
		} else if ( !FileSystem::fileRemove( file.getFilepath() ) ) {
			errFn();
		}
		msgBox->closeWindow();
	} );
	msgBox->setTitle( i18n( "remove_file_question", "Remove file?" ) );
	msgBox->center();
	msgBox->showWhenReady();
}

void SettingsMenu::createProjectMenu() {
	mProjectMenu
		->add( i18n( "open_project_settings_ellipsis", "Open Project Settings..." ),
			   findIcon( "folder-settings" ), getKeybind( "open-project-settings" ) )
		->setId( "open-project-settings" );
	mProjectMenu->addSeparator();
	mProjectMenu
		->add( i18n( "reset_project_file_associations", "Reset Project File Associations" ) )
		->setTooltipText( i18n( "reset_project_file_associations_tooltip",
								"Clears your saved language choices for ambiguous file extensions\n"
								"(e.g. choosing C++ for .h files) in the current project.\n"
								"After resetting, you'll be prompted to choose a language again\n"
								"when opening these files within this project." ) )
		->setId( "reset-project-file-associations" );
	mProjectMenu->on( Event::OnItemClicked, [this]( const Event* event ) {
		if ( event->getNode()->isType( UI_TYPE_MENUITEM ) )
			runCommand( event->getNode()->getId() );
	} );
}

void SettingsMenu::updateMenu() {
	bool showMenuBar = mApp->getConfig().ui.showMenuBar;
	mSettingsButton->setVisible( !showMenuBar );
	mMenuBar->setVisible( showMenuBar );

	const auto setMenuParent = [this]( UIPopUpMenu* menu ) {
		menu->setParent( mApp->getConfig().ui.showMenuBar ? mMenuBar->asType<UIWidget>()
														  : mSettingsMenu->asType<UIWidget>() );
	};

	mSettingsMenu->setParent( showMenuBar ? mMenuBar->asType<Node>()
										  : mApp->getUISceneNode()->asType<Node>() );

	setMenuParent( mEditMenu );
	setMenuParent( mViewMenu );
	setMenuParent( mDocMenu );
	setMenuParent( mTerminalMenu );
	setMenuParent( mProjectMenu );
	setMenuParent( mToolsMenu );
	setMenuParent( mWindowMenu );
	setMenuParent( mHelpMenu );

	mSettingsMenu->find( "settings-submenus-sep" )->setVisible( !showMenuBar );
	mSettingsMenu->getItemId( "project_settings" )->setVisible( !showMenuBar );
	mSettingsMenu->getItemId( "doc-menu" )->setVisible( !showMenuBar );
	mSettingsMenu->getItemId( "term-menu" )->setVisible( !showMenuBar );
	mSettingsMenu->getItemId( "edit-menu" )->setVisible( !showMenuBar );
	mSettingsMenu->getItemId( "view-menu" )->setVisible( !showMenuBar );
	mSettingsMenu->getItemId( "tools-menu" )->setVisible( !showMenuBar );
	mSettingsMenu->getItemId( "window-menu" )->setVisible( !showMenuBar );
	mSettingsMenu->getItemId( "help-menu" )->setVisible( !showMenuBar );
}

} // namespace ecode
