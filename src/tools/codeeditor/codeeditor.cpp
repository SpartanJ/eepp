#include "codeeditor.hpp"
#include "autocompletemodule.hpp"
#include "formattermodule.hpp"
#include "lintermodule.hpp"
#include <algorithm>
#include <args/args.hxx>

App* appInstance = nullptr;

void appLoop() {
	appInstance->mainLoop();
}

bool App::onCloseRequestCallback( EE::Window::Window* ) {
	if ( nullptr != mEditorSplitter->getCurEditor() &&
		 mEditorSplitter->getCurEditor()->isDirty() ) {
		UIMessageBox* msgBox = UIMessageBox::New(
			UIMessageBox::OK_CANCEL,
			"Do you really want to close the code editor?\nAll changes will be lost." );
		msgBox->addEventListener( Event::MsgBoxConfirmClick, [&]( const Event* ) {
			if ( !mCurrentProject.empty() )
				mConfig.saveProject( mCurrentProject, mEditorSplitter, mConfigPath );
			mWindow->close();
		} );
		msgBox->addEventListener( Event::OnClose, [&]( const Event* ) { msgBox = nullptr; } );
		msgBox->setTitle( "Close " + mWindowTitle + "?" );
		msgBox->center();
		msgBox->showWhenReady();
		return false;
	} else {
		if ( !mCurrentProject.empty() )
			mConfig.saveProject( mCurrentProject, mEditorSplitter, mConfigPath );
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
	dialog->setTitle( "Open File" );
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
	dialog->setTitle( "Open Folder" );
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

void App::openFontDialog( std::string& fontPath ) {
	std::string absoluteFontPath( fontPath );
	if ( FileSystem::isRelativePath( absoluteFontPath ) )
		absoluteFontPath = mResPath + fontPath;
	UIFileDialog* dialog =
		UIFileDialog::New( UIFileDialog::DefaultFlags, "*.ttf; *.otf; *.wolff; *.otb",
						   FileSystem::fileRemoveFileName( absoluteFontPath ) );
	ModelIndex index = dialog->getMultiView()->getListView()->findRowWithText(
		FileSystem::fileNameFromPath( fontPath ), true, true );
	if ( index.isValid() )
		dialog->runOnMainThread(
			[&, dialog, index]() { dialog->getMultiView()->setSelection( index ); } );
	dialog->setWinFlags( UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON | UI_WIN_MODAL );
	dialog->setTitle( "Select Font File" );
	dialog->setCloseShortcut( KEY_ESCAPE );
	dialog->addEventListener( Event::OpenFile, [&]( const Event* event ) {
		auto newPath = event->getNode()->asType<UIFileDialog>()->getFullPath();
		if ( String::startsWith( newPath, mResPath ) )
			newPath = newPath.substr( mResPath.size() );
		if ( fontPath != newPath ) {
			fontPath = newPath;
			auto fontName =
				FileSystem::fileRemoveExtension( FileSystem::fileNameFromPath( fontPath ) );
			FontTrueType* fontMono = loadFont( fontName, fontPath );
			if ( fontMono ) {
				mFontMono = fontMono;
				mFontMono->setBoldAdvanceSameAsRegular( true );
				if ( mEditorSplitter ) {
					mEditorSplitter->forEachEditor(
						[&]( UICodeEditor* editor ) { editor->setFont( mFontMono ); } );
				}
			}
		}
	} );
	dialog->addEventListener( Event::OnWindowClose, [&]( const Event* ) {
		if ( mEditorSplitter && mEditorSplitter->getCurEditor() &&
			 !SceneManager::instance()->isShootingDown() )
			mEditorSplitter->getCurEditor()->setFocus();
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
	mConfig.load( mConfigPath, mKeybindingsPath, mInitColorScheme, mRecentFiles, mRecentFolders,
				  mResPath, mDisplayDPI );
}

void App::saveConfig() {
	mConfig.save( mRecentFiles, mRecentFolders,
				  mProjectSplitter ? mProjectSplitter->getSplitPartition().toString() : "15%",
				  mWindow,
				  mEditorSplitter ? mEditorSplitter->getCurrentColorSchemeName()
								  : mConfig.editor.colorScheme );
}

static std::string keybindFormat( std::string str ) {
	if ( !str.empty() ) {
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
	} else {
		mWindow->getInput()->waitEvent( Milliseconds( mWindow->hasFocus() ? 16 : 100 ) );
	}
}

void App::onFileDropped( String file ) {
	Vector2f mousePos( mUISceneNode->getEventDispatcher()->getMousePosf() );
	Node* node = mUISceneNode->overFind( mousePos );
	UICodeEditor* codeEditor = mEditorSplitter->getCurEditor();
	if ( !node )
		node = codeEditor;
	if ( node && node->isType( UI_TYPE_CODEEDITOR ) ) {
		codeEditor = node->asType<UICodeEditor>();
		if ( !codeEditor->getDocument().isEmpty() && !Image::isImageExtension( file ) ) {
			auto d = mEditorSplitter->createCodeEditorInTabWidget(
				mEditorSplitter->tabWidgetFromEditor( codeEditor ) );
			codeEditor = d.second;
			d.first->getTabWidget()->setTabSelected( d.first );
		}
	}
	loadFileFromPath( file, false, codeEditor );
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
	eeSAFE_DELETE( mAutoCompleteModule );
	eeSAFE_DELETE( mLinterModule );
	eeSAFE_DELETE( mFormatterModule );
	eeSAFE_DELETE( mConsole );
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

UIMenu* App::createWindowMenu() {
	mWindowMenu = UIPopUpMenu::New();
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
			openFontDialog( mConfig.ui.serifFont );
		} else if ( item->getText() == "Monospace Font..." ) {
			openFontDialog( mConfig.ui.monospaceFont );
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
	mViewMenu->addCheckBox( "Highlight Matching Bracket" )
		->setActive( mConfig.editor.highlightMatchingBracket );
	mViewMenu->addCheckBox( "Highlight Current Line" )
		->setActive( mConfig.editor.highlightCurrentLine );
	mViewMenu->addCheckBox( "Highlight Selection Match" )
		->setActive( mConfig.editor.highlightSelectionMatch );
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
		->setTooltipText( "Enables the code formatter/prettifier module." );
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
	mViewMenu->add( "Line Breaking Column" );

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
		} else if ( item->getText() == "Line Breaking Column" ) {
			UIMessageBox* msgBox =
				UIMessageBox::New( UIMessageBox::INPUT, "Set Line Breaking Column:\n"
														"Set 0 to disable it.\n" );
			msgBox->setTitle( mWindowTitle );
			msgBox->setCloseShortcut( { KEY_ESCAPE, 0 } );
			msgBox->getTextInput()->setAllowOnlyNumbers( true, false );
			msgBox->getTextInput()->setText(
				String::toString( mConfig.editor.lineBreakingColumn ) );
			msgBox->showWhenReady();
			msgBox->addEventListener( Event::MsgBoxConfirmClick, [&, msgBox]( const Event* ) {
				int val;
				if ( String::fromString( val, msgBox->getTextInput()->getText() ) && val >= 0 ) {
					mConfig.editor.lineBreakingColumn = val;
					mEditorSplitter->forEachEditor(
						[val]( UICodeEditor* editor ) { editor->setLineBreakingColumn( val ); } );
					msgBox->closeWindow();
				}
			} );
			setFocusEditorOnClose( msgBox );
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

UIMenu* App::createEditMenu() {
	UIPopUpMenu* menu = UIPopUpMenu::New();
	menu->add( "Undo", findIcon( "undo" ), getKeybind( "undo" ) );
	menu->add( "Redo", findIcon( "redo" ), getKeybind( "redo" ) );
	menu->addSeparator();
	menu->add( "Cut", findIcon( "cut" ), getKeybind( "cut" ) );
	menu->add( "Copy", findIcon( "copy" ), getKeybind( "copy" ) );
	menu->add( "Paste", findIcon( "paste" ), getKeybind( "paste" ) );
	menu->addSeparator();
	menu->add( "Select All", findIcon( "select-all" ), getKeybind( "select-all" ) );
	menu->addSeparator();
	menu->add( "Find/Replace", findIcon( "find-replace" ), getKeybind( "find-replace" ) );
	menu->addSeparator();
	menu->add( "Key Bindings", findIcon( "keybindings" ), getKeybind( "keybindings" ) );
	menu->addEventListener( Event::OnItemClicked, [&]( const Event* event ) {
		if ( !event->getNode()->isType( UI_TYPE_MENUITEM ) )
			return;
		String text = String( event->getNode()->asType<UIMenuItem>()->getText() ).toLower();
		if ( "key bindings" == text ) {
			runCommand( "keybindings" );
		} else {
			String::replaceAll( text, " ", "-" );
			String::replaceAll( text, "/", "-" );
			runCommand( text );
		}
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
	mDocMenu = UIPopUpMenu::New();

	mDocMenu->addCheckBox( "Auto Detect Indent Type", mConfig.editor.autoDetectIndentType )
		->setId( "auto_indent" );

	UIPopUpMenu* tabTypeMenu = UIPopUpMenu::New();
	tabTypeMenu->addRadioButton( "Tabs" )->setId( "tabs" );
	tabTypeMenu->addRadioButton( "Spaces" )->setId( "spaces" );
	mDocMenu->addSubMenu( "Indentation Type", nullptr, tabTypeMenu )->setId( "indent_type" );
	tabTypeMenu->addEventListener( Event::OnItemClicked, [&]( const Event* event ) {
		const String& text = event->getNode()->asType<UIMenuRadioButton>()->getId();
		if ( mEditorSplitter->getCurEditor() ) {
			TextDocument::IndentType indentType = text == "tabs"
													  ? TextDocument::IndentType::IndentTabs
													  : TextDocument::IndentType::IndentSpaces;
			mEditorSplitter->getCurEditor()->getDocument().setIndentType( indentType );
			mConfig.editor.indentSpaces = indentType == TextDocument::IndentType::IndentSpaces;
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
	mDocMenu->addSubMenu( "Indent Width", nullptr, indentWidthMenu )->setId( "indent_width" );
	indentWidthMenu->addEventListener( Event::OnItemClicked, [&]( const Event* event ) {
		if ( mEditorSplitter->getCurEditor() ) {
			int width = event->getNode()->getData();
			mEditorSplitter->getCurEditor()->getDocument().setIndentWidth( width );
			mConfig.editor.indentWidth = width;
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
	mDocMenu->addSubMenu( "Tab Width", nullptr, tabWidthMenu )->setId( "tab_width" );
	tabWidthMenu->addEventListener( Event::OnItemClicked, [&]( const Event* event ) {
		if ( mEditorSplitter->getCurEditor() ) {
			int width = event->getNode()->getData();
			mEditorSplitter->getCurEditor()->setTabWidth( width );
			mConfig.editor.tabWidth = width;
		}
	} );

	UIPopUpMenu* lineEndingsMenu = UIPopUpMenu::New();
	lineEndingsMenu->addRadioButton( "Windows (CR/LF)", mConfig.editor.windowsLineEndings )
		->setId( "windows" );
	lineEndingsMenu->addRadioButton( "Unix (LF)", !mConfig.editor.windowsLineEndings )
		->setId( "unix" );
	mDocMenu->addSubMenu( "Line Endings", nullptr, lineEndingsMenu )->setId( "line_endings" );
	lineEndingsMenu->addEventListener( Event::OnItemClicked, [&]( const Event* event ) {
		bool winLe = event->getNode()->asType<UIRadioButton>()->getId() == "windows";
		if ( mEditorSplitter->getCurEditor() ) {
			mConfig.editor.windowsLineEndings = winLe;
			mEditorSplitter->getCurEditor()->getDocument().setLineEnding(
				winLe ? TextDocument::LineEnding::CRLF : TextDocument::LineEnding::LF );
			updateDocInfo( mEditorSplitter->getCurEditor()->getDocument() );
		}
	} );

	UIPopUpMenu* bracketsMenu = UIPopUpMenu::New();
	mDocMenu->addSubMenu( "Auto-Close Brackets", nullptr, bracketsMenu );
	auto& closeBrackets = mConfig.editor.autoCloseBrackets;
	auto shouldCloseCb = []( UIMenuItem* ) -> bool { return false; };
	bracketsMenu->addCheckBox( "Brackets ()", closeBrackets.find( '(' ) != std::string::npos )
		->setOnShouldCloseCb( shouldCloseCb )
		->setId( "()" );
	bracketsMenu->addCheckBox( "Curly Brackets {}", closeBrackets.find( '{' ) != std::string::npos )
		->setOnShouldCloseCb( shouldCloseCb )
		->setId( "{}" );
	bracketsMenu
		->addCheckBox( "Square Brackets []", closeBrackets.find( '[' ) != std::string::npos )
		->setOnShouldCloseCb( shouldCloseCb )
		->setId( "[]" );
	bracketsMenu->addCheckBox( "Single Quotes ''", closeBrackets.find( '\'' ) != std::string::npos )
		->setOnShouldCloseCb( shouldCloseCb )
		->setId( "''" );
	bracketsMenu
		->addCheckBox( "Double Quotes \"\"", closeBrackets.find( '"' ) != std::string::npos )
		->setOnShouldCloseCb( shouldCloseCb )
		->setId( "\"\"" );
	bracketsMenu->addCheckBox( "Back Quotes ``", closeBrackets.find( '`' ) != std::string::npos )
		->setOnShouldCloseCb( shouldCloseCb )
		->setId( "``" );
	bracketsMenu->addEventListener( Event::OnItemClicked, [&]( const Event* event ) {
		std::string id = event->getNode()->getId();
		if ( event->getNode()->isType( UI_TYPE_MENUCHECKBOX ) ) {
			UIMenuCheckBox* item = event->getNode()->asType<UIMenuCheckBox>();
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

	mDocMenu->addSeparator();

	mDocMenu->addCheckBox( "Read Only" )->setId( "read_only" );

	mDocMenu->addCheckBox( "Trim Trailing Whitespaces", mConfig.editor.trimTrailingWhitespaces )
		->setId( "trim_whitespaces" );

	mDocMenu->addCheckBox( "Force New Line at End of File", mConfig.editor.forceNewLineAtEndOfFile )
		->setId( "force_nl" );

	mDocMenu->addCheckBox( "Write Unicode BOM", mConfig.editor.writeUnicodeBOM )
		->setId( "write_bom" );

	mDocMenu->addEventListener( Event::OnItemClicked, [&]( const Event* event ) {
		if ( !mEditorSplitter->getCurEditor() ||
			 event->getNode()->isType( UI_TYPE_MENU_SEPARATOR ) ||
			 event->getNode()->isType( UI_TYPE_MENUSUBMENU ) )
			return;
		const String& id = event->getNode()->getId();
		TextDocument& doc = mEditorSplitter->getCurEditor()->getDocument();

		if ( event->getNode()->isType( UI_TYPE_MENUCHECKBOX ) ) {
			UIMenuCheckBox* item = event->getNode()->asType<UIMenuCheckBox>();
			if ( "auto_indent" == id ) {
				doc.setAutoDetectIndentType( item->isActive() );
				mConfig.editor.autoDetectIndentType = item->isActive();
			} else if ( "trim_whitespaces" == id ) {
				doc.setTrimTrailingWhitespaces( item->isActive() );
				mConfig.editor.trimTrailingWhitespaces = item->isActive();
			} else if ( "force_nl" == id ) {
				doc.setForceNewLineAtEndOfFile( item->isActive() );
				mConfig.editor.forceNewLineAtEndOfFile = item->isActive();
			} else if ( "write_bom" == id ) {
				doc.setBOM( item->isActive() );
				mConfig.editor.writeUnicodeBOM = item->isActive();
			} else if ( "read_only" == id ) {
				mEditorSplitter->getCurEditor()->setLocked( item->isActive() );
			}
		}
	} );
	return mDocMenu;
}

void App::updateDocumentMenu() {
	if ( !mEditorSplitter->getCurEditor() )
		return;

	const TextDocument& doc = mEditorSplitter->getCurEditor()->getDocument();

	mDocMenu->find( "auto_indent" )
		->asType<UIMenuCheckBox>()
		->setActive( doc.getAutoDetectIndentType() );

	auto* curIndent = mDocMenu->find( "indent_width" )
						  ->asType<UIMenuSubMenu>()
						  ->getSubMenu()
						  ->find( String::format( "indent_width_%d", doc.getIndentWidth() ) );
	if ( curIndent )
		curIndent->asType<UIMenuRadioButton>()->setActive( true );

	mDocMenu->find( "indent_type" )
		->asType<UIMenuSubMenu>()
		->getSubMenu()
		->find( doc.getIndentType() == TextDocument::IndentType::IndentTabs ? "tabs" : "spaces" )
		->asType<UIMenuRadioButton>()
		->setActive( true );

	mDocMenu->find( "tab_width" )
		->asType<UIMenuSubMenu>()
		->getSubMenu()
		->find( String::format( "tab_width_%d", mEditorSplitter->getCurEditor()->getTabWidth() ) )
		->asType<UIMenuRadioButton>()
		->setActive( true );

	mDocMenu->find( "trim_whitespaces" )
		->asType<UIMenuCheckBox>()
		->setActive( doc.getTrimTrailingWhitespaces() );

	mDocMenu->find( "force_nl" )
		->asType<UIMenuCheckBox>()
		->setActive( doc.getForceNewLineAtEndOfFile() );

	mDocMenu->find( "write_bom" )->asType<UIMenuCheckBox>()->setActive( doc.getBOM() );

	mDocMenu->find( "line_endings" )
		->asType<UIMenuSubMenu>()
		->getSubMenu()
		->find( doc.getLineEnding() == TextDocument::LineEnding::CRLF ? "windows" : "unix" )
		->asType<UIMenuRadioButton>()
		->setActive( true );

	mDocMenu->find( "read_only" )
		->asType<UIMenuCheckBox>()
		->setActive( mEditorSplitter->getCurEditor()->isLocked() );
}

void App::loadKeybindings() {
	if ( mKeybindings.empty() ) {
		KeyBindings bindings( mWindow->getInput() );
		auto defKeybindings = getDefaultKeybindings();
		IniFile ini( mKeybindingsPath );
		if ( FileSystem::fileExists( mKeybindingsPath ) ) {
			mKeybindings = ini.getKeyUnorderedMap( "keybindings" );
		} else {
			for ( const auto& it : defKeybindings )
				ini.setValue( "keybindings", bindings.getShortcutString( it.first ), it.second );
			ini.writeFile();
			mKeybindings = ini.getKeyUnorderedMap( "keybindings" );
		}
		for ( const auto& key : mKeybindings )
			mKeybindingsInvert[key.second] = key.first;

		if ( defKeybindings.size() != mKeybindings.size() ) {
			bool added = false;
			for ( const auto& key : defKeybindings ) {
				auto foundCmd = mKeybindingsInvert.find( key.second );
				auto shortcutStr = bindings.getShortcutString( key.first );
				if ( foundCmd == mKeybindingsInvert.end() &&
					 mKeybindings.find( shortcutStr ) == mKeybindings.end() ) {
					mKeybindings[shortcutStr] = key.second;
					mKeybindingsInvert[key.second] = shortcutStr;
					ini.setValue( "keybindings", shortcutStr, key.second );
					added = true;
				}
			}
			if ( added )
				ini.writeFile();
		}
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
	if ( mConfig.editor.showDocInfo && mDocInfoText ) {
		mDocInfoText->setText( String::format(
			"line: %lld / %lu  col: %lld    %s", doc.getSelection().start().line() + 1,
			doc.linesCount(), doc.getSelection().start().column(),
			doc.getLineEnding() == TextDocument::LineEnding::LF ? "LF" : "CRLF" ) );
	}
}

void App::syncProjectTreeWithEditor( UICodeEditor* editor ) {
	if ( mConfig.editor.syncProjectTreeWithEditor && editor->getDocument().hasFilepath() ) {
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
		updateCurrentFiletype();
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
			 { { KEY_F, KEYMOD_DEFAULT_MODIFIER }, "find-replace" },
			 { { KEY_Q, KEYMOD_DEFAULT_MODIFIER }, "close-app" },
			 { { KEY_O, KEYMOD_DEFAULT_MODIFIER }, "open-file" },
			 { { KEY_W, KEYMOD_DEFAULT_MODIFIER | KEYMOD_SHIFT }, "download-file-web" },
			 { { KEY_O, KEYMOD_DEFAULT_MODIFIER | KEYMOD_SHIFT }, "open-folder" },
			 { { KEY_F6, KEYMOD_NONE }, "debug-draw-highlight-toggle" },
			 { { KEY_F7, KEYMOD_NONE }, "debug-draw-boxes-toggle" },
			 { { KEY_F8, KEYMOD_NONE }, "debug-draw-debug-data" },
			 { { KEY_K, KEYMOD_DEFAULT_MODIFIER }, "open-locatebar" },
			 { { KEY_F, KEYMOD_DEFAULT_MODIFIER | KEYMOD_SHIFT }, "open-global-search" },
			 { { KEY_L, KEYMOD_DEFAULT_MODIFIER }, "go-to-line" },
			 { { KEY_M, KEYMOD_DEFAULT_MODIFIER }, "menu-toggle" },
			 { { KEY_S, KEYMOD_DEFAULT_MODIFIER | KEYMOD_SHIFT }, "save-all" },
			 { { KEY_F9, KEYMOD_LALT }, "switch-side-panel" },
			 { { KEY_F, KEYMOD_LALT }, "format-doc" } };
}

std::vector<std::string> App::getUnlockedCommands() {
	return { "fullscreen-toggle", "open-file",		  "open-folder",		"console-toggle",
			 "close-app",		  "open-locatebar",	  "open-global-search", "menu-toggle",
			 "switch-side-panel", "download-file-web" };
}

void App::closeEditors() {
	mConfig.saveProject( mCurrentProject, mEditorSplitter, mConfigPath );
	std::vector<UICodeEditor*> editors = mEditorSplitter->getAllEditors();
	for ( auto editor : editors ) {
		UITabWidget* tabWidget = mEditorSplitter->tabWidgetFromEditor( editor );
		tabWidget->removeTab( (UITab*)editor->getData() );
	}
	mCurrentProject = "";
	mDirTree = nullptr;
	if ( mFileSystemListener )
		mFileSystemListener->setDirTree( mDirTree );
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

void App::loadFileFromPath( const std::string& path, bool inNewTab, UICodeEditor* codeEditor ) {
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
#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN || defined( __EMSCRIPTEN_PTHREADS__ )
		if ( inNewTab ) {
			mEditorSplitter->loadAsyncFileFromPathInNewTab( path, mThreadPool );
		} else {
			mEditorSplitter->loadAsyncFileFromPath( path, mThreadPool, codeEditor );
		}
#else
		if ( inNewTab ) {
			mEditorSplitter->loadFileFromPathInNewTab( path );
		} else {
			mEditorSplitter->loadFileFromPath( path, codeEditor );
		}
#endif
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
	editor->setFontSize( config.fontSize.asDp( 0, Sizef(), mUISceneNode->getDPI() ) );
	editor->setEnableColorPickerOnSelection( true );
	editor->setColorScheme( mEditorSplitter->getCurrentColorScheme() );
	editor->setShowLineNumber( config.showLineNumbers );
	editor->setShowWhitespaces( config.showWhiteSpaces );
	editor->setHighlightMatchingBracket( config.highlightMatchingBracket );
	editor->setHorizontalScrollBarEnabled( config.horizontalScrollbar );
	editor->setHighlightCurrentLine( config.highlightCurrentLine );
	editor->setTabWidth( config.tabWidth );
	editor->setLineBreakingColumn( config.lineBreakingColumn );
	editor->setHighlightSelectionMatch( config.highlightSelectionMatch );
	editor->setEnableColorPickerOnSelection( config.colorPickerSelection );
	editor->setColorPreview( config.colorPreview );
	editor->setFont( mFontMono );
	editor->setMenuIconSize( mMenuIconSize );
	doc.setAutoCloseBrackets( !mConfig.editor.autoCloseBrackets.empty() );
	doc.setAutoCloseBracketsPairs( makeAutoClosePairs( mConfig.editor.autoCloseBrackets ) );
	doc.setAutoDetectIndentType( config.autoDetectIndentType );
	doc.setLineEnding( config.windowsLineEndings ? TextDocument::LineEnding::CRLF
												 : TextDocument::LineEnding::LF );
	doc.setTrimTrailingWhitespaces( config.trimTrailingWhitespaces );
	doc.setIndentType( config.indentSpaces ? TextDocument::IndentType::IndentSpaces
										   : TextDocument::IndentType::IndentTabs );
	doc.setForceNewLineAtEndOfFile( config.forceNewLineAtEndOfFile );
	doc.setIndentWidth( config.indentWidth );
	doc.setBOM( config.writeUnicodeBOM );

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

	if ( config.autoComplete && !mAutoCompleteModule )
		setAutoComplete( config.autoComplete );

	if ( config.linter && !mLinterModule )
		setLinter( config.linter );

	if ( config.formatter && !mFormatterModule )
		setFormatter( config.formatter );

	if ( config.autoComplete && mAutoCompleteModule )
		editor->registerModule( mAutoCompleteModule );

	if ( config.linter && mLinterModule )
		editor->registerModule( mLinterModule );

	if ( config.formatter && mFormatterModule )
		editor->registerModule( mFormatterModule );
}

bool App::setAutoComplete( bool enable ) {
	mConfig.editor.autoComplete = enable;
	if ( enable && !mAutoCompleteModule ) {
		mAutoCompleteModule = eeNew( AutoCompleteModule, ( mThreadPool ) );
		mEditorSplitter->forEachEditor(
			[&]( UICodeEditor* editor ) { editor->registerModule( mAutoCompleteModule ); } );
		return true;
	}
	if ( !enable && mAutoCompleteModule )
		eeSAFE_DELETE( mAutoCompleteModule );
	return false;
}

bool App::setLinter( bool enable ) {
	mConfig.editor.linter = enable;
	if ( enable && !mLinterModule ) {
		std::string path( mResPath + "assets/linters/linters.json" );
		if ( FileSystem::fileExists( mConfigPath + "linters.json" ) )
			path = mConfigPath + "linters.json";
		mLinterModule = eeNew( LinterModule, ( path, mThreadPool ) );
		mEditorSplitter->forEachEditor(
			[&]( UICodeEditor* editor ) { editor->registerModule( mLinterModule ); } );
		return true;
	}
	if ( !enable && mLinterModule )
		eeSAFE_DELETE( mLinterModule );
	return false;
}

bool App::setFormatter( bool enable ) {
	mConfig.editor.formatter = enable;
	if ( enable && !mFormatterModule ) {
		std::string path( mResPath + "assets/formatter/formatter.json" );
		if ( FileSystem::fileExists( mConfigPath + "formatter.json" ) )
			path = mConfigPath + "formatter.json";
		mFormatterModule = eeNew( FormatterModule, ( path, mThreadPool ) );
		mEditorSplitter->forEachEditor(
			[&]( UICodeEditor* editor ) { editor->registerModule( mFormatterModule ); } );
		return true;
	}
	if ( !enable && mFormatterModule )
		eeSAFE_DELETE( mFormatterModule );
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
	mSettingsMenu->addSubMenu( "Filetype", nullptr, createFiletypeMenu() );
	mSettingsMenu->addSubMenu( "Color Scheme", nullptr, createColorSchemeMenu() );
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
	for ( size_t i = 0; i < mColorSchemeMenu->getCount(); i++ ) {
		UIMenuRadioButton* menuItem = mColorSchemeMenu->getItem( i )->asType<UIMenuRadioButton>();
		menuItem->setActive( mEditorSplitter->getCurrentColorSchemeName() == menuItem->getText() );
	}
}

UIMenu* App::createColorSchemeMenu() {
	mColorSchemeMenu = UIPopUpMenu::New();
	for ( auto& colorScheme : mEditorSplitter->getColorSchemes() ) {
		mColorSchemeMenu->addRadioButton(
			colorScheme.first, mEditorSplitter->getCurrentColorSchemeName() == colorScheme.first );
	}
	mColorSchemeMenu->addEventListener( Event::OnItemClicked, [&]( const Event* event ) {
		UIMenuItem* item = event->getNode()->asType<UIMenuItem>();
		const String& name = item->getText();
		mEditorSplitter->setColorScheme( name );
	} );
	return mColorSchemeMenu;
}

UIMenu* App::createFiletypeMenu() {
	auto* dM = SyntaxDefinitionManager::instance();
	mFiletypeMenu = UIPopUpMenu::New();
	auto names = dM->getLanguageNames();
	for ( auto& name : names ) {
		mFiletypeMenu->addRadioButton(
			name,
			mEditorSplitter->getCurEditor() &&
				mEditorSplitter->getCurEditor()->getSyntaxDefinition().getLanguageName() == name );
	}
	mFiletypeMenu->addEventListener( Event::OnItemClicked, [&, dM]( const Event* event ) {
		UIMenuItem* item = event->getNode()->asType<UIMenuItem>();
		const String& name = item->getText();
		if ( mEditorSplitter->getCurEditor() ) {
			mEditorSplitter->getCurEditor()->setSyntaxDefinition(
				dM->getStyleByLanguageName( name ) );
			updateCurrentFiletype();
		}
	} );
	return mFiletypeMenu;
}

void App::updateCurrentFiletype() {
	if ( !mEditorSplitter->getCurEditor() )
		return;
	std::string curLang( mEditorSplitter->getCurEditor()->getSyntaxDefinition().getLanguageName() );
	for ( size_t i = 0; i < mFiletypeMenu->getCount(); i++ ) {
		UIMenuRadioButton* menuItem = mFiletypeMenu->getItem( i )->asType<UIMenuRadioButton>();
		std::string itemLang( menuItem->getText() );
		menuItem->setActive( curLang == itemLang );
	}
}

void App::updateEditorState() {
	if ( mEditorSplitter->getCurEditor() ) {
		updateEditorTitle( mEditorSplitter->getCurEditor() );
		updateCurrentFiletype();
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
	} else {
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

	mConfig.loadProject( rpath, mEditorSplitter, mConfigPath, mThreadPool );

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

void App::init( const std::string& file, const Float& pidelDensity,
				const std::string& colorScheme ) {
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
		if ( colorScheme == "light" )
			mUISceneNode->setColorSchemePreference( ColorSchemePreference::Light );

		mFont = loadFont( "sans-serif", mConfig.ui.serifFont, "assets/fonts/NotoSans-Regular.ttf" );
		mFontMono =
			loadFont( "monospace", mConfig.ui.monospaceFont, "assets/fonts/DejaVuSansMono.ttf" );
		if ( mFontMono )
			mFontMono->setBoldAdvanceSameAsRegular( true );

		loadFont( "NotoEmoji-Regular", "assets/fonts/NotoEmoji-Regular.ttf" );

#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN
		loadFont( "NotoColorEmoji", "assets/fonts/NotoColorEmoji.ttf" );
#endif

		FontTrueType* iconFont =
			FontTrueType::New( "icon", mResPath + "assets/fonts/remixicon.ttf" );

		if ( !mFont || !mFontMono || !iconFont ) {
			printf( "Font not found!" );
			return;
		}

		SceneManager::instance()->add( mUISceneNode );

		UITheme* theme =
			UITheme::load( "uitheme", "uitheme", "", mFont, mResPath + "assets/ui/breeze.css" );
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
		TextInput.small_input {
			padding-top: 0;
			padding-bottom: 0;
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
			color: #eff0f188;
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
						<CheckBox id="case_sensitive" layout_width="wrap_content" layout_height="wrap_content" text="Case sensitive" selected="true" />
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
							<hbox layout_width="match_parent" layout_height="wrap_content">
								<CheckBox id="case_sensitive" layout_width="wrap_content" layout_height="wrap_content" text="Case sensitive" selected="true" />
								<CheckBox id="whole_word" layout_width="wrap_content" layout_height="wrap_content" text="Match Whole Word" selected="false" margin-left="8dp" />
								<CheckBox id="lua_pattern" layout_width="wrap_content" layout_height="wrap_content" text="Lua Pattern" selected="false" margin-left="8dp" />
								<CheckBox id="escape_sequence" layout_width="wrap_content" layout_height="wrap_content" text="Use escape sequences" margin-left="8dp" selected="false" tooltip="Replace \\, \t, \n, \r and \uXXXX (Unicode characters) with the corresponding control" />
								<Widget layout_width="0" layout_weight="1" layout_height="match_parent" />
								<TextView layout_width="wrap_content" layout_height="wrap_content" text="History:" margin-right="4dp" layout_height="18dp" />
								<DropDownList id="global_search_history" layout_width="300dp" layout_height="18dp" margin-right="4dp" />
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

		UIIconTheme* iconTheme = UIIconTheme::New( "remixicon" );
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
			{ "folder", 0xed54 },
			{ "folder-open", 0xed70 },
			{ "folder-add", 0xed5a },
			{ "file", 0xecc3 },
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
			{ "tree-expanded", 0xea50 },
			{ "tree-contracted", 0xea54 },
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
		};
		for ( const auto& icon : icons )
			iconTheme->add( UIGlyphIcon::New( icon.first, iconFont, icon.second ) );

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
			SyntaxColorScheme::loadFromFile( mResPath + "assets/colorschemes/colorschemes.conf" ),
			mInitColorScheme );
		mEditorSplitter->setHideTabBarOnSingleTab( mConfig.editor.hideTabBarOnSingleTab );

#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN
		mFileWatcher = new efsw::FileWatcher();
		mFileSystemListener = new FileSystemListener( mEditorSplitter, mFileSystemModel );
		mFileWatcher->watch();
#endif

		mNotificationCenter = std::make_unique<NotificationCenter>(
			mUISceneNode->find<UILayout>( "notification_center" ) );

		mDocSearchController = std::make_unique<DocSearchController>( mEditorSplitter, this );
		mDocSearchController->initSearchBar( mUISceneNode->find<UISearchBar>( "search_bar" ) );

		mGlobalSearchController =
			std::make_unique<GlobalSearchController>( mEditorSplitter, mUISceneNode, this );
		mGlobalSearchController->initGlobalSearchBar(
			mUISceneNode->find<UIGlobalSearchBar>( "global_search_bar" ) );

		mFileLocator = std::make_unique<FileLocator>( mEditorSplitter, mUISceneNode, this );
		mFileLocator->initLocateBar( mUISceneNode->find<UILocateBar>( "locate_bar" ),
									 mUISceneNode->find<UITextInput>( "locate_find" ) );

		initImageView();

		createSettingsMenu();

		mEditorSplitter->createEditorWithTabWidget( mBaseLayout );

		mConsole = eeNew( Console, ( mFontMono, true, true, 1024 * 1000, 0, mWindow ) );

		initProjectTreeView( file );

#if EE_PLATFORM == EE_PLATFORM_EMSCRIPTEN
		downloadFileWeb( "https://raw.githubusercontent.com/SpartanJ/eepp/develop/README.md" );
#endif

		mWindow->runMainLoop( &appLoop );
	}
}

EE_MAIN_FUNC int main( int argc, char* argv[] ) {
#ifndef EE_DEBUG
	Log::create( LogLevel::Info, false, true );
#else
	Log::create( LogLevel::Debug, true, true );
#endif
	args::ArgumentParser parser( "ecode" );
	args::HelpFlag help( parser, "help", "Display this help menu", { 'h', "help" } );
	args::Positional<std::string> file( parser, "file", "The file path" );
	args::ValueFlag<Float> pixelDenstiyConf( parser, "pixel-density",
											 "Set default application pixel density",
											 { 'd', "pixel-density" } );
	args::ValueFlag<std::string> prefersColorScheme(
		parser, "prefers-color-scheme", "Set the preferred color scheme (\"light\" or \"dark\")",
		{ 'c', "prefers-color-scheme" } );

	try {
		parser.ParseCLI( argc, argv );
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
	appInstance->init( file.Get(), pixelDenstiyConf ? pixelDenstiyConf.Get() : 0.f,
					   prefersColorScheme ? prefersColorScheme.Get() : "dark" );
	eeSAFE_DELETE( appInstance );

	Engine::destroySingleton();
	MemoryManager::showResults();

	return EXIT_SUCCESS;
}
