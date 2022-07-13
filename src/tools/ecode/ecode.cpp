#include "ecode.hpp"
#include "plugins/autocomplete/autocompleteplugin.hpp"
#include "plugins/formatter/formatterplugin.hpp"
#include "plugins/linter/linterplugin.hpp"
#include "version.hpp"
#include <algorithm>
#include <args/args.hxx>

namespace ecode {

Clock globalClock;
bool firstFrame = true;
App* appInstance = nullptr;

void appLoop() {
	appInstance->mainLoop();
}

bool App::onCloseRequestCallback( EE::Window::Window* ) {
	if ( mSplitter->isAnyEditorDirty() ) {
		UIMessageBox* msgBox = UIMessageBox::New(
			UIMessageBox::OK_CANCEL,
			i18n( "confirm_ecode_exit",
				  "Do you really want to close the code editor?\nAll changes will be lost." )
				.unescape() );
		msgBox->addEventListener( Event::MsgBoxConfirmClick, [&]( const Event* ) {
			if ( !mCurrentProject.empty() )
				mConfig.saveProject( mCurrentProject, mSplitter, mConfigPath, mProjectDocConfig );
			mWindow->close();
		} );
		msgBox->addEventListener( Event::OnClose, [&]( const Event* ) { msgBox = nullptr; } );
		msgBox->setTitle( String::format( i18n( "close_title", "Close %s?" ).toUtf8().c_str(),
										  mWindowTitle.c_str() ) );
		msgBox->center();
		msgBox->showWhenReady();
		return false;
	} else {
		if ( !mCurrentProject.empty() )
			mConfig.saveProject( mCurrentProject, mSplitter, mConfigPath, mProjectDocConfig );
		return true;
	}
}

void App::saveDoc() {
	if ( !mSplitter->curEditorExistsAndFocused() )
		return;

	if ( mSplitter->getCurEditor()->getDocument().hasFilepath() ) {
		if ( mSplitter->getCurEditor()->save() )
			updateEditorState();
	} else {
		saveFileDialog( mSplitter->getCurEditor() );
	}
}

void App::saveAllProcess() {
	if ( mTmpDocs.empty() )
		return;

	mSplitter->forEachEditorStoppable( [&]( UICodeEditor* editor ) {
		if ( editor->getDocument().isDirty() &&
			 std::find( mTmpDocs.begin(), mTmpDocs.end(), &editor->getDocument() ) !=
				 mTmpDocs.end() ) {
			if ( editor->getDocument().hasFilepath() ) {
				editor->save();
				updateEditorTabTitle( editor );
				if ( mSplitter->getCurEditor() == editor )
					updateEditorTitle( editor );
				mTmpDocs.erase( &editor->getDocument() );
			} else {
				UIFileDialog* dialog = saveFileDialog( editor, false );
				dialog->addEventListener( Event::SaveFile, [&, editor]( const Event* ) {
					updateEditorTabTitle( editor );
					if ( mSplitter->getCurEditor() == editor )
						updateEditorTitle( editor );
				} );
				dialog->addEventListener( Event::OnWindowClose, [&, editor]( const Event* ) {
					mTmpDocs.erase( &editor->getDocument() );
					if ( !SceneManager::instance()->isShootingDown() && !mTmpDocs.empty() )
						saveAllProcess();
				} );
				return true;
			}
		}
		return false;
	} );
}

void App::saveAll() {
	mSplitter->forEachEditor( [&]( UICodeEditor* editor ) {
		if ( editor->isDirty() )
			mTmpDocs.insert( &editor->getDocument() );
	} );
	saveAllProcess();
}

std::string App::titleFromEditor( UICodeEditor* editor ) {
	std::string title( editor->getDocument().getFilename() );
	return editor->getDocument().isDirty() ? title + "*" : title;
}

void App::updateEditorTabTitle( UICodeEditor* editor ) {
	std::string title( titleFromEditor( editor ) );
	if ( editor->getData() ) {
		UITab* tab = (UITab*)editor->getData();
		tab->setText( title );
	}
}

void App::updateEditorTitle( UICodeEditor* editor ) {
	std::string title( titleFromEditor( editor ) );
	updateEditorTabTitle( editor );
	setAppTitle( title );
}

void App::setAppTitle( const std::string& title ) {
	std::string fullTitle( mWindowTitle );
	if ( !mCurrentProject.empty() ) {
		std::string currentProject( FileSystem::fileNameFromPath( mCurrentProject ) );
		if ( !currentProject.empty() )
			fullTitle += " - " + currentProject;
	}

	if ( !title.empty() )
		fullTitle += " - " + title;

	mWindow->setTitle( fullTitle );
}

void App::onDocumentModified( UICodeEditor* editor, TextDocument& ) {
	bool isDirty = editor->getDocument().isDirty();
	bool wasDirty =
		!mWindow->getTitle().empty() && mWindow->getTitle()[mWindow->getTitle().size() - 1] == '*';

	if ( isDirty != wasDirty )
		setAppTitle( titleFromEditor( editor ) );

	bool tabDirty = ( (UITab*)editor->getData() )->getText().lastChar() == '*';

	if ( isDirty != tabDirty )
		updateEditorTitle( editor );
}

void App::openFileDialog() {
	UIFileDialog* dialog = UIFileDialog::New( UIFileDialog::DefaultFlags, "*",
											  mLastFileFolder.empty() ? "." : mLastFileFolder );
	dialog->setWinFlags( UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON | UI_WIN_MODAL );
	dialog->setTitle( i18n( "open_file", "Open File" ) );
	dialog->setCloseShortcut( KEY_ESCAPE );
	dialog->addEventListener( Event::OpenFile, [&]( const Event* event ) {
		auto file = event->getNode()->asType<UIFileDialog>()->getFullPath();
		mLastFileFolder = FileSystem::fileRemoveFileName( file );
		loadFileFromPath( file );
	} );
	dialog->addEventListener( Event::OnWindowClose, [&]( const Event* ) {
		if ( mSplitter && mSplitter->getCurWidget() && !SceneManager::instance()->isShootingDown() )
			mSplitter->getCurWidget()->setFocus();
	} );
	dialog->center();
	dialog->show();
}

void App::openFolderDialog() {
	UIFileDialog* dialog =
		UIFileDialog::New( UIFileDialog::DefaultFlags | UIFileDialog::AllowFolderSelect |
							   UIFileDialog::ShowOnlyFolders,
						   "*", "." );
	dialog->setWinFlags( UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON | UI_WIN_MODAL );
	dialog->setTitle( i18n( "open_folder", "Open Folder" ) );
	dialog->setCloseShortcut( KEY_ESCAPE );
	dialog->addEventListener( Event::OpenFile, [&]( const Event* event ) {
		String path( event->getNode()->asType<UIFileDialog>()->getFullPath() );
		if ( FileSystem::isDirectory( path ) )
			loadFolder( path );
	} );
	dialog->addEventListener( Event::OnWindowClose, [&]( const Event* ) {
		if ( mSplitter && mSplitter->getCurWidget() && !SceneManager::instance()->isShootingDown() )
			mSplitter->getCurWidget()->setFocus();
	} );
	dialog->center();
	dialog->show();
}

void App::openFontDialog( std::string& fontPath, bool loadingMonoFont ) {
	std::string absoluteFontPath( fontPath );
	if ( FileSystem::isRelativePath( absoluteFontPath ) )
		absoluteFontPath = mResPath + fontPath;
	UIFileDialog* dialog =
		UIFileDialog::New( UIFileDialog::DefaultFlags, "*.ttf; *.otf; *.wolff; *.otb; *.bdf",
						   FileSystem::fileRemoveFileName( absoluteFontPath ) );
	ModelIndex index = dialog->getMultiView()->getListView()->findRowWithText(
		FileSystem::fileNameFromPath( fontPath ), true, true );
	if ( index.isValid() )
		dialog->runOnMainThread(
			[&, dialog, index]() { dialog->getMultiView()->setSelection( index ); } );
	dialog->setWinFlags( UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON | UI_WIN_MODAL );
	dialog->setTitle( i18n( "select_font_file", "Select Font File" ) );
	dialog->setCloseShortcut( KEY_ESCAPE );
	dialog->addEventListener( Event::OnWindowClose, [&]( const Event* ) {
		if ( mSplitter && mSplitter->getCurWidget() && !SceneManager::instance()->isShootingDown() )
			mSplitter->getCurWidget()->setFocus();
	} );
	dialog->addEventListener( Event::OpenFile, [&, loadingMonoFont]( const Event* event ) {
		auto newPath = event->getNode()->asType<UIFileDialog>()->getFullPath();
		if ( String::startsWith( newPath, mResPath ) )
			newPath = newPath.substr( mResPath.size() );
		if ( fontPath != newPath ) {
			fontPath = newPath;
			if ( !loadingMonoFont )
				return;
			auto fontName =
				FileSystem::fileRemoveExtension( FileSystem::fileNameFromPath( fontPath ) );
			FontTrueType* fontMono = loadFont( fontName, fontPath );
			if ( fontMono ) {
				auto loadMonoFont = [&]( FontTrueType* fontMono ) {
					mFontMono = fontMono;
					mFontMono->setBoldAdvanceSameAsRegular( true );
					if ( mSplitter ) {
						mSplitter->forEachEditor(
							[&]( UICodeEditor* editor ) { editor->setFont( mFontMono ); } );
					}
				};
				if ( !fontMono->isMonospace() ) {
					auto* msgBox = UIMessageBox::New(
						UIMessageBox::YES_NO,
						i18n(
							"confirm_loading_none_monospace_font",
							"The editor only supports monospaced fonts and the selected font isn't "
							"flagged as monospace.\nDo you want to load it anyways?\nPerformance "
							"and memory usage will be awful without a monospaced font." )
							.unescape() );
					msgBox->addEventListener(
						Event::MsgBoxConfirmClick,
						[&, loadMonoFont, fontMono]( const Event* ) { loadMonoFont( fontMono ); } );
					msgBox->addEventListener( Event::MsgBoxCancelClick, [fontMono]( const Event* ) {
						FontManager::instance()->remove( fontMono );
					} );
					msgBox->addEventListener( Event::OnClose,
											  [&]( const Event* ) { msgBox = nullptr; } );
					msgBox->setTitle( i18n( "confirm_loading_font", "Font loading confirmation" ) );
					msgBox->center();
					msgBox->showWhenReady();
				} else {
					loadMonoFont( fontMono );
				}
			}
		}
	} );
	dialog->center();
	dialog->show();
}

void App::downloadFileWebDialog() {
	UIMessageBox* msgBox = UIMessageBox::New(
		UIMessageBox::INPUT, i18n( "please_enter_file_url", "Please enter the file URL..." ) );

	msgBox->setTitle( mWindowTitle );
	msgBox->getTextInput()->setHint( i18n( "any_https_or_http_url", "Any https or http URL" ) );
	msgBox->setCloseShortcut( { KEY_ESCAPE, KEYMOD_NONE } );
	msgBox->showWhenReady();
	msgBox->addEventListener( Event::MsgBoxConfirmClick, [&, msgBox]( const Event* ) {
		std::string url( msgBox->getTextInput()->getText().toUtf8() );
		downloadFileWeb( url );
		if ( mSplitter->getCurWidget() )
			mSplitter->getCurWidget()->setFocus();
		msgBox->closeWindow();
	} );
}

void App::downloadFileWeb( const std::string& url ) {
	loadFileFromPath( url, true );
}

UIFileDialog* App::saveFileDialog( UICodeEditor* editor, bool focusOnClose ) {
	if ( !editor )
		return nullptr;
	UIFileDialog* dialog =
		UIFileDialog::New( UIFileDialog::DefaultFlags | UIFileDialog::SaveDialog, "." );
	dialog->setWinFlags( UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON | UI_WIN_MODAL );
	dialog->setTitle( i18n( "save_file_as", "Save File As" ) );
	dialog->setCloseShortcut( KEY_ESCAPE );
	std::string filename( editor->getDocument().getFilename() );
	if ( FileSystem::fileExtension( editor->getDocument().getFilename() ).empty() )
		filename += editor->getSyntaxDefinition().getFileExtension();
	dialog->setFileName( filename );
	dialog->addEventListener( Event::SaveFile, [&, editor]( const Event* event ) {
		if ( editor ) {
			std::string path( event->getNode()->asType<UIFileDialog>()->getFullPath() );
			if ( !path.empty() && !FileSystem::isDirectory( path ) &&
				 FileSystem::fileCanWrite( FileSystem::fileRemoveFileName( path ) ) ) {
				if ( editor->getDocument().save( path ) ) {
					updateEditorState();
				} else {
					UIMessageBox* msg =
						UIMessageBox::New( UIMessageBox::OK, i18n( "coudlnt_write_the_file",
																   "Couldn't write the file." ) );
					msg->setTitle( "Error" );
					msg->show();
				}
			} else {
				UIMessageBox* msg = UIMessageBox::New(
					UIMessageBox::OK,
					i18n( "empty_file_name", "You must set a name to the file." ) );
				msg->setTitle( "Error" );
				msg->show();
			}
		}
	} );
	if ( focusOnClose ) {
		dialog->addEventListener( Event::OnWindowClose, [&, editor]( const Event* ) {
			if ( editor && !SceneManager::instance()->isShootingDown() )
				editor->setFocus();
		} );
	}
	dialog->center();
	dialog->show();
	return dialog;
}

void App::runCommand( const std::string& command ) {
	if ( mSplitter->getCurWidget() ) {
		if ( mSplitter->getCurWidget()->isType( UI_TYPE_CODEEDITOR ) ) {
			mSplitter->getCurWidget()->asType<UICodeEditor>()->getDocument().execute( command );
		} else if ( mSplitter->getCurWidget()->isType( UI_TYPE_TERMINAL ) ) {
			mSplitter->getCurWidget()->asType<UITerminal>()->execute( command );
		}
	}
}

void App::loadConfig() {
	mConfigPath = Sys::getConfigPath( "ecode" );
	if ( !FileSystem::fileExists( mConfigPath ) )
		FileSystem::makeDir( mConfigPath );
	FileSystem::dirAddSlashAtEnd( mConfigPath );
	mPluginsPath = mConfigPath + "plugins";
	mColorSchemesPath = mConfigPath + "colorschemes";
	mTerminalColorSchemesPath =
		mConfigPath + "terminal" + FileSystem::getOSSlash() + "colorschemes";
	if ( !FileSystem::fileExists( mPluginsPath ) )
		FileSystem::makeDir( mPluginsPath );
	FileSystem::dirAddSlashAtEnd( mPluginsPath );
#ifndef EE_DEBUG
	Log::create( mConfigPath + "ecode.log", LogLevel::Info, false, true );
#else
	Log::create( mConfigPath + "ecode.log", LogLevel::Debug, true, true );
#endif

	mConfig.load( mConfigPath, mKeybindingsPath, mInitColorScheme, mRecentFiles, mRecentFolders,
				  mResPath, mDisplayDPI );
}

void App::saveConfig() {
	mConfig.save( mRecentFiles, mRecentFolders,
				  mProjectSplitter ? mProjectSplitter->getSplitPartition().toString() : "15%",
				  mWindow,
				  mSplitter ? mSplitter->getCurrentColorSchemeName() : mConfig.editor.colorScheme,
				  mDocSearchController->getSearchBarConfig(),
				  mGlobalSearchController->getGlobalSearchBarConfig() );
}

static std::string keybindFormat( std::string str ) {
	if ( !str.empty() ) {
		String::replace( str, "mod", KeyMod::getDefaultModifierString() );
		str[0] = std::toupper( str[0] );
		size_t found = str.find_first_of( '+' );
		while ( found != std::string::npos ) {
			if ( found + 1 < str.size() ) {
				str[found + 1] = std::toupper( str[found + 1] );
			}
			found = str.find_first_of( '+', found + 1 );
		}
		return str;
	}
	return "";
}

std::string App::getKeybind( const std::string& command ) {
	auto it = mKeybindingsInvert.find( command );
	if ( it != mKeybindingsInvert.end() )
		return keybindFormat( it->second );
	return "";
}

ProjectDirectoryTree* App::getDirTree() const {
	return mDirTree ? mDirTree.get() : nullptr;
}

std::shared_ptr<ThreadPool> App::getThreadPool() const {
	return mThreadPool;
}

bool App::trySendUnlockedCmd( const KeyEvent& keyEvent ) {
	if ( mSplitter->curEditorExistsAndFocused() ) {
		std::string cmd = mSplitter->getCurEditor()->getKeyBindings().getCommandFromKeyBind(
			{ keyEvent.getKeyCode(), keyEvent.getMod() } );
		if ( !cmd.empty() && mSplitter->getCurEditor()->isUnlockedCommand( cmd ) ) {
			mSplitter->getCurEditor()->getDocument().execute( cmd );
			return true;
		}
	}
	return false;
}

void App::closeApp() {
	if ( onCloseRequestCallback( mWindow ) )
		mWindow->close();
}

void App::mainLoop() {
	mWindow->getInput()->update();
	Time elapsed = SceneManager::instance()->getElapsed();
	SceneManager::instance()->update();

	if ( SceneManager::instance()->getUISceneNode()->invalidated() || mConsole->isActive() ||
		 mConsole->isFading() ) {
		mWindow->clear();
		SceneManager::instance()->draw();
		mConsole->draw( elapsed );
		mWindow->display();
		if ( firstFrame ) {
			Log::info( "First frame took: %.2fms", globalClock.getElapsed().asMilliseconds() );
			firstFrame = false;
		}
	} else {
		mWindow->getInput()->waitEvent( Milliseconds( mWindow->hasFocus() ? 16 : 100 ) );
	}
}

void App::onFileDropped( String file ) {
	Vector2f mousePos( mUISceneNode->getEventDispatcher()->getMousePosf() );
	Node* node = mUISceneNode->overFind( mousePos );
	UIWidget* widget = mSplitter->getCurWidget();
	UITab* tab = nullptr;
	UICodeEditor* codeEditor = nullptr;
	if ( !node )
		node = widget;
	if ( node && node->isType( UI_TYPE_CODEEDITOR ) ) {
		codeEditor = node->asType<UICodeEditor>();
		if ( ( codeEditor->getDocument().isLoading() || !codeEditor->getDocument().isEmpty() ) &&
			 !Image::isImageExtension( file ) ) {
			auto d = mSplitter->createCodeEditorInTabWidget(
				mSplitter->tabWidgetFromEditor( codeEditor ) );
			codeEditor = d.second;
			tab = d.first;
		}
	}
	loadFileFromPath( file, false, codeEditor, [tab]( UICodeEditor*, const std::string& ) {
		if ( tab )
			tab->getTabWidget()->setTabSelected( tab );
	} );
}

void App::onTextDropped( String text ) {
	Vector2f mousePos( mUISceneNode->getEventDispatcher()->getMousePosf() );
	Node* node = mUISceneNode->overFind( mousePos );
	UICodeEditor* codeEditor =
		mSplitter->curEditorExistsAndFocused() ? mSplitter->getCurEditor() : nullptr;
	if ( node && node->isType( UI_TYPE_CODEEDITOR ) )
		codeEditor = node->asType<UICodeEditor>();
	if ( codeEditor && !text.empty() ) {
		if ( text[text.size() - 1] != '\n' )
			text += '\n';
		codeEditor->getDocument().textInput( text );
	}
}

App::App() : mThreadPool( ThreadPool::createShared( eemax<int>( 2, Sys::getCPUCount() ) ) ) {}

App::~App() {
	if ( mFileWatcher )
		delete mFileWatcher;
	if ( mFileSystemListener )
		delete mFileSystemListener;
	saveConfig();
	eeSAFE_DELETE( mSplitter );
	eeSAFE_DELETE( mAutoCompletePlugin );
	eeSAFE_DELETE( mLinterPlugin );
	eeSAFE_DELETE( mFormatterPlugin );
	eeSAFE_DELETE( mConsole );
}

void App::createWidgetTreeView() {
	UIWindow* uiWin = UIWindow::NewOpt( UIWindow::LINEAR_LAYOUT );
	uiWin->setMinWindowSize( 600, 400 );
	uiWin->setWinFlags( UI_WIN_DEFAULT_FLAGS | UI_WIN_RESIZEABLE | UI_WIN_MAXIMIZE_BUTTON );
	UITreeView* widgetTree = UITreeView::New();
	widgetTree->setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::MatchParent );
	widgetTree->setParent( uiWin );
	widgetTree->setHeadersVisible( true );
	widgetTree->setAutoExpandOnSingleColumn( true );
	widgetTree->setExpanderIconSize( mMenuIconSize );
	widgetTree->setAutoColumnsWidth( true );
	widgetTree->setModel( WidgetTreeModel::New( mUISceneNode ) );
	uiWin->center();
}

void App::updateRecentFiles() {
	UINode* node = nullptr;
	if ( mSettingsMenu && ( node = mSettingsMenu->getItemId( "menu-recent-files" ) ) ) {
		UIMenuSubMenu* uiMenuSubMenu = static_cast<UIMenuSubMenu*>( node );
		UIMenu* menu = uiMenuSubMenu->getSubMenu();
		uiMenuSubMenu->setEnabled( !mRecentFiles.empty() );
		menu->removeAll();
		menu->removeEventsOfType( Event::OnItemClicked );
		if ( mRecentFiles.empty() )
			return;
		for ( const auto& file : mRecentFiles )
			menu->add( file );
		menu->addSeparator();
		menu->add( i18n( "clear_menu", "Clear Menu" ) )->setId( "clear-menu" );
		menu->addEventListener( Event::OnItemClicked, [&]( const Event* event ) {
			if ( !event->getNode()->isType( UI_TYPE_MENUITEM ) )
				return;
			const std::string& id = event->getNode()->asType<UIMenuItem>()->getId();
			if ( id != "clear-menu" ) {
				const String& txt = event->getNode()->asType<UIMenuItem>()->getText();
				std::string path( txt.toUtf8() );
				if ( ( FileSystem::fileExists( path ) && !FileSystem::isDirectory( path ) ) ||
					 String::startsWith( path, "https://" ) ||
					 String::startsWith( path, "http://" ) ) {
					loadFileFromPath( path );
				}
			} else {
				mRecentFiles.clear();
				updateRecentFiles();
			}
		} );
	}
}

void App::updateRecentFolders() {
	UINode* node = nullptr;
	if ( mSettingsMenu && ( node = mSettingsMenu->getItemId( "recent-folders" ) ) ) {
		UIMenuSubMenu* uiMenuSubMenu = static_cast<UIMenuSubMenu*>( node );
		UIMenu* menu = uiMenuSubMenu->getSubMenu();
		uiMenuSubMenu->setEnabled( !mRecentFolders.empty() );
		menu->removeAll();
		menu->removeEventsOfType( Event::OnItemClicked );
		if ( mRecentFolders.empty() )
			return;
		for ( const auto& file : mRecentFolders )
			menu->add( file );
		menu->addSeparator();
		menu->add( i18n( "clear_menu", "Clear Menu" ) )->setId( "clear-menu" );
		menu->addEventListener( Event::OnItemClicked, [&]( const Event* event ) {
			if ( !event->getNode()->isType( UI_TYPE_MENUITEM ) )
				return;
			const std::string& id = event->getNode()->asType<UIMenuItem>()->getId();
			if ( id != "clear-menu" ) {
				const String& txt = event->getNode()->asType<UIMenuItem>()->getText();
				loadFolder( txt );
			} else {
				mRecentFolders.clear();
				updateRecentFolders();
			}
		} );
	}
}

void App::showSidePanel( bool show ) {
	if ( show == mSidePanel->isVisible() )
		return;

	if ( show ) {
		mSidePanel->setVisible( true );
		mSidePanel->setParent( mProjectSplitter );
		mProjectSplitter->swap();
	} else {
		mSidePanel->setVisible( false );
		mSidePanel->setParent( mUISceneNode->getRoot() );
	}
}

void App::switchSidePanel() {
	mConfig.ui.showSidePanel = !mConfig.ui.showSidePanel;
	mWindowMenu->getItemId( "show-side-panel" )
		->asType<UIMenuCheckBox>()
		->setActive( mConfig.ui.showSidePanel );
	showSidePanel( mConfig.ui.showSidePanel );
}

void App::panelPosition( const PanelPosition& panelPosition ) {
	mConfig.ui.panelPosition = panelPosition;

	if ( !mSidePanel->isVisible() )
		return;

	if ( ( panelPosition == PanelPosition::Right &&
		   mProjectSplitter->getFirstWidget() == mSidePanel ) ||
		 ( panelPosition == PanelPosition::Left &&
		   mProjectSplitter->getFirstWidget() != mSidePanel ) ) {
		mProjectSplitter->swap( true );
	}
}

void App::setUIColorScheme( const ColorSchemePreference& colorScheme ) {
	if ( colorScheme == mUIColorScheme )
		return;
	mUIColorScheme = mConfig.ui.colorScheme = colorScheme;
	mUISceneNode->setColorSchemePreference( colorScheme );
}

UIMenu* App::createHelpMenu() {
	UIPopUpMenu* helpMenu = UIPopUpMenu::New();
	helpMenu->add( i18n( "ecode_source", "ecode source code..." ), findIcon( "github" ) )
		->setId( "ecode_source" );
	helpMenu->add( i18n( "about_ecode", "About ecode..." ) )->setId( "about_ecode" );
	helpMenu->addEventListener( Event::OnItemClicked, [&]( const Event* event ) {
		const std::string& id = event->getNode()->getId();

		if ( "ecode_source" == id ) {
			Engine::instance()->openURI( "https://github.com/SpartanJ/ecode" );
		} else if ( "about_ecode" == id ) {
			String msg( ecode::Version::getVersionName() + " (codename: \"" +
						ecode::Version::getCodename() + "\")" );
			UIMessageBox* msgBox = UIMessageBox::New( UIMessageBox::OK, msg );
			msgBox->setTitle( i18n( "about_ecode", "About ecode..." ) );
			msgBox->show();
		}
	} );
	return helpMenu;
}

UIMenu* App::createWindowMenu() {
	mWindowMenu = UIPopUpMenu::New();
	UIPopUpMenu* colorsMenu = UIPopUpMenu::New();
	colorsMenu
		->addRadioButton( i18n( "light", "Light" ), mUIColorScheme == ColorSchemePreference::Light )
		->setId( "light" );
	colorsMenu
		->addRadioButton( i18n( "dark", "Dark" ), mUIColorScheme == ColorSchemePreference::Dark )
		->setId( "dark" );
	colorsMenu->addEventListener( Event::OnItemClicked, [&]( const Event* event ) {
		if ( !event->getNode()->isType( UI_TYPE_MENUITEM ) )
			return;
		UIMenuItem* item = event->getNode()->asType<UIMenuItem>();
		setUIColorScheme( item->getId() == "light" ? ColorSchemePreference::Light
												   : ColorSchemePreference::Dark );
	} );
	mWindowMenu->addSubMenu( i18n( "ui_prefes_color_scheme", "UI Prefers Color Scheme" ),
							 findIcon( "color-scheme" ), colorsMenu );
	mWindowMenu
		->add( i18n( "ui_scale_factor", "UI Scale Factor (Pixel Density)" ),
			   findIcon( "pixel-density" ) )
		->setId( "ui-scale-factor" );
	mWindowMenu->add( i18n( "ui_font_size", "UI Font Size" ), findIcon( "font-size" ) )
		->setId( "ui-font-size" );
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
	mWindowMenu->addSeparator();
	mWindowMenu
		->addCheckBox( i18n( "fullscreen_mode", "Full Screen Mode" ), false,
					   getKeybind( "fullscreen-toggle" ) )
		->setId( "fullscreen-toggle" );
	mWindowMenu
		->addCheckBox( i18n( "show_side_panel", "Show Side Panel" ), mConfig.ui.showSidePanel,
					   getKeybind( "switch-side-panel" ) )
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
	mWindowMenu->addEventListener( Event::OnItemClicked, [&]( const Event* event ) {
		if ( !event->getNode()->isType( UI_TYPE_MENUITEM ) )
			return;
		UIMenuItem* item = event->getNode()->asType<UIMenuItem>();
		if ( item->getId() == "show-side-panel" ) {
			mConfig.ui.showSidePanel = item->asType<UIMenuCheckBox>()->isActive();
			showSidePanel( mConfig.ui.showSidePanel );
		} else if ( item->getId() == "ui-scale-factor" ) {
			UIMessageBox* msgBox = UIMessageBox::New(
				UIMessageBox::INPUT,
				i18n( "set_ui_scale_factor",
					  "Set the UI scale factor (pixel density):\nMinimum value is "
					  "1, and maximum 6. Requires restart." ) );
			msgBox->setTitle( mWindowTitle );
			msgBox->getTextInput()->setText(
				String::format( "%.2f", mConfig.window.pixelDensity ) );
			msgBox->setCloseShortcut( { KEY_ESCAPE, 0 } );
			msgBox->showWhenReady();
			msgBox->addEventListener( Event::MsgBoxConfirmClick, [&, msgBox]( const Event* ) {
				msgBox->closeWindow();
				Float val;
				if ( String::fromString( val, msgBox->getTextInput()->getText() ) && val >= 1 &&
					 val <= 6 ) {
					if ( mConfig.window.pixelDensity != val ) {
						mConfig.window.pixelDensity = val;
						UIMessageBox* msg = UIMessageBox::New(
							UIMessageBox::OK,
							i18n( "new_ui_scale_factor", "New UI scale factor assigned.\nPlease "
														 "restart the application." ) );
						msg->show();
						setFocusEditorOnClose( msg );
					} else if ( mSplitter && mSplitter->getCurWidget() ) {
						mSplitter->getCurWidget()->setFocus();
					}
				} else {
					UIMessageBox* msg = UIMessageBox::New( UIMessageBox::OK, "Invalid value!" );
					msg->show();
					setFocusEditorOnClose( msg );
				}
			} );
		} else if ( item->getId() == "editor-font-size" ) {
			UIMessageBox* msgBox = UIMessageBox::New(
				UIMessageBox::INPUT, i18n( "set_editor_font_size", "Set the editor font size:" ) );
			msgBox->setTitle( mWindowTitle );
			msgBox->getTextInput()->setText( mConfig.editor.fontSize.toString() );
			msgBox->setCloseShortcut( { KEY_ESCAPE, 0 } );
			msgBox->showWhenReady();
			msgBox->addEventListener( Event::MsgBoxConfirmClick, [&, msgBox]( const Event* ) {
				mConfig.term.fontSize = StyleSheetLength( msgBox->getTextInput()->getText() );
				mSplitter->forEachEditor( [&]( UICodeEditor* editor ) {
					editor->setFontSize( mConfig.editor.fontSize.asDp( 0, Sizef(), mDisplayDPI ) );
				} );
			} );
			setFocusEditorOnClose( msgBox );
		} else if ( item->getId() == "terminal-font-size" ) {
			UIMessageBox* msgBox =
				UIMessageBox::New( UIMessageBox::INPUT, i18n( "set_terminal_font_size",
															  "Set the terminal font size:" ) );
			msgBox->setTitle( mWindowTitle );
			msgBox->getTextInput()->setText( mConfig.term.fontSize.toString() );
			msgBox->setCloseShortcut( { KEY_ESCAPE, 0 } );
			msgBox->showWhenReady();
			msgBox->addEventListener( Event::MsgBoxConfirmClick, [&, msgBox]( const Event* ) {
				mConfig.term.fontSize = StyleSheetLength( msgBox->getTextInput()->getText() );
				mSplitter->forEachWidget( [&]( UIWidget* widget ) {
					if ( widget && widget->isType( UI_TYPE_TERMINAL ) )
						widget->asType<UITerminal>()->setFontSize(
							mConfig.term.fontSize.asPixels( 0, Sizef(), mDisplayDPI ) );
				} );
			} );
			setFocusEditorOnClose( msgBox );
		} else if ( item->getId() == "ui-font-size" ) {
			UIMessageBox* msgBox = UIMessageBox::New(
				UIMessageBox::INPUT, i18n( "set_ui_font_size", "Set the UI font size:" ) );
			msgBox->setTitle( mWindowTitle );
			msgBox->getTextInput()->setText( mConfig.ui.fontSize.toString() );
			msgBox->setCloseShortcut( { KEY_ESCAPE, 0 } );
			msgBox->showWhenReady();
			msgBox->addEventListener( Event::MsgBoxConfirmClick, [&, msgBox]( const Event* ) {
				mConfig.ui.fontSize = StyleSheetLength( msgBox->getTextInput()->getText() );
				Float fontSize = mConfig.ui.fontSize.asDp( 0, Sizef(), mDisplayDPI );
				UIThemeManager* manager = mUISceneNode->getUIThemeManager();
				manager->setDefaultFontSize( fontSize );
				manager->getDefaultTheme()->setDefaultFontSize( fontSize );
				mUISceneNode->forEachNode( [&]( Node* node ) {
					if ( node->isType( UI_TYPE_TEXTVIEW ) ) {
						UITextView* textView = node->asType<UITextView>();
						if ( !textView->getUIStyle()->hasProperty( PropertyId::FontSize ) ) {
							textView->setFontSize( mConfig.ui.fontSize.asDp(
								node->getParent()->getPixelsSize().getWidth(), Sizef(),
								mUISceneNode->getDPI() ) );
						}
					}
				} );
				msgBox->closeWindow();
			} );
			setFocusEditorOnClose( msgBox );
		} else if ( item->getId() == "serif-font" ) {
			openFontDialog( mConfig.ui.serifFont, false );
		} else if ( item->getId() == "monospace-font" ) {
			openFontDialog( mConfig.ui.monospaceFont, true );
		} else if ( item->getId() == "terminal-font" ) {
			openFontDialog( mConfig.ui.terminalFont, false );
		} else if ( "zoom-in" == item->getId() ) {
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

UIMenu* App::createViewMenu() {
	mViewMenu = UIPopUpMenu::New();
	mViewMenu->addCheckBox( i18n( "show_line_numbers", "Show Line Numbers" ) )
		->setActive( mConfig.editor.showLineNumbers )
		->setId( "show-line-numbers" );
	mViewMenu->addCheckBox( i18n( "show_white_spaces", "Show White Spaces" ) )
		->setActive( mConfig.editor.showWhiteSpaces )
		->setId( "show-white-spaces" );
	mViewMenu->addCheckBox( i18n( "show_doc_info", "Show Document Info" ) )
		->setActive( mConfig.editor.showDocInfo )
		->setId( "show-doc-info" );
	mViewMenu->addCheckBox( i18n( "show_minimap", "Show Minimap" ) )
		->setActive( mConfig.editor.minimap )
		->setId( "show-minimap" );
	mViewMenu->addCheckBox( i18n( "highlight_matching_brackets", "Highlight Matching Bracket" ) )
		->setActive( mConfig.editor.highlightMatchingBracket )
		->setId( "highlight-matching-brackets" );
	mViewMenu->addCheckBox( i18n( "highlight_current_line", "Highlight Current Line" ) )
		->setActive( mConfig.editor.highlightCurrentLine )
		->setId( "highlight-current-line" );
	mViewMenu->addCheckBox( i18n( "highlight_selection_match", "Highlight Selection Match" ) )
		->setActive( mConfig.editor.highlightSelectionMatch )
		->setId( "highlight-selection-match" );
	mViewMenu->addCheckBox( i18n( "enable-vertical-scrollbar", "Enable Vertical ScrollBar" ) )
		->setActive( mConfig.editor.verticalScrollbar )
		->setId( "enable-vertical-scrollbar" );
	mViewMenu->addCheckBox( i18n( "enable_horizontal_scrollbar", "Enable Horizontal ScrollBar" ) )
		->setActive( mConfig.editor.horizontalScrollbar )
		->setId( "enable-horizontal-scrollbar" );
	mViewMenu->addCheckBox( i18n( "enable_color_preview", "Enable Color Preview" ) )
		->setActive( mConfig.editor.colorPreview )
		->setTooltipText( i18n( "enable_color_preview_tooltip",
								"Enables a quick preview of a color when the mouse\n"
								"is hover a word that represents a color." ) )
		->setId( "enable-color-preview" );
	mViewMenu->addCheckBox( i18n( "enable_color_picker", "Enable Color Picker" ) )
		->setActive( mConfig.editor.colorPickerSelection )
		->setTooltipText( i18n( "enable_color_picker_tooltip",
								"Enables the color picker tool when a double click selection\n"
								"is done over a word representing a color." ) )
		->setId( "enable-color-picker" );
	mViewMenu->addCheckBox( i18n( "enable_autocomplete", "Enable Auto Complete" ) )
		->setActive( mConfig.editor.autoComplete )
		->setTooltipText(
			i18n( "enable_autocomplete_tooltip",
				  "Auto complete shows the completion popup as you type, so you can fill\n"
				  "in long words by typing only a few characters." ) )
		->setId( "enable-autocomplete" );
	mViewMenu->addCheckBox( i18n( "enable_linter", "Enable Linter" ) )
		->setActive( mConfig.editor.linter )
		->setTooltipText(
			i18n( "enable_linter_tooltip",
				  "Use static code analysis tool used to flag programming errors, bugs,\n"
				  "stylistic errors, and suspicious constructs." ) )
		->setId( "enable-linter" );
	mViewMenu->addCheckBox( i18n( "enable_code_formatter", "Enable Code Formatter" ) )
		->setActive( mConfig.editor.formatter )
		->setTooltipText( i18n( "enable_code_formatter_tooltip",
								"Enables the code formatter/prettifier plugin." ) )
		->setId( "enable-code-formatter" );
	mViewMenu->addCheckBox( i18n( "hide_tabbar_on_single_tab", "Hide tabbar on single tab" ) )
		->setActive( mConfig.editor.hideTabBarOnSingleTab )
		->setTooltipText(
			i18n( "hide_tabbar_on_single_tab_tooltip",
				  "Hides the tabbar if there's only one element in the tab widget." ) )
		->setId( "hide-tabbar-on-single-tab" );
	mViewMenu
		->addCheckBox( i18n( "treeview_single_click_nav", "Single Click Navigation in Tree View" ) )
		->setActive( mConfig.editor.singleClickTreeNavigation )
		->setTooltipText( i18n(
			"treeview_single_click_nav_tooltip",
			"Uses single click to open files and expand subfolders in\nthe directory tree." ) )
		->setId( "treeview-single-click-nav" );
	mViewMenu->addCheckBox( i18n( "sync_project_tree", "Synchronize project tree with editor" ) )
		->setActive( mConfig.editor.syncProjectTreeWithEditor )
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
			mConfig.editor.showLineNumbers = item->asType<UIMenuCheckBox>()->isActive();
			mSplitter->forEachEditor( [&]( UICodeEditor* editor ) {
				editor->setShowLineNumber( mConfig.editor.showLineNumbers );
			} );
		} else if ( item->getId() == "show-white-spaces" ) {
			mConfig.editor.showWhiteSpaces = item->asType<UIMenuCheckBox>()->isActive();
			mSplitter->forEachEditor( [&]( UICodeEditor* editor ) {
				editor->setShowWhitespaces( mConfig.editor.showWhiteSpaces );
			} );
		} else if ( item->getId() == "show-doc-info" ) {
			mConfig.editor.showDocInfo = item->asType<UIMenuCheckBox>()->isActive();
			if ( mDocInfo )
				mDocInfo->setVisible( mConfig.editor.showDocInfo );
			if ( mSplitter->curEditorExistsAndFocused() )
				updateDocInfo( mSplitter->getCurEditor()->getDocument() );
		} else if ( item->getId() == "show-minimap" ) {
			mConfig.editor.minimap = item->asType<UIMenuCheckBox>()->isActive();
			mSplitter->forEachEditor(
				[&]( UICodeEditor* editor ) { editor->showMinimap( mConfig.editor.minimap ); } );
		} else if ( item->getId() == "highlight-matching-brackets" ) {
			mConfig.editor.highlightMatchingBracket = item->asType<UIMenuCheckBox>()->isActive();
			mSplitter->forEachEditor( [&]( UICodeEditor* editor ) {
				editor->setHighlightMatchingBracket( mConfig.editor.highlightMatchingBracket );
			} );
		} else if ( item->getId() == "highlight-current-line" ) {
			mConfig.editor.highlightCurrentLine = item->asType<UIMenuCheckBox>()->isActive();
			mSplitter->forEachEditor( [&]( UICodeEditor* editor ) {
				editor->setHighlightCurrentLine( mConfig.editor.highlightCurrentLine );
			} );
		} else if ( item->getId() == "highlight-selection-match" ) {
			mConfig.editor.highlightSelectionMatch = item->asType<UIMenuCheckBox>()->isActive();
			mSplitter->forEachEditor( [&]( UICodeEditor* editor ) {
				editor->setHighlightSelectionMatch( mConfig.editor.highlightSelectionMatch );
			} );
		} else if ( item->getId() == "enable-vertical-scrollbar" ) {
			mConfig.editor.verticalScrollbar = item->asType<UIMenuCheckBox>()->isActive();
			mSplitter->forEachEditor( [&]( UICodeEditor* editor ) {
				editor->setVerticalScrollBarEnabled( mConfig.editor.verticalScrollbar );
			} );
		} else if ( item->getId() == "enable-horizontal-scrollbar" ) {
			mConfig.editor.horizontalScrollbar = item->asType<UIMenuCheckBox>()->isActive();
			mSplitter->forEachEditor( [&]( UICodeEditor* editor ) {
				editor->setHorizontalScrollBarEnabled( mConfig.editor.horizontalScrollbar );
			} );
		} else if ( item->getId() == "enable-color-preview" ) {
			mConfig.editor.colorPreview = item->asType<UIMenuCheckBox>()->isActive();
			mSplitter->forEachEditor( [&]( UICodeEditor* editor ) {
				editor->setEnableColorPickerOnSelection( mConfig.editor.colorPreview );
			} );
		} else if ( item->getId() == "enable-color-picker" ) {
			mConfig.editor.colorPickerSelection = item->asType<UIMenuCheckBox>()->isActive();
			mSplitter->forEachEditor( [&]( UICodeEditor* editor ) {
				editor->setEnableColorPickerOnSelection( mConfig.editor.colorPickerSelection );
			} );
		} else if ( item->getId() == "enable-autocomplete" ) {
			setAutoComplete( item->asType<UIMenuCheckBox>()->isActive() );
		} else if ( item->getId() == "enable-linter" ) {
			setLinter( item->asType<UIMenuCheckBox>()->isActive() );
		} else if ( item->getId() == "enable-code-formatter" ) {
			setFormatter( item->asType<UIMenuCheckBox>()->isActive() );
		} else if ( item->getId() == "hide-tabbar-on-single-tab" ) {
			mConfig.editor.hideTabBarOnSingleTab = item->asType<UIMenuCheckBox>()->isActive();
			mSplitter->setHideTabBarOnSingleTab( mConfig.editor.hideTabBarOnSingleTab );
		} else if ( item->getId() == "treeview-single-click-nav" ) {
			mConfig.editor.singleClickTreeNavigation = item->asType<UIMenuCheckBox>()->isActive();
			mProjectTreeView->setSingleClickNavigation( mConfig.editor.singleClickTreeNavigation );
		} else if ( item->getId() == "sync-project-tree" ) {
			mConfig.editor.syncProjectTreeWithEditor = item->asType<UIMenuCheckBox>()->isActive();
		} else {
			String text = String( event->getNode()->asType<UIMenuItem>()->getId() ).toLower();
			String::replaceAll( text, " ", "-" );
			String::replaceAll( text, "/", "-" );
			runCommand( text );
		}
	} );
	return mViewMenu;
}

void App::setFocusEditorOnClose( UIMessageBox* msgBox ) {
	msgBox->addEventListener( Event::OnClose, [&]( const Event* ) {
		if ( mSplitter && mSplitter->getCurWidget() )
			mSplitter->getCurWidget()->setFocus();
	} );
}

Drawable* App::findIcon( const std::string& name ) {
	UIIcon* icon = mUISceneNode->findIcon( name );
	if ( icon )
		return icon->getSize( mMenuIconSize );
	return nullptr;
}

String App::i18n( const std::string& key, const String& def ) {
	return mUISceneNode->getTranslatorStringFromKey( key, def );
}

UIMenu* App::createEditMenu() {
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

static std::vector<std::pair<String::StringBaseType, String::StringBaseType>>
makeAutoClosePairs( const std::string& strPairs ) {
	auto curPairs = String::split( strPairs, ',' );
	std::vector<std::pair<String::StringBaseType, String::StringBaseType>> pairs;
	for ( auto pair : curPairs ) {
		if ( pair.size() == 2 )
			pairs.emplace_back( std::make_pair( pair[0], pair[1] ) );
	}
	return pairs;
}

UIMenu* App::createDocumentMenu() {
	auto shouldCloseCb = []( UIMenuItem* ) -> bool { return false; };

	mDocMenu = UIPopUpMenu::New();

	// **** CURRENT DOCUMENT ****
	mDocMenu->add( i18n( "current_document", "Current Document" ) )
		->setTextAlign( UI_HALIGN_CENTER );

	mDocMenu
		->addCheckBox(
			i18n( "auto_detect_indent_type_and_width", "Auto Detect Indent Type & Width" ),
			mConfig.doc.autoDetectIndentType )
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
	lineEndingsMenu->addRadioButton( "Windows (CR/LF)", mConfig.doc.windowsLineEndings )
		->setId( "windows" );
	lineEndingsMenu->addRadioButton( "Unix (LF)", !mConfig.doc.windowsLineEndings )
		->setId( "unix" );
	mDocMenu->addSubMenu( i18n( "line_endings", "Line Endings" ), nullptr, lineEndingsMenu )
		->setId( "line_endings_cur" );
	lineEndingsMenu->addEventListener( Event::OnItemClicked, [&]( const Event* event ) {
		bool winLe = event->getNode()->asType<UIRadioButton>()->getId() == "windows";
		if ( mSplitter->curEditorExistsAndFocused() ) {
			mSplitter->getCurEditor()->getDocument().setLineEnding(
				winLe ? TextDocument::LineEnding::CRLF : TextDocument::LineEnding::LF );
			updateDocInfo( mSplitter->getCurEditor()->getDocument() );
		}
	} );

	mDocMenu->addCheckBox( i18n( "read_only", "Read Only" ) )->setId( "read_only" );

	mDocMenu
		->addCheckBox( i18n( "trim_trailing_whitespaces", "Trim Trailing Whitespaces" ),
					   mConfig.doc.trimTrailingWhitespaces )
		->setId( "trim_whitespaces_cur" );

	mDocMenu
		->addCheckBox( i18n( "force_new_line_at_end_of_file", "Force New Line at End of File" ),
					   mConfig.doc.forceNewLineAtEndOfFile )
		->setId( "force_nl_cur" );

	mDocMenu
		->addCheckBox( i18n( "write_unicode_bom", "Write Unicode BOM" ),
					   mConfig.doc.writeUnicodeBOM )
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
			mConfig.doc.autoDetectIndentType )
		->setId( "auto_indent" );

	UIPopUpMenu* tabTypeMenuGlobal = UIPopUpMenu::New();
	tabTypeMenuGlobal->addRadioButton( i18n( "tabs", "Tabs" ) )
		->setActive( !mConfig.doc.indentSpaces )
		->setId( "tabs" );
	tabTypeMenuGlobal->addRadioButton( i18n( "spaces", "Spaces" ) )
		->setActive( mConfig.doc.indentSpaces )
		->setId( "spaces" );
	globalMenu
		->addSubMenu( i18n( "indentation_type", "Indentation Type" ), nullptr, tabTypeMenuGlobal )
		->setId( "indent_type" );
	tabTypeMenuGlobal->addEventListener( Event::OnItemClicked, [&]( const Event* event ) {
		const String& text = event->getNode()->asType<UIMenuRadioButton>()->getId();
		mConfig.doc.indentSpaces = text != "tabs";
	} );

	UIPopUpMenu* indentWidthMenuGlobal = UIPopUpMenu::New();
	for ( int w = 2; w <= 12; w++ )
		indentWidthMenuGlobal->addRadioButton( String::toString( w ), mConfig.doc.indentWidth == w )
			->setId( String::format( "indent_width_%d", w ) )
			->setData( w );
	globalMenu->addSubMenu( i18n( "indent_width", "Indent Width" ), nullptr, indentWidthMenuGlobal )
		->setId( "indent_width" );
	indentWidthMenuGlobal->addEventListener( Event::OnItemClicked, [&]( const Event* event ) {
		int width = event->getNode()->getData();
		mConfig.doc.indentWidth = width;
	} );

	UIPopUpMenu* tabWidthMenuGlobal = UIPopUpMenu::New();
	for ( int w = 2; w <= 12; w++ )
		tabWidthMenuGlobal->addRadioButton( String::toString( w ), mConfig.doc.tabWidth == w )
			->setId( String::format( "tab_width_%d", w ) )
			->setData( w );
	globalMenu->addSubMenu( i18n( "tab_width", "Tab Width" ), nullptr, tabWidthMenuGlobal )
		->setId( "tab_width_cur" );
	tabWidthMenuGlobal->addEventListener( Event::OnItemClicked, [&]( const Event* event ) {
		int width = event->getNode()->getData();
		mConfig.doc.tabWidth = width;
	} );

	UIPopUpMenu* lineEndingsGlobalMenu = UIPopUpMenu::New();
	lineEndingsGlobalMenu->addRadioButton( "Windows (CR/LF)", mConfig.doc.windowsLineEndings )
		->setId( "windows" );
	lineEndingsGlobalMenu->addRadioButton( "Unix (LF)", !mConfig.doc.windowsLineEndings )
		->setId( "unix" );
	globalMenu->addSubMenu( i18n( "line_endings", "Line Endings" ), nullptr, lineEndingsGlobalMenu )
		->setId( "line_endings" );
	lineEndingsGlobalMenu->addEventListener( Event::OnItemClicked, [&]( const Event* event ) {
		bool winLe = event->getNode()->asType<UIRadioButton>()->getId() == "windows";
		mConfig.doc.windowsLineEndings = winLe;
	} );

	UIPopUpMenu* bracketsMenu = UIPopUpMenu::New();
	globalMenu->addSubMenu( i18n( "auto_close_brackets_and_tags", "Auto-Close Brackets & Tags" ),
							nullptr, bracketsMenu );
	auto& closeBrackets = mConfig.editor.autoCloseBrackets;
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
					   mConfig.editor.autoCloseXMLTags )
		->setOnShouldCloseCb( shouldCloseCb )
		->setId( "XML" );
	bracketsMenu->addEventListener( Event::OnItemClicked, [&]( const Event* event ) {
		std::string id = event->getNode()->getId();
		if ( event->getNode()->isType( UI_TYPE_MENUCHECKBOX ) ) {
			UIMenuCheckBox* item = event->getNode()->asType<UIMenuCheckBox>();
			if ( item->getId() == "XML" ) {
				mConfig.editor.autoCloseXMLTags = item->isActive();
				mSplitter->forEachEditor( [&]( UICodeEditor* editor ) {
					editor->setAutoCloseXMLTags( mConfig.editor.autoCloseXMLTags );
				} );
				return;
			}
			auto curPairs = String::split( mConfig.editor.autoCloseBrackets, ',' );
			auto found = std::find( curPairs.begin(), curPairs.end(), id );
			if ( item->isActive() ) {
				if ( found == curPairs.end() )
					curPairs.push_back( id );
			} else if ( found != curPairs.end() ) {
				curPairs.erase( found );
			}
			mConfig.editor.autoCloseBrackets = String::join( curPairs, ',' );
			auto pairs = makeAutoClosePairs( mConfig.editor.autoCloseBrackets );
			mSplitter->forEachEditor( [&, pairs]( UICodeEditor* editor ) {
				editor->getDocument().setAutoCloseBrackets( !pairs.empty() );
				editor->getDocument().setAutoCloseBracketsPairs( pairs );
			} );
		}
	} );

	globalMenu
		->addCheckBox( i18n( "trim_trailing_whitespaces", "Trim Trailing Whitespaces" ),
					   mConfig.doc.trimTrailingWhitespaces )
		->setId( "trim_whitespaces" );

	globalMenu
		->addCheckBox( i18n( "force_new_line_at_end_of_file", "Force New Line at End of File" ),
					   mConfig.doc.forceNewLineAtEndOfFile )
		->setId( "force_nl" );

	globalMenu
		->addCheckBox( i18n( "write_unicode_bom", "Write Unicode BOM" ),
					   mConfig.doc.writeUnicodeBOM )
		->setId( "write_bom" );

	globalMenu->addSeparator();

	globalMenu->add( i18n( "line_breaking_column", "Line Breaking Column" ) )
		->setId( "line_breaking_column" );

	globalMenu->addEventListener( Event::OnItemClicked, [&]( const Event* event ) {
		if ( !mSplitter->curEditorExistsAndFocused() ||
			 event->getNode()->isType( UI_TYPE_MENU_SEPARATOR ) ||
			 event->getNode()->isType( UI_TYPE_MENUSUBMENU ) )
			return;
		const String& id = event->getNode()->getId();

		if ( event->getNode()->isType( UI_TYPE_MENUCHECKBOX ) ) {
			UIMenuCheckBox* item = event->getNode()->asType<UIMenuCheckBox>();
			if ( "trim_whitespaces" == id ) {
				mConfig.doc.trimTrailingWhitespaces = item->isActive();
			} else if ( "force_nl" == id ) {
				mConfig.doc.forceNewLineAtEndOfFile = item->isActive();
			} else if ( "write_bom" == id ) {
				mConfig.doc.writeUnicodeBOM = item->isActive();
			} else if ( "auto_indent" == id ) {
				mConfig.doc.autoDetectIndentType = item->isActive();
			}
		} else if ( "line_breaking_column" == id ) {
			UIMessageBox* msgBox = UIMessageBox::New(
				UIMessageBox::INPUT, i18n( "set_line_breaking_column",
										   "Set Line Breaking Column:\nSet 0 to disable it.\n" )
										 .unescape() );
			msgBox->setTitle( mWindowTitle );
			msgBox->setCloseShortcut( { KEY_ESCAPE, 0 } );
			msgBox->getTextInput()->setAllowOnlyNumbers( true, false );
			msgBox->getTextInput()->setText( String::toString( mConfig.doc.lineBreakingColumn ) );
			msgBox->showWhenReady();
			msgBox->addEventListener( Event::MsgBoxConfirmClick, [&, msgBox]( const Event* ) {
				int val;
				if ( String::fromString( val, msgBox->getTextInput()->getText() ) && val >= 0 ) {
					mConfig.doc.lineBreakingColumn = val;
					mSplitter->forEachEditor(
						[val]( UICodeEditor* editor ) { editor->setLineBreakingColumn( val ); } );
					msgBox->closeWindow();
				}
			} );
			setFocusEditorOnClose( msgBox );
		}
	} );

	mDocMenu->addSeparator();

	// **** PROJECT SETTINGS ****
	mProjectDocConfig = mConfig.doc;
	mProjectMenu = UIPopUpMenu::New();
	mProjectMenu
		->addCheckBox( i18n( "use_global_settings", "Use Global Settings" ),
					   mProjectDocConfig.useGlobalSettings )
		->setOnShouldCloseCb( shouldCloseCb )
		->setId( "use_global_settings" );

	mProjectMenu
		->addCheckBox(
			i18n( "auto_detect_indent_type_and_width", "Auto Detect Indent Type & Width" ),
			mConfig.doc.autoDetectIndentType )
		->setId( "auto_indent" )
		->setEnabled( !mProjectDocConfig.useGlobalSettings );

	UIPopUpMenu* tabTypeMenuProject = UIPopUpMenu::New();
	tabTypeMenuProject->addRadioButton( i18n( "tabs", "Tabs" ) )
		->setActive( !mProjectDocConfig.doc.indentSpaces )
		->setId( "tabs" );
	tabTypeMenuProject->addRadioButton( i18n( "spaces", "Spaces" ) )
		->setActive( mProjectDocConfig.doc.indentSpaces )
		->setId( "spaces" );
	mProjectMenu
		->addSubMenu( i18n( "indentation_type", "Indentation Type" ), nullptr, tabTypeMenuProject )
		->setId( "indent_type" )
		->setEnabled( !mProjectDocConfig.useGlobalSettings );
	tabTypeMenuProject->addEventListener( Event::OnItemClicked, [&]( const Event* event ) {
		const String& text = event->getNode()->asType<UIMenuRadioButton>()->getId();
		mProjectDocConfig.doc.indentSpaces = text != "tabs";
	} );

	UIPopUpMenu* indentWidthMenuProject = UIPopUpMenu::New();
	for ( int w = 2; w <= 12; w++ )
		indentWidthMenuProject
			->addRadioButton( String::toString( w ), mProjectDocConfig.doc.indentWidth == w )
			->setId( String::format( "indent_width_%d", w ) )
			->setData( w );
	mProjectMenu
		->addSubMenu( i18n( "indent_width", "Indent Width" ), nullptr, indentWidthMenuProject )
		->setId( "indent_width" )
		->setEnabled( !mProjectDocConfig.useGlobalSettings );
	indentWidthMenuProject->addEventListener( Event::OnItemClicked, [&]( const Event* event ) {
		int width = event->getNode()->getData();
		mProjectDocConfig.doc.indentWidth = width;
	} );

	UIPopUpMenu* tabWidthMenuProject = UIPopUpMenu::New();
	for ( int w = 2; w <= 12; w++ )
		tabWidthMenuProject
			->addRadioButton( String::toString( w ), mProjectDocConfig.doc.tabWidth == w )
			->setId( String::format( "tab_width_%d", w ) )
			->setData( w );
	mProjectMenu->addSubMenu( i18n( "tab_width", "Tab Width" ), nullptr, tabWidthMenuProject )
		->setId( "tab_width" )
		->setEnabled( !mProjectDocConfig.useGlobalSettings );
	tabWidthMenuProject->addEventListener( Event::OnItemClicked, [&]( const Event* event ) {
		int width = event->getNode()->getData();
		mProjectDocConfig.doc.tabWidth = width;
	} );

	UIPopUpMenu* lineEndingsProjectMenu = UIPopUpMenu::New();
	lineEndingsProjectMenu
		->addRadioButton( "Windows (CR/LF)", mProjectDocConfig.doc.windowsLineEndings )
		->setId( "windows" );
	lineEndingsProjectMenu->addRadioButton( "Unix (LF)", !mProjectDocConfig.doc.windowsLineEndings )
		->setId( "unix" );
	mProjectMenu
		->addSubMenu( i18n( "line_endings", "Line Endings" ), nullptr, lineEndingsProjectMenu )
		->setId( "line_endings" )
		->setEnabled( !mProjectDocConfig.useGlobalSettings );
	lineEndingsProjectMenu->addEventListener( Event::OnItemClicked, [&]( const Event* event ) {
		bool winLe = event->getNode()->asType<UIRadioButton>()->getId() == "windows";
		mProjectDocConfig.doc.windowsLineEndings = winLe;
	} );

	mProjectMenu
		->addCheckBox( i18n( "trim_trailing_whitespaces", "Trim Trailing Whitespaces" ),
					   mConfig.doc.trimTrailingWhitespaces )
		->setId( "trim_whitespaces" )
		->setEnabled( !mProjectDocConfig.useGlobalSettings );

	mProjectMenu
		->addCheckBox( i18n( "force_new_line_at_end_of_file", "Force New Line at End of File" ),
					   mConfig.doc.forceNewLineAtEndOfFile )
		->setId( "force_nl" )
		->setEnabled( !mProjectDocConfig.useGlobalSettings );

	mProjectMenu
		->addCheckBox( i18n( "write_unicode_bom", "Write Unicode BOM" ),
					   mConfig.doc.writeUnicodeBOM )
		->setId( "write_bom" )
		->setEnabled( !mProjectDocConfig.useGlobalSettings );

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
				mProjectDocConfig.useGlobalSettings = item->isActive();
				updateProjectSettingsMenu();
			} else if ( "trim_whitespaces" == id ) {
				mProjectDocConfig.doc.trimTrailingWhitespaces = item->isActive();
			} else if ( "force_nl" == id ) {
				mProjectDocConfig.doc.forceNewLineAtEndOfFile = item->isActive();
			} else if ( "write_bom" == id ) {
				mProjectDocConfig.doc.writeUnicodeBOM = item->isActive();
			} else if ( "auto_indent" == id ) {
				mProjectDocConfig.doc.autoDetectIndentType = item->isActive();
			}
		} else if ( "line_breaking_column" == id ) {
			UIMessageBox* msgBox = UIMessageBox::New(
				UIMessageBox::INPUT, i18n( "set_line_breaking_column",
										   "Set Line Breaking Column:\nSet 0 to disable it.\n" )
										 .unescape() );
			msgBox->setTitle( mWindowTitle );
			msgBox->setCloseShortcut( { KEY_ESCAPE, 0 } );
			msgBox->getTextInput()->setAllowOnlyNumbers( true, false );
			msgBox->getTextInput()->setText(
				String::toString( mProjectDocConfig.doc.lineBreakingColumn ) );
			msgBox->showWhenReady();
			msgBox->addEventListener( Event::MsgBoxConfirmClick, [&, msgBox]( const Event* ) {
				int val;
				if ( String::fromString( val, msgBox->getTextInput()->getText() ) && val >= 0 ) {
					mProjectDocConfig.doc.lineBreakingColumn = val;
					mSplitter->forEachEditor(
						[val]( UICodeEditor* editor ) { editor->setLineBreakingColumn( val ); } );
					msgBox->closeWindow();
				}
			} );
			setFocusEditorOnClose( msgBox );
		}
	} );

	mDocMenu
		->addSubMenu( i18n( "folder_project_settings", "Folder/Project Settings" ),
					  findIcon( "folder-user" ), mProjectMenu )
		->setId( "project_settings" );

	return mDocMenu;
}

void App::updateProjectSettingsMenu() {
	mDocMenu->getItemId( "project_settings" )->setEnabled( !mCurrentProject.empty() );

	for ( size_t i = 0; i < mProjectMenu->getCount(); i++ ) {
		mProjectMenu->getItem( i )->setEnabled( !mCurrentProject.empty() &&
												!mProjectDocConfig.useGlobalSettings );
	}

	mSplitter->forEachEditor( [&]( UICodeEditor* editor ) {
		editor->setLineBreakingColumn( !mCurrentProject.empty() &&
											   !mProjectDocConfig.useGlobalSettings
										   ? mProjectDocConfig.doc.lineBreakingColumn
										   : mConfig.doc.lineBreakingColumn );
	} );

	mProjectMenu->getItemId( "trim_whitespaces" )
		->asType<UIMenuCheckBox>()
		->setActive( mProjectDocConfig.doc.trimTrailingWhitespaces );

	mProjectMenu->getItemId( "force_nl" )
		->asType<UIMenuCheckBox>()
		->setActive( mProjectDocConfig.doc.forceNewLineAtEndOfFile );

	mProjectMenu->getItemId( "write_bom" )
		->asType<UIMenuCheckBox>()
		->setActive( mProjectDocConfig.doc.writeUnicodeBOM );

	mProjectMenu->getItemId( "auto_indent" )
		->asType<UIMenuCheckBox>()
		->setActive( mProjectDocConfig.doc.autoDetectIndentType );

	auto* curIndent =
		mProjectMenu->find( "indent_width" )
			->asType<UIMenuSubMenu>()
			->getSubMenu()
			->find( String::format( "indent_width_%d", mProjectDocConfig.doc.indentWidth ) );

	if ( curIndent )
		curIndent->asType<UIMenuRadioButton>()->setActive( true );

	mProjectMenu->find( "indent_type" )
		->asType<UIMenuSubMenu>()
		->getSubMenu()
		->find( !mProjectDocConfig.doc.indentSpaces ? "tabs" : "spaces" )
		->asType<UIMenuRadioButton>()
		->setActive( true );

	mProjectMenu->find( "tab_width" )
		->asType<UIMenuSubMenu>()
		->getSubMenu()
		->find( String::format( "tab_width_%d", mProjectDocConfig.doc.tabWidth ) )
		->asType<UIMenuRadioButton>()
		->setActive( true );

	mProjectMenu->find( "line_endings" )
		->asType<UIMenuSubMenu>()
		->getSubMenu()
		->find( mProjectDocConfig.doc.windowsLineEndings ? "windows" : "unix" )
		->asType<UIMenuRadioButton>()
		->setActive( true );

	mProjectMenu->getItemId( "use_global_settings" )
		->setEnabled( true )
		->asType<UIMenuCheckBox>()
		->setActive( mProjectDocConfig.useGlobalSettings );
}

void App::updateDocumentMenu() {
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
		->find( doc.getLineEnding() == TextDocument::LineEnding::CRLF ? "windows" : "unix" )
		->asType<UIMenuRadioButton>()
		->setActive( true );

	mDocMenu->find( "read_only" )
		->asType<UIMenuCheckBox>()
		->setActive( mSplitter->getCurEditor()->isLocked() );
}

static void updateKeybindings( IniFile& ini, const std::string& group, Input* input,
							   std::unordered_map<std::string, std::string>& keybindings,
							   const std::unordered_map<std::string, std::string>& defKeybindings,
							   bool forceRebind = false ) {
	KeyBindings bindings( input );
	bool added = false;

	if ( ini.findKey( group ) != IniFile::noID ) {
		keybindings = ini.getKeyUnorderedMap( group );
	} else {
		for ( const auto& it : defKeybindings )
			ini.setValue( group, it.first, it.second );
		added = true;
	}
	std::unordered_map<std::string, std::string> invertedKeybindings;
	for ( const auto& key : keybindings )
		invertedKeybindings[key.second] = key.first;

	if ( defKeybindings.size() != keybindings.size() || forceRebind ) {
		for ( const auto& key : defKeybindings ) {
			auto foundCmd = invertedKeybindings.find( key.second );
			auto& shortcutStr = key.first;
			if ( foundCmd == invertedKeybindings.end() &&
				 keybindings.find( shortcutStr ) == keybindings.end() ) {
				keybindings[shortcutStr] = key.second;
				invertedKeybindings[key.second] = shortcutStr;
				ini.setValue( group, shortcutStr, key.second );
				added = true;
			} else if ( foundCmd == invertedKeybindings.end() ) {
				// Override the shortcut if the command that holds that
				// shortcut does not exists anymore
				auto kb = keybindings.find( shortcutStr );
				if ( kb != keybindings.end() ) {
					bool found = false;
					for ( const auto& val : defKeybindings )
						if ( val.second == kb->second )
							found = true;
					if ( !found ) {
						keybindings[shortcutStr] = key.second;
						invertedKeybindings[key.second] = shortcutStr;
						ini.setValue( group, shortcutStr, key.second );
						added = true;
					}
				}
			}
		}
	}
	if ( added )
		ini.writeFile();
}

static void updateKeybindings( IniFile& ini, const std::string& group, Input* input,
							   std::unordered_map<std::string, std::string>& keybindings,
							   std::unordered_map<std::string, std::string>& invertedKeybindings,
							   const std::map<KeyBindings::Shortcut, std::string>& defKeybindings,
							   bool forceRebind = false ) {
	KeyBindings bindings( input );
	bool added = false;

	if ( ini.findKey( group ) != IniFile::noID ) {
		keybindings = ini.getKeyUnorderedMap( group );
	} else {
		for ( const auto& it : defKeybindings )
			ini.setValue( group, bindings.getShortcutString( it.first ), it.second );
		added = true;
	}
	for ( const auto& key : keybindings )
		invertedKeybindings[key.second] = key.first;

	if ( defKeybindings.size() != keybindings.size() || forceRebind ) {
		for ( const auto& key : defKeybindings ) {
			auto foundCmd = invertedKeybindings.find( key.second );
			auto shortcutStr = bindings.getShortcutString( key.first );
			if ( foundCmd == invertedKeybindings.end() &&
				 keybindings.find( shortcutStr ) == keybindings.end() ) {
				keybindings[shortcutStr] = key.second;
				invertedKeybindings[key.second] = shortcutStr;
				ini.setValue( group, shortcutStr, key.second );
				added = true;
			} else if ( foundCmd == invertedKeybindings.end() ) {
				// Override the shortcut if the command that holds that
				// shortcut does not exists anymore
				auto kb = keybindings.find( shortcutStr );
				if ( kb != keybindings.end() ) {
					bool found = false;
					for ( const auto& val : defKeybindings )
						if ( val.second == kb->second )
							found = true;
					if ( !found ) {
						keybindings[shortcutStr] = key.second;
						invertedKeybindings[key.second] = shortcutStr;
						ini.setValue( group, shortcutStr, key.second );
						added = true;
					}
				}
			}
		}
	}
	if ( added )
		ini.writeFile();
}

void App::loadKeybindings() {
	if ( mKeybindings.empty() ) {
		KeyBindings bindings( mWindow->getInput() );
		IniFile ini( mKeybindingsPath );

		std::string defMod = ini.getValue( "modifier", "mod", "" );
		if ( defMod.empty() ) {
			defMod = KeyMod::getDefaultModifierString();
			ini.setValue( "modifier", "mod", defMod );
			ini.writeFile();
		}

		bool forceRebind = false;
		auto version = ini.getValueU( "version", "version", 0 );
		if ( version != ecode::Version::getVersionNum() ) {
			ini.setValueU( "version", "version", ecode::Version::getVersionNum() );
			ini.writeFile();
			forceRebind = true;
		}

		Uint32 defModKeyCode = KeyMod::getKeyMod( defMod );
		if ( KEYMOD_NONE != defModKeyCode )
			KeyMod::setDefaultModifier( defModKeyCode );

		updateKeybindings( ini, "editor", mWindow->getInput(), mKeybindings, mKeybindingsInvert,
						   getDefaultKeybindings(), forceRebind );

		updateKeybindings( ini, "global_search", mWindow->getInput(), mGlobalSearchKeybindings,
						   GlobalSearchController::getDefaultKeybindings(), forceRebind );

		updateKeybindings( ini, "document_search", mWindow->getInput(), mDocumentSearchKeybindings,
						   DocSearchController::getDefaultKeybindings(), forceRebind );
	}
}

void App::onDocumentStateChanged( UICodeEditor*, TextDocument& ) {
	updateEditorState();
}

void App::onDocumentSelectionChange( UICodeEditor* editor, TextDocument& doc ) {
	onDocumentModified( editor, doc );
}

void App::onDocumentCursorPosChange( UICodeEditor*, TextDocument& doc ) {
	updateDocInfo( doc );
}

void App::updateDocInfo( TextDocument& doc ) {
	if ( mConfig.editor.showDocInfo && mDocInfoText && mSplitter->curEditorExistsAndFocused() ) {
		if ( mDocInfo )
			mDocInfo->setVisible( true );
		mDocInfoText->setText( String::format(
			"%s: %lld / %lu  %s: %lld    %s", i18n( "line_abbr", "line" ).toUtf8().c_str(),
			doc.getSelection().start().line() + 1, doc.linesCount(),
			i18n( "col_abbr", "col" ).toUtf8().c_str(),
			mSplitter->getCurEditor()->getCurrentColumnCount(),
			doc.getLineEnding() == TextDocument::LineEnding::LF ? "LF" : "CRLF" ) );
	}
}

void App::syncProjectTreeWithEditor( UICodeEditor* editor ) {
	if ( mConfig.editor.syncProjectTreeWithEditor && editor != nullptr &&
		 editor->getDocument().hasFilepath() ) {
		std::string path = editor->getDocument().getFilePath();
		if ( path.size() >= mCurrentProject.size() ) {
			path = path.substr( mCurrentProject.size() );
			mProjectTreeView->setFocusOnSelection( false );
			mProjectTreeView->selectRowWithPath( path );
			mProjectTreeView->setFocusOnSelection( true );
		}
	}
}

void App::onWidgetFocusChange( UIWidget* widget ) {
	if ( mConfig.editor.showDocInfo && mDocInfoText )
		mDocInfo->setVisible( widget && widget->isType( UI_TYPE_CODEEDITOR ) );

	updateDocumentMenu();
	if ( widget && !widget->isType( UI_TYPE_CODEEDITOR ) ) {
		if ( widget->isType( UI_TYPE_TERMINAL ) )
			setAppTitle( widget->asType<UITerminal>()->getTitle() );
	}
}

void App::onCodeEditorFocusChange( UICodeEditor* editor ) {
	updateDocInfo( editor->getDocument() );
	mDocSearchController->onCodeEditorFocusChange( editor );
	syncProjectTreeWithEditor( editor );
}

void App::onColorSchemeChanged( const std::string& ) {
	updateColorSchemeMenu();
	mGlobalSearchController->updateColorScheme( mSplitter->getCurrentColorScheme() );
	mNotificationCenter->addNotification(
		String::format( i18n( "color_scheme_set", "Color scheme: %s" ).toUtf8().c_str(),
						mSplitter->getCurrentColorScheme().getName().c_str() ) );
}

void App::onDocumentLoaded( UICodeEditor* editor, const std::string& path ) {
	updateEditorTitle( editor );
	if ( mSplitter->curEditorExistsAndFocused() && editor == mSplitter->getCurEditor() )
		updateCurrentFileType();
	mSplitter->removeUnusedTab( mSplitter->tabWidgetFromEditor( editor ) );
	auto found = std::find( mRecentFiles.begin(), mRecentFiles.end(), path );
	if ( found != mRecentFiles.end() )
		mRecentFiles.erase( found );
	mRecentFiles.insert( mRecentFiles.begin(), path );
	if ( mRecentFiles.size() > 10 )
		mRecentFiles.resize( 10 );
	updateRecentFiles();
	if ( mSplitter->curEditorExistsAndFocused() && mSplitter->getCurEditor() == editor ) {
		updateDocumentMenu();
		updateDocInfo( editor->getDocument() );
	}

	if ( !path.empty() ) {
		UITab* tab = reinterpret_cast<UITab*>( editor->getData() );
		tab->setTooltipText( path );
	}

	TextDocument& doc = editor->getDocument();

	if ( mFileWatcher && doc.hasFilepath() &&
		 ( !mDirTree || !mDirTree->isDirInTree( doc.getFileInfo().getFilepath() ) ) ) {
		std::string dir( FileSystem::fileRemoveFileName( doc.getFileInfo().getFilepath() ) );
		Lock l( mWatchesLock );
		mFilesFolderWatches[dir] = mFileWatcher->addWatch( dir, mFileSystemListener );
	}
}

const CodeEditorConfig& App::getCodeEditorConfig() const {
	return mConfig.editor;
}

std::map<KeyBindings::Shortcut, std::string> App::getTerminalKeybindings() {
	return {
		{ { KEY_T, KeyMod::getDefaultModifier() | KEYMOD_SHIFT }, "create-new-terminal" },
		{ { KEY_E, KEYMOD_CTRL | KEYMOD_LALT | KEYMOD_SHIFT },
		  UITerminal::getExclusiveModeToggleCommandName() },
		{ { KEY_S, KEYMOD_LALT | KEYMOD_CTRL }, "terminal-rename" },
	};
}

std::map<KeyBindings::Shortcut, std::string> App::getDefaultKeybindings() {
	auto bindings = UICodeEditorSplitter::getDefaultKeybindings();
	auto local = getLocalKeybindings();
	auto app = getTerminalKeybindings();
	local.insert( bindings.begin(), bindings.end() );
	local.insert( app.begin(), app.end() );
	return local;
}

std::map<KeyBindings::Shortcut, std::string> App::getLocalKeybindings() {
	return {
		{ { KEY_RETURN, KEYMOD_LALT }, "fullscreen-toggle" },
		{ { KEY_F3, KEYMOD_NONE }, "repeat-find" },
		{ { KEY_F3, KEYMOD_SHIFT }, "find-prev" },
		{ { KEY_F12, KEYMOD_NONE }, "console-toggle" },
		{ { KEY_F, KeyMod::getDefaultModifier() }, "find-replace" },
		{ { KEY_Q, KeyMod::getDefaultModifier() | KEYMOD_SHIFT }, "close-app" },
		{ { KEY_O, KeyMod::getDefaultModifier() }, "open-file" },
		{ { KEY_W, KeyMod::getDefaultModifier() | KEYMOD_SHIFT }, "download-file-web" },
		{ { KEY_O, KeyMod::getDefaultModifier() | KEYMOD_SHIFT }, "open-folder" },
		{ { KEY_F6, KEYMOD_NONE }, "debug-draw-highlight-toggle" },
		{ { KEY_F7, KEYMOD_NONE }, "debug-draw-boxes-toggle" },
		{ { KEY_F8, KEYMOD_NONE }, "debug-draw-debug-data" },
		{ { KEY_F11, KEYMOD_NONE }, "debug-widget-tree-view" },
		{ { KEY_K, KeyMod::getDefaultModifier() }, "open-locatebar" },
		{ { KEY_F, KeyMod::getDefaultModifier() | KEYMOD_SHIFT }, "open-global-search" },
		{ { KEY_L, KeyMod::getDefaultModifier() }, "go-to-line" },
		{ { KEY_M, KeyMod::getDefaultModifier() }, "menu-toggle" },
		{ { KEY_S, KeyMod::getDefaultModifier() | KEYMOD_SHIFT }, "save-all" },
		{ { KEY_F9, KEYMOD_LALT }, "switch-side-panel" },
		{ { KEY_F, KEYMOD_LALT }, "format-doc" },
		{ { KEY_J, KEYMOD_CTRL | KEYMOD_LALT | KEYMOD_SHIFT }, "terminal-split-left" },
		{ { KEY_L, KEYMOD_CTRL | KEYMOD_LALT | KEYMOD_SHIFT }, "terminal-split-right" },
		{ { KEY_I, KEYMOD_CTRL | KEYMOD_LALT | KEYMOD_SHIFT }, "terminal-split-top" },
		{ { KEY_K, KEYMOD_CTRL | KEYMOD_LALT | KEYMOD_SHIFT }, "terminal-split-bottom" },
		{ { KEY_S, KEYMOD_CTRL | KEYMOD_LALT | KEYMOD_SHIFT }, "terminal-split-swap" },
	};
}

std::vector<std::string> App::getUnlockedCommands() {
	return { "fullscreen-toggle",	 "open-file",			"open-folder",
			 "console-toggle",		 "close-app",			"open-locatebar",
			 "open-global-search",	 "menu-toggle",			"switch-side-panel",
			 "download-file-web",	 "create-new-terminal", "terminal-split-left",
			 "terminal-split-right", "terminal-split-top",	"terminal-split-bottom",
			 "terminal-split-swap" };
}

bool App::isUnlockedCommand( const std::string& command ) {
	auto cmds = getUnlockedCommands();
	return std::find( cmds.begin(), cmds.end(), command ) != cmds.end();
}

void App::closeEditors() {
	mConfig.saveProject( mCurrentProject, mSplitter, mConfigPath, mProjectDocConfig );
	std::vector<UICodeEditor*> editors = mSplitter->getAllEditors();
	while ( !editors.empty() ) {
		UICodeEditor* editor = editors[0];
		UITabWidget* tabWidget = mSplitter->tabWidgetFromEditor( editor );
		tabWidget->removeTab( (UITab*)editor->getData(), true, true );
		editors = mSplitter->getAllEditors();
		if ( editors.size() == 1 && editors[0]->getDocument().isEmpty() )
			break;
	};

	mCurrentProject = "";
	mDirTree = nullptr;
	if ( mFileSystemListener )
		mFileSystemListener->setDirTree( mDirTree );

	mProjectDocConfig = ProjectDocumentConfig( mConfig.doc );
	updateProjectSettingsMenu();
	if ( !mSplitter->getTabWidgets().empty() && mSplitter->getTabWidgets()[0]->getTabCount() == 0 )
		mSplitter->createCodeEditorInTabWidget( mSplitter->getTabWidgets()[0] );
}

void App::closeFolder() {
	if ( mCurrentProject.empty() )
		return;

	if ( mSplitter->isAnyEditorDirty() ) {
		UIMessageBox* msgBox = UIMessageBox::New(
			UIMessageBox::OK_CANCEL,
			i18n( "confirm_close_folder",
				  "Do you really want to close the folder?\nSome files haven't been saved." ) );
		msgBox->addEventListener( Event::MsgBoxConfirmClick,
								  [&]( const Event* ) { closeEditors(); } );
		msgBox->addEventListener( Event::OnClose, [&]( const Event* ) { msgBox = nullptr; } );
		msgBox->setTitle( i18n( "close_folder_question", "Close Folder?" ) );
		msgBox->center();
		msgBox->showWhenReady();
	} else {
		closeEditors();
	}
}

void App::createDocAlert( UICodeEditor* editor ) {
	UILinearLayout* docAlert = editor->findByClass<UILinearLayout>( "doc_alert" );

	if ( docAlert )
		return;

	// TODO: Add text translations
	const std::string& msg = R"xml(
	<hbox class="doc_alert" layout_width="wrap_content" layout_height="wrap_content" layout_gravity="top|right">
		<TextView id="doc_alert_text" layout_width="wrap_content" layout_height="wrap_content" margin-right="24dp"
			text="The file on the disk is more recent that the current buffer.&#xA;Do you want to reload it?"
		/>
		<PushButton id="file_reload" layout_width="wrap_content" layout_height="18dp" text="Reload" margin-right="4dp"
					tooltip="Reload the file from disk. Unsaved changes will be lost." />
		<PushButton id="file_overwrite" layout_width="wrap_content" layout_height="18dp" text="Overwrite" margin-right="4dp"
					tooltip="Writes the local changes on disk, overwriting the disk changes" />
		<PushButton id="file_ignore" layout_width="wrap_content" layout_height="18dp" text="Ignore"
					tooltip="Ignores the changes on disk without any action." />
	</hbox>
	)xml";
	docAlert = static_cast<UILinearLayout*>( mUISceneNode->loadLayoutFromString( msg, editor ) );

	editor->enableReportSizeChangeToChilds();

	docAlert->find( "file_reload" )
		->addEventListener( Event::MouseClick, [editor, docAlert]( const Event* event ) {
			const MouseEvent* mouseEvent = static_cast<const MouseEvent*>( event );
			if ( mouseEvent->getFlags() & EE_BUTTON_LMASK ) {
				editor->getDocument().reload();
				editor->disableReportSizeChangeToChilds();
				docAlert->close();
				editor->setFocus();
			}
		} );

	docAlert->find( "file_overwrite" )
		->addEventListener( Event::MouseClick, [editor, docAlert]( const Event* event ) {
			const MouseEvent* mouseEvent = static_cast<const MouseEvent*>( event );
			if ( mouseEvent->getFlags() & EE_BUTTON_LMASK ) {
				editor->getDocument().save();
				editor->disableReportSizeChangeToChilds();
				docAlert->close();
				editor->setFocus();
			}
		} );

	docAlert->find( "file_ignore" )
		->addEventListener( Event::MouseClick, [docAlert, editor]( const Event* event ) {
			const MouseEvent* mouseEvent = static_cast<const MouseEvent*>( event );
			if ( mouseEvent->getFlags() & EE_BUTTON_LMASK ) {
				editor->disableReportSizeChangeToChilds();
				docAlert->close();
				editor->setFocus();
			}
		} );
}

void App::loadFileFromPath(
	const std::string& path, bool inNewTab, UICodeEditor* codeEditor,
	std::function<void( UICodeEditor* codeEditor, const std::string& path )> onLoaded ) {
	if ( Image::isImageExtension( path ) && Image::isImage( path ) ) {
		UIImage* imageView = mImageLayout->findByType<UIImage>( UI_TYPE_IMAGE );
		UILoader* loaderView = mImageLayout->findByType<UILoader>( UI_TYPE_LOADER );
		if ( imageView ) {
			mImageLayout->setEnabled( true )->setVisible( true );
			loaderView->setVisible( true );
#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN || defined( __EMSCRIPTEN_PTHREADS__ )
			mThreadPool->run(
				[&, imageView, loaderView, path]() {
#endif
					Texture* image = TextureFactory::instance()->getTexture(
						TextureFactory::instance()->loadFromFile( path ) );
					if ( mImageLayout->isVisible() ) {
						imageView->runOnMainThread( [imageView, loaderView, image]() {
							imageView->setDrawable( image, true );
							loaderView->setVisible( false );
						} );
					} else {
						TextureFactory::instance()->remove( image );
						imageView->setDrawable( nullptr );
						loaderView->setVisible( false );
					}
#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN || defined( __EMSCRIPTEN_PTHREADS__ )
				},
				[]() {} );
#endif
		}
	} else {
		if ( inNewTab ) {
			mSplitter->loadAsyncFileFromPathInNewTab( path, mThreadPool, onLoaded );
		} else {
			mSplitter->loadAsyncFileFromPath( path, mThreadPool, codeEditor, onLoaded );
		}
	}
}

void App::hideGlobalSearchBar() {
	mGlobalSearchController->hideGlobalSearchBar();
}

void App::hideSearchBar() {
	mDocSearchController->hideSearchBar();
}

void App::hideLocateBar() {
	mFileLocator->hideLocateBar();
}

bool App::isDirTreeReady() const {
	return mDirTreeReady && mDirTree != nullptr;
}

NotificationCenter* App::getNotificationCenter() const {
	return mNotificationCenter.get();
}

void App::fullscreenToggle() {
	mWindow->toggleFullscreen();
	mWindowMenu->find( "fullscreen-toggle" )
		->asType<UIMenuCheckBox>()
		->setActive( !mWindow->isWindowed() );
}

void App::loadTerminalColorSchemes() {
	auto colorSchemes =
		TerminalColorScheme::loadFromFile( mResPath + "colorschemes/terminalcolorschemes.conf" );
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
	mTerminalCurrentColorScheme = mConfig.term.colorScheme;
	if ( mTerminalColorSchemes.find( mTerminalCurrentColorScheme ) == mTerminalColorSchemes.end() )
		mTerminalCurrentColorScheme = mTerminalColorSchemes.begin()->first;
}

UITerminal* App::createNewTerminal( const std::string& title, UITabWidget* inTabWidget,
									const std::string& workingDir ) {
#if EE_PLATFORM == EE_PLATFORM_EMSCRIPTEN
	UIMessageBox* msgBox = UIMessageBox::New(
		UIMessageBox::OK,
		i18n( "feature_not_supported_in_emscripten",
			  "This feature is only supported in the desktop version of ecode." ) );
	msgBox->showWhenReady();
	return nullptr;
#else
	if ( mTerminalColorSchemes.empty() )
		loadTerminalColorSchemes();
	UITabWidget* tabWidget = nullptr;

	if ( !inTabWidget ) {
		UIWidget* curWidget = mSplitter->getCurWidget();
		if ( !curWidget )
			return nullptr;
		tabWidget = mSplitter->tabWidgetFromWidget( curWidget );
	} else {
		tabWidget = inTabWidget;
	}

	if ( !tabWidget ) {
		if ( !mSplitter->getTabWidgets().empty() ) {
			tabWidget = mSplitter->getTabWidgets()[0];
		} else {
			return nullptr;
		}
	}
	UITerminal* term = UITerminal::New(
		mFontMonoNerdFont ? mFontMonoNerdFont : mFontMono,
		mConfig.term.fontSize.asPixels( 0, Sizef(), mDisplayDPI ), Sizef( 16, 16 ), "", {},
		!workingDir.empty() ? workingDir : ( !mCurrentProject.empty() ? mCurrentProject : "" ) );
	auto ret = mSplitter->createWidgetInTabWidget(
		tabWidget, term, title.empty() ? i18n( "shell", "Shell" ).toUtf8() : title, true );
	mSplitter->removeUnusedTab( tabWidget );
	ret.first->setIcon( findIcon( "filetype-bash" ) );
	term->setTitle( title );
	auto csIt = mTerminalColorSchemes.find( mTerminalCurrentColorScheme );
	term->setColorScheme( csIt != mTerminalColorSchemes.end()
							  ? mTerminalColorSchemes.at( mTerminalCurrentColorScheme )
							  : TerminalColorScheme::getDefault() );
	term->addEventListener( Event::OnTitleChange, [&]( const Event* event ) {
		if ( event->getNode() != mSplitter->getCurWidget() )
			return;
		setAppTitle( event->getNode()->asType<UITerminal>()->getTitle() );
	} );
	term->addKeyBinds( getLocalKeybindings() );
	term->addKeyBinds( UICodeEditorSplitter::getLocalDefaultKeybindings() );
	term->addKeyBinds( getTerminalKeybindings() );
	// Remove the keybinds that are problematic for a terminal
	term->getKeyBindings().removeCommandsKeybind(
		{ "open-file", "download-file-web", "open-folder", "debug-draw-highlight-toggle",
		  "debug-draw-boxes-toggle", "debug-draw-debug-data", "debug-widget-tree-view",
		  "open-locatebar", "open-global-search", "menu-toggle", "console-toggle", "go-to-line" } );
	term->setCommand( "switch-side-panel", [&] { switchSidePanel(); } );
	term->setCommand( "fullscreen-toggle", [&]() { fullscreenToggle(); } );
	for ( int i = 1; i <= 10; i++ )
		term->setCommand( String::format( "switch-to-tab-%d", i ),
						  [&, i] { mSplitter->switchToTab( i - 1 ); } );
	term->setCommand( "switch-to-first-tab", [&] {
		UITabWidget* tabWidget = mSplitter->tabWidgetFromWidget( mSplitter->getCurWidget() );
		if ( tabWidget && tabWidget->getTabCount() ) {
			mSplitter->switchToTab( 0 );
		}
	} );
	term->setCommand( "switch-to-last-tab", [&] {
		UITabWidget* tabWidget = mSplitter->tabWidgetFromWidget( mSplitter->getCurWidget() );
		if ( tabWidget && tabWidget->getTabCount() ) {
			mSplitter->switchToTab( tabWidget->getTabCount() - 1 );
		}
	} );
	term->setCommand( "switch-to-previous-split",
					  [&] { mSplitter->switchPreviousSplit( mSplitter->getCurWidget() ); } );
	term->setCommand( "switch-to-next-split",
					  [&] { mSplitter->switchNextSplit( mSplitter->getCurWidget() ); } );
	term->setCommand( "close-tab", [&] { mSplitter->tryTabClose( mSplitter->getCurWidget() ); } );
	term->setCommand( "create-new", [&] {
		auto d = mSplitter->createCodeEditorInTabWidget(
			mSplitter->tabWidgetFromWidget( mSplitter->getCurWidget() ) );
		d.first->getTabWidget()->setTabSelected( d.first );
	} );
	term->setCommand( "next-tab", [&] {
		UITabWidget* tabWidget = mSplitter->tabWidgetFromWidget( mSplitter->getCurWidget() );
		if ( tabWidget && tabWidget->getTabCount() > 1 ) {
			UITab* tab = (UITab*)mSplitter->getCurWidget()->getData();
			Uint32 tabIndex = tabWidget->getTabIndex( tab );
			mSplitter->switchToTab( ( tabIndex + 1 ) % tabWidget->getTabCount() );
		}
	} );
	term->setCommand( "previous-tab", [&] {
		UITabWidget* tabWidget = mSplitter->tabWidgetFromWidget( mSplitter->getCurWidget() );
		if ( tabWidget && tabWidget->getTabCount() > 1 ) {
			UITab* tab = (UITab*)mSplitter->getCurWidget()->getData();
			Uint32 tabIndex = tabWidget->getTabIndex( tab );
			Int32 newTabIndex = (Int32)tabIndex - 1;
			mSplitter->switchToTab( newTabIndex < 0 ? tabWidget->getTabCount() - newTabIndex
													: newTabIndex );
		}
	} );
	term->setCommand( "split-right", [&] {
		mSplitter->split( UICodeEditorSplitter::SplitDirection::Right, mSplitter->getCurWidget(),
						  mSplitter->curEditorExistsAndFocused() );
	} );
	term->setCommand( "split-bottom", [&] {
		mSplitter->split( UICodeEditorSplitter::SplitDirection::Bottom, mSplitter->getCurWidget(),
						  mSplitter->curEditorExistsAndFocused() );
	} );
	term->setCommand( "split-left", [&] {
		mSplitter->split( UICodeEditorSplitter::SplitDirection::Left, mSplitter->getCurWidget(),
						  mSplitter->curEditorExistsAndFocused() );
	} );
	term->setCommand( "split-top", [&] {
		mSplitter->split( UICodeEditorSplitter::SplitDirection::Top, mSplitter->getCurWidget(),
						  mSplitter->curEditorExistsAndFocused() );
	} );
	term->setCommand( "split-swap", [&] {
		if ( UISplitter* splitter = mSplitter->splitterFromWidget( mSplitter->getCurWidget() ) )
			splitter->swap();
	} );
	term->setCommand( "terminal-split-right", [&, term] {
		mSplitter->split( UICodeEditorSplitter::SplitDirection::Right, mSplitter->getCurWidget(),
						  false );
		term->execute( "create-new-terminal" );
	} );
	term->setCommand( "terminal-split-bottom", [&, term] {
		mSplitter->split( UICodeEditorSplitter::SplitDirection::Bottom, mSplitter->getCurWidget(),
						  false );
		term->execute( "create-new-terminal" );
	} );
	term->setCommand( "terminal-split-left", [&, term] {
		mSplitter->split( UICodeEditorSplitter::SplitDirection::Left, mSplitter->getCurWidget(),
						  false );
		term->execute( "create-new-terminal" );
	} );
	term->setCommand( "terminal-split-top", [&, term] {
		mSplitter->split( UICodeEditorSplitter::SplitDirection::Top, mSplitter->getCurWidget(),
						  false );
		term->execute( "create-new-terminal" );
	} );
	term->setCommand( "split-swap", [&] {
		if ( UISplitter* splitter = mSplitter->splitterFromWidget( mSplitter->getCurWidget() ) )
			splitter->swap();
	} );
	term->setCommand( UITerminal::getExclusiveModeToggleCommandName(),
					  [term] { term->setExclusiveMode( !term->getExclusiveMode() ); } );
	term->setCommand( "create-new-terminal", [&] { createNewTerminal(); } );
	term->setCommand( "terminal-rename", [&, term] {
		UIMessageBox* msgBox = UIMessageBox::New(
			UIMessageBox::INPUT, i18n( "new_terminal_name", "New terminal name:" ) );
		msgBox->setTitle( "ecode" );
		msgBox->getTextInput()->setHint( i18n( "any_name", "Any name..." ) );
		msgBox->setCloseShortcut( { KEY_ESCAPE, KEYMOD_NONE } );
		msgBox->showWhenReady();
		msgBox->addEventListener( Event::MsgBoxConfirmClick, [&, msgBox, term]( const Event* ) {
			std::string title( msgBox->getTextInput()->getText().toUtf8() );
			term->setTitle( title );
			msgBox->close();
			term->setFocus();
		} );
	} );
	term->setCommand( "move-panel-left", [&] { panelPosition( PanelPosition::Left ); } );
	term->setCommand( "move-panel-right", [&] { panelPosition( PanelPosition::Right ); } );
	term->setCommand( "close-app", [&] { closeApp(); } );
	term->setCommand( "open-file", [&] { openFileDialog(); } );
	term->setCommand( "open-folder", [&] { openFolderDialog(); } );
	term->setCommand( "console-toggle", [&] {
		mConsole->toggle();
		bool lock = mConsole->isActive();
		mSplitter->forEachEditor( [lock]( UICodeEditor* editor ) { editor->setLocked( lock ); } );
	} );
	term->setCommand( "menu-toggle", [&] { toggleSettingsMenu(); } );
	term->setCommand( "open-global-search", [&] { mGlobalSearchController->showGlobalSearch(); } );
	term->setCommand( "open-locatebar", [&] { mFileLocator->showLocateBar(); } );
	term->setCommand( "download-file-web", [&] { downloadFileWebDialog(); } );

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
	// debug-draw-highlight-toggle
	// debug-draw-boxes-toggle
	// debug-draw-debug-data
	// debug-widget-tree-view
	term->setFocus();
	return term;
#endif
}

void App::applyTerminalColorScheme( const TerminalColorScheme& colorScheme ) {
	mSplitter->forEachWidget( [colorScheme]( UIWidget* widget ) {
		if ( widget->isType( UI_TYPE_TERMINAL ) )
			widget->asType<UITerminal>()->setColorScheme( colorScheme );
	} );
}

void App::setTerminalColorScheme( const std::string& name ) {
	if ( name != mTerminalCurrentColorScheme ) {
		mTerminalCurrentColorScheme = name;
		mConfig.term.colorScheme = name;
		auto csIt = mTerminalColorSchemes.find( mTerminalCurrentColorScheme );
		applyTerminalColorScheme( csIt != mTerminalColorSchemes.end()
									  ? mTerminalColorSchemes.at( mTerminalCurrentColorScheme )
									  : TerminalColorScheme::getDefault() );
		mNotificationCenter->addNotification( String::format(
			i18n( "terminal_color_scheme_set", "Terminal color scheme: %s" ).toUtf8().c_str(),
			mTerminalCurrentColorScheme.c_str() ) );
	}
}

void App::onCodeEditorCreated( UICodeEditor* editor, TextDocument& doc ) {
	const CodeEditorConfig& config = mConfig.editor;
	const DocumentConfig& docc = !mCurrentProject.empty() && !mProjectDocConfig.useGlobalSettings
									 ? mProjectDocConfig.doc
									 : mConfig.doc;
	editor->setFontSize( config.fontSize.asDp( 0, Sizef(), mUISceneNode->getDPI() ) );
	editor->setEnableColorPickerOnSelection( true );
	editor->setColorScheme( mSplitter->getCurrentColorScheme() );
	editor->setShowLineNumber( config.showLineNumbers );
	editor->setShowWhitespaces( config.showWhiteSpaces );
	editor->setHighlightMatchingBracket( config.highlightMatchingBracket );
	editor->setVerticalScrollBarEnabled( config.verticalScrollbar );
	editor->setHorizontalScrollBarEnabled( config.horizontalScrollbar );
	editor->setHighlightCurrentLine( config.highlightCurrentLine );
	editor->setTabWidth( docc.tabWidth );
	editor->setLineBreakingColumn( docc.lineBreakingColumn );
	editor->setHighlightSelectionMatch( config.highlightSelectionMatch );
	editor->setEnableColorPickerOnSelection( config.colorPickerSelection );
	editor->setColorPreview( config.colorPreview );
	editor->setFont( mFontMono );
	editor->setMenuIconSize( mMenuIconSize );
	editor->setAutoCloseXMLTags( config.autoCloseXMLTags );
	doc.setAutoCloseBrackets( !mConfig.editor.autoCloseBrackets.empty() );
	doc.setAutoCloseBracketsPairs( makeAutoClosePairs( mConfig.editor.autoCloseBrackets ) );
	doc.setLineEnding( docc.windowsLineEndings ? TextDocument::LineEnding::CRLF
											   : TextDocument::LineEnding::LF );
	doc.setTrimTrailingWhitespaces( docc.trimTrailingWhitespaces );
	doc.setForceNewLineAtEndOfFile( docc.forceNewLineAtEndOfFile );
	doc.setIndentType( docc.indentSpaces ? TextDocument::IndentType::IndentSpaces
										 : TextDocument::IndentType::IndentTabs );
	doc.setIndentWidth( docc.indentWidth );
	doc.setAutoDetectIndentType( docc.autoDetectIndentType );
	doc.setBOM( docc.writeUnicodeBOM );

	editor->addKeyBinds( getLocalKeybindings() );
	editor->addUnlockedCommands( getUnlockedCommands() );
	doc.setCommand( "save-doc", [&] { saveDoc(); } );
	doc.setCommand( "save-as-doc", [&] {
		if ( mSplitter->curEditorExistsAndFocused() )
			saveFileDialog( mSplitter->getCurEditor() );
	} );
	doc.setCommand( "save-all", [&] { saveAll(); } );
	doc.setCommand( "find-replace", [&] { mDocSearchController->showFindView(); } );
	doc.setCommand( "open-global-search", [&] { mGlobalSearchController->showGlobalSearch(); } );
	doc.setCommand( "open-locatebar", [&] { mFileLocator->showLocateBar(); } );
	doc.setCommand( "repeat-find", [&] {
		mDocSearchController->findNextText( mDocSearchController->getSearchState() );
	} );
	doc.setCommand( "find-prev", [&] {
		mDocSearchController->findPrevText( mDocSearchController->getSearchState() );
	} );
	doc.setCommand( "close-folder", [&] { closeFolder(); } );
	doc.setCommand( "close-app", [&] { closeApp(); } );
	doc.setCommand( "fullscreen-toggle", [&]() { fullscreenToggle(); } );
	doc.setCommand( "open-file", [&] { openFileDialog(); } );
	doc.setCommand( "open-folder", [&] { openFolderDialog(); } );
	doc.setCommand( "console-toggle", [&] {
		mConsole->toggle();
		bool lock = mConsole->isActive();
		mSplitter->forEachEditor( [lock]( UICodeEditor* editor ) { editor->setLocked( lock ); } );
	} );
	doc.setCommand( "lock", [&] {
		if ( mSplitter->curEditorExistsAndFocused() ) {
			mSplitter->getCurEditor()->setLocked( true );
			updateDocumentMenu();
		}
	} );
	doc.setCommand( "unlock", [&] {
		if ( mSplitter->curEditorExistsAndFocused() ) {
			mSplitter->getCurEditor()->setLocked( false );
			updateDocumentMenu();
		}
	} );
	doc.setCommand( "lock-toggle", [&] {
		if ( mSplitter->curEditorExistsAndFocused() ) {
			mSplitter->getCurEditor()->setLocked( !mSplitter->getCurEditor()->isLocked() );
			updateDocumentMenu();
		}
	} );
	doc.setCommand( "keybindings", [&] { loadFileFromPath( mKeybindingsPath ); } );
	doc.setCommand( "debug-draw-boxes-toggle",
					[&] { mUISceneNode->setDrawBoxes( !mUISceneNode->getDrawBoxes() ); } );
	doc.setCommand( "debug-draw-highlight-toggle", [&] {
		mUISceneNode->setHighlightFocus( !mUISceneNode->getHighlightFocus() );
		mUISceneNode->setHighlightOver( !mUISceneNode->getHighlightOver() );
	} );
	doc.setCommand( "debug-draw-debug-data",
					[&] { mUISceneNode->setDrawDebugData( !mUISceneNode->getDrawDebugData() ); } );
	doc.setCommand( "debug-widget-tree-view", [&] { createWidgetTreeView(); } );
	doc.setCommand( "go-to-line", [&] { mFileLocator->goToLine(); } );
	doc.setCommand( "load-current-dir", [&] { loadCurrentDirectory(); } );
	doc.setCommand( "menu-toggle", [&] { toggleSettingsMenu(); } );
	doc.setCommand( "switch-side-panel", [&] { switchSidePanel(); } );
	doc.setCommand( "download-file-web", [&] { downloadFileWebDialog(); } );
	doc.setCommand( "move-panel-left", [&] { panelPosition( PanelPosition::Left ); } );
	doc.setCommand( "move-panel-right", [&] { panelPosition( PanelPosition::Right ); } );
	doc.setCommand( "create-new-terminal", [&] { createNewTerminal(); } );
	doc.setCommand( "terminal-split-right", [&] {
		mSplitter->split( UICodeEditorSplitter::SplitDirection::Right, mSplitter->getCurWidget(),
						  false );
		doc.execute( "create-new-terminal" );
	} );
	doc.setCommand( "terminal-split-bottom", [&] {
		mSplitter->split( UICodeEditorSplitter::SplitDirection::Bottom, mSplitter->getCurWidget(),
						  false );
		doc.execute( "create-new-terminal" );
	} );
	doc.setCommand( "terminal-split-left", [&] {
		mSplitter->split( UICodeEditorSplitter::SplitDirection::Left, mSplitter->getCurWidget(),
						  false );
		doc.execute( "create-new-terminal" );
	} );
	doc.setCommand( "terminal-split-top", [&] {
		mSplitter->split( UICodeEditorSplitter::SplitDirection::Top, mSplitter->getCurWidget(),
						  false );
		doc.execute( "create-new-terminal" );
	} );

	editor->addEventListener( Event::OnDocumentSave, [&]( const Event* event ) {
		UICodeEditor* editor = event->getNode()->asType<UICodeEditor>();
		updateEditorTabTitle( editor );
		if ( mSplitter->curEditorExistsAndFocused() && mSplitter->getCurEditor() == editor )
			editor->setFocus();
		if ( editor->getDocument().getFilePath() == mKeybindingsPath ) {
			mKeybindings.clear();
			mKeybindingsInvert.clear();
			loadKeybindings();
			mSplitter->forEachEditor( [&]( UICodeEditor* ed ) {
				ed->getKeyBindings().reset();
				ed->getKeyBindings().addKeybindsStringUnordered( mKeybindings );
			} );
		}
		if ( !editor->getDocument().hasSyntaxDefinition() ) {
			editor->getDocument().resetSyntax();
			editor->setSyntaxDefinition( editor->getDocument().getSyntaxDefinition() );
		}
	} );

	editor->addEventListener(
		Event::OnDocumentDirtyOnFileSysten, [&, editor]( const Event* event ) {
			const DocEvent* docEvent = static_cast<const DocEvent*>( event );
			FileInfo file( docEvent->getDoc()->getFileInfo().getFilepath() );
			TextDocument* doc = docEvent->getDoc();
			if ( doc->getFileInfo() != file ) {
				if ( doc->isDirty() ) {
					editor->runOnMainThread( [&, editor]() { createDocAlert( editor ); } );
				} else {
					auto hash = String::hash( docEvent->getDoc()->getFilePath() );
					editor->removeActionsByTag( hash );
					editor->runOnMainThread( [doc]() { doc->reload(); }, Seconds( 0.5f ), hash );
				}
			}
		} );

	if ( !mKeybindings.empty() ) {
		editor->getKeyBindings().reset();
		editor->getKeyBindings().addKeybindsStringUnordered( mKeybindings );
	}

	editor->addEventListener( Event::OnDocumentClosed, [&]( const Event* event ) {
		if ( !appInstance )
			return;
		const DocEvent* docEvent = static_cast<const DocEvent*>( event );
		std::string dir( FileSystem::fileRemoveFileName( docEvent->getDoc()->getFilePath() ) );
		if ( dir.empty() )
			return;
		Lock l( mWatchesLock );
		auto itWatch = mFilesFolderWatches.find( dir );
		if ( mFileWatcher && itWatch != mFilesFolderWatches.end() ) {
			if ( !mDirTree || !mDirTree->isDirInTree( dir ) ) {
				mFileWatcher->removeWatch( itWatch->second );
			}
			mFilesFolderWatches.erase( itWatch );
		}
	} );

	editor->addEventListener( Event::OnDocumentMoved, [&]( const Event* event ) {
		if ( !appInstance )
			return;
		UICodeEditor* editor = event->getNode()->asType<UICodeEditor>();
		updateEditorTabTitle( editor );
	} );

	auto docChanged = [&]( const Event* event ) {
		const DocEvent* synEvent = static_cast<const DocEvent*>( event );
		UICodeEditor* editor = event->getNode()->asType<UICodeEditor>();
		UIIcon* icon = mUISceneNode->findIcon(
			UIIconThemeManager::getIconNameFromFileName( synEvent->getDoc()->getFilename() ) );
		if ( !icon )
			icon = mUISceneNode->findIcon( "file" );
		if ( !icon )
			return;
		if ( editor->getData() ) {
			UITab* tab = (UITab*)editor->getData();
			tab->setIcon( icon->getSize( mMenuIconSize ) );
		}
	};

	editor->addEventListener( Event::OnDocumentLoaded, docChanged );
	editor->addEventListener( Event::OnDocumentChanged, docChanged );
	editor->addEventListener( Event::OnDocumentSave, docChanged );
	editor->addEventListener( Event::OnEditorTabReady, docChanged );

	editor->showMinimap( config.minimap );

	if ( config.autoComplete && !mAutoCompletePlugin )
		setAutoComplete( config.autoComplete );

	if ( config.linter && !mLinterPlugin )
		setLinter( config.linter );

	if ( config.formatter && !mFormatterPlugin )
		setFormatter( config.formatter );

	if ( config.autoComplete && mAutoCompletePlugin )
		editor->registerPlugin( mAutoCompletePlugin );

	if ( config.linter && mLinterPlugin )
		editor->registerPlugin( mLinterPlugin );

	if ( config.formatter && mFormatterPlugin )
		editor->registerPlugin( mFormatterPlugin );
}

bool App::setAutoComplete( bool enable ) {
	mConfig.editor.autoComplete = enable;
	if ( enable && !mAutoCompletePlugin ) {
		mAutoCompletePlugin = eeNew( AutoCompletePlugin, ( mThreadPool ) );
		mSplitter->forEachEditor(
			[&]( UICodeEditor* editor ) { editor->registerPlugin( mAutoCompletePlugin ); } );
		return true;
	}
	if ( !enable && mAutoCompletePlugin )
		eeSAFE_DELETE( mAutoCompletePlugin );
	return false;
}

bool App::setLinter( bool enable ) {
	mConfig.editor.linter = enable;
	if ( enable && !mLinterPlugin ) {
		std::string path( mResPath + "plugins/linters.json" );
		if ( FileSystem::fileExists( mPluginsPath + "linters.json" ) )
			path = mPluginsPath + "linters.json";
		mLinterPlugin = eeNew( LinterPlugin, ( path, mThreadPool ) );
		mSplitter->forEachEditor(
			[&]( UICodeEditor* editor ) { editor->registerPlugin( mLinterPlugin ); } );
		return true;
	}
	if ( !enable && mLinterPlugin )
		eeSAFE_DELETE( mLinterPlugin );
	return false;
}

bool App::setFormatter( bool enable ) {
	mConfig.editor.formatter = enable;
	if ( enable && !mFormatterPlugin ) {
		std::string path( mResPath + "plugins/formatters.json" );
		if ( FileSystem::fileExists( mPluginsPath + "formatters.json" ) )
			path = mPluginsPath + "formatter.json";
		mFormatterPlugin = eeNew( FormatterPlugin, ( path, mThreadPool ) );
		mSplitter->forEachEditor(
			[&]( UICodeEditor* editor ) { editor->registerPlugin( mFormatterPlugin ); } );
		return true;
	}
	if ( !enable && mFormatterPlugin )
		eeSAFE_DELETE( mFormatterPlugin );
	return false;
}

void App::loadCurrentDirectory() {
	if ( !mSplitter->curEditorExistsAndFocused() )
		return;
	std::string path( mSplitter->getCurEditor()->getDocument().getFilePath() );
	if ( path.empty() )
		return;
	path = FileSystem::fileRemoveFileName( path );
	if ( !FileSystem::isDirectory( path ) )
		return;
	loadFolder( path );
}

UIPopUpMenu* App::createToolsMenu() {
	mToolsMenu = UIPopUpMenu::New();
	mToolsMenu
		->add( i18n( "locate", "Locate..." ), findIcon( "search" ), getKeybind( "open-locatebar" ) )
		->setId( "open-locatebar" );
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
		UIMenuItem* item = event->getNode()->asType<UIMenuItem>();
		std::string id( item->getText() );
		if ( id == "open-locatebar" ) {
			mFileLocator->showLocateBar();
		} else if ( id == "open-global-search" ) {
			mGlobalSearchController->showGlobalSearch( false );
		} else if ( id == "go-to-line" ) {
			mFileLocator->goToLine();
		} else if ( id == "load-current-dir" ) {
			loadCurrentDirectory();
		}
	} );
	return mToolsMenu;
}

void App::toggleSettingsMenu() {
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

void App::createSettingsMenu() {
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
					  UIPopUpMenu::New() )
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
	UIMenuSubMenu* fileTypeMenu = mSettingsMenu->addSubMenu( i18n( "file_type", "File Type" ),
															 nullptr, createFileTypeMenu() );
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
	UIMenuSubMenu* colorSchemeMenu = mSettingsMenu->addSubMenu(
		i18n( "color_scheme", "Color Scheme" ), nullptr, createColorSchemeMenu() );
	colorSchemeMenu->addEventListener( Event::OnMenuShow, [&, fileTypeMenu]( const Event* ) {
		if ( mFileTypeMenuesCreatedWithHeight != mUISceneNode->getPixelsSize().getHeight() ) {
			for ( UIPopUpMenu* menu : mFileTypeMenues )
				menu->close();
			mFileTypeMenues.clear();
			auto* newMenu = createFileTypeMenu();
			newMenu->reloadStyle( true, true );
			fileTypeMenu->setSubMenu( newMenu );
		}
	} );
	mSettingsMenu->addSubMenu( i18n( "document", "Document" ), nullptr, createDocumentMenu() )
		->setId( "doc-menu" );
	mSettingsMenu->addSubMenu( i18n( "edit", "Edit" ), nullptr, createEditMenu() );
	mSettingsMenu->addSubMenu( i18n( "view", "View" ), nullptr, createViewMenu() );
	mSettingsMenu->addSubMenu( i18n( "tools", "Tools" ), nullptr, createToolsMenu() );
	mSettingsMenu->addSubMenu( i18n( "window", "Window" ), nullptr, createWindowMenu() );
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
	updateRecentFiles();
	updateRecentFolders();
}

void App::updateColorSchemeMenu() {
	for ( UIPopUpMenu* menu : mFileTypeMenues ) {
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

UIMenu* App::createColorSchemeMenu() {
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

UIMenu* App::createFileTypeMenu() {
	mFileTypeMenuesCreatedWithHeight = mUISceneNode->getPixelsSize().getHeight();
	size_t maxItems = 19;
	auto* dM = SyntaxDefinitionManager::instance();
	auto names = dM->getLanguageNames();
	auto cb = [&, dM]( const Event* event ) {
		UIMenuItem* item = event->getNode()->asType<UIMenuItem>();
		const String& name = item->getText();
		if ( mSplitter->curEditorExistsAndFocused() ) {
			mSplitter->getCurEditor()->setSyntaxDefinition( dM->getStyleByLanguageName( name ) );
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

void App::updateCurrentFileType() {
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

void App::updateEditorState() {
	if ( mSplitter->curEditorExistsAndFocused() ) {
		updateEditorTitle( mSplitter->getCurEditor() );
		updateCurrentFileType();
		updateDocumentMenu();
	}
}

void App::removeFolderWatches() {
	if ( mFileWatcher ) {
		std::unordered_set<efsw::WatchID> folderWatches;
		std::unordered_map<std::string, efsw::WatchID> filesFolderWatches;
		{
			Lock l( mWatchesLock );
			folderWatches = mFolderWatches;
			filesFolderWatches = mFilesFolderWatches;
			mFolderWatches.clear();
			mFilesFolderWatches.clear();
		}

		for ( const auto& dir : folderWatches )
			mFileWatcher->removeWatch( dir );

		for ( const auto& fileFolder : filesFolderWatches )
			mFileWatcher->removeWatch( fileFolder.second );
	}
}

void App::loadDirTree( const std::string& path ) {
	Clock* clock = eeNew( Clock, () );
	mDirTreeReady = false;
	mDirTree = std::make_shared<ProjectDirectoryTree>( path, mThreadPool );
	Log::info( "Loading DirTree: %s", path.c_str() );
	mDirTree->scan(
		[&, clock]( ProjectDirectoryTree& dirTree ) {
			Log::info( "DirTree read in: %.2fms. Found %ld files.",
					   clock->getElapsedTime().asMilliseconds(), dirTree.getFilesCount() );
			eeDelete( clock );
			mDirTreeReady = true;
			mUISceneNode->runOnMainThread( [&] {
				mFileLocator->updateLocateTable();
				if ( mSplitter->curEditorExistsAndFocused() )
					syncProjectTreeWithEditor( mSplitter->getCurEditor() );
			} );
			if ( mFileWatcher ) {
				removeFolderWatches();
				{
					Lock l( mWatchesLock );
					mFolderWatches.insert(
						mFileWatcher->addWatch( dirTree.getPath(), mFileSystemListener, true ) );
				}
				mFileSystemListener->setDirTree( mDirTree );
			}
		},
		SyntaxDefinitionManager::instance()->getExtensionsPatternsSupported() );
}

UIMessageBox* App::errorMsgBox( const String& msg ) {
	UIMessageBox* msgBox = UIMessageBox::New( UIMessageBox::OK, msg );
	msgBox->setTitle( i18n( "error", "Error" ) );
	msgBox->showWhenReady();
	return msgBox;
}

UIMessageBox* App::fileAlreadyExistsMsgBox() {
	return errorMsgBox( i18n( "file_already_exists", "File already exists!" ) );
}

std::string getNewFilePath( const FileInfo& file, UIMessageBox* msgBox ) {
	auto folderName( msgBox->getTextInput()->getText() );
	auto folderPath( file.getDirectoryPath() );
	FileSystem::dirAddSlashAtEnd( folderPath );
	return folderPath + folderName;
}

UIMessageBox* newInputMsgBox( const String& title, const String& msg ) {
	UIMessageBox* msgBox = UIMessageBox::New( UIMessageBox::INPUT, msg );
	msgBox->setTitle( title );
	msgBox->setCloseShortcut( { KEY_ESCAPE, 0 } );
	msgBox->showWhenReady();
	return msgBox;
}

void App::renameFile( const FileInfo& file ) {
	if ( !file.exists() )
		return;
	UIMessageBox* msgBox =
		newInputMsgBox( i18n( "rename_file", "Rename file" ) + " \"" + file.getFileName() + "\"",
						i18n( "enter_new_file_name", "Enter new file name:" ) );
	msgBox->getTextInput()->setText( file.getFileName() );
	msgBox->addEventListener( Event::MsgBoxConfirmClick, [&, file, msgBox]( const Event* ) {
		auto newFilePath( getNewFilePath( file, msgBox ) );
		if ( !FileSystem::fileExists( newFilePath ) ) {
			if ( 0 != std::rename( file.getFilepath().c_str(), newFilePath.c_str() ) )
				errorMsgBox( i18n( "error_renaming_file", "Error renaming file." ) );
			msgBox->closeWindow();
		} else {
			fileAlreadyExistsMsgBox();
		}
	} );
}

void App::createProjectTreeMenu( const FileInfo& file ) {
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
		mProjectTreeMenu->add( i18n( "open_fole", "Open File" ), findIcon( "document-open" ) )
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
					   !mFileSystemModel->getDisplayConfig().ignoreHidden )
		->setId( "show_hidden_files" );

	mProjectTreeMenu->addEventListener( Event::OnItemClicked, [&, file]( const Event* event ) {
		if ( !event->getNode()->isType( UI_TYPE_MENUITEM ) )
			return;
		UIMenuItem* item = event->getNode()->asType<UIMenuItem>();
		std::string id( item->getId() );

		if ( "new_file" == id || "new_file_in_place" == id ) {
			UIMessageBox* msgBox =
				newInputMsgBox( i18n( "create_new_file", "Create new file" ),
								i18n( "enter_new_file_name", "Enter new file name:" ) );
			msgBox->addEventListener( Event::MsgBoxConfirmClick, [&, file, msgBox]( const Event* ) {
				auto newFilePath( getNewFilePath( file, msgBox ) );
				if ( !FileSystem::fileExists( newFilePath ) ) {
					if ( !FileSystem::fileWrite( newFilePath, nullptr, 0 ) )
						errorMsgBox( i18n( "couldnt_create_file", "Couldn't create file." ) );
					msgBox->closeWindow();
				} else {
					fileAlreadyExistsMsgBox();
				}
			} );
		} else if ( "new_folder" == id ) {
			UIMessageBox* msgBox =
				newInputMsgBox( i18n( "create_new_folder", "Create new folder" ),
								i18n( "enter_new_folder_name", "Enter new folder name:" ) );
			msgBox->addEventListener( Event::MsgBoxConfirmClick, [&, file, msgBox]( const Event* ) {
				auto newFolderPath( getNewFilePath( file, msgBox ) );
				if ( !FileSystem::fileExists( newFolderPath ) ) {
					if ( !FileSystem::makeDir( newFolderPath ) )
						errorMsgBox(
							i18n( "couldnt_create_directory", "Couldn't create directory." ) );
					msgBox->closeWindow();
				} else {
					fileAlreadyExistsMsgBox();
				}
			} );
		} else if ( "open_file" == id ) {
			loadFileFromPath( file.getFilepath() );
		} else if ( "remove" == id ) {
			if ( file.isDirectory() && !FileSystem::filesGetInPath( file.getFilepath() ).empty() ) {
				errorMsgBox(
					i18n( "cannot_remove_non_empty_dir", "Cannot remove non-empty directory." ) );
				return;
			}

			UIMessageBox* msgBox =
				UIMessageBox::New( UIMessageBox::OK_CANCEL,
								   String::format( i18n( "confirm_remove_file",
														 "Do you really want to remove \"%s\"?" )
													   .toUtf8()
													   .c_str(),
												   file.getFileName().c_str() ) );
			msgBox->addEventListener( Event::MsgBoxConfirmClick, [&, file, msgBox]( const Event* ) {
				if ( !FileSystem::fileRemove( file.getFilepath() ) ) {
					errorMsgBox( String::format(
						std::string( i18n( "couldnt_remove", "Couldn't remove" ).toUtf8() + "%s." )
							.c_str(),
						file.isDirectory() ? i18n( "directory", "directory" ).toUtf8().c_str()
										   : i18n( "file", "file" ).toUtf8().c_str() ) );
				}
				msgBox->closeWindow();
			} );
			msgBox->setTitle( i18n( "remove_file_question", "Remove file?" ) );
			msgBox->center();
			msgBox->showWhenReady();
		} else if ( "duplicate_file" == id ) {
			UIMessageBox* msgBox = newInputMsgBox(
				String::format( "%s \"%s\"",
								i18n( "duplicate_file", "Duplicate file" ).toUtf8().c_str(),
								file.getFileName().c_str() ),
				i18n( "enter_duplicate_file_name", "Enter duplicate file name:" ) );
			msgBox->addEventListener( Event::MsgBoxConfirmClick, [&, file, msgBox]( const Event* ) {
				auto newFilePath( getNewFilePath( file, msgBox ) );
				if ( !FileSystem::fileExists( newFilePath ) ) {
					if ( !FileSystem::fileCopy( file.getFilepath(), newFilePath ) )
						errorMsgBox( i18n( "error_copying_file", "Error copying file." ) );
					msgBox->closeWindow();
				} else {
					fileAlreadyExistsMsgBox();
				}
			} );
		} else if ( "rename" == id ) {
			renameFile( file );
		} else if ( "open_containing_folder" == id ) {
			Engine::instance()->openURI( file.getDirectoryPath() );
		} else if ( "open_folder" == id ) {
			Engine::instance()->openURI( file.getFilepath() );
		} else if ( "show_hidden_files" == id ) {
			mFileSystemModel = FileSystemModel::New(
				mFileSystemModel->getRootPath(), FileSystemModel::Mode::FilesAndDirectories,
				{ true, true, !mFileSystemModel->getDisplayConfig().ignoreHidden } );
			mProjectTreeView->setModel( mFileSystemModel );
		} else if ( "execute_in_terminal" == id ) {
			UITerminal* term = createNewTerminal( "", nullptr, file.getDirectoryPath() );
			if ( !term )
				return;
			term->executeFile( file.getFilepath() );
		} else if ( "execute_dir_in_terminal" == id ) {
			createNewTerminal( "", nullptr, file.getDirectoryPath() );
		}
	} );

	Vector2f pos( mWindow->getInput()->getMousePosf() );
	mProjectTreeMenu->nodeToWorldTranslation( pos );
	UIMenu::findBestMenuPos( pos, mProjectTreeMenu );
	mProjectTreeMenu->setPixelsPosition( pos );
	mProjectTreeMenu->show();
}

void App::initProjectTreeView( const std::string& path ) {
	mProjectTreeView = mUISceneNode->find<UITreeView>( "project_view" );
	mProjectTreeView->setColumnsHidden(
		{ FileSystemModel::Icon, FileSystemModel::Size, FileSystemModel::Group,
		  FileSystemModel::Inode, FileSystemModel::Owner, FileSystemModel::SymlinkTarget,
		  FileSystemModel::Permissions, FileSystemModel::ModificationTime, FileSystemModel::Path },
		true );
	mProjectTreeView->setIconSize( mMenuIconSize );
	mProjectTreeView->setExpanderIconSize( mMenuIconSize );
	mProjectTreeView->setExpandedIcon( "folder-open" );
	mProjectTreeView->setContractedIcon( "folder" );
	mProjectTreeView->setHeadersVisible( false );
	mProjectTreeView->setExpandersAsIcons( true );
	mProjectTreeView->setSingleClickNavigation( mConfig.editor.singleClickTreeNavigation );
	mProjectTreeView->addEventListener( Event::OnModelEvent, [&]( const Event* event ) {
		const ModelEvent* modelEvent = static_cast<const ModelEvent*>( event );
		ModelEventType type = modelEvent->getModelEventType();
		if ( type == ModelEventType::Open || type == ModelEventType::OpenMenu ) {
			Variant vPath(
				modelEvent->getModel()->data( modelEvent->getModelIndex(), ModelRole::Custom ) );
			if ( vPath.isValid() && vPath.is( Variant::Type::cstr ) ) {
				std::string path( vPath.asCStr() );
				if ( type == ModelEventType::Open ) {
					UITab* tab = mSplitter->isDocumentOpen( path );
					if ( !tab ) {
						FileInfo fileInfo( path );
						if ( fileInfo.exists() && fileInfo.isRegularFile() )
							loadFileFromPath( path );
					} else {
						tab->getTabWidget()->setTabSelected( tab );
					}
				} else { // ModelEventType::OpenMenu
					bool focusOnSelection = mProjectTreeView->getFocusOnSelection();
					mProjectTreeView->setFocusOnSelection( false );
					mProjectTreeView->getSelection().set( modelEvent->getModelIndex() );
					mProjectTreeView->setFocusOnSelection( focusOnSelection );
					createProjectTreeMenu( FileInfo( path ) );
				}
			}
		}
	} );
	mProjectTreeView->addEventListener( Event::KeyDown, [&]( const Event* event ) {
		const KeyEvent* keyEvent = static_cast<const KeyEvent*>( event );
		if ( keyEvent->getKeyCode() == KEY_F2 ) {
			ModelIndex modelIndex = mProjectTreeView->getSelection().first();
			if ( !modelIndex.isValid() )
				return 0;
			Variant vPath( mProjectTreeView->getModel()->data( modelIndex, ModelRole::Custom ) );
			if ( vPath.isValid() && vPath.is( Variant::Type::cstr ) )
				renameFile( FileInfo( vPath.asCStr() ) );
			return 1;
		}

		if ( mSplitter->curEditorExistsAndFocused() ) {
			std::string cmd = mSplitter->getCurEditor()->getKeyBindings().getCommandFromKeyBind(
				{ keyEvent->getKeyCode(), keyEvent->getMod() } );
			if ( !cmd.empty() && mSplitter->getCurEditor()->isUnlockedCommand( cmd ) ) {
				mSplitter->getCurEditor()->getDocument().execute( cmd );
			}
		}

		return 1;
	} );

	if ( !path.empty() ) {
		if ( FileSystem::isDirectory( path ) ) {
			loadFolder( path );
		} else if ( String::startsWith( path, "https://" ) ||
					String::startsWith( path, "http://" ) ) {
			loadFolder( "." );
			loadFileFromPath( path, false );
		} else {
			std::string rpath( FileSystem::getRealPath( path ) );
			std::string folderPath( FileSystem::fileRemoveFileName( rpath ) );

			if ( FileSystem::isDirectory( folderPath ) ) {
				mFileSystemModel = FileSystemModel::New(
					folderPath, FileSystemModel::Mode::FilesAndDirectories, { true, true, true } );

				mProjectTreeView->setModel( mFileSystemModel );

				if ( mFileSystemListener )
					mFileSystemListener->setFileSystemModel( mFileSystemModel );

				if ( FileSystem::fileExists( rpath ) ) {
					loadFileFromPath( rpath, false );
				} else if ( FileSystem::fileCanWrite( folderPath ) ) {
					loadFileFromPath( path, false );
				}
			}
		}
	} else if ( !mIsBundledApp ) {
		loadFolder( "." );
	}

	mProjectTreeView->setAutoExpandOnSingleColumn( true );
}

void App::initImageView() {
	mImageLayout->on( Event::MouseClick, [&]( const Event* ) {
		mImageLayout->findByType<UIImage>( UI_TYPE_IMAGE )->setDrawable( nullptr );
		mImageLayout->setEnabled( false )->setVisible( false );
	} );
}

void App::loadFolder( const std::string& path ) {
	if ( !mCurrentProject.empty() )
		closeEditors();

	std::string rpath( FileSystem::getRealPath( path ) );
	mCurrentProject = rpath;
	loadDirTree( rpath );

	mConfig.loadProject( rpath, mSplitter, mConfigPath, mProjectDocConfig, mThreadPool, this );

	mFileSystemModel = FileSystemModel::New( rpath, FileSystemModel::Mode::FilesAndDirectories,
											 { true, true, true } );

	mProjectTreeView->setModel( mFileSystemModel );

	if ( mFileSystemListener )
		mFileSystemListener->setFileSystemModel( mFileSystemModel );

	auto found = std::find( mRecentFolders.begin(), mRecentFolders.end(), rpath );
	if ( found != mRecentFolders.end() )
		mRecentFolders.erase( found );
	mRecentFolders.insert( mRecentFolders.begin(), rpath );
	if ( mRecentFolders.size() > 10 )
		mRecentFolders.resize( 10 );

	updateRecentFolders();
	updateProjectSettingsMenu();

	if ( mSplitter->getCurWidget() )
		mSplitter->getCurWidget()->setFocus();
}

FontTrueType* App::loadFont( const std::string& name, std::string fontPath,
							 const std::string& fallback ) {
	if ( FileSystem::isRelativePath( fontPath ) )
		fontPath = mResPath + fontPath;
	if ( fontPath.empty() || !FileSystem::fileExists( fontPath ) ) {
		fontPath = fallback;
		if ( !fontPath.empty() && FileSystem::isRelativePath( fontPath ) )
			fontPath = mResPath + fontPath;
	}
	if ( fontPath.empty() )
		return nullptr;
	return FontTrueType::New( name, fontPath );
}

void App::init( std::string file, const Float& pidelDensity, const std::string& colorScheme,
				bool terminal ) {
	DisplayManager* displayManager = Engine::instance()->getDisplayManager();
	Display* currentDisplay = displayManager->getDisplayIndex( 0 );
	mDisplayDPI = currentDisplay->getDPI();

	loadConfig();

	currentDisplay = displayManager->getDisplayIndex( mConfig.window.displayIndex <
															  displayManager->getDisplayCount()
														  ? mConfig.window.displayIndex
														  : 0 );
	mDisplayDPI = currentDisplay->getDPI();
	mResPath = Sys::getProcessPath();
#if EE_PLATFORM == EE_PLATFORM_MACOSX
	if ( String::contains( mResPath, "ecode.app" ) ) {
		mResPath = FileSystem::getCurrentWorkingDirectory();
		FileSystem::dirAddSlashAtEnd( mResPath );
		mIsBundledApp = true;
	}
#elif EE_PLATFORM == EE_PLATFORM_LINUX
	if ( String::contains( mResPath, ".mount_" ) ) {
		mResPath = FileSystem::getCurrentWorkingDirectory();
		FileSystem::dirAddSlashAtEnd( mResPath );
		mIsBundledApp = true;
	}
#elif EE_PLATFORM == EE_PLATFORM_EMSCRIPTEN
	mResPath += "ecode/";
#endif
	mResPath += "assets";
	FileSystem::dirAddSlashAtEnd( mResPath );

	mConfig.window.pixelDensity =
		pidelDensity > 0 ? pidelDensity
						 : ( mConfig.window.pixelDensity > 0 ? mConfig.window.pixelDensity
															 : currentDisplay->getPixelDensity() );

	displayManager->enableScreenSaver();
	displayManager->enableMouseFocusClickThrough();
	displayManager->disableBypassCompositor();

	Engine* engine = Engine::instance();

	WindowSettings winSettings = engine->createWindowSettings( &mConfig.iniState, "window" );
	winSettings.PixelDensity = 1;
	winSettings.Width = mConfig.window.size.getWidth();
	winSettings.Height = mConfig.window.size.getHeight();
	if ( winSettings.Icon.empty() ) {
		winSettings.Icon = mConfig.window.winIcon;
		if ( FileSystem::isRelativePath( winSettings.Icon ) )
			winSettings.Icon = mResPath + winSettings.Icon;
	}
	ContextSettings contextSettings = engine->createContextSettings( &mConfig.ini, "window" );
	contextSettings.SharedGLContext = true;
	mWindow = engine->createWindow( winSettings, contextSettings );

	Log::info( "%s (codename: \"%s\") initializing", ecode::Version::getVersionName().c_str(),
			   ecode::Version::getCodename().c_str() );

	if ( mWindow->isOpen() ) {
		Log::info( "Window creation took: %.2fms", globalClock.getElapsedTime().asMilliseconds() );

		if ( mConfig.window.position != Vector2i( -1, -1 ) &&
			 mConfig.window.displayIndex < displayManager->getDisplayCount() ) {
			mWindow->setPosition( mConfig.window.position.x, mConfig.window.position.y );
		}

		loadKeybindings();

		PixelDensity::setPixelDensity( mConfig.window.pixelDensity );

		if ( mConfig.window.maximized )
			mWindow->maximize();

		mWindow->setCloseRequestCallback(
			[&]( EE::Window::Window* win ) -> bool { return onCloseRequestCallback( win ); } );

		mWindow->getInput()->pushCallback( [&]( InputEvent* event ) {
			if ( event->Type == InputEvent::FileDropped ) {
				onFileDropped( event->file.file );
			} else if ( event->Type == InputEvent::TextDropped ) {
				onTextDropped( event->textdrop.text );
			}
		} );

		PixelDensity::setPixelDensity( eemax( mWindow->getScale(), mConfig.window.pixelDensity ) );

		mUISceneNode = UISceneNode::New();
		mUIColorScheme = mConfig.ui.colorScheme;
		if ( !colorScheme.empty() ) {
			mUIColorScheme =
				colorScheme == "light" ? ColorSchemePreference::Light : ColorSchemePreference::Dark;
		}
		mUISceneNode->setColorSchemePreference( mUIColorScheme );

		mFont = loadFont( "sans-serif", mConfig.ui.serifFont, "fonts/NotoSans-Regular.ttf" );
		mFontMono = loadFont( "monospace", mConfig.ui.monospaceFont, "fonts/DejaVuSansMono.ttf" );
		if ( mFontMono )
			mFontMono->setBoldAdvanceSameAsRegular( true );

		loadFont( "NotoEmoji-Regular", "fonts/NotoEmoji-Regular.ttf" );

#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN
		loadFont( "NotoColorEmoji", "fonts/NotoColorEmoji.ttf" );
#endif

		FontTrueType* iconFont = loadFont( "icon", "fonts/remixicon.ttf" );
		FontTrueType* mimeIconFont = loadFont( "nonicons", "fonts/nonicons.ttf" );

		if ( !mFont || !mFontMono || !iconFont || !mimeIconFont ) {
			printf( "Font not found!" );
			Log::error( "Font not found!" );
			return;
		}

		mFontMonoNerdFont =
			loadFont( "monospace-nerdfont", "fonts/DejaVuSansMonoNerdFontComplete.ttf" );

		SceneManager::instance()->add( mUISceneNode );

		UITheme* theme =
			UITheme::load( "uitheme", "uitheme", "", mFont, mResPath + "ui/breeze.css" );
		theme->setDefaultFontSize( mConfig.ui.fontSize.asDp( 0, Sizef(), mDisplayDPI ) );
		mUISceneNode->setStyleSheet( theme->getStyleSheet() );
		mUISceneNode
			->getUIThemeManager()
			//->setDefaultEffectsEnabled( true )
			->setDefaultTheme( theme )
			->setDefaultFont( mFont )
			->setDefaultFontSize( mConfig.ui.fontSize.asDp( 0, Sizef(), mDisplayDPI ) )
			->add( theme );

		mUISceneNode->getRoot()->addClass( "appbackground" );

		// TODO: Add text translations
		const std::string baseUI = R"html(
		<style>
		TextInput#search_find,
		TextInput#search_replace,
		TextInput#locate_find,
		TextInput#global_search_find,
		TextInput.small_input,
		.search_str {
			padding-top: 0;
			padding-bottom: 0;
			font-family: monospace;
		}
		#search_bar,
		#global_search_bar,
		#locate_bar {
			padding-left: 4dp;
			padding-right: 4dp;
			padding-bottom: 3dp;
			margin-bottom: 2dp;
			margin-top: 2dp;
		}
		.close_button {
			width: 12dp;
			height: 12dp;
			border-radius: 6dp;
			background-color: var(--icon-back-hover);
			foreground-image: poly(line, var(--icon-line-hover), "0dp 0dp, 6dp 6dp"), poly(line, var(--icon-line-hover), "6dp 0dp, 0dp 6dp");
			foreground-position: 3dp 3dp, 3dp 3dp;
			transition: all 0.15s;
		}
		.close_button:hover {
			background-color: var(--icon-back-alert);
		}
		#settings {
			color: var(--floating-icon);
			font-family: icon;
			font-size: 16dp;
			margin-top: 6dp;
			margin-right: 22dp;
			transition: all 0.15s;
		}
		#settings:hover {
			color: var(--primary);
		}
		#doc_info {
			background-color: var(--back);
			margin-bottom: 22dp;
			margin-right: 22dp;
			border-radius: 8dp;
			padding: 6dp;
			opacity: 0.8;
		}
		#doc_info > TextView {
			color: var(--font);
		}
		#search_find.error,
		#search_replace.error {
			border-color: #ff4040;
		}
		TableView#locate_bar_table > tableview::row > tableview::cell:nth-child(2) {
			color: var(--font-hint);
		}
		TableView#locate_bar_table > tableview::row:selected > tableview::cell:nth-child(2) {
			color: var(--font);
		}
		.search_tree treeview::cell {
			font-family: monospace;
		}
		#global_search_history {
			padding-top: 0dp;
			padding-bottom: 0dp;
		}
		.doc_alert {
			padding: 16dp;
			border-width: 2dp;
			border-radius: 4dp;
			border-color: var(--primary);
			background-color: var(--back);
			margin-right: 24dp;
			margin-top: 24dp;
			cursor: arrow;
		}
		#image_container {
			background-color: #00000066;
		}
		#image_close {
			color: var(--floating-icon);
			font-family: icon;
			font-size: 22dp;
			margin-top: 32dp;
			margin-right: 22dp;
		}
		#global_search_layout {
			background-color: var(--back);
		}
		#global_search_layout > .status_box,
		#global_search_layout > .replace_box {
			padding: 4dp;
		}
		.notification {
			background-color: var(--button-back);
			border-color: var(--button-border);
			border-width: 1dp;
			border-radius: 8dp;
			color: var(--font);
			padding: 4dp;
			min-height: 48dp;
			margin-bottom: 8dp;
			opacity: 0.8;
		}
		</style>
		<RelativeLayout id="main_layout" layout_width="match_parent" layout_height="match_parent">
		<Splitter id="project_splitter" layout_width="match_parent" layout_height="match_parent">
			<TabWidget id="panel" tabbar-hide-on-single-tab="true" tabbar-allow-rearrange="true">
				<TreeView id="project_view" />
				<Tab text="Project" owns="project_view" />
			</TabWidget>
			<vbox>
				<RelativeLayout layout_width="match_parent" layout_height="0" layout_weight="1">
					<vbox id="code_container" layout_width="match_parent" layout_height="match_parent"></vbox>
					<hbox id="doc_info" layout_width="wrap_content" layout_height="wrap_content" layout_gravity="bottom|right" enabled="false">
						<TextView id="doc_info_text" layout_width="wrap_content" layout_height="wrap_content" />
					</hbox>
					<RelativeLayout id="image_container" layout_width="match_parent" layout_height="match_parent" visible="false" enabled="false">
						<Image layout_width="match_parent" layout_height="match_parent" scaleType="fit_inside" gravity="center" enabled="false" layout_gravity="center" />
						<TextView id="image_close" layout_width="wrap_content" layout_height="wrap_content" text="&#xeb99;" layout_gravity="top|right" enabled="false" />
						<Loader id="image_loader" layout_width="64dp" layout_height="64dp" outline-thickness="6dp" layout_gravity="center" visible="false" />
					</RelativeLayout>
					<vbox id="notification_center" layout_width="256dp" layout_height="wrap_content"
						  layout_gravity="right|bottom" margin-right="22dp" margin-bottom="56dp">
					</vbox>
				</RelativeLayout>
				<searchbar id="search_bar" layout_width="match_parent" layout_height="wrap_content">
					<vbox layout_width="wrap_content" layout_height="wrap_content" margin-right="4dp">
						<TextView layout_width="wrap_content" layout_height="18dp" text="Find:" margin-bottom="2dp" />
						<TextView layout_width="wrap_content" layout_height="18dp" text="Replace with:" />
					</vbox>
					<vbox layout_width="0" layout_weight="1" layout_height="wrap_content" margin-right="4dp">
						<TextInput id="search_find" layout_width="match_parent" layout_height="18dp" padding="0" margin-bottom="2dp" />
						<TextInput id="search_replace" layout_width="match_parent" layout_height="18dp" padding="0" />
					</vbox>
					<vbox layout_width="wrap_content" layout_height="wrap_content" margin-right="4dp">
						<CheckBox id="case_sensitive" layout_width="wrap_content" layout_height="wrap_content" text="Case sensitive" selected="false" />
						<CheckBox id="lua_pattern" layout_width="wrap_content" layout_height="wrap_content" text="Lua Pattern" selected="false" />
					</vbox>
					<vbox layout_width="wrap_content" layout_height="wrap_content" margin-right="4dp">
						<CheckBox id="whole_word" layout_width="wrap_content" layout_height="wrap_content" text="Match Whole Word" selected="false" />
						<CheckBox id="escape_sequence" layout_width="wrap_content" layout_height="wrap_content" text="Use escape sequences" selected="false" tooltip="Replace \\, \t, \n, \r and \uXXXX (Unicode characters) with the corresponding control" />
					</vbox>
					<vbox layout_width="wrap_content" layout_height="wrap_content">
						<hbox layout_width="wrap_content" layout_height="wrap_content" margin-bottom="2dp">
							<PushButton id="find_prev" layout_width="wrap_content" layout_height="18dp" text="Previous" margin-right="4dp" />
							<PushButton id="find_next" layout_width="wrap_content" layout_height="18dp" text="Next" margin-right="4dp" />
							<RelativeLayout layout_width="0" layout_weight="1" layout_height="18dp">
								<Widget id="searchbar_close" class="close_button" layout_width="wrap_content" layout_height="wrap_content" layout_gravity="center_vertical|right" margin-right="2dp" />
							</RelativeLayout>
						</hbox>
						<hbox layout_width="wrap_content" layout_height="wrap_content">
							<PushButton id="replace" layout_width="wrap_content" layout_height="18dp" text="Replace" margin-right="4dp" />
							<PushButton id="replace_find" layout_width="wrap_content" layout_height="18dp" text="Replace & Find" margin-right="4dp" />
							<PushButton id="replace_all" layout_width="wrap_content" layout_height="18dp" text="Replace All" />
						</hbox>
					</vbox>
				</searchbar>
				<locatebar id="locate_bar" layout_width="match_parent" layout_height="wrap_content" visible="false">
					<TextInput id="locate_find" layout_width="0" layout_weight="1" layout_height="18dp" padding="0" margin-bottom="2dp" margin-right="4dp" hint="Search files by name ( append `l ` to go to line )" />
					<Widget id="locatebar_close" class="close_button" layout_width="wrap_content" layout_height="wrap_content" layout_gravity="center_vertical|right"/>
				</locatebar>
				<globalsearchbar id="global_search_bar" layout_width="match_parent" layout_height="wrap_content">
					<hbox layout_width="match_parent" layout_height="wrap_content">
						<TextView layout_width="wrap_content" layout_height="wrap_content" text="Search for:" margin-right="4dp" />
						<vbox layout_width="0" layout_weight="1" layout_height="wrap_content">
							<TextInput id="global_search_find" layout_width="match_parent" layout_height="wrap_content" layout_height="18dp" padding="0" margin-bottom="2dp" />
							<hbox layout_width="match_parent" layout_height="wrap_content" margin-bottom="4dp">
								<CheckBox id="case_sensitive" layout_width="wrap_content" layout_height="wrap_content" text="Case sensitive" selected="true" />
								<CheckBox id="whole_word" layout_width="wrap_content" layout_height="wrap_content" text="Match Whole Word" selected="false" margin-left="8dp" />
								<CheckBox id="lua_pattern" layout_width="wrap_content" layout_height="wrap_content" text="Lua Pattern" selected="false" margin-left="8dp" />
								<CheckBox id="escape_sequence" layout_width="wrap_content" layout_height="wrap_content" text="Use escape sequences" margin-left="8dp" selected="false" tooltip="Replace \\, \t, \n, \r and \uXXXX (Unicode characters) with the corresponding control" />
							</hbox>
							<hbox layout_width="match_parent" layout_height="wrap_content">
								<TextView layout_width="wrap_content" layout_height="wrap_content" text="History:" margin-right="4dp" layout_height="18dp" />
								<DropDownList id="global_search_history" layout_width="0" layout_height="18dp" layout_weight="1" margin-right="4dp" />
								<PushButton id="global_search_clear_history" layout_width="wrap_content" layout_height="18dp" text="Clear History" margin-right="4dp" />
								<PushButton id="global_search" layout_width="wrap_content" layout_height="18dp" text="Search" margin-right="4dp" />
								<PushButton id="global_search_replace" layout_width="wrap_content" layout_height="18dp" text="Search & Replace" />
							</hbox>
						</vbox>
						<Widget id="global_searchbar_close" class="close_button" layout_width="wrap_content" layout_height="wrap_content" layout_gravity="top|right" margin-left="4dp" margin-top="4dp" />
					</hbox>
				</globalsearchbar>
			</vbox>
		</Splitter>
		<TextView id="settings" layout_width="wrap_content" layout_height="wrap_content" text="&#xf0e9;" layout_gravity="top|right" />
		</RelativeLayout>
		)html";

		UIIconTheme* iconTheme = UIIconTheme::New( "ecode" );
		mMenuIconSize = mConfig.ui.fontSize.asPixels( 0, Sizef(), mDisplayDPI );
		std::unordered_map<std::string, Uint32> icons = {
			{ "document-new", 0xecc3 },
			{ "document-open", 0xed70 },
			{ "document-save", 0xf0b3 },
			{ "document-save-as", 0xf0b3 },
			{ "document-close", 0xeb99 },
			{ "quit", 0xeb97 },
			{ "undo", 0xea58 },
			{ "redo", 0xea5a },
			{ "cut", 0xf0c1 },
			{ "copy", 0xecd5 },
			{ "paste", 0xeb91 },
			{ "edit", 0xec86 },
			{ "split-horizontal", 0xf17a },
			{ "split-vertical", 0xf17b },
			{ "find-replace", 0xed2b },
			//			{ "folder", 0xed54 },
			//			{ "folder-open", 0xed70 },
			{ "folder-add", 0xed5a },
			//			{ "file", 0xecc3 },
			{ "file-add", 0xecc9 },
			{ "file-copy", 0xecd3 },
			{ "file-code", 0xecd1 },
			{ "file-edit", 0xecdb },
			{ "font-size", 0xed8d },
			{ "delete-bin", 0xec1e },
			{ "delete-text", 0xec1e },
			{ "zoom-in", 0xf2db },
			{ "zoom-out", 0xf2dd },
			{ "zoom-reset", 0xeb47 },
			{ "fullscreen", 0xed9c },
			{ "keybindings", 0xee75 },
			//			{ "tree-expanded", 0xea50 },
			//			{ "tree-contracted", 0xea54 },
			{ "search", 0xf0d1 },
			{ "go-up", 0xea78 },
			{ "ok", 0xeb7a },
			{ "cancel", 0xeb98 },
			{ "color-picker", 0xf13d },
			{ "pixel-density", 0xed8c },
			{ "go-to-line", 0xf1f8 },
			{ "table-view", 0xf1de },
			{ "list-view", 0xecf1 },
			{ "menu-unfold", 0xef40 },
			{ "menu-fold", 0xef3d },
			{ "download-cloud", 0xec58 },
			{ "layout-left", 0xee94 },
			{ "layout-right", 0xee9b },
			{ "color-scheme", 0xebd4 },
			{ "global-settings", 0xedcf },
			{ "folder-user", 0xed84 },
			{ "help", 0xf045 },
			{ "terminal", 0xf1f6 },
			{ "earth", 0xec7a },
		};
		for ( const auto& icon : icons )
			iconTheme->add( UIGlyphIcon::New( icon.first, iconFont, icon.second ) );

		if ( mimeIconFont && mimeIconFont->loaded() ) {
			std::unordered_map<std::string, Uint32> mimeIcons = {
				{ "filetype-lua", 61826 },		  { "filetype-c", 61718 },
				{ "filetype-h", 61792 },		  { "filetype-cs", 61720 },
				{ "filetype-cpp", 61719 },		  { "filetype-css", 61743 },
				{ "filetype-conf", 61781 },		  { "filetype-cfg", 61781 },
				{ "filetype-desktop", 61781 },	  { "filetype-service", 61781 },
				{ "filetype-env", 61781 },		  { "filetype-properties", 61781 },
				{ "filetype-ini", 61781 },		  { "filetype-dart", 61744 },
				{ "filetype-diff", 61752 },		  { "filetype-zip", 61775 },
				{ "filetype-go", 61789 },		  { "filetype-htm", 61799 },
				{ "filetype-html", 61799 },		  { "filetype-java", 61809 },
				{ "filetype-js", 61810 },		  { "filetype-json", 61811 },
				{ "filetype-kt", 61814 },		  { "filetype-md", 61829 },
				{ "filetype-perl", 61853 },		  { "filetype-php", 61855 },
				{ "filetype-py", 61863 },		  { "filetype-pyc", 61863 },
				{ "filetype-pyd", 61863 },		  { "filetype-swift", 61906 },
				{ "filetype-rb", 61880 },		  { "filetype-rs", 61881 },
				{ "filetype-ts", 61923 },		  { "filetype-yaml", 61945 },
				{ "filetype-yml", 61945 },		  { "filetype-jpg", 61801 },
				{ "filetype-png", 61801 },		  { "filetype-jpeg", 61801 },
				{ "filetype-bmp", 61801 },		  { "filetype-tga", 61801 },
				{ "filetype-sh", 61911 },		  { "filetype-bash", 61911 },
				{ "filetype-fish", 61911 },		  { "filetype-scala", 61882 },
				{ "filetype-r", 61866 },		  { "filetype-rake", 61880 },
				{ "filetype-rss", 61879 },		  { "filetype-sql", 61746 },
				{ "filetype-elm", 61763 },		  { "filetype-ex", 61971 },
				{ "filetype-exs", 61971 },		  { "filetype-awk", 61971 },
				{ "filetype-nim", 61734 },		  { "filetype-xml", 61769 },
				{ "filetype-dockerfile", 61758 }, { "file", 61766 },
				{ "file-symlink", 61774 },		  { "folder", 0xF23B },
				{ "folder-open", 0xF23C },		  { "tree-expanded", 0xF11E },
				{ "tree-contracted", 0xF120 },	  { "github", 0xF184 },
			};
			for ( const auto& icon : mimeIcons )
				iconTheme->add( UIGlyphIcon::New( icon.first, mimeIconFont, icon.second ) );
		}

		mUISceneNode->getUIIconThemeManager()->setCurrentTheme( iconTheme );

		UIWidgetCreator::registerWidget( "searchbar", UISearchBar::New );
		UIWidgetCreator::registerWidget( "locatebar", UILocateBar::New );
		UIWidgetCreator::registerWidget( "globalsearchbar", UIGlobalSearchBar::New );
		mUISceneNode->loadLayoutFromString( baseUI );
		mUISceneNode->bind( "main_layout", mMainLayout );
		mUISceneNode->bind( "code_container", mBaseLayout );
		mUISceneNode->bind( "image_container", mImageLayout );
		mUISceneNode->bind( "doc_info", mDocInfo );
		mUISceneNode->bind( "doc_info_text", mDocInfoText );
		mUISceneNode->bind( "panel", mSidePanel );
		mUISceneNode->bind( "project_splitter", mProjectSplitter );
		mUISceneNode->addEventListener( Event::KeyDown, [&]( const Event* event ) {
			trySendUnlockedCmd( *static_cast<const KeyEvent*>( event ) );
		} );
		mDocInfo->setVisible( mConfig.editor.showDocInfo );

		mProjectSplitter->setSplitPartition( StyleSheetLength( mConfig.window.panelPartition ) );
		if ( mConfig.ui.panelPosition == PanelPosition::Right )
			mProjectSplitter->swap();

		if ( !mConfig.ui.showSidePanel )
			showSidePanel( mConfig.ui.showSidePanel );

		auto colorSchemes(
			SyntaxColorScheme::loadFromFile( mResPath + "colorschemes/colorschemes.conf" ) );
		if ( FileSystem::isDirectory( mColorSchemesPath ) ) {
			auto colorSchemesFiles = FileSystem::filesGetInPath( mColorSchemesPath );
			for ( auto& file : colorSchemesFiles ) {
				auto colorSchemesInFile = SyntaxColorScheme::loadFromFile( file );
				for ( auto& coloScheme : colorSchemesInFile )
					colorSchemes.emplace_back( coloScheme );
			}
		}
		mSplitter = UICodeEditorSplitter::New( this, mUISceneNode, colorSchemes, mInitColorScheme );
		mSplitter->setHideTabBarOnSingleTab( mConfig.editor.hideTabBarOnSingleTab );

		Log::info( "Base UI took: %.2fms", globalClock.getElapsedTime().asMilliseconds() );

#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN
		mFileWatcher = new efsw::FileWatcher();
		mFileSystemListener = new FileSystemListener( mSplitter, mFileSystemModel );
		mFileWatcher->watch();
#endif

		mNotificationCenter = std::make_unique<NotificationCenter>(
			mUISceneNode->find<UILayout>( "notification_center" ) );

		mDocSearchController = std::make_unique<DocSearchController>( mSplitter, this );
		mDocSearchController->initSearchBar( mUISceneNode->find<UISearchBar>( "search_bar" ),
											 mConfig.searchBarConfig, mDocumentSearchKeybindings );

		mGlobalSearchController =
			std::make_unique<GlobalSearchController>( mSplitter, mUISceneNode, this );
		mGlobalSearchController->initGlobalSearchBar(
			mUISceneNode->find<UIGlobalSearchBar>( "global_search_bar" ),
			mConfig.globalSearchBarConfig, mGlobalSearchKeybindings );

		mFileLocator = std::make_unique<FileLocator>( mSplitter, mUISceneNode, this );
		mFileLocator->initLocateBar( mUISceneNode->find<UILocateBar>( "locate_bar" ),
									 mUISceneNode->find<UITextInput>( "locate_find" ) );

		initImageView();

		createSettingsMenu();

		mSplitter->createEditorWithTabWidget( mBaseLayout );

		mConsole = eeNew( Console, ( mFontMono, true, true, 1024 * 1000, 0, mWindow ) );

		Log::info( "Complete UI took: %.2fms", globalClock.getElapsedTime().asMilliseconds() );

#if EE_PLATFORM == EE_PLATFORM_EMSCRIPTEN
		if ( file == "./this.program" )
			file = "";
#endif

		if ( terminal && file.empty() ) {
			showSidePanel( false );
			createNewTerminal();
		} else {
			initProjectTreeView( file );
		}

		Log::info( "Init ProjectTreeView took: %.2fms",
				   globalClock.getElapsedTime().asMilliseconds() );

#if EE_PLATFORM == EE_PLATFORM_EMSCRIPTEN
		if ( file.empty() )
			downloadFileWeb( "https://raw.githubusercontent.com/SpartanJ/eepp/develop/README.md" );
#endif

		mWindow->runMainLoop( &appLoop );
	}
}

} // namespace ecode

using namespace ecode;

#if EE_PLATFORM == EE_PLATFORM_EMSCRIPTEN
std::vector<std::string> parseEmscriptenArgs( int argc, char* argv[] ) {
	if ( argc < 1 )
		return {};
	std::vector<std::string> args;
	args.emplace_back( argv[0] );
	for ( int i = 1; i < argc; i++ ) {
		auto split = String::split( std::string( argv[i] ), '=' );
		if ( split.size() == 2 ) {
			std::string arg( split[0] + "=" + URI::decode( split[1] ) );
			args.emplace_back( !String::startsWith( arg, "--" ) ? ( std::string( "--" ) + arg )
																: arg );
		}
	}
	return args;
}
#endif

EE_MAIN_FUNC int main( int argc, char* argv[] ) {
	args::ArgumentParser parser( "ecode" );
	args::HelpFlag help( parser, "help", "Display this help menu", { 'h', "help" } );
	args::Positional<std::string> file( parser, "file", "The file or folder path" );
	args::ValueFlag<std::string> filePos( parser, "file", "The file or folder path",
										  { 'f', "file", "folder" } );
	args::ValueFlag<Float> pixelDenstiyConf( parser, "pixel-density",
											 "Set default application pixel density",
											 { 'd', "pixel-density" } );
	args::ValueFlag<std::string> prefersColorScheme(
		parser, "prefers-color-scheme", "Set the preferred color scheme (\"light\" or \"dark\")",
		{ 'c', "prefers-color-scheme" } );
	args::Flag terminal( parser, "terminal", "Open a new terminal", { 't', "terminal" } );
	try {
#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN
		parser.ParseCLI( argc, argv );
#else
		parser.ParseCLI( parseEmscriptenArgs( argc, argv ) );
#endif
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

	appInstance = eeNew( App, () );
	appInstance->init( filePos ? filePos.Get() : file.Get(),
					   pixelDenstiyConf ? pixelDenstiyConf.Get() : 0.f,
					   prefersColorScheme ? prefersColorScheme.Get() : "", terminal.Get() );
	eeSAFE_DELETE( appInstance );

	Engine::destroySingleton();
	MemoryManager::showResults();

	return EXIT_SUCCESS;
}
