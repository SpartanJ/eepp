#include "settingsmenu.hpp"
#include <filesystem>
namespace fs = std::filesystem;

namespace ecode {

String SettingsMenu::i18n( const std::string& key, const String& def ) {
	return mApp->i18n( key, def );
}

std::string SettingsMenu::getKeybind( const std::string& command ) {
	return mApp->getKeybind( command );
}

Drawable* SettingsMenu::findIcon( const std::string& name ) {
	return mApp->findIcon( name );
}

void SettingsMenu::runCommand( const std::string& command ) {
	mApp->runCommand( command );
}

void SettingsMenu::createSettingsMenu( App* app ) {
	Clock clock;
	mApp = app;
	mUISceneNode = app->getUISceneNode();
	mSplitter = app->getSplitter();

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
		->add( i18n( "open_file", "Open File..." ), findIcon( "document-open" ),
			   getKeybind( "open-file" ) )
		->setId( "open-file" );
	mSettingsMenu
		->add( i18n( "open_folder", "Open Folder..." ), findIcon( "document-open" ),
			   getKeybind( "open-folder" ) )
		->setId( "open-folder" );
	mSettingsMenu
		->add( i18n( "open_file_from_web", "Open File from Web..." ), findIcon( "download-cloud" ),
			   getKeybind( "download-file-web" ) )
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
		->add( i18n( "save_as", "Save as..." ), findIcon( "document-save-as" ),
			   getKeybind( "save-as-doc" ) )
		->setId( "save-as-doc" );
	mSettingsMenu
		->add( i18n( "save_all", "Save All" ), findIcon( "document-save-as" ),
			   getKeybind( "save-all" ) )
		->setId( "save-all" );
	mSettingsMenu->addSeparator();
	UIMenuSubMenu* fileTypeMenu = mSettingsMenu->addSubMenu(
		i18n( "file_type", "File Type" ), findIcon( "file-code" ), createFileTypeMenu() );
	fileTypeMenu->addEventListener( Event::OnMenuShow, [&, fileTypeMenu]( const Event* ) {
		if ( mFileTypeMenuesCreatedWithHeight != mUISceneNode->getPixelsSize().getHeight() ) {
			for ( UIPopUpMenu* menu : mFileTypeMenues )
				menu->close();
			mFileTypeMenues.clear();
			auto* newMenu = createFileTypeMenu();
			newMenu->reloadStyle( true, true );
			fileTypeMenu->setSubMenu( newMenu );
		}
	} );
	UIMenuSubMenu* colorSchemeMenu =
		mSettingsMenu->addSubMenu( i18n( "syntax_color_scheme", "Syntax Color Scheme" ),
								   findIcon( "palette" ), createColorSchemeMenu() );
	colorSchemeMenu->addEventListener( Event::OnMenuShow, [&, colorSchemeMenu]( const Event* ) {
		if ( mColorSchemeMenuesCreatedWithHeight != mUISceneNode->getPixelsSize().getHeight() ) {
			for ( UIPopUpMenu* menu : mColorSchemeMenues )
				menu->close();
			mColorSchemeMenues.clear();
			auto* newMenu = createColorSchemeMenu();
			newMenu->reloadStyle( true, true );
			colorSchemeMenu->setSubMenu( newMenu );
		}
	} );
	mSettingsMenu
		->addSubMenu( i18n( "document", "Document" ), findIcon( "file" ), createDocumentMenu() )
		->setId( "doc-menu" );
	mSettingsMenu
		->addSubMenu( i18n( "terminal", "Terminal" ), findIcon( "terminal" ), createTerminalMenu() )
		->setId( "term-menu" );
	mSettingsMenu->addSubMenu( i18n( "edit", "Edit" ), nullptr, createEditMenu() );
	mSettingsMenu->addSubMenu( i18n( "view", "View" ), nullptr, createViewMenu() );
	mSettingsMenu->addSubMenu( i18n( "tools", "Tools" ), nullptr, createToolsMenu() );
	mSettingsMenu->addSubMenu( i18n( "window", "Window" ), nullptr, createWindowMenu() );
	mSettingsMenu->add( i18n( "plugin_manager", "Plugin Manager" ), findIcon( "package" ) )
		->setId( "plugin-manager-open" );
	mSettingsMenu->addSubMenu( i18n( "help", "Help" ), findIcon( "help" ), createHelpMenu() );
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
	mSettingsButton = mUISceneNode->find<UITextView>( "settings" );
	mSettingsButton->addEventListener( Event::MouseClick,
									   [&]( const Event* ) { toggleSettingsMenu(); } );
	mSettingsMenu->addEventListener( Event::OnItemClicked, [&]( const Event* event ) {
		if ( !event->getNode()->isType( UI_TYPE_MENUITEM ) )
			return;
		const String& id = event->getNode()->asType<UIMenuItem>()->getId();
		runCommand( id );
	} );
	mApp->updateRecentFiles();
	mApp->updateRecentFolders();
	Log::info( "Settings Menu took: %.2f ms", clock.getElapsedTime().asMilliseconds() );
}

UIMenu* SettingsMenu::createFileTypeMenu() {
	mFileTypeMenuesCreatedWithHeight = mUISceneNode->getPixelsSize().getHeight();
	size_t maxItems = 19;
	auto* dM = SyntaxDefinitionManager::instance();
	auto names = dM->getLanguageNames();
	auto cb = [&, dM]( const Event* event ) {
		UIMenuItem* item = event->getNode()->asType<UIMenuItem>();
		const String& name = item->getText();
		if ( mSplitter->curEditorExistsAndFocused() ) {
			mSplitter->getCurEditor()->setSyntaxDefinition( dM->getByLanguageName( name ) );
			updateCurrentFileType();
		}
	};

	UIPopUpMenu* menu = UIPopUpMenu::New();
	menu->addEventListener( Event::OnItemClicked, cb );
	mFileTypeMenues.push_back( menu );
	size_t total = 0;

	for ( const auto& name : names ) {
		menu->addRadioButton(
			name, mSplitter->curEditorExistsAndFocused() &&
					  mSplitter->getCurEditor()->getSyntaxDefinition().getLanguageName() == name );

		if ( mFileTypeMenues.size() == 1 && menu->getCount() == 1 ) {
			menu->reloadStyle( true, true );
			Float height = menu->getPixelsSize().getHeight();
			Float tHeight = mUISceneNode->getPixelsSize().getHeight();
			maxItems = (int)eeceil( tHeight / height ) - 2;
		}

		total++;

		if ( menu->getCount() == maxItems && names.size() - total > 1 ) {
			UIPopUpMenu* newMenu = UIPopUpMenu::New();
			menu->addSubMenu( i18n( "more...", "More..." ), nullptr, newMenu );
			newMenu->addEventListener( Event::OnItemClicked, cb );
			mFileTypeMenues.push_back( newMenu );
			menu = newMenu;
		}
	}

	return mFileTypeMenues[0];
}

UIMenu* SettingsMenu::createColorSchemeMenu() {
	mColorSchemeMenuesCreatedWithHeight = mUISceneNode->getPixelsSize().getHeight();
	size_t maxItems = 19;
	auto cb = [&]( const Event* event ) {
		UIMenuItem* item = event->getNode()->asType<UIMenuItem>();
		const String& name = item->getText();
		mSplitter->setColorScheme( name );
	};

	UIPopUpMenu* menu = UIPopUpMenu::New();
	menu->addEventListener( Event::OnItemClicked, cb );
	mColorSchemeMenues.push_back( menu );
	size_t total = 0;
	const auto& colorSchemes = mSplitter->getColorSchemes();

	for ( auto& colorScheme : colorSchemes ) {
		menu->addRadioButton( colorScheme.first,
							  mSplitter->getCurrentColorSchemeName() == colorScheme.first );

		if ( mColorSchemeMenues.size() == 1 && menu->getCount() == 1 ) {
			menu->reloadStyle( true, true );
			Float height = menu->getPixelsSize().getHeight();
			Float tHeight = mUISceneNode->getPixelsSize().getHeight();
			maxItems = (int)eeceil( tHeight / height ) - 2;
		}

		total++;

		if ( menu->getCount() == maxItems && colorSchemes.size() - total > 1 ) {
			UIPopUpMenu* newMenu = UIPopUpMenu::New();
			menu->addSubMenu( i18n( "more...", "More..." ), nullptr, newMenu );
			newMenu->addEventListener( Event::OnItemClicked, cb );
			mColorSchemeMenues.push_back( newMenu );
			menu = newMenu;
		}
	}

	return mColorSchemeMenues[0];
}

UIMenu* SettingsMenu::createDocumentMenu() {
	auto shouldCloseCb = []( UIMenuItem* ) -> bool { return false; };

	mDocMenu = UIPopUpMenu::New();

	// **** CURRENT DOCUMENT ****
	mDocMenu->add( i18n( "current_document", "Current Document" ) )
		->setTextAlign( UI_HALIGN_CENTER );

	mDocMenu
		->addCheckBox(
			i18n( "auto_detect_indent_type_and_width", "Auto Detect Indent Type & Width" ),
			mApp->getConfig().doc.autoDetectIndentType )
		->setId( "auto_indent_cur" );

	UIPopUpMenu* tabTypeMenu = UIPopUpMenu::New();
	tabTypeMenu->addRadioButton( i18n( "tabs", "Tabs" ) )->setId( "tabs" );
	tabTypeMenu->addRadioButton( i18n( "spaces", "Spaces" ) )->setId( "spaces" );
	mDocMenu->addSubMenu( i18n( "indentation_type", "Indentation Type" ), nullptr, tabTypeMenu )
		->setId( "indent_type_cur" );
	tabTypeMenu->addEventListener( Event::OnItemClicked, [&]( const Event* event ) {
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
			->addRadioButton( String::toString( w ),
							  mSplitter->curEditorExistsAndFocused() &&
								  mSplitter->getCurEditor()->getDocument().getIndentWidth() == w )
			->setId( String::format( "indent_width_%zu", w ) )
			->setData( w );
	mDocMenu->addSubMenu( i18n( "indent_width", "Indent Width" ), nullptr, indentWidthMenu )
		->setId( "indent_width_cur" );
	indentWidthMenu->addEventListener( Event::OnItemClicked, [&]( const Event* event ) {
		if ( mSplitter->curEditorExistsAndFocused() ) {
			int width = event->getNode()->getData();
			mSplitter->getCurEditor()->getDocument().setIndentWidth( width );
		}
	} );

	UIPopUpMenu* tabWidthMenu = UIPopUpMenu::New();
	for ( size_t w = 2; w <= 12; w++ )
		tabWidthMenu
			->addRadioButton( String::toString( w ),
							  mSplitter->curEditorExistsAndFocused() &&
								  mSplitter->getCurEditor()->getTabWidth() == w )
			->setId( String::format( "tab_width_%zu", w ) )
			->setData( w );
	mDocMenu->addSubMenu( i18n( "tab_width", "Tab Width" ), nullptr, tabWidthMenu )
		->setId( "tab_width_cur" );
	tabWidthMenu->addEventListener( Event::OnItemClicked, [&]( const Event* event ) {
		if ( mSplitter->curEditorExistsAndFocused() ) {
			int width = event->getNode()->getData();
			mSplitter->getCurEditor()->setTabWidth( width );
		}
	} );

	UIPopUpMenu* lineEndingsMenu = UIPopUpMenu::New();
	lineEndingsMenu
		->addRadioButton( "Windows/DOS (CR/LF)",
						  mApp->getConfig().doc.lineEndings == TextDocument::LineEnding::CRLF )
		->setId( "CRLF" );
	lineEndingsMenu
		->addRadioButton( "Unix (LF)",
						  mApp->getConfig().doc.lineEndings == TextDocument::LineEnding::LF )
		->setId( "LF" );
	lineEndingsMenu
		->addRadioButton( "Macintosh (CR)",
						  mApp->getConfig().doc.lineEndings == TextDocument::LineEnding::CR )
		->setId( "CR" );
	mDocMenu->addSubMenu( i18n( "line_endings", "Line Endings" ), nullptr, lineEndingsMenu )
		->setId( "line_endings_cur" );
	lineEndingsMenu->addEventListener( Event::OnItemClicked, [&]( const Event* event ) {
		auto le =
			TextDocument::stringToLineEnding( event->getNode()->asType<UIRadioButton>()->getId() );
		if ( mSplitter->curEditorExistsAndFocused() ) {
			mSplitter->getCurEditor()->getDocument().setLineEnding( le );
			mApp->updateDocInfo( mSplitter->getCurEditor()->getDocument() );
		}
	} );

	mDocMenu->addCheckBox( i18n( "read_only", "Read Only" ) )->setId( "read_only" );

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

	mDocMenu->addEventListener( Event::OnItemClicked, [&]( const Event* event ) {
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

	// **** GLOBAL SETTINGS ****
	mDocMenu->addSeparator();

	UIPopUpMenu* globalMenu = UIPopUpMenu::New();
	mDocMenu->addSubMenu( i18n( "global_settings", "Global Settings" ),
						  findIcon( "global-settings" ), globalMenu );

	globalMenu
		->addCheckBox(
			i18n( "auto_detect_indent_type_and_width", "Auto Detect Indent Type & Width" ),
			mApp->getConfig().doc.autoDetectIndentType )
		->setId( "auto_indent" );

	UIPopUpMenu* tabTypeMenuGlobal = UIPopUpMenu::New();
	tabTypeMenuGlobal->addRadioButton( i18n( "tabs", "Tabs" ) )
		->setActive( !mApp->getConfig().doc.indentSpaces )
		->setId( "tabs" );
	tabTypeMenuGlobal->addRadioButton( i18n( "spaces", "Spaces" ) )
		->setActive( mApp->getConfig().doc.indentSpaces )
		->setId( "spaces" );
	globalMenu
		->addSubMenu( i18n( "indentation_type", "Indentation Type" ), nullptr, tabTypeMenuGlobal )
		->setId( "indent_type" );
	tabTypeMenuGlobal->addEventListener( Event::OnItemClicked, [&]( const Event* event ) {
		const String& text = event->getNode()->asType<UIMenuRadioButton>()->getId();
		mApp->getConfig().doc.indentSpaces = text != "tabs";
	} );

	UIPopUpMenu* indentWidthMenuGlobal = UIPopUpMenu::New();
	for ( int w = 2; w <= 12; w++ )
		indentWidthMenuGlobal
			->addRadioButton( String::toString( w ), mApp->getConfig().doc.indentWidth == w )
			->setId( String::format( "indent_width_%d", w ) )
			->setData( w );
	globalMenu->addSubMenu( i18n( "indent_width", "Indent Width" ), nullptr, indentWidthMenuGlobal )
		->setId( "indent_width" );
	indentWidthMenuGlobal->addEventListener( Event::OnItemClicked, [&]( const Event* event ) {
		int width = event->getNode()->getData();
		mApp->getConfig().doc.indentWidth = width;
	} );

	UIPopUpMenu* tabWidthMenuGlobal = UIPopUpMenu::New();
	for ( int w = 2; w <= 12; w++ )
		tabWidthMenuGlobal
			->addRadioButton( String::toString( w ), mApp->getConfig().doc.tabWidth == w )
			->setId( String::format( "tab_width_%d", w ) )
			->setData( w );
	globalMenu->addSubMenu( i18n( "tab_width", "Tab Width" ), nullptr, tabWidthMenuGlobal )
		->setId( "tab_width_cur" );
	tabWidthMenuGlobal->addEventListener( Event::OnItemClicked, [&]( const Event* event ) {
		int width = event->getNode()->getData();
		mApp->getConfig().doc.tabWidth = width;
	} );

	UIPopUpMenu* lineEndingsGlobalMenu = UIPopUpMenu::New();
	lineEndingsGlobalMenu
		->addRadioButton( "Windows/DOS (CR/LF)",
						  mApp->getConfig().doc.lineEndings == TextDocument::LineEnding::CRLF )
		->setId( "CRLF" );
	lineEndingsGlobalMenu
		->addRadioButton( "Unix (LF)",
						  mApp->getConfig().doc.lineEndings == TextDocument::LineEnding::LF )
		->setId( "LF" );
	lineEndingsGlobalMenu
		->addRadioButton( "Macintosh (CR)",
						  mApp->getConfig().doc.lineEndings == TextDocument::LineEnding::CR )
		->setId( "CR" );
	globalMenu->addSubMenu( i18n( "line_endings", "Line Endings" ), nullptr, lineEndingsGlobalMenu )
		->setId( "line_endings" );
	lineEndingsGlobalMenu->addEventListener( Event::OnItemClicked, [&]( const Event* event ) {
		mApp->getConfig().doc.lineEndings =
			TextDocument::stringToLineEnding( event->getNode()->asType<UIRadioButton>()->getId() );
	} );

	UIPopUpMenu* bracketsMenu = UIPopUpMenu::New();
	globalMenu->addSubMenu( i18n( "auto_close_brackets_and_tags", "Auto-Close Brackets & Tags" ),
							nullptr, bracketsMenu );
	auto& closeBrackets = mApp->getConfig().editor.autoCloseBrackets;
	bracketsMenu
		->addCheckBox( i18n( "brackets", "Brackets ()" ),
					   closeBrackets.find( '(' ) != std::string::npos )
		->setOnShouldCloseCb( shouldCloseCb )
		->setId( "()" );
	bracketsMenu
		->addCheckBox( i18n( "curly_brackets", "Curly Brackets {}" ),
					   closeBrackets.find( '{' ) != std::string::npos )
		->setOnShouldCloseCb( shouldCloseCb )
		->setId( "{}" );
	bracketsMenu
		->addCheckBox( i18n( "square_brakcets", "Square Brackets []" ),
					   closeBrackets.find( '[' ) != std::string::npos )
		->setOnShouldCloseCb( shouldCloseCb )
		->setId( "[]" );
	bracketsMenu
		->addCheckBox( i18n( "single_quotes", "Single Quotes ''" ),
					   closeBrackets.find( '\'' ) != std::string::npos )
		->setOnShouldCloseCb( shouldCloseCb )
		->setId( "''" );
	bracketsMenu
		->addCheckBox( i18n( "double_quotes", "Double Quotes \"\"" ),
					   closeBrackets.find( '"' ) != std::string::npos )
		->setOnShouldCloseCb( shouldCloseCb )
		->setId( "\"\"" );
	bracketsMenu
		->addCheckBox( i18n( "back_quotes", "Back Quotes ``" ),
					   closeBrackets.find( '`' ) != std::string::npos )
		->setOnShouldCloseCb( shouldCloseCb )
		->setId( "``" );
	bracketsMenu
		->addCheckBox( i18n( "auto_close_xml_tags", "Auto Close XML Tags" ),
					   mApp->getConfig().editor.autoCloseXMLTags )
		->setOnShouldCloseCb( shouldCloseCb )
		->setId( "XML" );
	bracketsMenu->addEventListener( Event::OnItemClicked, [&]( const Event* event ) {
		std::string id = event->getNode()->getId();
		if ( event->getNode()->isType( UI_TYPE_MENUCHECKBOX ) ) {
			UIMenuCheckBox* item = event->getNode()->asType<UIMenuCheckBox>();
			if ( item->getId() == "XML" ) {
				mApp->getConfig().editor.autoCloseXMLTags = item->isActive();
				mSplitter->forEachEditor( [&]( UICodeEditor* editor ) {
					editor->setAutoCloseXMLTags( mApp->getConfig().editor.autoCloseXMLTags );
				} );
				return;
			}
			auto curPairs = String::split( mApp->getConfig().editor.autoCloseBrackets, ',' );
			auto found = std::find( curPairs.begin(), curPairs.end(), id );
			if ( item->isActive() ) {
				if ( found == curPairs.end() )
					curPairs.push_back( id );
			} else if ( found != curPairs.end() ) {
				curPairs.erase( found );
			}
			mApp->getConfig().editor.autoCloseBrackets = String::join( curPairs, ',' );
			auto pairs = mApp->makeAutoClosePairs( mApp->getConfig().editor.autoCloseBrackets );
			mSplitter->forEachEditor( [&, pairs]( UICodeEditor* editor ) {
				editor->getDocument().setAutoCloseBrackets( !pairs.empty() );
				editor->getDocument().setAutoCloseBracketsPairs( pairs );
			} );
		}
	} );

	globalMenu
		->addCheckBox( i18n( "trim_trailing_whitespaces", "Trim Trailing Whitespaces" ),
					   mApp->getConfig().doc.trimTrailingWhitespaces )
		->setId( "trim_whitespaces" );

	globalMenu
		->addCheckBox( i18n( "force_new_line_at_end_of_file", "Force New Line at End of File" ),
					   mApp->getConfig().doc.forceNewLineAtEndOfFile )
		->setId( "force_nl" );

	globalMenu
		->addCheckBox( i18n( "write_unicode_bom", "Write Unicode BOM" ),
					   mApp->getConfig().doc.writeUnicodeBOM )
		->setId( "write_bom" );

	globalMenu->addSeparator();

	globalMenu->add( i18n( "line_breaking_column", "Line Breaking Column" ) )
		->setId( "line_breaking_column" );

	globalMenu->add( i18n( "line_spacing", "Line Spacing" ) )->setId( "line_spacing" );

	globalMenu->add( i18n( "cursor_blinking_time", "Cursor Blinking Time" ) )
		->setId( "cursor_blinking_time" );

	globalMenu->addEventListener( Event::OnItemClicked, [&]( const Event* event ) {
		if ( !mSplitter->curEditorExistsAndFocused() ||
			 event->getNode()->isType( UI_TYPE_MENU_SEPARATOR ) ||
			 event->getNode()->isType( UI_TYPE_MENUSUBMENU ) )
			return;
		const String& id = event->getNode()->getId();

		if ( event->getNode()->isType( UI_TYPE_MENUCHECKBOX ) ) {
			UIMenuCheckBox* item = event->getNode()->asType<UIMenuCheckBox>();
			if ( "trim_whitespaces" == id ) {
				mApp->getConfig().doc.trimTrailingWhitespaces = item->isActive();
			} else if ( "force_nl" == id ) {
				mApp->getConfig().doc.forceNewLineAtEndOfFile = item->isActive();
			} else if ( "write_bom" == id ) {
				mApp->getConfig().doc.writeUnicodeBOM = item->isActive();
			} else if ( "auto_indent" == id ) {
				mApp->getConfig().doc.autoDetectIndentType = item->isActive();
			}
		} else if ( "line_breaking_column" == id ) {
			mApp->setLineBreakingColumn();
		} else if ( "line_spacing" == id ) {
			mApp->setLineSpacing();
		} else if ( "cursor_blinking_time" == id ) {
			mApp->setCursorBlinkingTime();
		}
	} );

	mDocMenu->addSeparator();

	// **** PROJECT SETTINGS ****
	mProjectMenu = UIPopUpMenu::New();
	mProjectMenu
		->addCheckBox( i18n( "use_global_settings", "Use Global Settings" ),
					   mApp->getProjectDocConfig().useGlobalSettings )
		->setOnShouldCloseCb( shouldCloseCb )
		->setId( "use_global_settings" );

	mProjectMenu
		->addCheckBox(
			i18n( "auto_detect_indent_type_and_width", "Auto Detect Indent Type & Width" ),
			mApp->getConfig().doc.autoDetectIndentType )
		->setId( "auto_indent" )
		->setEnabled( !mApp->getProjectDocConfig().useGlobalSettings );

	UIPopUpMenu* tabTypeMenuProject = UIPopUpMenu::New();
	tabTypeMenuProject->addRadioButton( i18n( "tabs", "Tabs" ) )
		->setActive( !mApp->getProjectDocConfig().doc.indentSpaces )
		->setId( "tabs" );
	tabTypeMenuProject->addRadioButton( i18n( "spaces", "Spaces" ) )
		->setActive( mApp->getProjectDocConfig().doc.indentSpaces )
		->setId( "spaces" );
	mProjectMenu
		->addSubMenu( i18n( "indentation_type", "Indentation Type" ), nullptr, tabTypeMenuProject )
		->setId( "indent_type" )
		->setEnabled( !mApp->getProjectDocConfig().useGlobalSettings );
	tabTypeMenuProject->addEventListener( Event::OnItemClicked, [&]( const Event* event ) {
		const String& text = event->getNode()->asType<UIMenuRadioButton>()->getId();
		mApp->getProjectDocConfig().doc.indentSpaces = text != "tabs";
	} );

	UIPopUpMenu* indentWidthMenuProject = UIPopUpMenu::New();
	for ( int w = 2; w <= 12; w++ )
		indentWidthMenuProject
			->addRadioButton( String::toString( w ),
							  mApp->getProjectDocConfig().doc.indentWidth == w )
			->setId( String::format( "indent_width_%d", w ) )
			->setData( w );
	mProjectMenu
		->addSubMenu( i18n( "indent_width", "Indent Width" ), nullptr, indentWidthMenuProject )
		->setId( "indent_width" )
		->setEnabled( !mApp->getProjectDocConfig().useGlobalSettings );
	indentWidthMenuProject->addEventListener( Event::OnItemClicked, [&]( const Event* event ) {
		int width = event->getNode()->getData();
		mApp->getProjectDocConfig().doc.indentWidth = width;
	} );

	UIPopUpMenu* tabWidthMenuProject = UIPopUpMenu::New();
	for ( int w = 2; w <= 12; w++ )
		tabWidthMenuProject
			->addRadioButton( String::toString( w ), mApp->getProjectDocConfig().doc.tabWidth == w )
			->setId( String::format( "tab_width_%d", w ) )
			->setData( w );
	mProjectMenu->addSubMenu( i18n( "tab_width", "Tab Width" ), nullptr, tabWidthMenuProject )
		->setId( "tab_width" )
		->setEnabled( !mApp->getProjectDocConfig().useGlobalSettings );
	tabWidthMenuProject->addEventListener( Event::OnItemClicked, [&]( const Event* event ) {
		int width = event->getNode()->getData();
		mApp->getProjectDocConfig().doc.tabWidth = width;
	} );

	UIPopUpMenu* lineEndingsProjectMenu = UIPopUpMenu::New();
	lineEndingsProjectMenu
		->addRadioButton( "Windows (CR/LF)", mApp->getProjectDocConfig().doc.lineEndings ==
												 TextDocument::LineEnding::CRLF )
		->setId( "CRLF" );
	lineEndingsProjectMenu
		->addRadioButton( "Unix (LF)", mApp->getProjectDocConfig().doc.lineEndings ==
										   TextDocument::LineEnding::LF )
		->setId( "LF" );
	lineEndingsProjectMenu
		->addRadioButton( "Macintosh (CR)", mApp->getProjectDocConfig().doc.lineEndings ==
												TextDocument::LineEnding::CR )
		->setId( "CR" );
	mProjectMenu
		->addSubMenu( i18n( "line_endings", "Line Endings" ), nullptr, lineEndingsProjectMenu )
		->setId( "line_endings" )
		->setEnabled( !mApp->getProjectDocConfig().useGlobalSettings );
	lineEndingsProjectMenu->addEventListener( Event::OnItemClicked, [&]( const Event* event ) {
		mApp->getProjectDocConfig().doc.lineEndings =
			TextDocument::stringToLineEnding( event->getNode()->asType<UIRadioButton>()->getId() );
	} );

	mProjectMenu
		->addCheckBox( i18n( "trim_trailing_whitespaces", "Trim Trailing Whitespaces" ),
					   mApp->getConfig().doc.trimTrailingWhitespaces )
		->setId( "trim_whitespaces" )
		->setEnabled( !mApp->getProjectDocConfig().useGlobalSettings );

	mProjectMenu
		->addCheckBox( i18n( "force_new_line_at_end_of_file", "Force New Line at End of File" ),
					   mApp->getConfig().doc.forceNewLineAtEndOfFile )
		->setId( "force_nl" )
		->setEnabled( !mApp->getProjectDocConfig().useGlobalSettings );

	mProjectMenu
		->addCheckBox( i18n( "write_unicode_bom", "Write Unicode BOM" ),
					   mApp->getConfig().doc.writeUnicodeBOM )
		->setId( "write_bom" )
		->setEnabled( !mApp->getProjectDocConfig().useGlobalSettings );

	mProjectMenu->addSeparator();

	mProjectMenu->add( i18n( "line_breaking_column", "Line Breaking Column" ) )
		->setId( "line_breaking_column" );

	mProjectMenu->addEventListener( Event::OnItemClicked, [&]( const Event* event ) {
		if ( !mSplitter->curEditorExistsAndFocused() ||
			 event->getNode()->isType( UI_TYPE_MENU_SEPARATOR ) ||
			 event->getNode()->isType( UI_TYPE_MENUSUBMENU ) )
			return;
		const String& id = event->getNode()->getId();

		if ( event->getNode()->isType( UI_TYPE_MENUCHECKBOX ) ) {
			UIMenuCheckBox* item = event->getNode()->asType<UIMenuCheckBox>();
			if ( "use_global_settings" == id ) {
				mApp->getProjectDocConfig().useGlobalSettings = item->isActive();
				updateProjectSettingsMenu();
			} else if ( "trim_whitespaces" == id ) {
				mApp->getProjectDocConfig().doc.trimTrailingWhitespaces = item->isActive();
			} else if ( "force_nl" == id ) {
				mApp->getProjectDocConfig().doc.forceNewLineAtEndOfFile = item->isActive();
			} else if ( "write_bom" == id ) {
				mApp->getProjectDocConfig().doc.writeUnicodeBOM = item->isActive();
			} else if ( "auto_indent" == id ) {
				mApp->getProjectDocConfig().doc.autoDetectIndentType = item->isActive();
			}
		} else if ( "line_breaking_column" == id ) {
			UIMessageBox* msgBox = UIMessageBox::New(
				UIMessageBox::INPUT, i18n( "set_line_breaking_column",
										   "Set Line Breaking Column:\nSet 0 to disable it.\n" )
										 .unescape() );
			msgBox->setTitle( mApp->getWindowTitle() );
			msgBox->setCloseShortcut( { KEY_ESCAPE, 0 } );
			msgBox->getTextInput()->setAllowOnlyNumbers( true, false );
			msgBox->getTextInput()->setText(
				String::toString( mApp->getProjectDocConfig().doc.lineBreakingColumn ) );
			msgBox->showWhenReady();
			msgBox->addEventListener( Event::OnConfirm, [&, msgBox]( const Event* ) {
				int val;
				if ( String::fromString( val, msgBox->getTextInput()->getText() ) && val >= 0 ) {
					mApp->getProjectDocConfig().doc.lineBreakingColumn = val;
					mSplitter->forEachEditor(
						[val]( UICodeEditor* editor ) { editor->setLineBreakingColumn( val ); } );
					msgBox->closeWindow();
				}
			} );
			mApp->setFocusEditorOnClose( msgBox );
		}
	} );

	mDocMenu
		->addSubMenu( i18n( "folder_project_settings", "Folder/Project Settings" ),
					  findIcon( "folder-user" ), mProjectMenu )
		->setId( "project_settings" );

	return mDocMenu;
}

UIMenu* SettingsMenu::createTerminalMenu() {
	mTerminalMenu = UIPopUpMenu::New();

#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN
	UIMenuSubMenu* termColorSchemeMenu = mTerminalMenu->addSubMenu(
		i18n( "terminal_color_scheme", "Terminal Color Scheme" ), findIcon( "palette" ),
		mApp->getTerminalManager()->createColorSchemeMenu() );
	termColorSchemeMenu->addEventListener(
		Event::OnMenuShow, [&, termColorSchemeMenu]( const Event* ) {
			mApp->getTerminalManager()->updateMenuColorScheme( termColorSchemeMenu );
		} );
#endif

	UIMenuCheckBox* exclusiveChk =
		mTerminalMenu->addCheckBox( i18n( "exclusive_mode", "Exclusive Mode" ), false,
									getKeybind( UITerminal::getExclusiveModeToggleCommandName() ) );
	exclusiveChk
		->setTooltipText( i18n(
			"exclusive_mode_tooltip",
			"Global Keybindings are disabled when exclusive mode is enabled.\nThis is to "
			"avoid keyboard shortcut overlapping between the terminal and the application." ) )
		->setId( "exclusive-mode" );

	mTerminalMenu
		->add( i18n( "rename_session", "Rename Session" ), nullptr,
			   getKeybind( "terminal-rename" ) )
		->setId( "terminal-rename" );

	mTerminalMenu->addEventListener( Event::OnItemClicked, [&]( const Event* event ) {
		const std::string& id( event->getNode()->getId() );
		if ( mSplitter->getCurWidget() && mSplitter->getCurWidget()->isType( UI_TYPE_TERMINAL ) ) {
			UITerminal* terminal = mSplitter->getCurWidget()->asType<UITerminal>();
			if ( "exclusive-mode" == id ) {
				terminal->setExclusiveMode(
					event->getNode()->asType<UIMenuCheckBox>()->isActive() );
			} else {
				terminal->execute( id );
			}
		}
	} );

	return mTerminalMenu;
}

UIMenu* SettingsMenu::createEditMenu() {
	UIPopUpMenu* menu = UIPopUpMenu::New();
	menu->add( i18n( "undo", "Undo" ), findIcon( "undo" ), getKeybind( "undo" ) )->setId( "undo" );
	menu->add( i18n( "redo", "Redo" ), findIcon( "redo" ), getKeybind( "redo" ) )->setId( "redo" );
	menu->addSeparator();
	menu->add( i18n( "cut", "Cut" ), findIcon( "cut" ), getKeybind( "cut" ) )->setId( "cut" );
	menu->add( i18n( "copy", "Copy" ), findIcon( "copy" ), getKeybind( "copy" ) )->setId( "copy" );
	menu->add( i18n( "paste", "Paste" ), findIcon( "paste" ), getKeybind( "paste" ) )
		->setId( "paste" );
	menu->add( i18n( "delete", "Delete" ), findIcon( "delete-text" ),
			   getKeybind( "delete-to-next-char" ) )
		->setId( "delete-to-next-char" );
	menu->addSeparator();
	menu->add( i18n( "select_all", "Select All" ), findIcon( "select-all" ),
			   getKeybind( "select-all" ) )
		->setId( "select-all" );
	menu->addSeparator();
	menu->add( i18n( "find_replace", "Find/Replace" ), findIcon( "find-replace" ),
			   getKeybind( "find-replace" ) )
		->setId( "find-replace" );
	menu->addSeparator();
	menu->add( i18n( "open_containing_folder", "Open Containing Folder..." ),
			   findIcon( "folder-open" ), getKeybind( "open-containing-folder" ) )
		->setId( "open-containing-folder" );
	menu->add( i18n( "copy_containing_folder_path", "Copy Containing Folder Path..." ),
			   findIcon( "copy" ), getKeybind( "copy-containing-folder-path" ) )
		->setId( "copy-containing-folder-path" );
	menu->add( i18n( "copy_file_path", "Copy File Path" ), findIcon( "copy" ),
			   getKeybind( "copy-file-path" ) )
		->setId( "copy-file-path" );
	UIMenuSeparator* fileSep = menu->addSeparator();
	menu->add( i18n( "key_bindings", "Key Bindings" ), findIcon( "keybindings" ),
			   getKeybind( "keybindings" ) )
		->setId( "keybindings" );
	menu->addEventListener( Event::OnItemClicked, [&]( const Event* event ) {
		if ( !event->getNode()->isType( UI_TYPE_MENUITEM ) )
			return;
		runCommand( event->getNode()->getId() );
	} );
	menu->addEventListener( Event::OnMenuShow, [&, menu, fileSep]( const Event* ) {
		if ( !mSplitter->curEditorExistsAndFocused() )
			return;
		auto doc = mSplitter->getCurEditor()->getDocumentRef();
		menu->getItemId( "undo" )->setEnabled( doc->hasUndo() );
		menu->getItemId( "redo" )->setEnabled( doc->hasRedo() );
		menu->getItemId( "copy" )->setEnabled( doc->hasSelection() );
		menu->getItemId( "cut" )->setEnabled( doc->hasSelection() );
		menu->getItemId( "open-containing-folder" )->setVisible( doc->hasFilepath() );
		menu->getItemId( "copy-file-path" )->setVisible( doc->hasFilepath() );
		fileSep->setVisible( doc->hasFilepath() );
	} );
	return menu;
}

UIMenu* SettingsMenu::createWindowMenu() {
	mWindowMenu = UIPopUpMenu::New();
	UIPopUpMenu* colorsMenu = UIPopUpMenu::New();
	colorsMenu
		->addRadioButton( i18n( "light", "Light" ),
						  mApp->getUIColorScheme() == ColorSchemePreference::Light )
		->setId( "light" );
	colorsMenu
		->addRadioButton( i18n( "dark", "Dark" ),
						  mApp->getUIColorScheme() == ColorSchemePreference::Dark )
		->setId( "dark" );
	colorsMenu->addEventListener( Event::OnItemClicked, [&]( const Event* event ) {
		if ( !event->getNode()->isType( UI_TYPE_MENUITEM ) )
			return;
		UIMenuItem* item = event->getNode()->asType<UIMenuItem>();
		mApp->setUIColorScheme( item->getId() == "light" ? ColorSchemePreference::Light
														 : ColorSchemePreference::Dark );
	} );
	mWindowMenu->addSubMenu( i18n( "ui_prefes_color_scheme", "UI Prefers Color Scheme" ),
							 findIcon( "color-scheme" ), colorsMenu );
	mWindowMenu->addSubMenu( i18n( "ui_renderer", "Renderer" ), findIcon( "renderer" ),
							 createRendererMenu() );
	mWindowMenu
		->add( i18n( "ui_scale_factor", "UI Scale Factor (Pixel Density)" ),
			   findIcon( "pixel-density" ) )
		->setId( "ui-scale-factor" );
	mWindowMenu->add( i18n( "ui_font_size", "UI Font Size" ), findIcon( "font-size" ) )
		->setId( "ui-font-size" );
	mWindowMenu->add( i18n( "ui_panel_font_size", "UI Panel Font Size" ), findIcon( "font-size" ) )
		->setId( "ui-panel-font-size" );
	mWindowMenu->add( i18n( "editor_font_size", "Editor Font Size" ), findIcon( "font-size" ) )
		->setId( "editor-font-size" );
	mWindowMenu->add( i18n( "terminal_font_size", "Terminal Font Size" ), findIcon( "font-size" ) )
		->setId( "terminal-font-size" );
	mWindowMenu->add( i18n( "serif_font", "Serif Font..." ), findIcon( "font-size" ) )
		->setId( "serif-font" );
	mWindowMenu->add( i18n( "monospace_font", "Monospace Font..." ), findIcon( "font-size" ) )
		->setId( "monospace-font" );
	mWindowMenu->add( i18n( "terminal_font", "Terminal Font..." ), findIcon( "font-size" ) )
		->setId( "terminal-font" );
	mWindowMenu->add( i18n( "fallback_font", "Fallback Font..." ), findIcon( "font-size" ) )
		->setId( "fallback-font" );
	mWindowMenu->addSeparator();
	mWindowMenu
		->addCheckBox( i18n( "fullscreen_mode", "Full Screen Mode" ), false,
					   getKeybind( "fullscreen-toggle" ) )
		->setId( "fullscreen-toggle" );
	mWindowMenu
		->addCheckBox( i18n( "show_side_panel", "Show Side Panel" ),
					   mApp->getConfig().ui.showSidePanel, getKeybind( "switch-side-panel" ) )
		->setId( "show-side-panel" );
	mWindowMenu
		->add( i18n( "move_panel_left", "Move panel to left..." ), findIcon( "layout-left" ),
			   getKeybind( "layout-left" ) )
		->setId( "move-panel-left" );
	mWindowMenu
		->add( i18n( "move_panel_right", "Move panel to right..." ), findIcon( "layout-right" ),
			   getKeybind( "layout-rigth" ) )
		->setId( "move-panel-right" );
	mWindowMenu->addSeparator();
	mWindowMenu
		->add( i18n( "split_left", "Split Left" ), findIcon( "split-horizontal" ),
			   getKeybind( "split-left" ) )
		->setId( "split-left" );
	mWindowMenu
		->add( i18n( "split_right", "Split Right" ), findIcon( "split-horizontal" ),
			   getKeybind( "split-right" ) )
		->setId( "split-right" );
	mWindowMenu
		->add( i18n( "split_top", "Split Top" ), findIcon( "split-vertical" ),
			   getKeybind( "split-top" ) )
		->setId( "split-top" );
	mWindowMenu
		->add( i18n( "split_bottom", "Split Bottom" ), findIcon( "split-vertical" ),
			   getKeybind( "split-bottom" ) )
		->setId( "split-bottom" );
	mWindowMenu->addSeparator();
	mWindowMenu
		->add( i18n( "terminal_split_left", "Split Terminal Left" ), findIcon( "split-horizontal" ),
			   getKeybind( "terminal-split-left" ) )
		->setId( "terminal-split-left" );
	mWindowMenu
		->add( i18n( "terminal_split_right", "Split Terminal Right" ),
			   findIcon( "split-horizontal" ), getKeybind( "terminal-split-right" ) )
		->setId( "terminal-split-right" );
	mWindowMenu
		->add( i18n( "terminal_split_top", "Split Terminal Top" ), findIcon( "split-vertical" ),
			   getKeybind( "terminal-split-top" ) )
		->setId( "terminal-split-top" );
	mWindowMenu
		->add( i18n( "terminal_split_bottom", "Split Terminal Bottom" ),
			   findIcon( "split-vertical" ), getKeybind( "terminal-split-bottom" ) )
		->setId( "terminal-split-bottom" );
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
	mWindowMenu
		->add( i18n( "show_console", "Show Console" ), findIcon( "terminal" ),
			   getKeybind( "console-toggle" ) )
		->setId( "console-toggle" );
	mWindowMenu
		->add( i18n( "inspect_widgets", "Inspect Widgets" ), findIcon( "package" ),
			   getKeybind( "debug-widget-tree-view" ) )
		->setId( "debug-widget-tree-view" );
	mWindowMenu->addEventListener( Event::OnItemClicked, [&]( const Event* event ) {
		if ( !event->getNode()->isType( UI_TYPE_MENUITEM ) )
			return;
		UIMenuItem* item = event->getNode()->asType<UIMenuItem>();
		if ( "zoom-in" == item->getId() ) {
			mSplitter->zoomIn();
		} else if ( "zoom-out" == item->getId() ) {
			mSplitter->zoomOut();
		} else if ( "zoom-reset" == item->getId() ) {
			mSplitter->zoomReset();
		} else {
			String text = String( event->getNode()->asType<UIMenuItem>()->getId() ).toLower();
			String::replaceAll( text, " ", "-" );
			String::replaceAll( text, "/", "-" );
			runCommand( text );
		}
	} );
	return mWindowMenu;
}

UIMenu* SettingsMenu::createRendererMenu() {
	mRendererMenu = UIPopUpMenu::New();

	mRendererMenu->addCheckBox( i18n( "vsync", "VSync" ), mApp->getConfig().context.VSync )
		->setId( "vsync" );
	mRendererMenu->add( i18n( "frame_rate_limit", "Frame Rate Limit" ), findIcon( "fps" ) )
		->setId( "frame_rate_limit" );

	mRendererMenu->addEventListener( Event::OnItemClicked, [&]( const Event* event ) {
		if ( !event->getNode()->isType( UI_TYPE_MENUITEM ) )
			return;
		UIMenuItem* item = event->getNode()->asType<UIMenuItem>();
		if ( "vsync" == item->getId() ) {
			mApp->getConfig().context.VSync = item->asType<UIMenuCheckBox>()->isActive();
			mApp->saveConfig();
			mApp->getNotificationCenter()->addNotification(
				i18n( "vsync_changed",
					  "Vsync configuration changed.\nRestart ecode to see the changes." )
					.unescape() );
		} else if ( "frame_rate_limit" == item->getId() ) {
			UIMessageBox* msgBox = UIMessageBox::New(
				UIMessageBox::INPUT,
				i18n( "set_frame_rate_limit", "Set Frame Rate Limit:\nSet 0 to disable it.\n" )
					.unescape() );
			msgBox->setTitle( mApp->getWindowTitle() );
			msgBox->setCloseShortcut( { KEY_ESCAPE, 0 } );
			msgBox->getTextInput()->setAllowOnlyNumbers( true, false );
			msgBox->getTextInput()->setText(
				String::toString( mApp->getConfig().context.FrameRateLimit ) );
			msgBox->showWhenReady();
			msgBox->addEventListener( Event::OnConfirm, [&, msgBox]( const Event* ) {
				int val;
				if ( String::fromString( val, msgBox->getTextInput()->getText() ) && val >= 0 ) {
					mApp->getConfig().context.FrameRateLimit = val;
					mApp->saveConfig();
					mApp->getWindow()->setFrameRateLimit( val );
					mApp->getNotificationCenter()->addNotification(
						i18n( "frame_rate_limit_applied", "Frame Rate Limit Applied" ) );
					msgBox->closeWindow();
				}
			} );
			mApp->setFocusEditorOnClose( msgBox );
		}
	} );

	UIPopUpMenu* glVersion = UIPopUpMenu::New();
	std::vector<GraphicsLibraryVersion> versions = Renderer::getAvailableGraphicsLibraryVersions();
	for ( const auto& ver : versions )
		glVersion
			->addRadioButton( Renderer::graphicsLibraryVersionToString( ver ),
							  GLi->version() == ver )
			->setData( ver );
	glVersion->addEventListener( Event::OnItemClicked, [&]( const Event* event ) {
		if ( !event->getNode()->isType( UI_TYPE_MENUITEM ) )
			return;
		UIMenuItem* item = event->getNode()->asType<UIMenuItem>();
		mApp->getConfig().context.Version = static_cast<GraphicsLibraryVersion>( item->getData() );
		mApp->saveConfig();
		mApp->getNotificationCenter()->addNotification(
			i18n( "glversion_changed",
				  "Renderer version changed.\nRestart ecode to see the changes." )
				.unescape() );
	} );
	mRendererMenu->addSubMenu( i18n( "ui_renderer_version", "Renderer Version" ),
							   findIcon( "renderer" ), glVersion );

	UIPopUpMenu* multisampleLvl = UIPopUpMenu::New();
	const std::vector<Uint32> msaaVals = { 0, 2, 4, 8, 16 };
	for ( const auto& val : msaaVals )
		multisampleLvl
			->addRadioButton( String::toString( val ),
							  mApp->getConfig().context.Multisamples == val )
			->setData( val );
	mRendererMenu->addSubMenu( i18n( "ui_multisamples_level", "Multisample Anti-Aliasing Level" ),
							   findIcon( "multisamples" ), multisampleLvl );
	multisampleLvl->addEventListener( Event::OnItemClicked, [&]( const Event* event ) {
		if ( !event->getNode()->isType( UI_TYPE_MENUITEM ) )
			return;
		UIMenuItem* item = event->getNode()->asType<UIMenuItem>();
		mApp->getConfig().context.Multisamples = item->getData();
		mApp->saveConfig();
		mApp->getNotificationCenter()->addNotification(
			i18n( "multisamples_changed",
				  "Multisample Anti-Aliasing Level applied.\nRestart ecode to see the changes." )
				.unescape() );
	} );

	return mRendererMenu;
}

UIMenu* SettingsMenu::createViewMenu() {
	mViewMenu = UIPopUpMenu::New();
	mViewMenu->addCheckBox( i18n( "show_line_numbers", "Show Line Numbers" ) )
		->setActive( mApp->getConfig().editor.showLineNumbers )
		->setId( "show-line-numbers" );
	mViewMenu->addCheckBox( i18n( "show_white_spaces", "Show White Spaces" ) )
		->setActive( mApp->getConfig().editor.showWhiteSpaces )
		->setId( "show-white-spaces" );
	mViewMenu->addCheckBox( i18n( "show_line_endings", "Show Line Endings" ) )
		->setActive( mApp->getConfig().editor.showLineEndings )
		->setId( "show-line-endings" );
	mViewMenu->addCheckBox( i18n( "show_indentation_guides", "Show Indentation Guides" ) )
		->setActive( mApp->getConfig().editor.showIndentationGuides )
		->setId( "show-indentation-guides" );
	mViewMenu->addCheckBox( i18n( "show_doc_info", "Show Document Info" ) )
		->setActive( mApp->getConfig().editor.showDocInfo )
		->setId( "show-doc-info" );
	mViewMenu->addCheckBox( i18n( "show_minimap", "Show Minimap" ) )
		->setActive( mApp->getConfig().editor.minimap )
		->setId( "show-minimap" );
	mViewMenu->addCheckBox( i18n( "highlight_matching_brackets", "Highlight Matching Bracket" ) )
		->setActive( mApp->getConfig().editor.highlightMatchingBracket )
		->setId( "highlight-matching-brackets" );
	mViewMenu->addCheckBox( i18n( "highlight_current_line", "Highlight Current Line" ) )
		->setActive( mApp->getConfig().editor.highlightCurrentLine )
		->setId( "highlight-current-line" );
	mViewMenu->addCheckBox( i18n( "highlight_selection_match", "Highlight Selection Match" ) )
		->setActive( mApp->getConfig().editor.highlightSelectionMatch )
		->setId( "highlight-selection-match" );
	mViewMenu->addCheckBox( i18n( "enable-vertical-scrollbar", "Enable Vertical ScrollBar" ) )
		->setActive( mApp->getConfig().editor.verticalScrollbar )
		->setId( "enable-vertical-scrollbar" );
	mViewMenu->addCheckBox( i18n( "enable_horizontal_scrollbar", "Enable Horizontal ScrollBar" ) )
		->setActive( mApp->getConfig().editor.horizontalScrollbar )
		->setId( "enable-horizontal-scrollbar" );
	mViewMenu->addCheckBox( i18n( "enable_color_preview", "Enable Color Preview" ) )
		->setActive( mApp->getConfig().editor.colorPreview )
		->setTooltipText( i18n( "enable_color_preview_tooltip",
								"Enables a quick preview of a color when the mouse\n"
								"is hover a word that represents a color." ) )
		->setId( "enable-color-preview" );
	mViewMenu->addCheckBox( i18n( "enable_color_picker", "Enable Color Picker" ) )
		->setActive( mApp->getConfig().editor.colorPickerSelection )
		->setTooltipText( i18n( "enable_color_picker_tooltip",
								"Enables the color picker tool when a double click selection\n"
								"is done over a word representing a color." ) )
		->setId( "enable-color-picker" );
	mViewMenu->addCheckBox( i18n( "hide_tabbar_on_single_tab", "Hide tabbar on single tab" ) )
		->setActive( mApp->getConfig().editor.hideTabBarOnSingleTab )
		->setTooltipText(
			i18n( "hide_tabbar_on_single_tab_tooltip",
				  "Hides the tabbar if there's only one element in the tab widget." ) )
		->setId( "hide-tabbar-on-single-tab" );
	mViewMenu
		->addCheckBox( i18n( "treeview_single_click_nav", "Single Click Navigation in Tree View" ) )
		->setActive( mApp->getConfig().editor.singleClickTreeNavigation )
		->setTooltipText( i18n(
			"treeview_single_click_nav_tooltip",
			"Uses single click to open files and expand subfolders in\nthe directory tree." ) )
		->setId( "treeview-single-click-nav" );
	mViewMenu->addCheckBox( i18n( "sync_project_tree", "Synchronize project tree with editor" ) )
		->setActive( mApp->getConfig().editor.syncProjectTreeWithEditor )
		->setTooltipText(
			i18n( "sync_project_tree_tooltip",
				  "Syncronizes the current focused document as the selected\nfile in the "
				  "directory tree." ) )
		->setId( "sync-project-tree" );

	mViewMenu->addEventListener( Event::OnItemClicked, [&]( const Event* event ) {
		if ( !event->getNode()->isType( UI_TYPE_MENUITEM ) )
			return;
		UIMenuItem* item = event->getNode()->asType<UIMenuItem>();
		if ( item->getId() == "show-line-numbers" ) {
			mApp->getConfig().editor.showLineNumbers = item->asType<UIMenuCheckBox>()->isActive();
			mSplitter->forEachEditor( [&]( UICodeEditor* editor ) {
				editor->setShowLineNumber( mApp->getConfig().editor.showLineNumbers );
			} );
		} else if ( item->getId() == "show-white-spaces" ) {
			mApp->getConfig().editor.showWhiteSpaces = item->asType<UIMenuCheckBox>()->isActive();
			mSplitter->forEachEditor( [&]( UICodeEditor* editor ) {
				editor->setShowWhitespaces( mApp->getConfig().editor.showWhiteSpaces );
			} );
		} else if ( item->getId() == "show-line-endings" ) {
			mApp->getConfig().editor.showLineEndings = item->asType<UIMenuCheckBox>()->isActive();
			mSplitter->forEachEditor( [&]( UICodeEditor* editor ) {
				editor->setShowLineEndings( mApp->getConfig().editor.showLineEndings );
			} );
		} else if ( item->getId() == "show-indentation-guides" ) {
			mApp->getConfig().editor.showIndentationGuides =
				item->asType<UIMenuCheckBox>()->isActive();
			mSplitter->forEachEditor( [&]( UICodeEditor* editor ) {
				editor->setShowIndentationGuides( mApp->getConfig().editor.showIndentationGuides );
			} );
		} else if ( item->getId() == "show-doc-info" ) {
			mApp->getConfig().editor.showDocInfo = item->asType<UIMenuCheckBox>()->isActive();
			if ( mApp->getDocInfo() )
				mApp->getDocInfo()->setVisible( mApp->getConfig().editor.showDocInfo );
			if ( mSplitter->curEditorExistsAndFocused() )
				mApp->updateDocInfo( mSplitter->getCurEditor()->getDocument() );
		} else if ( item->getId() == "show-minimap" ) {
			mApp->getConfig().editor.minimap = item->asType<UIMenuCheckBox>()->isActive();
			mSplitter->forEachEditor( [&]( UICodeEditor* editor ) {
				editor->showMinimap( mApp->getConfig().editor.minimap );
			} );
		} else if ( item->getId() == "highlight-matching-brackets" ) {
			mApp->getConfig().editor.highlightMatchingBracket =
				item->asType<UIMenuCheckBox>()->isActive();
			mSplitter->forEachEditor( [&]( UICodeEditor* editor ) {
				editor->setHighlightMatchingBracket(
					mApp->getConfig().editor.highlightMatchingBracket );
			} );
		} else if ( item->getId() == "highlight-current-line" ) {
			mApp->getConfig().editor.highlightCurrentLine =
				item->asType<UIMenuCheckBox>()->isActive();
			mSplitter->forEachEditor( [&]( UICodeEditor* editor ) {
				editor->setHighlightCurrentLine( mApp->getConfig().editor.highlightCurrentLine );
			} );
		} else if ( item->getId() == "highlight-selection-match" ) {
			mApp->getConfig().editor.highlightSelectionMatch =
				item->asType<UIMenuCheckBox>()->isActive();
			mSplitter->forEachEditor( [&]( UICodeEditor* editor ) {
				editor->setHighlightSelectionMatch(
					mApp->getConfig().editor.highlightSelectionMatch );
			} );
		} else if ( item->getId() == "enable-vertical-scrollbar" ) {
			mApp->getConfig().editor.verticalScrollbar = item->asType<UIMenuCheckBox>()->isActive();
			mSplitter->forEachEditor( [&]( UICodeEditor* editor ) {
				editor->setVerticalScrollBarEnabled( mApp->getConfig().editor.verticalScrollbar );
			} );
		} else if ( item->getId() == "enable-horizontal-scrollbar" ) {
			mApp->getConfig().editor.horizontalScrollbar =
				item->asType<UIMenuCheckBox>()->isActive();
			mSplitter->forEachEditor( [&]( UICodeEditor* editor ) {
				editor->setHorizontalScrollBarEnabled(
					mApp->getConfig().editor.horizontalScrollbar );
			} );
		} else if ( item->getId() == "enable-color-preview" ) {
			mApp->getConfig().editor.colorPreview = item->asType<UIMenuCheckBox>()->isActive();
			mSplitter->forEachEditor( [&]( UICodeEditor* editor ) {
				editor->setEnableColorPickerOnSelection( mApp->getConfig().editor.colorPreview );
			} );
		} else if ( item->getId() == "enable-color-picker" ) {
			mApp->getConfig().editor.colorPickerSelection =
				item->asType<UIMenuCheckBox>()->isActive();
			mSplitter->forEachEditor( [&]( UICodeEditor* editor ) {
				editor->setEnableColorPickerOnSelection(
					mApp->getConfig().editor.colorPickerSelection );
			} );
		} else if ( item->getId() == "hide-tabbar-on-single-tab" ) {
			mApp->getConfig().editor.hideTabBarOnSingleTab =
				item->asType<UIMenuCheckBox>()->isActive();
			mSplitter->setHideTabBarOnSingleTab( mApp->getConfig().editor.hideTabBarOnSingleTab );
		} else if ( item->getId() == "treeview-single-click-nav" ) {
			mApp->getConfig().editor.singleClickTreeNavigation =
				item->asType<UIMenuCheckBox>()->isActive();
			if ( mApp->getProjectTreeView() )
				mApp->getProjectTreeView()->setSingleClickNavigation(
					mApp->getConfig().editor.singleClickTreeNavigation );
		} else if ( item->getId() == "sync-project-tree" ) {
			mApp->getConfig().editor.syncProjectTreeWithEditor =
				item->asType<UIMenuCheckBox>()->isActive();
		} else {
			String text = String( event->getNode()->asType<UIMenuItem>()->getId() ).toLower();
			String::replaceAll( text, " ", "-" );
			String::replaceAll( text, "/", "-" );
			runCommand( text );
		}
	} );
	return mViewMenu;
}

UIPopUpMenu* SettingsMenu::createToolsMenu() {
	mToolsMenu = UIPopUpMenu::New();
	mToolsMenu
		->add( i18n( "locate", "Locate..." ), findIcon( "search" ), getKeybind( "open-locatebar" ) )
		->setId( "open-locatebar" );
	mToolsMenu
		->add( i18n( "command_palette", "Command Palette..." ), findIcon( "search" ),
			   getKeybind( "open-command-palette" ) )
		->setId( "open-command-palette" );
	mToolsMenu
		->add( i18n( "project_find", "Project Find..." ), findIcon( "search" ),
			   getKeybind( "open-global-search" ) )
		->setId( "open-global-search" );
	mToolsMenu
		->add( i18n( "go_to_line", "Go to line..." ), findIcon( "go-to-line" ),
			   getKeybind( "go-to-line" ) )
		->setId( "go-to-line" );
	mToolsMenu
		->add( i18n( "load_cur_dir_as_folder", "Load current document directory as folder" ),
			   findIcon( "folder" ), getKeybind( "load-current-dir" ) )
		->setId( "load-current-dir" );
	mToolsMenu->addEventListener( Event::OnItemClicked, [&]( const Event* event ) {
		if ( !event->getNode()->isType( UI_TYPE_MENUITEM ) )
			return;
		runCommand( event->getNode()->getId() );
	} );
	return mToolsMenu;
}

UIMenu* SettingsMenu::createHelpMenu() {
	UIPopUpMenu* helpMenu = UIPopUpMenu::New();
	helpMenu->add( i18n( "ecode_source", "ecode source code..." ), findIcon( "github" ) )
		->setId( "ecode-source" );
	helpMenu->add( i18n( "check_for_updates", "Check for Updates..." ), findIcon( "refresh" ) )
		->setId( "check-for-updates" );
	helpMenu->add( i18n( "about_ecode", "About ecode..." ) )->setId( "about-ecode" );
	helpMenu->addEventListener( Event::OnItemClicked, [&]( const Event* event ) {
		runCommand( event->getNode()->getId() );
	} );
	return helpMenu;
}

void SettingsMenu::toggleSettingsMenu() {
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

void SettingsMenu::updateProjectSettingsMenu() {
	mDocMenu->getItemId( "project_settings" )->setEnabled( !mApp->getCurrentProject().empty() );

	for ( size_t i = 0; i < mProjectMenu->getCount(); i++ ) {
		mProjectMenu->getItem( i )->setEnabled( !mApp->getCurrentProject().empty() &&
												!mApp->getProjectDocConfig().useGlobalSettings );
	}

	mSplitter->forEachEditor( [&]( UICodeEditor* editor ) {
		editor->setLineBreakingColumn( !mApp->getCurrentProject().empty() &&
											   !mApp->getProjectDocConfig().useGlobalSettings
										   ? mApp->getProjectDocConfig().doc.lineBreakingColumn
										   : mApp->getConfig().doc.lineBreakingColumn );
	} );

	mProjectMenu->getItemId( "trim_whitespaces" )
		->asType<UIMenuCheckBox>()
		->setActive( mApp->getProjectDocConfig().doc.trimTrailingWhitespaces );

	mProjectMenu->getItemId( "force_nl" )
		->asType<UIMenuCheckBox>()
		->setActive( mApp->getProjectDocConfig().doc.forceNewLineAtEndOfFile );

	mProjectMenu->getItemId( "write_bom" )
		->asType<UIMenuCheckBox>()
		->setActive( mApp->getProjectDocConfig().doc.writeUnicodeBOM );

	mProjectMenu->getItemId( "auto_indent" )
		->asType<UIMenuCheckBox>()
		->setActive( mApp->getProjectDocConfig().doc.autoDetectIndentType );

	auto* curIndent = mProjectMenu->find( "indent_width" )
						  ->asType<UIMenuSubMenu>()
						  ->getSubMenu()
						  ->find( String::format( "indent_width_%d",
												  mApp->getProjectDocConfig().doc.indentWidth ) );

	if ( curIndent )
		curIndent->asType<UIMenuRadioButton>()->setActive( true );

	mProjectMenu->find( "indent_type" )
		->asType<UIMenuSubMenu>()
		->getSubMenu()
		->find( !mApp->getProjectDocConfig().doc.indentSpaces ? "tabs" : "spaces" )
		->asType<UIMenuRadioButton>()
		->setActive( true );

	mProjectMenu->find( "tab_width" )
		->asType<UIMenuSubMenu>()
		->getSubMenu()
		->find( String::format( "tab_width_%d", mApp->getProjectDocConfig().doc.tabWidth ) )
		->asType<UIMenuRadioButton>()
		->setActive( true );

	mProjectMenu->find( "line_endings" )
		->asType<UIMenuSubMenu>()
		->getSubMenu()
		->find( TextDocument::lineEndingToString( mApp->getProjectDocConfig().doc.lineEndings ) )
		->asType<UIMenuRadioButton>()
		->setActive( true );

	mProjectMenu->getItemId( "use_global_settings" )
		->setEnabled( true )
		->asType<UIMenuCheckBox>()
		->setActive( mApp->getProjectDocConfig().useGlobalSettings );
}

void SettingsMenu::updateTerminalMenu() {
	bool enabled =
		mSplitter->getCurWidget() && mSplitter->getCurWidget()->isType( UI_TYPE_TERMINAL );
	mSettingsMenu->getItemId( "term-menu" )->setEnabled( enabled );
	if ( !enabled )
		return;
	mTerminalMenu->getItemId( "exclusive-mode" )
		->asType<UIMenuCheckBox>()
		->setActive( mSplitter->getCurWidget()->asType<UITerminal>()->getExclusiveMode() );
}

void SettingsMenu::updateDocumentMenu() {
	if ( !mSplitter->getCurWidget() || !mSplitter->getCurWidget()->isType( UI_TYPE_CODEEDITOR ) ) {
		mSettingsMenu->getItemId( "doc-menu" )->setEnabled( false );
		return;
	}

	mSettingsMenu->getItemId( "doc-menu" )->setEnabled( true );

	const TextDocument& doc = mSplitter->getCurEditor()->getDocument();

	mDocMenu->find( "auto_indent_cur" )
		->asType<UIMenuCheckBox>()
		->setActive( doc.getAutoDetectIndentType() );

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

	mDocMenu->find( "write_bom_cur" )->asType<UIMenuCheckBox>()->setActive( doc.getBOM() );

	mDocMenu->find( "line_endings_cur" )
		->asType<UIMenuSubMenu>()
		->getSubMenu()
		->find( TextDocument::lineEndingToString( doc.getLineEnding() ) )
		->asType<UIMenuRadioButton>()
		->setActive( true );

	mDocMenu->find( "read_only" )
		->asType<UIMenuCheckBox>()
		->setActive( mSplitter->getCurEditor()->isLocked() );
}

void SettingsMenu::showProjectTreeMenu() {
	Vector2f pos( mApp->getWindow()->getInput()->getMousePosf() );
	mProjectTreeMenu->nodeToWorldTranslation( pos );
	UIMenu::findBestMenuPos( pos, mProjectTreeMenu );
	mProjectTreeMenu->setPixelsPosition( pos );
	mProjectTreeMenu->show();
}

void SettingsMenu::createProjectTreeMenu() {
	if ( mProjectTreeMenu && mProjectTreeMenu->isVisible() )
		mProjectTreeMenu->close();

	mProjectTreeMenu = UIPopUpMenu::New();

	if ( !mApp->getCurrentProject().empty() ) {
		mProjectTreeMenu->add( i18n( "new_file", "New File..." ), findIcon( "file-add" ) )
			->setId( "new_file" );
		mProjectTreeMenu->add( i18n( "new_folder", "New Folder..." ), findIcon( "folder-add" ) )
			->setId( "new_folder" );
		mProjectTreeMenu->add( i18n( "open_folder", "Open Folder..." ), findIcon( "folder-open" ) )
			->setId( "open_folder" );
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
		mProjectTreeMenu->add( i18n( "refresh_view", "Refresh View..." ), findIcon( "refresh" ) )
			->setId( "refresh-view" );
		mProjectTreeMenu->addSeparator();
		mProjectTreeMenu->add( i18n( "configure_ignore_files", "Configure Ignore Files..." ) )
			->setId( "configure-ignore-files" );
	} else if ( !mApp->getFileSystemModel() ) {
		mProjectTreeMenu->add( i18n( "open_folder", "Open Folder..." ), findIcon( "folder-open" ) )
			->setId( "open-folder" );
	}

	mProjectTreeMenu->addEventListener( Event::OnItemClicked, [&]( const Event* event ) {
		if ( !event->getNode()->isType( UI_TYPE_MENUITEM ) )
			return;
		UIMenuItem* item = event->getNode()->asType<UIMenuItem>();
		std::string id( item->getId() );
		if ( "new_file" == id || "new_file_in_place" == id ) {
			mApp->newFile( FileInfo( mApp->getCurrentProject() ) );
		} else if ( "new_folder" == id ) {
			mApp->newFolder( FileInfo( mApp->getCurrentProject() ) );
		} else if ( "open_folder" == id ) {
			Engine::instance()->openURI( mApp->getCurrentProject() );
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

static void fsRemoveAll( const std::string& fpath ) {
#if EE_PLATFORM == EE_PLATFORM_WIN
	fs::remove_all( std::filesystem::path( String( fpath ).toWideString() ) );
#else
	fs::remove_all( fpath );
#endif
}

void SettingsMenu::createProjectTreeMenu( const FileInfo& file ) {
	if ( mProjectTreeMenu && mProjectTreeMenu->isVisible() )
		mProjectTreeMenu->close();
	mProjectTreeMenu = UIPopUpMenu::New();

	if ( file.isDirectory() ) {
		mProjectTreeMenu->add( i18n( "new_file", "New File..." ), findIcon( "file-add" ) )
			->setId( "new_file" );
		mProjectTreeMenu->add( i18n( "new_folder", "New Folder..." ), findIcon( "folder-add" ) )
			->setId( "new_folder" );
		mProjectTreeMenu->add( i18n( "open_folder", "Open Folder..." ), findIcon( "folder-open" ) )
			->setId( "open_folder" );
	} else {
		if ( file.isRegularFile() ) {
			auto curDir( mApp->getCurrentWorkingDir() );
			FileSystem::dirAddSlashAtEnd( curDir );
			if ( curDir == file.getDirectoryPath() ) {
				mProjectTreeMenu
					->add( i18n( "new_file_in_file_folder", "New File in File Folder..." ),
						   findIcon( "file-add" ) )
					->setId( "new_file" );
			}
		}

		mProjectTreeMenu->add( i18n( "open_folder", "Open File" ), findIcon( "document-open" ) )
			->setId( "open_file" );
		mProjectTreeMenu
			->add( i18n( "open_containin_folder", "Open Containing Folder..." ),
				   findIcon( "folder-open" ) )
			->setId( "open_containing_folder" );
		mProjectTreeMenu
			->add( i18n( "new_file_in_directory", "New File in directory..." ),
				   findIcon( "file-add" ) )
			->setId( "new_file_in_place" );
		mProjectTreeMenu
			->add( i18n( "duplicate_file", "Duplicate File..." ), findIcon( "file-copy" ) )
			->setId( "duplicate_file" );
	}
	mProjectTreeMenu->add( i18n( "rename", "Rename" ), findIcon( "edit" ), "F2" )
		->setId( "rename" );
	mProjectTreeMenu->add( i18n( "remove", "Remove..." ), findIcon( "delete-bin" ) )
		->setId( "remove" );

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
	mProjectTreeMenu->add( i18n( "refresh_view", "Refresh View..." ), findIcon( "refresh" ) )
		->setId( "refresh-view" );

	if ( !mApp->getCurrentProject().empty() ) {
		mProjectTreeMenu->addSeparator();
		mProjectTreeMenu->add( i18n( "configure_ignore_files", "Configure Ignore Files..." ) )
			->setId( "configure-ignore-files" );
	}

	mProjectTreeMenu->addEventListener( Event::OnItemClicked, [&, file]( const Event* event ) {
		if ( !event->getNode()->isType( UI_TYPE_MENUITEM ) )
			return;
		UIMenuItem* item = event->getNode()->asType<UIMenuItem>();
		std::string id( item->getId() );

		if ( "new_file" == id || "new_file_in_place" == id ) {
			mApp->newFile( file );
		} else if ( "new_folder" == id ) {
			mApp->newFolder( file );
		} else if ( "open_file" == id ) {
			mApp->loadFileFromPath( file.getFilepath() );
		} else if ( "remove" == id ) {
			UIMessageBox* msgBox =
				UIMessageBox::New( UIMessageBox::OK_CANCEL,
								   String::format( i18n( "confirm_remove_file",
														 "Do you really want to remove \"%s\"?" )
													   .toUtf8()
													   .c_str(),
												   file.getFileName().c_str() ) );
			msgBox->addEventListener( Event::OnConfirm, [&, file, msgBox]( const Event* ) {
				auto errFn = [&, file] {
					mApp->errorMsgBox( String::format(
						std::string( i18n( "couldnt_remove", "Couldn't remove" ).toUtf8() + "%s." )
							.c_str(),
						file.isDirectory() ? i18n( "directory", "directory" ).toUtf8().c_str()
										   : i18n( "file", "file" ).toUtf8().c_str() ) );
				};

				if ( file.isDirectory() ) {
					try {
						std::string fpath( file.getFilepath() );
						FileSystem::dirRemoveSlashAtEnd( fpath );
						fsRemoveAll( fpath );
					} catch ( const fs::filesystem_error& err ) {
						errFn();
					}
				} else if ( !FileSystem::fileRemove( file.getFilepath() ) ) {
					errFn();
				}
				msgBox->closeWindow();
			} );
			msgBox->setTitle( i18n( "remove_file_question", "Remove file?" ) );
			msgBox->center();
			msgBox->showWhenReady();
		} else if ( "duplicate_file" == id ) {
			UIMessageBox* msgBox = mApp->newInputMsgBox(
				String::format( "%s \"%s\"",
								i18n( "duplicate_file", "Duplicate file" ).toUtf8().c_str(),
								file.getFileName().c_str() ),
				i18n( "enter_duplicate_file_name", "Enter duplicate file name:" ) );
			msgBox->addEventListener( Event::OnConfirm, [&, file, msgBox]( const Event* ) {
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
		} else if ( "open_containing_folder" == id ) {
			Engine::instance()->openURI( file.getDirectoryPath() );
		} else if ( "open_folder" == id ) {
			Engine::instance()->openURI( file.getFilepath() );
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
	for ( UIPopUpMenu* menu : mColorSchemeMenues ) {
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
	for ( UIPopUpMenu* menu : mFileTypeMenues ) {
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

UIPopUpMenu* SettingsMenu::getWindowMenu() const {
	return mWindowMenu;
}

UIPopUpMenu* SettingsMenu::getSettingsMenu() const {
	return mSettingsMenu;
}

} // namespace ecode
