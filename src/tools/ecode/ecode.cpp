#include "ecode.hpp"
#include "plugins/autocomplete/autocompleteplugin.hpp"
#include "plugins/formatter/formatterplugin.hpp"
#include "plugins/linter/linterplugin.hpp"
#include <algorithm>
#include <args/args.hxx>

Clock globalClock;
bool firstFrame = true;
App* appInstance = nullptr;

void appLoop() {
	appInstance->mainLoop();
}

bool App::onCloseRequestCallback( EE::Window::Window* ) {
	if ( nullptr != mEditorSplitter->getCurEditor() &&
		 mEditorSplitter->getCurEditor()->isDirty() ) {
		UIMessageBox* msgBox = UIMessageBox::New(
			UIMessageBox::OK_CANCEL,
			i18n( "confirm_ecode_exit",
				  "Do you really want to close the code editor?\nAll changes will be lost." )
				.unescape() );
		msgBox->addEventListener( Event::MsgBoxConfirmClick, [&]( const Event* ) {
			if ( !mCurrentProject.empty() )
				mConfig.saveProject( mCurrentProject, mEditorSplitter, mConfigPath,
									 mProjectDocConfig );
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
			mConfig.saveProject( mCurrentProject, mEditorSplitter, mConfigPath, mProjectDocConfig );
		return true;
	}
}

void App::saveDoc() {
	if ( mEditorSplitter->getCurEditor()->getDocument().hasFilepath() ) {
		if ( mEditorSplitter->getCurEditor()->save() )
			updateEditorState();
	} else {
		saveFileDialog( mEditorSplitter->getCurEditor() );
	}
}

void App::saveAllProcess() {
	if ( mTmpDocs.empty() )
		return;

	mEditorSplitter->forEachEditorStoppable( [&]( UICodeEditor* editor ) {
		if ( editor->getDocument().isDirty() &&
			 std::find( mTmpDocs.begin(), mTmpDocs.end(), &editor->getDocument() ) !=
				 mTmpDocs.end() ) {
			if ( editor->getDocument().hasFilepath() ) {
				editor->save();
				updateEditorTabTitle( editor );
				if ( mEditorSplitter->getCurEditor() == editor )
					updateEditorTitle( editor );
				mTmpDocs.erase( &editor->getDocument() );
			} else {
				UIFileDialog* dialog = saveFileDialog( editor, false );
				dialog->addEventListener( Event::SaveFile, [&, editor]( const Event* ) {
					updateEditorTabTitle( editor );
					if ( mEditorSplitter->getCurEditor() == editor )
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
	mEditorSplitter->forEachEditor( [&]( UICodeEditor* editor ) {
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
		if ( mEditorSplitter && mEditorSplitter->getCurEditor() &&
			 !SceneManager::instance()->isShootingDown() )
			mEditorSplitter->getCurEditor()->setFocus();
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
		if ( mEditorSplitter && mEditorSplitter->getCurEditor() &&
			 !SceneManager::instance()->isShootingDown() )
			mEditorSplitter->getCurEditor()->setFocus();
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
	dialog->setTitle( "Select Font File" );
	dialog->setCloseShortcut( KEY_ESCAPE );
	dialog->addEventListener( Event::OnWindowClose, [&]( const Event* ) {
		if ( mEditorSplitter && mEditorSplitter->getCurEditor() &&
			 !SceneManager::instance()->isShootingDown() )
			mEditorSplitter->getCurEditor()->setFocus();
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
					mFontMono->setForceIsMonospace( true );
					if ( mEditorSplitter ) {
						mEditorSplitter->forEachEditor(
							[&]( UICodeEditor* editor ) { editor->setFont( mFontMono ); } );
					}
				};
				if ( !fontMono->isMonospace() ) {
					auto* msgBox = UIMessageBox::New(
						UIMessageBox::YES_NO,
						i18n(
							"confirm_loading_none_monospace_font",
							"The editor only supports monospaced fonts and the selected font isn't "
							"flagged as monospace.\nDo you want to load it anyways?" )
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

void App::downloadFileWeb( const std::string& url ) {
	loadFileFromPath( url, true );
}

UIFileDialog* App::saveFileDialog( UICodeEditor* editor, bool focusOnClose ) {
	if ( !editor )
		return nullptr;
	UIFileDialog* dialog =
		UIFileDialog::New( UIFileDialog::DefaultFlags | UIFileDialog::SaveDialog, "." );
	dialog->setWinFlags( UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON | UI_WIN_MODAL );
	dialog->setTitle( "Save File As" );
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
						UIMessageBox::New( UIMessageBox::OK, "Couldn't write the file." );
					msg->setTitle( "Error" );
					msg->show();
				}
			} else {
				UIMessageBox* msg =
					UIMessageBox::New( UIMessageBox::OK, "You must set a name to the file." );
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
	if ( mEditorSplitter->getCurEditor() )
		mEditorSplitter->getCurEditor()->getDocument().execute( command );
}

void App::loadConfig() {
	mConfigPath = Sys::getConfigPath( "ecode" );
	if ( !FileSystem::fileExists( mConfigPath ) )
		FileSystem::makeDir( mConfigPath );
	FileSystem::dirAddSlashAtEnd( mConfigPath );
	mPluginsPath = mConfigPath + "plugins";
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
	mConfig.save(
		mRecentFiles, mRecentFolders,
		mProjectSplitter ? mProjectSplitter->getSplitPartition().toString() : "15%", mWindow,
		mEditorSplitter ? mEditorSplitter->getCurrentColorSchemeName() : mConfig.editor.colorScheme,
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
	if ( mEditorSplitter->getCurEditor() ) {
		std::string cmd = mEditorSplitter->getCurEditor()->getKeyBindings().getCommandFromKeyBind(
			{ keyEvent.getKeyCode(), keyEvent.getMod() } );
		if ( !cmd.empty() && mEditorSplitter->getCurEditor()->isUnlockedCommand( cmd ) ) {
			mEditorSplitter->getCurEditor()->getDocument().execute( cmd );
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
	UICodeEditor* codeEditor = mEditorSplitter->getCurEditor();
	UITab* tab = nullptr;
	if ( !node )
		node = codeEditor;
	if ( node && node->isType( UI_TYPE_CODEEDITOR ) ) {
		codeEditor = node->asType<UICodeEditor>();
		if ( ( codeEditor->getDocument().isLoading() || !codeEditor->getDocument().isEmpty() ) &&
			 !Image::isImageExtension( file ) ) {
			auto d = mEditorSplitter->createCodeEditorInTabWidget(
				mEditorSplitter->tabWidgetFromEditor( codeEditor ) );
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
	UICodeEditor* codeEditor = mEditorSplitter->getCurEditor();
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
	eeSAFE_DELETE( mEditorSplitter );
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
	if ( mSettingsMenu && ( node = mSettingsMenu->getItem( "Recent Files" ) ) ) {
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
		menu->add( "Clear Menu" );
		menu->addEventListener( Event::OnItemClicked, [&]( const Event* event ) {
			if ( !event->getNode()->isType( UI_TYPE_MENUITEM ) )
				return;
			const String& txt = event->getNode()->asType<UIMenuItem>()->getText();
			if ( txt != "Clear Menu" ) {
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
	if ( mSettingsMenu && ( node = mSettingsMenu->getItem( "Recent Folders" ) ) ) {
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
		menu->add( "Clear Menu" );
		menu->addEventListener( Event::OnItemClicked, [&]( const Event* event ) {
			if ( !event->getNode()->isType( UI_TYPE_MENUITEM ) )
				return;
			const String& txt = event->getNode()->asType<UIMenuItem>()->getText();
			if ( txt != "Clear Menu" ) {
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
	mWindowMenu->getItem( "Show Side Panel" )
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

UIMenu* App::createWindowMenu() {
	mWindowMenu = UIPopUpMenu::New();
	UIPopUpMenu* colorsMenu = UIPopUpMenu::New();
	colorsMenu->addRadioButton( "Light", mUIColorScheme == ColorSchemePreference::Light )
		->setId( "light" );
	colorsMenu->addRadioButton( "Dark", mUIColorScheme == ColorSchemePreference::Dark )
		->setId( "dark" );
	colorsMenu->addEventListener( Event::OnItemClicked, [&]( const Event* event ) {
		if ( !event->getNode()->isType( UI_TYPE_MENUITEM ) )
			return;
		UIMenuItem* item = event->getNode()->asType<UIMenuItem>();
		setUIColorScheme( item->getId() == "light" ? ColorSchemePreference::Light
												   : ColorSchemePreference::Dark );
	} );
	mWindowMenu->addSubMenu( "UI Prefers Color Scheme", findIcon( "color-scheme" ), colorsMenu );
	mWindowMenu->add( "UI Scale Factor (Pixel Density)", findIcon( "pixel-density" ) );
	mWindowMenu->add( "UI Font Size", findIcon( "font-size" ) );
	mWindowMenu->add( "Editor Font Size", findIcon( "font-size" ) );
	mWindowMenu->add( "Serif Font...", findIcon( "font-size" ) );
	mWindowMenu->add( "Monospace Font...", findIcon( "font-size" ) );
	mWindowMenu->addSeparator();
	mWindowMenu->addCheckBox( "Full Screen Mode", false, getKeybind( "fullscreen-toggle" ) )
		->setId( "fullscreen-mode" );
	mWindowMenu->addCheckBox( "Show Side Panel", mConfig.ui.showSidePanel,
							  getKeybind( "switch-side-panel" ) );
	mWindowMenu->add( "Move panel to left...", findIcon( "layout-left" ),
					  getKeybind( "layout-left" ) );
	mWindowMenu->add( "Move panel to right...", findIcon( "layout-right" ),
					  getKeybind( "layout-rigth" ) );
	mWindowMenu->addSeparator();
	mWindowMenu->add( "Split Left", findIcon( "split-horizontal" ), getKeybind( "split-left" ) );
	mWindowMenu->add( "Split Right", findIcon( "split-horizontal" ), getKeybind( "split-right" ) );
	mWindowMenu->add( "Split Top", findIcon( "split-vertical" ), getKeybind( "split-top" ) );
	mWindowMenu->add( "Split Bottom", findIcon( "split-vertical" ), getKeybind( "split-bottom" ) );
	mWindowMenu->addSeparator();
	mWindowMenu->add( "Zoom In", findIcon( "zoom-in" ), getKeybind( "font-size-grow" ) );
	mWindowMenu->add( "Zoom Out", findIcon( "zoom-out" ), getKeybind( "font-size-shrink" ) );
	mWindowMenu->add( "Zoom Reset", findIcon( "zoom-reset" ), getKeybind( "font-size-reset" ) );
	mWindowMenu->addEventListener( Event::OnItemClicked, [&]( const Event* event ) {
		if ( !event->getNode()->isType( UI_TYPE_MENUITEM ) )
			return;
		UIMenuItem* item = event->getNode()->asType<UIMenuItem>();
		if ( item->getText() == "Show Side Panel" ) {
			mConfig.ui.showSidePanel = item->asType<UIMenuCheckBox>()->isActive();
			showSidePanel( mConfig.ui.showSidePanel );
		} else if ( item->getText() == "UI Scale Factor (Pixel Density)" ) {
			UIMessageBox* msgBox = UIMessageBox::New(
				UIMessageBox::INPUT, "Set the UI scale factor (pixel density):\nMinimum value is "
									 "1, and maximum 6. Requires restart." );
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
							"New UI scale factor assigned.\nPlease restart the application." );
						msg->show();
						setFocusEditorOnClose( msg );
					} else if ( mEditorSplitter && mEditorSplitter->getCurEditor() ) {
						mEditorSplitter->getCurEditor()->setFocus();
					}
				} else {
					UIMessageBox* msg = UIMessageBox::New( UIMessageBox::OK, "Invalid value!" );
					msg->show();
					setFocusEditorOnClose( msg );
				}
			} );
		} else if ( item->getText() == "Editor Font Size" ) {
			UIMessageBox* msgBox =
				UIMessageBox::New( UIMessageBox::INPUT, "Set the editor font size:" );
			msgBox->setTitle( mWindowTitle );
			msgBox->getTextInput()->setText( mConfig.editor.fontSize.toString() );
			msgBox->setCloseShortcut( { KEY_ESCAPE, 0 } );
			msgBox->showWhenReady();
			msgBox->addEventListener( Event::MsgBoxConfirmClick, [&, msgBox]( const Event* ) {
				mConfig.editor.fontSize = StyleSheetLength( msgBox->getTextInput()->getText() );
				mEditorSplitter->forEachEditor( [&]( UICodeEditor* editor ) {
					editor->setFontSize( mConfig.editor.fontSize.asDp( 0, Sizef(), mDisplayDPI ) );
				} );
			} );
			setFocusEditorOnClose( msgBox );
		} else if ( item->getText() == "UI Font Size" ) {
			UIMessageBox* msgBox =
				UIMessageBox::New( UIMessageBox::INPUT, "Set the UI font size:" );
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
		} else if ( item->getText() == "Serif Font..." ) {
			openFontDialog( mConfig.ui.serifFont, false );
		} else if ( item->getText() == "Monospace Font..." ) {
			openFontDialog( mConfig.ui.monospaceFont, true );
		} else if ( "Zoom In" == item->getText() ) {
			mEditorSplitter->zoomIn();
		} else if ( "Zoom Out" == item->getText() ) {
			mEditorSplitter->zoomOut();
		} else if ( "Zoom Reset" == item->getText() ) {
			mEditorSplitter->zoomReset();
		} else if ( "Full Screen Mode" == item->getText() ) {
			runCommand( "fullscreen-toggle" );
		} else if ( "Move panel to left..." == item->getText() ) {
			runCommand( "move-panel-left" );
		} else if ( "Move panel to right..." == item->getText() ) {
			runCommand( "move-panel-right" );
		} else {
			String text = String( event->getNode()->asType<UIMenuItem>()->getText() ).toLower();
			String::replaceAll( text, " ", "-" );
			String::replaceAll( text, "/", "-" );
			runCommand( text );
		}
	} );
	return mWindowMenu;
}

UIMenu* App::createViewMenu() {
	mViewMenu = UIPopUpMenu::New();
	mViewMenu->addCheckBox( "Show Line Numbers" )->setActive( mConfig.editor.showLineNumbers );
	mViewMenu->addCheckBox( "Show White Space" )->setActive( mConfig.editor.showWhiteSpaces );
	mViewMenu->addCheckBox( "Show Document Info" )->setActive( mConfig.editor.showDocInfo );
	mViewMenu->addCheckBox( "Show Minimap" )->setActive( mConfig.editor.minimap );
	mViewMenu->addCheckBox( "Highlight Matching Bracket" )
		->setActive( mConfig.editor.highlightMatchingBracket );
	mViewMenu->addCheckBox( "Highlight Current Line" )
		->setActive( mConfig.editor.highlightCurrentLine );
	mViewMenu->addCheckBox( "Highlight Selection Match" )
		->setActive( mConfig.editor.highlightSelectionMatch );
	mViewMenu->addCheckBox( "Enable Vertical ScrollBar" )
		->setActive( mConfig.editor.verticalScrollbar );
	mViewMenu->addCheckBox( "Enable Horizontal ScrollBar" )
		->setActive( mConfig.editor.horizontalScrollbar );
	mViewMenu->addCheckBox( "Enable Color Preview" )
		->setActive( mConfig.editor.colorPreview )
		->setTooltipText( "Enables a quick preview of a color when the mouse\n"
						  "is hover a word that represents a color." );
	mViewMenu->addCheckBox( "Enable Color Picker" )
		->setActive( mConfig.editor.colorPickerSelection )
		->setTooltipText( "Enables the color picker tool when a double click selection\n"
						  "is done over a word representing a color." );
	mViewMenu->addCheckBox( "Enable Auto Complete" )
		->setActive( mConfig.editor.autoComplete )
		->setTooltipText( "Auto complete shows the completion popup as you type, so you can fill\n"
						  "in long words by typing only a few characters." );
	mViewMenu->addCheckBox( "Enable Linter" )
		->setActive( mConfig.editor.linter )
		->setTooltipText( "Use static code analysis tool used to flag programming errors, bugs,\n"
						  "stylistic errors, and suspicious constructs." );
	mViewMenu->addCheckBox( "Enable Code Formatter" )
		->setActive( mConfig.editor.formatter )
		->setTooltipText( "Enables the code formatter/prettifier plugin." );
	mViewMenu->addCheckBox( "Hide tabbar on single tab" )
		->setActive( mConfig.editor.hideTabBarOnSingleTab )
		->setTooltipText( "Hides the tabbar if there's only one element in the tab widget." );
	mViewMenu->addCheckBox( "Single Click Navigation in Tree View" )
		->setActive( mConfig.editor.singleClickTreeNavigation )
		->setTooltipText(
			"Uses single click to open files and expand subfolders in\nthe directory tree." );
	mViewMenu->addCheckBox( "Synchronize project tree with editor" )
		->setActive( mConfig.editor.syncProjectTreeWithEditor )
		->setTooltipText( "Syncronizes the current focused document as the selected\nfile in the "
						  "directory tree." );

	mViewMenu->addEventListener( Event::OnItemClicked, [&]( const Event* event ) {
		if ( !event->getNode()->isType( UI_TYPE_MENUITEM ) )
			return;
		UIMenuItem* item = event->getNode()->asType<UIMenuItem>();
		if ( item->getText() == "Show Line Numbers" ) {
			mConfig.editor.showLineNumbers = item->asType<UIMenuCheckBox>()->isActive();
			mEditorSplitter->forEachEditor( [&]( UICodeEditor* editor ) {
				editor->setShowLineNumber( mConfig.editor.showLineNumbers );
			} );
		} else if ( item->getText() == "Show White Space" ) {
			mConfig.editor.showWhiteSpaces = item->asType<UIMenuCheckBox>()->isActive();
			mEditorSplitter->forEachEditor( [&]( UICodeEditor* editor ) {
				editor->setShowWhitespaces( mConfig.editor.showWhiteSpaces );
			} );
		} else if ( item->getText() == "Highlight Matching Bracket" ) {
			mConfig.editor.highlightMatchingBracket = item->asType<UIMenuCheckBox>()->isActive();
			mEditorSplitter->forEachEditor( [&]( UICodeEditor* editor ) {
				editor->setHighlightMatchingBracket( mConfig.editor.highlightMatchingBracket );
			} );
		} else if ( item->getText() == "Highlight Current Line" ) {
			mConfig.editor.highlightCurrentLine = item->asType<UIMenuCheckBox>()->isActive();
			mEditorSplitter->forEachEditor( [&]( UICodeEditor* editor ) {
				editor->setHighlightCurrentLine( mConfig.editor.highlightCurrentLine );
			} );
		} else if ( item->getText() == "Highlight Selection Match" ) {
			mConfig.editor.highlightSelectionMatch = item->asType<UIMenuCheckBox>()->isActive();
			mEditorSplitter->forEachEditor( [&]( UICodeEditor* editor ) {
				editor->setHighlightSelectionMatch( mConfig.editor.highlightSelectionMatch );
			} );
		} else if ( item->getText() == "Enable Vertical ScrollBar" ) {
			mConfig.editor.verticalScrollbar = item->asType<UIMenuCheckBox>()->isActive();
			mEditorSplitter->forEachEditor( [&]( UICodeEditor* editor ) {
				editor->setVerticalScrollBarEnabled( mConfig.editor.verticalScrollbar );
			} );
		} else if ( item->getText() == "Enable Horizontal ScrollBar" ) {
			mConfig.editor.horizontalScrollbar = item->asType<UIMenuCheckBox>()->isActive();
			mEditorSplitter->forEachEditor( [&]( UICodeEditor* editor ) {
				editor->setHorizontalScrollBarEnabled( mConfig.editor.horizontalScrollbar );
			} );
		} else if ( item->getText() == "Enable Color Picker" ) {
			mConfig.editor.colorPickerSelection = item->asType<UIMenuCheckBox>()->isActive();
			mEditorSplitter->forEachEditor( [&]( UICodeEditor* editor ) {
				editor->setEnableColorPickerOnSelection( mConfig.editor.colorPickerSelection );
			} );
		} else if ( item->getText() == "Show Minimap" ) {
			mConfig.editor.minimap = item->asType<UIMenuCheckBox>()->isActive();
			mEditorSplitter->forEachEditor(
				[&]( UICodeEditor* editor ) { editor->showMinimap( mConfig.editor.minimap ); } );
		} else if ( item->getText() == "Enable Auto Complete" ) {
			setAutoComplete( item->asType<UIMenuCheckBox>()->isActive() );
		} else if ( item->getText() == "Enable Linter" ) {
			setLinter( item->asType<UIMenuCheckBox>()->isActive() );
		} else if ( item->getText() == "Enable Code Formatter" ) {
			setFormatter( item->asType<UIMenuCheckBox>()->isActive() );
		} else if ( item->getText() == "Enable Color Preview" ) {
			mConfig.editor.colorPreview = item->asType<UIMenuCheckBox>()->isActive();
			mEditorSplitter->forEachEditor( [&]( UICodeEditor* editor ) {
				editor->setEnableColorPickerOnSelection( mConfig.editor.colorPreview );
			} );
		} else if ( item->getText() == "Show Document Info" ) {
			mConfig.editor.showDocInfo = item->asType<UIMenuCheckBox>()->isActive();
			if ( mDocInfo )
				mDocInfo->setVisible( mConfig.editor.showDocInfo );
			if ( mEditorSplitter->getCurEditor() )
				updateDocInfo( mEditorSplitter->getCurEditor()->getDocument() );
		} else if ( item->getText() == "Hide tabbar on single tab" ) {
			mConfig.editor.hideTabBarOnSingleTab = item->asType<UIMenuCheckBox>()->isActive();
			mEditorSplitter->setHideTabBarOnSingleTab( mConfig.editor.hideTabBarOnSingleTab );
		} else if ( item->getText() == "Single Click Navigation in Tree View" ) {
			mConfig.editor.singleClickTreeNavigation = item->asType<UIMenuCheckBox>()->isActive();
			mProjectTreeView->setSingleClickNavigation( mConfig.editor.singleClickTreeNavigation );
		} else if ( item->getText() == "Synchronize project tree with editor" ) {
			mConfig.editor.syncProjectTreeWithEditor = item->asType<UIMenuCheckBox>()->isActive();
		} else {
			String text = String( event->getNode()->asType<UIMenuItem>()->getText() ).toLower();
			String::replaceAll( text, " ", "-" );
			String::replaceAll( text, "/", "-" );
			runCommand( text );
		}
	} );
	return mViewMenu;
}

void App::setFocusEditorOnClose( UIMessageBox* msgBox ) {
	msgBox->addEventListener( Event::OnClose, [&]( const Event* ) {
		if ( mEditorSplitter && mEditorSplitter->getCurEditor() )
			mEditorSplitter->getCurEditor()->setFocus();
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
		if ( nullptr == mEditorSplitter->getCurEditor() )
			return;
		auto doc = mEditorSplitter->getCurEditor()->getDocumentRef();
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
		if ( mEditorSplitter->getCurEditor() ) {
			TextDocument::IndentType indentType = text == "tabs"
													  ? TextDocument::IndentType::IndentTabs
													  : TextDocument::IndentType::IndentSpaces;
			mEditorSplitter->getCurEditor()->getDocument().setIndentType( indentType );
		}
	} );

	UIPopUpMenu* indentWidthMenu = UIPopUpMenu::New();
	for ( size_t w = 2; w <= 12; w++ )
		indentWidthMenu
			->addRadioButton( String::toString( w ),
							  mEditorSplitter->getCurEditor() &&
								  mEditorSplitter->getCurEditor()->getDocument().getIndentWidth() ==
									  w )
			->setId( String::format( "indent_width_%zu", w ) )
			->setData( w );
	mDocMenu->addSubMenu( i18n( "indent_width", "Indent Width" ), nullptr, indentWidthMenu )
		->setId( "indent_width_cur" );
	indentWidthMenu->addEventListener( Event::OnItemClicked, [&]( const Event* event ) {
		if ( mEditorSplitter->getCurEditor() ) {
			int width = event->getNode()->getData();
			mEditorSplitter->getCurEditor()->getDocument().setIndentWidth( width );
		}
	} );

	UIPopUpMenu* tabWidthMenu = UIPopUpMenu::New();
	for ( size_t w = 2; w <= 12; w++ )
		tabWidthMenu
			->addRadioButton( String::toString( w ),
							  mEditorSplitter->getCurEditor() &&
								  mEditorSplitter->getCurEditor()->getTabWidth() == w )
			->setId( String::format( "tab_width_%zu", w ) )
			->setData( w );
	mDocMenu->addSubMenu( i18n( "tab_width", "Tab Width" ), nullptr, tabWidthMenu )
		->setId( "tab_width_cur" );
	tabWidthMenu->addEventListener( Event::OnItemClicked, [&]( const Event* event ) {
		if ( mEditorSplitter->getCurEditor() ) {
			int width = event->getNode()->getData();
			mEditorSplitter->getCurEditor()->setTabWidth( width );
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
		if ( mEditorSplitter->getCurEditor() ) {
			mEditorSplitter->getCurEditor()->getDocument().setLineEnding(
				winLe ? TextDocument::LineEnding::CRLF : TextDocument::LineEnding::LF );
			updateDocInfo( mEditorSplitter->getCurEditor()->getDocument() );
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
		if ( !mEditorSplitter->getCurEditor() ||
			 event->getNode()->isType( UI_TYPE_MENU_SEPARATOR ) ||
			 event->getNode()->isType( UI_TYPE_MENUSUBMENU ) )
			return;
		const String& id = event->getNode()->getId();
		TextDocument& doc = mEditorSplitter->getCurEditor()->getDocument();

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
				mEditorSplitter->getCurEditor()->setLocked( item->isActive() );
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
				mEditorSplitter->forEachEditor( [&]( UICodeEditor* editor ) {
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
			mEditorSplitter->forEachEditor( [&, pairs]( UICodeEditor* editor ) {
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
		if ( !mEditorSplitter->getCurEditor() ||
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
					mEditorSplitter->forEachEditor(
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
		if ( !mEditorSplitter->getCurEditor() ||
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
					mEditorSplitter->forEachEditor(
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

	mEditorSplitter->forEachEditor( [&]( UICodeEditor* editor ) {
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
	if ( !mEditorSplitter->getCurEditor() )
		return;

	const TextDocument& doc = mEditorSplitter->getCurEditor()->getDocument();

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
		->find( String::format( "tab_width_%d", mEditorSplitter->getCurEditor()->getTabWidth() ) )
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
		->setActive( mEditorSplitter->getCurEditor()->isLocked() );
}

static void
updateKeybindings( IniFile& ini, const std::string& group, Input* input,
				   std::unordered_map<std::string, std::string>& keybindings,
				   const std::unordered_map<std::string, std::string>& defKeybindings ) {
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

	if ( defKeybindings.size() != keybindings.size() ) {
		for ( const auto& key : defKeybindings ) {
			auto foundCmd = invertedKeybindings.find( key.second );
			auto& shortcutStr = key.first;
			if ( foundCmd == invertedKeybindings.end() &&
				 keybindings.find( shortcutStr ) == keybindings.end() ) {
				keybindings[shortcutStr] = key.second;
				invertedKeybindings[key.second] = shortcutStr;
				ini.setValue( group, shortcutStr, key.second );
				added = true;
			}
		}
	}
	if ( added )
		ini.writeFile();
}

static void
updateKeybindings( IniFile& ini, const std::string& group, Input* input,
				   std::unordered_map<std::string, std::string>& keybindings,
				   std::unordered_map<std::string, std::string>& invertedKeybindings,
				   const std::map<KeyBindings::Shortcut, std::string>& defKeybindings ) {
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

	if ( defKeybindings.size() != keybindings.size() ) {
		for ( const auto& key : defKeybindings ) {
			auto foundCmd = invertedKeybindings.find( key.second );
			auto shortcutStr = bindings.getShortcutString( key.first );
			if ( foundCmd == invertedKeybindings.end() &&
				 keybindings.find( shortcutStr ) == keybindings.end() ) {
				keybindings[shortcutStr] = key.second;
				invertedKeybindings[key.second] = shortcutStr;
				ini.setValue( group, shortcutStr, key.second );
				added = true;
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

		Uint32 defModKeyCode = KeyMod::getKeyMod( defMod );
		if ( 0 != defModKeyCode )
			KeyMod::setDefaultModifier( defModKeyCode );

		updateKeybindings( ini, "editor", mWindow->getInput(), mKeybindings, mKeybindingsInvert,
						   getDefaultKeybindings() );

		updateKeybindings( ini, "global_search", mWindow->getInput(), mGlobalSearchKeybindings,
						   GlobalSearchController::getDefaultKeybindings() );

		updateKeybindings( ini, "document_search", mWindow->getInput(), mDocumentSearchKeybindings,
						   DocSearchController::getDefaultKeybindings() );
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
	if ( mConfig.editor.showDocInfo && mDocInfoText && mEditorSplitter->getCurEditor() ) {
		mDocInfoText->setText( String::format(
			"line: %lld / %lu  col: %lld    %s", doc.getSelection().start().line() + 1,
			doc.linesCount(), mEditorSplitter->getCurEditor()->getCurrentColumnCount(),
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

void App::onCodeEditorFocusChange( UICodeEditor* editor ) {
	updateDocInfo( editor->getDocument() );
	updateDocumentMenu();
	mDocSearchController->onCodeEditorFocusChange( editor );
	syncProjectTreeWithEditor( editor );
}

void App::onColorSchemeChanged( const std::string& ) {
	updateColorSchemeMenu();
	mGlobalSearchController->updateColorScheme( mEditorSplitter->getCurrentColorScheme() );
}

void App::onDocumentLoaded( UICodeEditor* editor, const std::string& path ) {
	updateEditorTitle( editor );
	if ( editor == mEditorSplitter->getCurEditor() )
		updateCurrentFileType();
	mEditorSplitter->removeUnusedTab( mEditorSplitter->tabWidgetFromEditor( editor ) );
	auto found = std::find( mRecentFiles.begin(), mRecentFiles.end(), path );
	if ( found != mRecentFiles.end() )
		mRecentFiles.erase( found );
	mRecentFiles.insert( mRecentFiles.begin(), path );
	if ( mRecentFiles.size() > 10 )
		mRecentFiles.resize( 10 );
	updateRecentFiles();
	if ( mEditorSplitter->getCurEditor() == editor ) {
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

std::map<KeyBindings::Shortcut, std::string> App::getDefaultKeybindings() {
	auto bindings = UICodeEditorSplitter::getDefaultKeybindings();
	auto local = getLocalKeybindings();
	local.insert( bindings.begin(), bindings.end() );
	return local;
}

std::map<KeyBindings::Shortcut, std::string> App::getLocalKeybindings() {
	return { { { KEY_RETURN, KEYMOD_LALT }, "fullscreen-toggle" },
			 { { KEY_F3, KEYMOD_NONE }, "repeat-find" },
			 { { KEY_F3, KEYMOD_SHIFT }, "find-prev" },
			 { { KEY_F12, KEYMOD_NONE }, "console-toggle" },
			 { { KEY_F, KeyMod::getDefaultModifier() }, "find-replace" },
			 { { KEY_Q, KeyMod::getDefaultModifier() }, "close-app" },
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
			 { { KEY_F, KEYMOD_LALT }, "format-doc" } };
}

std::vector<std::string> App::getUnlockedCommands() {
	return { "fullscreen-toggle", "open-file",		  "open-folder",		"console-toggle",
			 "close-app",		  "open-locatebar",	  "open-global-search", "menu-toggle",
			 "switch-side-panel", "download-file-web" };
}

void App::closeEditors() {
	mConfig.saveProject( mCurrentProject, mEditorSplitter, mConfigPath, mProjectDocConfig );
	std::vector<UICodeEditor*> editors = mEditorSplitter->getAllEditors();
	for ( auto editor : editors ) {
		UITabWidget* tabWidget = mEditorSplitter->tabWidgetFromEditor( editor );
		tabWidget->removeTab( (UITab*)editor->getData(), true, true );
	}
	mCurrentProject = "";
	mDirTree = nullptr;
	if ( mFileSystemListener )
		mFileSystemListener->setDirTree( mDirTree );

	mProjectDocConfig = ProjectDocumentConfig( mConfig.doc );
	updateProjectSettingsMenu();
	mEditorSplitter->createCodeEditorInTabWidget( mEditorSplitter->getTabWidgets()[0] );
}

void App::closeFolder() {
	if ( mCurrentProject.empty() )
		return;

	if ( mEditorSplitter->isAnyEditorDirty() ) {
		UIMessageBox* msgBox = UIMessageBox::New(
			UIMessageBox::OK_CANCEL,
			"Do you really want to close the folder?\nSome files haven't been saved." );
		msgBox->addEventListener( Event::MsgBoxConfirmClick,
								  [&]( const Event* ) { closeEditors(); } );
		msgBox->addEventListener( Event::OnClose, [&]( const Event* ) { msgBox = nullptr; } );
		msgBox->setTitle( "Close Folder?" );
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
			mEditorSplitter->loadAsyncFileFromPathInNewTab( path, mThreadPool, onLoaded );
		} else {
			mEditorSplitter->loadAsyncFileFromPath( path, mThreadPool, codeEditor, onLoaded );
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

void App::onCodeEditorCreated( UICodeEditor* editor, TextDocument& doc ) {
	const CodeEditorConfig& config = mConfig.editor;
	const DocumentConfig& docc = !mCurrentProject.empty() && !mProjectDocConfig.useGlobalSettings
									 ? mProjectDocConfig.doc
									 : mConfig.doc;
	editor->setFontSize( config.fontSize.asDp( 0, Sizef(), mUISceneNode->getDPI() ) );
	editor->setEnableColorPickerOnSelection( true );
	editor->setColorScheme( mEditorSplitter->getCurrentColorScheme() );
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
	doc.setCommand( "save-as-doc", [&] { saveFileDialog( mEditorSplitter->getCurEditor() ); } );
	doc.setCommand( "save-all", [&] { saveAll(); } );
	doc.setCommand( "find-replace", [&] { mDocSearchController->showFindView(); } );
	doc.setCommand( "open-global-search", [&] {
		mGlobalSearchController->showGlobalSearch(
			mGlobalSearchController->isUsingSearchReplaceTree() );
	} );
	doc.setCommand( "open-locatebar", [&] { mFileLocator->showLocateBar(); } );
	doc.setCommand( "repeat-find", [&] {
		mDocSearchController->findNextText( mDocSearchController->getSearchState() );
	} );
	doc.setCommand( "find-prev", [&] {
		mDocSearchController->findPrevText( mDocSearchController->getSearchState() );
	} );
	doc.setCommand( "close-folder", [&] { closeFolder(); } );
	doc.setCommand( "close-app", [&] { closeApp(); } );
	doc.setCommand( "fullscreen-toggle", [&]() {
		mWindow->toggleFullscreen();
		mWindowMenu->find( "fullscreen-mode" )
			->asType<UIMenuCheckBox>()
			->setActive( !mWindow->isWindowed() );
	} );
	doc.setCommand( "open-file", [&] { openFileDialog(); } );
	doc.setCommand( "open-folder", [&] { openFolderDialog(); } );
	doc.setCommand( "console-toggle", [&] {
		mConsole->toggle();
		bool lock = mConsole->isActive();
		mEditorSplitter->forEachEditor(
			[lock]( UICodeEditor* editor ) { editor->setLocked( lock ); } );
	} );
	doc.setCommand( "lock", [&] {
		if ( mEditorSplitter->getCurEditor() ) {
			mEditorSplitter->getCurEditor()->setLocked( true );
			updateDocumentMenu();
		}
	} );
	doc.setCommand( "unlock", [&] {
		if ( mEditorSplitter->getCurEditor() ) {
			mEditorSplitter->getCurEditor()->setLocked( false );
			updateDocumentMenu();
		}
	} );
	doc.setCommand( "lock-toggle", [&] {
		if ( mEditorSplitter->getCurEditor() ) {
			mEditorSplitter->getCurEditor()->setLocked(
				!mEditorSplitter->getCurEditor()->isLocked() );
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
	doc.setCommand( "download-file-web", [&] {
		UIMessageBox* msgBox =
			UIMessageBox::New( UIMessageBox::INPUT, "Please enter the file URL..." );

		msgBox->setTitle( mWindowTitle );
		msgBox->getTextInput()->setHint( "Any https or http URL" );
		msgBox->setCloseShortcut( { KEY_ESCAPE, 0 } );
		msgBox->showWhenReady();
		msgBox->addEventListener( Event::MsgBoxConfirmClick, [&, msgBox]( const Event* ) {
			std::string url( msgBox->getTextInput()->getText().toUtf8() );
			downloadFileWeb( url );
			if ( mEditorSplitter->getCurEditor() )
				mEditorSplitter->getCurEditor()->setFocus();
			msgBox->closeWindow();
		} );
	} );
	doc.setCommand( "move-panel-left", [&] { panelPosition( PanelPosition::Left ); } );
	doc.setCommand( "move-panel-right", [&] { panelPosition( PanelPosition::Right ); } );

	editor->addEventListener( Event::OnDocumentSave, [&]( const Event* event ) {
		UICodeEditor* editor = event->getNode()->asType<UICodeEditor>();
		updateEditorTabTitle( editor );
		if ( mEditorSplitter->getCurEditor() == editor )
			editor->setFocus();
		if ( editor->getDocument().getFilePath() == mKeybindingsPath ) {
			mKeybindings.clear();
			mKeybindingsInvert.clear();
			loadKeybindings();
			mEditorSplitter->forEachEditor( [&]( UICodeEditor* ed ) {
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
		mEditorSplitter->forEachEditor(
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
		mEditorSplitter->forEachEditor(
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
		mEditorSplitter->forEachEditor(
			[&]( UICodeEditor* editor ) { editor->registerPlugin( mFormatterPlugin ); } );
		return true;
	}
	if ( !enable && mFormatterPlugin )
		eeSAFE_DELETE( mFormatterPlugin );
	return false;
}

void App::loadCurrentDirectory() {
	if ( !mEditorSplitter->getCurEditor() )
		return;
	std::string path( mEditorSplitter->getCurEditor()->getDocument().getFilePath() );
	if ( path.empty() )
		return;
	path = FileSystem::fileRemoveFileName( path );
	if ( !FileSystem::isDirectory( path ) )
		return;
	loadFolder( path );
}

UIPopUpMenu* App::createToolsMenu() {
	mToolsMenu = UIPopUpMenu::New();
	mToolsMenu->add( "Locate...", findIcon( "search" ), getKeybind( "open-locatebar" ) );
	mToolsMenu->add( "Project Find...", findIcon( "search" ), getKeybind( "open-global-search" ) );
	mToolsMenu->add( "Go to line...", findIcon( "go-to-line" ), getKeybind( "go-to-line" ) );
	mToolsMenu->add( "Load current document directory as folder", findIcon( "folder" ),
					 getKeybind( "load-current-dir" ) );
	mToolsMenu->addEventListener( Event::OnItemClicked, [&]( const Event* event ) {
		if ( !event->getNode()->isType( UI_TYPE_MENUITEM ) )
			return;
		UIMenuItem* item = event->getNode()->asType<UIMenuItem>();
		std::string txt( item->getText() );
		if ( txt == "Locate..." ) {
			mFileLocator->showLocateBar();
		} else if ( txt == "Project Find..." ) {
			mGlobalSearchController->showGlobalSearch();
		} else if ( txt == "Go to line..." ) {
			mFileLocator->goToLine();
		} else if ( txt == "Load current document directory as folder" ) {
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
	mSettingsMenu->add( "New", findIcon( "document-new" ), getKeybind( "create-new" ) );
	mSettingsMenu->add( "Open File...", findIcon( "document-open" ), getKeybind( "open-file" ) );
	mSettingsMenu->add( "Open Folder...", findIcon( "document-open" ),
						getKeybind( "open-folder" ) );
	mSettingsMenu->add( "Open File from Web...", findIcon( "download-cloud" ),
						getKeybind( "download-file-web" ) );
	mSettingsMenu->addSubMenu( "Recent Files", findIcon( "document-recent" ), UIPopUpMenu::New() );
	mSettingsMenu->addSubMenu( "Recent Folders", findIcon( "document-recent" ),
							   UIPopUpMenu::New() );
	mSettingsMenu->addSeparator();
	mSettingsMenu->add( "Save", findIcon( "document-save" ), getKeybind( "save-doc" ) );
	mSettingsMenu->add( "Save as...", findIcon( "document-save-as" ), getKeybind( "save-as-doc" ) );
	mSettingsMenu->add( "Save All", findIcon( "document-save-as" ), getKeybind( "save-all" ) );
	mSettingsMenu->addSeparator();
	UIMenuSubMenu* fileTypeMenu =
		mSettingsMenu->addSubMenu( "File Type", nullptr, createFileTypeMenu() );
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
		mSettingsMenu->addSubMenu( "Color Scheme", nullptr, createColorSchemeMenu() );
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
	mSettingsMenu->addSubMenu( "Document", nullptr, createDocumentMenu() );
	mSettingsMenu->addSubMenu( "Edit", nullptr, createEditMenu() );
	mSettingsMenu->addSubMenu( "View", nullptr, createViewMenu() );
	mSettingsMenu->addSubMenu( "Tools", nullptr, createToolsMenu() );
	mSettingsMenu->addSubMenu( "Window", nullptr, createWindowMenu() );
	mSettingsMenu->addSeparator();
	mSettingsMenu->add( "Close", findIcon( "document-close" ), getKeybind( "close-doc" ) );
	mSettingsMenu->add( "Close Folder", findIcon( "document-close" ),
						getKeybind( "close-folder" ) );
	mSettingsMenu->addSeparator();
	mSettingsMenu->add( "Quit", findIcon( "quit" ), getKeybind( "close-app" ) );
	mSettingsButton = mUISceneNode->find<UITextView>( "settings" );
	mSettingsButton->addEventListener( Event::MouseClick,
									   [&]( const Event* ) { toggleSettingsMenu(); } );
	mSettingsMenu->addEventListener( Event::OnItemClicked, [&]( const Event* event ) {
		if ( !event->getNode()->isType( UI_TYPE_MENUITEM ) )
			return;
		const String& name = event->getNode()->asType<UIMenuItem>()->getText();
		if ( name == "New" ) {
			runCommand( "create-new" );
		} else if ( name == "Open File..." ) {
			runCommand( "open-file" );
		} else if ( name == "Open Folder..." ) {
			runCommand( "open-folder" );
		} else if ( name == "Open File from Web..." ) {
			runCommand( "download-file-web" );
		} else if ( name == "Save" ) {
			runCommand( "save-doc" );
		} else if ( name == "Save as..." ) {
			runCommand( "save-as-doc" );
		} else if ( name == "Save All" ) {
			runCommand( "save-all" );
		} else if ( name == "Close" ) {
			runCommand( "close-doc" );
		} else if ( name == "Close Folder" ) {
			runCommand( "close-folder" );
		} else if ( name == "Quit" ) {
			runCommand( "close-app" );
		}
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
				menuItem->setActive( mEditorSplitter->getCurrentColorSchemeName() ==
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
		mEditorSplitter->setColorScheme( name );
	};

	UIPopUpMenu* menu = UIPopUpMenu::New();
	menu->addEventListener( Event::OnItemClicked, cb );
	mColorSchemeMenues.push_back( menu );
	size_t total = 0;
	const auto& colorSchemes = mEditorSplitter->getColorSchemes();

	for ( auto& colorScheme : colorSchemes ) {
		menu->addRadioButton( colorScheme.first,
							  mEditorSplitter->getCurrentColorSchemeName() == colorScheme.first );

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
		if ( mEditorSplitter->getCurEditor() ) {
			mEditorSplitter->getCurEditor()->setSyntaxDefinition(
				dM->getStyleByLanguageName( name ) );
			updateCurrentFileType();
		}
	};

	UIPopUpMenu* menu = UIPopUpMenu::New();
	menu->addEventListener( Event::OnItemClicked, cb );
	mFileTypeMenues.push_back( menu );
	size_t total = 0;

	for ( const auto& name : names ) {
		menu->addRadioButton(
			name,
			mEditorSplitter->getCurEditor() &&
				mEditorSplitter->getCurEditor()->getSyntaxDefinition().getLanguageName() == name );

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
	if ( !mEditorSplitter->getCurEditor() )
		return;
	std::string curLang( mEditorSplitter->getCurEditor()->getSyntaxDefinition().getLanguageName() );
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
	if ( mEditorSplitter->getCurEditor() ) {
		updateEditorTitle( mEditorSplitter->getCurEditor() );
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
				syncProjectTreeWithEditor( mEditorSplitter->getCurEditor() );
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

UIMessageBox* errorMsgBox( const String& msg ) {
	UIMessageBox* msgBox = UIMessageBox::New( UIMessageBox::OK, msg );
	msgBox->setTitle( "Error" );
	msgBox->showWhenReady();
	return msgBox;
}

UIMessageBox* fileAlreadyExistsMsgBox() {
	return errorMsgBox( "File already exists!" );
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

void renameFile( const FileInfo& file ) {
	if ( !file.exists() )
		return;
	UIMessageBox* msgBox =
		newInputMsgBox( "Rename file \"" + file.getFileName() + "\"", "Enter new file name:" );
	msgBox->getTextInput()->setText( file.getFileName() );
	msgBox->addEventListener( Event::MsgBoxConfirmClick, [file, msgBox]( const Event* ) {
		auto newFilePath( getNewFilePath( file, msgBox ) );
		if ( !FileSystem::fileExists( newFilePath ) ) {
			if ( 0 != std::rename( file.getFilepath().c_str(), newFilePath.c_str() ) )
				errorMsgBox( "Error renaming file." );
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
		mProjectTreeMenu->add( "New File...", findIcon( "file-add" ) )->setId( "new_file" );
		mProjectTreeMenu->add( "New Folder...", findIcon( "folder-add" ) )->setId( "new_folder" );
		mProjectTreeMenu->add( "Open Folder...", findIcon( "folder-open" ) )
			->setId( "open_folder" );
	} else {
		mProjectTreeMenu->add( "Open File", findIcon( "document-open" ) )->setId( "open_file" );
		mProjectTreeMenu->add( "Open Containing Folder...", findIcon( "folder-open" ) )
			->setId( "open_containing_folder" );
		mProjectTreeMenu->add( "New File in directory...", findIcon( "file-add" ) )
			->setId( "new_file_in_place" );
		mProjectTreeMenu->add( "Duplicate File...", findIcon( "file-copy" ) )
			->setId( "duplicate_file" );
	}
	mProjectTreeMenu->add( "Rename", findIcon( "edit" ), "F2" )->setId( "rename" );
	mProjectTreeMenu->add( "Remove...", findIcon( "delete-bin" ) )->setId( "remove" );
	mProjectTreeMenu->addEventListener( Event::OnItemClicked, [&, file]( const Event* event ) {
		if ( !event->getNode()->isType( UI_TYPE_MENUITEM ) )
			return;
		UIMenuItem* item = event->getNode()->asType<UIMenuItem>();
		std::string txt( item->getId() );

		if ( "new_file" == txt || "new_file_in_place" == txt ) {
			UIMessageBox* msgBox = newInputMsgBox( "Create new file", "Enter new file name:" );
			msgBox->addEventListener( Event::MsgBoxConfirmClick, [file, msgBox]( const Event* ) {
				auto newFilePath( getNewFilePath( file, msgBox ) );
				if ( !FileSystem::fileExists( newFilePath ) ) {
					if ( !FileSystem::fileWrite( newFilePath, nullptr, 0 ) )
						errorMsgBox( "Couldn't create file." );
					msgBox->closeWindow();
				} else {
					fileAlreadyExistsMsgBox();
				}
			} );
		} else if ( "new_folder" == txt ) {
			UIMessageBox* msgBox = newInputMsgBox( "Create new folder", "Enter new folder name:" );
			msgBox->addEventListener( Event::MsgBoxConfirmClick, [file, msgBox]( const Event* ) {
				auto newFolderPath( getNewFilePath( file, msgBox ) );
				if ( !FileSystem::fileExists( newFolderPath ) ) {
					if ( !FileSystem::makeDir( newFolderPath ) )
						errorMsgBox( "Couldn't create directory." );
					msgBox->closeWindow();
				} else {
					fileAlreadyExistsMsgBox();
				}
			} );
		} else if ( "open_file" == txt ) {
			loadFileFromPath( file.getFilepath() );
		} else if ( "remove" == txt ) {
			if ( file.isDirectory() && !FileSystem::filesGetInPath( file.getFilepath() ).empty() ) {
				errorMsgBox( "Cannot remove non-empty directory." );
				return;
			}

			UIMessageBox* msgBox =
				UIMessageBox::New( UIMessageBox::OK_CANCEL,
								   "Do you really want to remove \"" + file.getFileName() + "\"?" );
			msgBox->addEventListener( Event::MsgBoxConfirmClick, [file, msgBox]( const Event* ) {
				if ( !FileSystem::fileRemove( file.getFilepath() ) ) {
					errorMsgBox( String::format( "Couldn't remove %s.",
												 file.isDirectory() ? "directory" : "file" ) );
				}
				msgBox->closeWindow();
			} );
			msgBox->setTitle( "Remove file?" );
			msgBox->center();
			msgBox->showWhenReady();
		} else if ( "duplicate_file" == txt ) {
			UIMessageBox* msgBox = newInputMsgBox( "Duplicate file \"" + file.getFileName() + "\"",
												   "Enter duplicate file name:" );
			msgBox->addEventListener( Event::MsgBoxConfirmClick, [file, msgBox]( const Event* ) {
				auto newFilePath( getNewFilePath( file, msgBox ) );
				if ( !FileSystem::fileExists( newFilePath ) ) {
					if ( !FileSystem::fileCopy( file.getFilepath(), newFilePath ) )
						errorMsgBox( "Error copying file." );
					msgBox->closeWindow();
				} else {
					fileAlreadyExistsMsgBox();
				}
			} );
		} else if ( "rename" == txt ) {
			renameFile( file );
		} else if ( "open_containing_folder" == txt ) {
			Engine::instance()->openURL( file.getDirectoryPath() );
		} else if ( "open_folder" == txt ) {
			Engine::instance()->openURL( file.getFilepath() );
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
					UITab* tab = mEditorSplitter->isDocumentOpen( path );
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

		if ( mEditorSplitter->getCurEditor() ) {
			std::string cmd =
				mEditorSplitter->getCurEditor()->getKeyBindings().getCommandFromKeyBind(
					{ keyEvent->getKeyCode(), keyEvent->getMod() } );
			if ( !cmd.empty() && mEditorSplitter->getCurEditor()->isUnlockedCommand( cmd ) ) {
				mEditorSplitter->getCurEditor()->getDocument().execute( cmd );
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

	mConfig.loadProject( rpath, mEditorSplitter, mConfigPath, mProjectDocConfig, mThreadPool );

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

	if ( mEditorSplitter->getCurEditor() )
		mEditorSplitter->getCurEditor()->setFocus();
}

FontTrueType* App::loadFont( const std::string& name, std::string fontPath,
							 const std::string& fallback ) {
	if ( FileSystem::isRelativePath( fontPath ) )
		fontPath = mResPath + fontPath;
	if ( fontPath.empty() || !FileSystem::fileExists( fontPath ) )
		fontPath = fallback;
	return FontTrueType::New( name, fontPath );
}

void App::init( std::string file, const Float& pidelDensity, const std::string& colorScheme ) {
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

	if ( mWindow->isOpen() ) {
		Log::info( "Window creation took: %.2fms", globalClock.getElapsedTime().asMilliseconds() );

		if ( mConfig.window.position != Vector2i( -1, -1 ) &&
			 mConfig.window.displayIndex < displayManager->getDisplayCount() )
			mWindow->setPosition( mConfig.window.position.x, mConfig.window.position.y );

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
		if ( mFontMono ) {
			mFontMono->setBoldAdvanceSameAsRegular( true );
			mFontMono->setForceIsMonospace( true );
		}
		loadFont( "NotoEmoji-Regular", "fonts/NotoEmoji-Regular.ttf" );

#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN
		loadFont( "NotoColorEmoji", "fonts/NotoColorEmoji.ttf" );
#endif

		FontTrueType* iconFont = FontTrueType::New( "icon", mResPath + "fonts/remixicon.ttf" );

		if ( !mFont || !mFontMono || !iconFont ) {
			printf( "Font not found!" );
			return;
		}

		FontTrueType* mimeIconFont =
			FontTrueType::New( "nonicons", mResPath + "fonts/nonicons.ttf" );

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
				{ "tree-contracted", 0xF120 },
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

		mEditorSplitter = UICodeEditorSplitter::New(
			this, mUISceneNode,
			SyntaxColorScheme::loadFromFile( mResPath + "colorschemes/colorschemes.conf" ),
			mInitColorScheme );
		mEditorSplitter->setHideTabBarOnSingleTab( mConfig.editor.hideTabBarOnSingleTab );

		Log::info( "Base UI took: %.2fms", globalClock.getElapsedTime().asMilliseconds() );

#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN
		mFileWatcher = new efsw::FileWatcher();
		mFileSystemListener = new FileSystemListener( mEditorSplitter, mFileSystemModel );
		mFileWatcher->watch();
#endif

		mNotificationCenter = std::make_unique<NotificationCenter>(
			mUISceneNode->find<UILayout>( "notification_center" ) );

		mDocSearchController = std::make_unique<DocSearchController>( mEditorSplitter, this );
		mDocSearchController->initSearchBar( mUISceneNode->find<UISearchBar>( "search_bar" ),
											 mConfig.searchBarConfig, mDocumentSearchKeybindings );

		mGlobalSearchController =
			std::make_unique<GlobalSearchController>( mEditorSplitter, mUISceneNode, this );
		mGlobalSearchController->initGlobalSearchBar(
			mUISceneNode->find<UIGlobalSearchBar>( "global_search_bar" ),
			mConfig.globalSearchBarConfig, mGlobalSearchKeybindings );

		mFileLocator = std::make_unique<FileLocator>( mEditorSplitter, mUISceneNode, this );
		mFileLocator->initLocateBar( mUISceneNode->find<UILocateBar>( "locate_bar" ),
									 mUISceneNode->find<UITextInput>( "locate_find" ) );

		initImageView();

		createSettingsMenu();

		mEditorSplitter->createEditorWithTabWidget( mBaseLayout );

		mConsole = eeNew( Console, ( mFontMono, true, true, 1024 * 1000, 0, mWindow ) );

		Log::info( "Complete UI took: %.2fms", globalClock.getElapsedTime().asMilliseconds() );

#if EE_PLATFORM == EE_PLATFORM_EMSCRIPTEN
		if ( file == "./this.program" )
			file = "";
#endif

		initProjectTreeView( file );

		Log::info( "Init ProjectTreeView took: %.2fms",
				   globalClock.getElapsedTime().asMilliseconds() );

#if EE_PLATFORM == EE_PLATFORM_EMSCRIPTEN
		if ( file.empty() )
			downloadFileWeb( "https://raw.githubusercontent.com/SpartanJ/eepp/develop/README.md" );
#endif

		mWindow->runMainLoop( &appLoop );
	}
}

#if EE_PLATFORM == EE_PLATFORM_EMSCRIPTEN
std::vector<std::string> parseEmscriptenArgs( int argc, char* argv[] ) {
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
					   prefersColorScheme ? prefersColorScheme.Get() : "" );
	eeSAFE_DELETE( appInstance );

	Engine::destroySingleton();
	MemoryManager::showResults();

	return EXIT_SUCCESS;
}
