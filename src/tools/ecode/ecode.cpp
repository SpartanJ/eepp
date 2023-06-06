#include "ecode.hpp"
#include "featureshealth.hpp"
#include "plugins/autocomplete/autocompleteplugin.hpp"
#include "plugins/formatter/formatterplugin.hpp"
#include "plugins/linter/linterplugin.hpp"
#include "plugins/lsp/lspclientplugin.hpp"
#include "settingsmenu.hpp"
#include "uibuildsettings.hpp"
#include "uiwelcomescreen.hpp"
#include "version.hpp"
#include <algorithm>
#include <args/args.hxx>
#include <filesystem>
#include <nlohmann/json.hpp>
namespace fs = std::filesystem;
using json = nlohmann::json;

#ifdef ECODE_USE_BACKWARD
#if EE_PLATFORM == EE_PLATFORM_LINUX
#define BACKWARD_HAS_DW 1
#endif
#include <backward-cpp/backward.hpp>
#endif
#if EE_PLATFORM == EE_PLATFORM_MACOSX
#include "macos/macos.hpp"
#endif

namespace ecode {

Clock globalClock;
bool firstFrame = true;
bool firstUpdate = true;
App* appInstance = nullptr;

static bool pathHasPosition( const std::string& path ) {
#if EE_PLATFORM == EE_PLATFORM_WIN
	return std::count( path.begin(), path.end(), ':' ) > 1;
#else
	return std::count( path.begin(), path.end(), ':' ) > 0;
#endif
}

static std::pair<std::string, TextPosition> getPathAndPosition( const std::string& path ) {
	if ( pathHasPosition( path ) ) {
		auto parts = String::split( path, ':' );
		if ( parts.size() >= 2 ) {
			Int64 line = 0;
			Int64 col = 0;
#if EE_PLATFORM == EE_PLATFORM_WIN
			size_t partCount = 4;
#else
			size_t partCount = 3;
#endif
			int linePos = parts.size() >= partCount ? parts.size() - 2 : parts.size() - 1;
			int colPos = parts.size() >= partCount ? parts.size() - 1 : -1;
			if ( String::fromString( line, parts[linePos] ) ) {
				if ( colPos > 0 )
					String::fromString( col, parts[colPos] );
			}
			std::string npath( parts[0] );
			if ( parts.size() >= 2 ) {
				for ( Int64 i = 1; i < linePos; i++ ) {
					npath += ":" + parts[i];
				}
			}
			return { npath, { eemax( (Int64)0, line - 1 ), col } };
		}
	}
	return { path, { 0, 0 } };
}

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
		msgBox->addEventListener( Event::OnConfirm, [&]( const Event* ) {
			if ( !mCurrentProject.empty() )
				mConfig.saveProject( mCurrentProject, mSplitter, mConfigPath, mProjectDocConfig,
									 mProjectBuildManager ? mProjectBuildManager->getConfig()
														  : ProjectBuildConfiguration() );
			saveConfig();
			mWindow->close();
		} );
		msgBox->setTitle( String::format( i18n( "close_title", "Close %s?" ).toUtf8().c_str(),
										  mWindowTitle.c_str() ) );
		msgBox->center();
		msgBox->showWhenReady();
		return false;
	} else {
		if ( !mCurrentProject.empty() )
			mConfig.saveProject( mCurrentProject, mSplitter, mConfigPath, mProjectDocConfig,
								 mProjectBuildManager ? mProjectBuildManager->getConfig()
													  : ProjectBuildConfiguration() );
		saveConfig();
		return true;
	}
}

void App::saveDoc() {
	if ( !mSplitter->curEditorExistsAndFocused() )
		return;

	if ( mSplitter->getCurEditor()->getDocument().hasFilepath() ) {
		if ( mSplitter->getCurEditor()->save() ) {
			updateEditorState();
		} else {
			UIMessageBox* msgBox = errorMsgBox(
				i18n( "could_not_write_file",
					  "Could not write file to disk.\nPlease check if the file "
					  "is read-only or if ecode does not have permissions to write to that file.\n"
					  "File path is: " ) +
				mSplitter->getCurEditor()->getDocument().getFilePath() );

			setFocusEditorOnClose( msgBox );
		}
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
					if ( !SceneManager::instance()->isShuttingDown() && !mTmpDocs.empty() )
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
	if ( editor == nullptr )
		return;
	std::string title( titleFromEditor( editor ) );
	if ( editor->getData() ) {
		UITab* tab = (UITab*)editor->getData();
		tab->setText( title );
	}
}

void App::updateEditorTitle( UICodeEditor* editor ) {
	if ( editor == nullptr )
		return;
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

	if ( mBenchmarkMode )
		fullTitle += " - " + String::toString( mWindow->getFPS() ) + " FPS";

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
	dialog->setWindowFlags( UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON | UI_WIN_MODAL );
	dialog->setTitle( i18n( "open_file", "Open File" ) );
	dialog->setCloseShortcut( KEY_ESCAPE );
	dialog->addEventListener( Event::OpenFile, [&]( const Event* event ) {
		auto file = event->getNode()->asType<UIFileDialog>()->getFullPath();
		mLastFileFolder = FileSystem::fileRemoveFileName( file );
		loadFileFromPath( file );
	} );
	dialog->addEventListener( Event::OnWindowClose, [&]( const Event* ) {
		if ( mSplitter && mSplitter->getCurWidget() && !SceneManager::instance()->isShuttingDown() )
			mSplitter->getCurWidget()->setFocus();
	} );
	dialog->center();
	dialog->show();
}

std::string App::getLastUsedFolder() {
	if ( !mCurrentProject.empty() )
		return mCurrentProject;
	if ( !mRecentFolders.empty() )
		return mRecentFolders.front();
	return ".";
}

void App::refreshFolderView() {
	if ( !mFileSystemModel )
		return;
	mThreadPool->run( [this]() { mFileSystemModel->refresh(); } );
}

void App::openFolderDialog() {
	UIFileDialog* dialog =
		UIFileDialog::New( UIFileDialog::DefaultFlags | UIFileDialog::AllowFolderSelect |
							   UIFileDialog::ShowOnlyFolders,
						   "*", getLastUsedFolder() );
	dialog->setWindowFlags( UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON | UI_WIN_MODAL );
	dialog->setTitle( i18n( "open_folder", "Open Folder" ) );
	dialog->setCloseShortcut( KEY_ESCAPE );
	dialog->addEventListener( Event::OpenFile, [&]( const Event* event ) {
		String path( event->getNode()->asType<UIFileDialog>()->getFullPath() );
		if ( FileSystem::isDirectory( path ) )
			loadFolder( path );
	} );
	dialog->addEventListener( Event::OnWindowClose, [&]( const Event* ) {
		if ( mSplitter && mSplitter->getCurWidget() && !SceneManager::instance()->isShuttingDown() )
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
		UIFileDialog::New( UIFileDialog::DefaultFlags, "*.ttf; *.otf; *.wolff; *.otb; *.bdf; *.ttc",
						   FileSystem::fileRemoveFileName( absoluteFontPath ) );
	ModelIndex index = dialog->getMultiView()->getListView()->findRowWithText(
		FileSystem::fileNameFromPath( fontPath ), true, true );
	if ( index.isValid() )
		dialog->runOnMainThread(
			[&, dialog, index]() { dialog->getMultiView()->setSelection( index ); } );
	dialog->setWindowFlags( UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON | UI_WIN_MODAL );
	dialog->setTitle( i18n( "select_font_file", "Select Font File" ) );
	dialog->setCloseShortcut( KEY_ESCAPE );
	dialog->addEventListener( Event::OnWindowClose, [&]( const Event* ) {
		if ( mSplitter && mSplitter->getCurWidget() && !SceneManager::instance()->isShuttingDown() )
			mSplitter->getCurWidget()->setFocus();
	} );
	dialog->addEventListener( Event::OpenFile, [&, loadingMonoFont]( const Event* event ) {
		auto newPath = event->getNode()->asType<UIFileDialog>()->getFullPath();
		if ( String::startsWith( newPath, mResPath ) )
			newPath = newPath.substr( mResPath.size() );
		if ( fontPath != newPath ) {
			if ( !loadingMonoFont ) {
				fontPath = newPath;
				return;
			}
			auto fontName =
				FileSystem::fileRemoveExtension( FileSystem::fileNameFromPath( newPath ) );
			FontTrueType* fontMono = loadFont( fontName, newPath );
			if ( fontMono ) {
				auto loadMonoFont = [&, newPath]( FontTrueType* fontMono ) {
					fontPath = newPath;
					mFontMono = fontMono;
					mFontMono->setEnableDynamicMonospace( true );
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
						Event::OnConfirm,
						[&, loadMonoFont, fontMono]( const Event* ) { loadMonoFont( fontMono ); } );
					msgBox->addEventListener( Event::OnCancel, [fontMono]( const Event* ) {
						FontManager::instance()->remove( fontMono );
					} );
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
	msgBox->addEventListener( Event::OnConfirm, [&, msgBox]( const Event* ) {
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
		UIFileDialog::New( UIFileDialog::DefaultFlags | UIFileDialog::SaveDialog, "*" );
	dialog->setWindowFlags( UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON | UI_WIN_MODAL );
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
			if ( editor && !SceneManager::instance()->isShuttingDown() )
				editor->setFocus();
		} );
	}
	dialog->center();
	dialog->show();
	return dialog;
}

void App::runCommand( const std::string& command ) {
	if ( mSplitter->getCurWidget() && mSplitter->getCurWidget()->isType( UI_TYPE_CODEEDITOR ) ) {
		mSplitter->getCurWidget()->asType<UICodeEditor>()->getDocument().execute( command );
	} else if ( mSplitter->getCurWidget() &&
				mSplitter->getCurWidget()->isType( UI_TYPE_TERMINAL ) ) {
		mSplitter->getCurWidget()->asType<UITerminal>()->execute( command );
	} else {
		mMainLayout->execute( command );
	}
}

void App::onPluginEnabled( UICodeEditorPlugin* plugin ) {
	if ( mSplitter )
		mSplitter->forEachEditor(
			[&]( UICodeEditor* editor ) { editor->registerPlugin( plugin ); } );
}

void App::initPluginManager() {
	mPluginManager = std::make_unique<PluginManager>( mResPath, mPluginsPath, mThreadPool );
	mPluginManager->onPluginEnabled = [&]( UICodeEditorPlugin* plugin ) {
		if ( nullptr == mUISceneNode || plugin->isReady() ) {
			onPluginEnabled( plugin );
		} else {
			// If plugin loads asynchronously and is not ready, delay the plugin enabled callback
			plugin->addOnReadyCallback( [&]( UICodeEditorPlugin* plugin, const Uint32& cbId ) {
				mUISceneNode->runOnMainThread( [&, plugin]() { onPluginEnabled( plugin ); } );
				plugin->removeReadyCallback( cbId );
			} );
		}
	};
	mPluginManager->registerPlugin( LinterPlugin::Definition() );
	mPluginManager->registerPlugin( FormatterPlugin::Definition() );
	mPluginManager->registerPlugin( AutoCompletePlugin::Definition() );
	mPluginManager->registerPlugin( LSPClientPlugin::Definition() );
}

void App::loadConfig( const LogLevel& logLevel, const Sizeu& displaySize, bool sync,
					  bool stdOutLogs, bool disableFileLogs ) {
	mConfigPath = Sys::getConfigPath( "ecode" );
	if ( !FileSystem::fileExists( mConfigPath ) )
		FileSystem::makeDir( mConfigPath );
	FileSystem::dirAddSlashAtEnd( mConfigPath );
	mPluginsPath = mConfigPath + "plugins";
	mLanguagesPath = mConfigPath + "languages";
	mThemesPath = mConfigPath + "themes";
	mColorSchemesPath = mConfigPath + "editor" + FileSystem::getOSSlash() + "colorschemes" +
						FileSystem::getOSSlash();
	mTerminalManager = std::make_unique<TerminalManager>( this );
	mTerminalManager->setTerminalColorSchemesPath( mConfigPath + "terminal" +
												   FileSystem::getOSSlash() + "colorschemes" +
												   FileSystem::getOSSlash() );
	mTerminalManager->setUseFrameBuffer( mUseFrameBuffer );

	if ( !FileSystem::fileExists( mPluginsPath ) )
		FileSystem::makeDir( mPluginsPath );
	FileSystem::dirAddSlashAtEnd( mPluginsPath );

	if ( !FileSystem::fileExists( mLanguagesPath ) )
		FileSystem::makeDir( mLanguagesPath );
	FileSystem::dirAddSlashAtEnd( mLanguagesPath );

	if ( !FileSystem::fileExists( mThemesPath ) )
		FileSystem::makeDir( mThemesPath );
	FileSystem::dirAddSlashAtEnd( mThemesPath );

#ifndef EE_DEBUG
	Log::create( mConfigPath + "ecode.log", logLevel, stdOutLogs, !disableFileLogs );
#else
	Log::create( mConfigPath + "ecode.log", logLevel, stdOutLogs, !disableFileLogs );
#endif

	Log::instance()->setKeepLog( true );

	if ( !mArgs.empty() ) {
		std::string strargs( String::join( mArgs ) );
		Log::info( "ecode starting with these command line arguments: %s", strargs.c_str() );
	}

	initPluginManager();

	mConfig.load( mConfigPath, mKeybindingsPath, mInitColorScheme, mRecentFiles, mRecentFolders,
				  mResPath, mPluginManager.get(), displaySize.asInt(), sync );
}

void App::saveConfig() {
	mConfig.save( mRecentFiles, mRecentFolders,
				  mProjectSplitter ? mProjectSplitter->getSplitPartition().toString() : "15%",
				  mMainSplitter ? mMainSplitter->getSplitPartition().toString() : "85%", mWindow,
				  mSplitter ? mSplitter->getCurrentColorSchemeName() : mConfig.editor.colorScheme,
				  mDocSearchController->getSearchBarConfig(),
				  mGlobalSearchController->getGlobalSearchBarConfig(), mPluginManager.get() );
}

std::string App::getKeybind( const std::string& command ) {
	auto it = mKeybindingsInvert.find( command );
	if ( it != mKeybindingsInvert.end() )
		return KeyBindings::keybindFormat( it->second );
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
	} else if ( mSplitter->getCurWidget() != nullptr &&
				mSplitter->getCurWidget()->isType( UI_TYPE_TERMINAL ) ) {
		UITerminal* terminal = mSplitter->getCurWidget()->asType<UITerminal>();
		std::string cmd = terminal->getKeyBindings().getCommandFromKeyBind(
			{ keyEvent.getKeyCode(), keyEvent.getMod() } );
		if ( !cmd.empty() )
			terminal->execute( cmd );
	} else {
		std::string cmd = mMainLayout->getKeyBindings().getCommandFromKeyBind(
			{ keyEvent.getKeyCode(), keyEvent.getMod() } );
		if ( !cmd.empty() )
			mMainLayout->execute( cmd );
	}
	return false;
}

void App::closeApp() {
	if ( onCloseRequestCallback( mWindow ) )
		mWindow->close();
}

void App::mainLoop() {
	mWindow->getInput()->update();
	SceneManager::instance()->update();
	if ( firstUpdate ) {
		Log::info( "First update took: %.2f ms", globalClock.getElapsedTime().asMilliseconds() );
		firstUpdate = false;
	}

	if ( SceneManager::instance()->getUISceneNode()->invalidated() || mBenchmarkMode ) {
		mWindow->clear();
		SceneManager::instance()->draw();
		mWindow->display();
		if ( firstFrame ) {
			Log::info( "First frame took: %.2f ms", globalClock.getElapsedTime().asMilliseconds() );
			firstFrame = false;
		}
	} else {
#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN
		mWindow->getInput()->waitEvent( Milliseconds( mWindow->hasFocus() ? 16 : 100 ) );
#endif
	}

	if ( mBenchmarkMode && mSecondsCounter.getElapsedTime() >= Seconds( 1 ) ) {
		setAppTitle( mWindowTitle );
		mSecondsCounter.restart();
	}
}

void App::onFileDropped( String file ) {
	Vector2f mousePos( mUISceneNode->getEventDispatcher()->getMousePosf() );
	Node* node = mUISceneNode->overFind( mousePos );
	UIWidget* widget = mSplitter->getCurWidget();
	UITab* tab = mSplitter->isDocumentOpen( file );
	UICodeEditor* codeEditor = nullptr;

	if ( tab ) {
		tab->getTabWidget()->setTabSelected( tab );
		return;
	}

	if ( !node )
		node = widget;
	if ( node && node->isType( UI_TYPE_CODEEDITOR ) ) {
		codeEditor = node->asType<UICodeEditor>();
		if ( ( codeEditor->getDocument().isLoading() || !codeEditor->getDocument().isEmpty() ) &&
			 ( !Image::isImageExtension( file ) ||
			   FileSystem::fileExtension( file.toUtf8() ) == "svg" ) ) {
			auto d = mSplitter->createCodeEditorInTabWidget(
				mSplitter->tabWidgetFromEditor( codeEditor ) );
			codeEditor = d.second;
			tab = d.first;
		}
	} else if ( widget && widget->isType( UI_TYPE_TERMINAL ) ) {
		if ( !Image::isImageExtension( file ) &&
			 ( !Image::isImageExtension( file ) ||
			   FileSystem::fileExtension( file.toUtf8() ) == "svg" ) ) {
			auto d =
				mSplitter->createCodeEditorInTabWidget( mSplitter->tabWidgetFromWidget( widget ) );
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

App::App( const size_t& jobs, const std::vector<std::string>& args ) :
	mArgs( args ),
	mThreadPool(
		ThreadPool::createShared( jobs > 0 ? jobs : eemax<int>( 2, Sys::getCPUCount() ) ) ) {}

App::~App() {
	mThreadPool.reset();
	if ( mFileWatcher ) {
		Lock l( mWatchesLock );
		delete mFileWatcher;
		mFileWatcher = nullptr;
	}
	mPluginManager.reset();
	eeSAFE_DELETE( mSplitter );
	eeSAFE_DELETE( mConsole );
	if ( mFileSystemListener ) {
		delete mFileSystemListener;
		mFileSystemListener = nullptr;
	}
	mDirTree.reset();
}

void App::updateRecentFiles() {
	UINode* node = nullptr;
	if ( mSettings && mSettings->getSettingsMenu() &&
		 ( node = mSettings->getSettingsMenu()->getItemId( "menu-recent-files" ) ) ) {
		UIMenuSubMenu* uiMenuSubMenu = static_cast<UIMenuSubMenu*>( node );
		UIMenu* menu = uiMenuSubMenu->getSubMenu();
		uiMenuSubMenu->setEnabled( !mRecentFiles.empty() );
		menu->removeAll();
		menu->removeEventsOfType( Event::OnItemClicked );
		if ( mRecentFiles.empty() )
			return;
		menu->add( i18n( "reopen_closed_document", "Reopen Closed Document" ), nullptr,
				   getKeybind( "reopen-closed-tab" ) )
			->setId( "reopen-closed-tab" )
			->setEnabled( !mRecentClosedFiles.empty() );
		menu->addSeparator();
		for ( const auto& file : mRecentFiles )
			menu->add( file );
		menu->addSeparator();
		menu->add( i18n( "clear_menu", "Clear Menu" ) )->setId( "clear-menu" );
		menu->addEventListener( Event::OnItemClicked, [&]( const Event* event ) {
			if ( !event->getNode()->isType( UI_TYPE_MENUITEM ) )
				return;
			const std::string& id = event->getNode()->asType<UIMenuItem>()->getId();
			if ( id == "reopen-closed-tab" ) {
				reopenClosedTab();
			} else if ( id == "clear-menu" ) {
				mRecentFiles.clear();
				updateRecentFiles();
			} else {
				const String& txt = event->getNode()->asType<UIMenuItem>()->getText();
				std::string path( txt.toUtf8() );
				if ( ( FileSystem::fileExists( path ) && !FileSystem::isDirectory( path ) ) ||
					 String::startsWith( path, "https://" ) ||
					 String::startsWith( path, "http://" ) ) {
					loadFileFromPathOrFocus( path );
				}
			}
		} );
	}
}

void App::updateRecentFolders() {
	UINode* node = nullptr;
	updateOpenRecentFolderBtn();
	if ( mSettings && mSettings->getSettingsMenu() &&
		 ( node = mSettings->getSettingsMenu()->getItemId( "recent-folders" ) ) ) {
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
		menu->addCheckBox(
				i18n( "restore_last_session_at_startup", "Restart last session at startup" ) )
			->setActive( mConfig.workspace.restoreLastSession )
			->setId( "restore-last-session-at-startup" );
		menu->addEventListener( Event::OnItemClicked, [&]( const Event* event ) {
			if ( !event->getNode()->isType( UI_TYPE_MENUITEM ) )
				return;
			const std::string& id = event->getNode()->asType<UIMenuItem>()->getId();
			if ( id == "clear-menu" ) {
				mRecentFolders.clear();
				updateRecentFolders();
			} else if ( id == "restore-last-session-at-startup" ) {
				mConfig.workspace.restoreLastSession =
					event->getNode()->asType<UIMenuCheckBox>()->isActive();
			} else {
				const String& txt = event->getNode()->asType<UIMenuItem>()->getText();
				loadFolder( txt );
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

void App::showStatusBar( bool show ) {
	if ( show == mStatusBar->isVisible() )
		return;
	mStatusBar->setVisible( show );
}

ProjectBuildManager* App::getProjectBuildManager() const {
	return mProjectBuildManager.get();
}

UITabWidget* App::getSidePanel() const {
	return mSidePanel;
}

void App::switchSidePanel() {
	mConfig.ui.showSidePanel = !mConfig.ui.showSidePanel;
	mSettings->getWindowMenu()
		->getItemId( "show-side-panel" )
		->asType<UIMenuCheckBox>()
		->setActive( mConfig.ui.showSidePanel );
	showSidePanel( mConfig.ui.showSidePanel );
}

void App::switchStatusBar() {
	mConfig.ui.showStatusBar = !mConfig.ui.showStatusBar;
	mSettings->getWindowMenu()
		->getItemId( "toggle-status-bar" )
		->asType<UIMenuCheckBox>()
		->setActive( mConfig.ui.showStatusBar );
	showStatusBar( mConfig.ui.showStatusBar );
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

ColorSchemePreference App::getUIColorScheme() const {
	return mUIColorScheme;
}

EE::Window::Window* App::getWindow() const {
	return mWindow;
}

UILinearLayout* App::getDocInfo() const {
	return mDocInfo;
}

UITreeView* App::getProjectTreeView() const {
	return mProjectTreeView;
}

void App::checkForUpdatesResponse( Http::Response response, bool fromStartup ) {
	auto updatesError = [&, fromStartup]() {
		if ( fromStartup )
			return;
		UIMessageBox* msg = UIMessageBox::New(
			UIMessageBox::OK, i18n( "error_checking_version", "Failed checking for updates." ) );
		msg->setTitle( "Error" );
		msg->setCloseShortcut( { KEY_ESCAPE, 0 } );
		msg->showWhenReady();
	};

	if ( response.getStatus() != Http::Response::Status::Ok || response.getBody().empty() ) {
		updatesError();
		return;
	}

	auto addStartUpCheckbox = [this]( UIMessageBox* msg ) {
		msg->setId( "check_for_updates" );
		msg->addEventListener( Event::OnWindowReady, [this, msg]( const Event* ) {
			msg->setVisible( false );
			UICheckBox* cbox = UICheckBox::New();
			cbox->addClass( "check_at_startup" );
			cbox->setParent( msg->getLayoutCont()->getFirstChild() );
			cbox->setLayoutSizePolicy( SizePolicy::WrapContent, SizePolicy::WrapContent );
			cbox->setText( i18n( "check_for_new_updates_at_startup",
								 "Always check for new updates at startup." ) );
			cbox->setChecked( mConfig.workspace.checkForUpdatesAtStartup );
			cbox->toPosition( 1 );
			cbox->runOnMainThread( [msg]() {
				msg->setMinWindowSize( msg->getLayoutCont()->getSize() );
				msg->center();
				msg->show();
			} );
			cbox->addEventListener( Event::OnValueChange, [this, cbox]( const Event* ) {
				mConfig.workspace.checkForUpdatesAtStartup = cbox->isChecked();
			} );
		} );
	};

	json j;
	try {
		j = json::parse( response.getBody(), nullptr, true, true );

		if ( j.contains( "tag_name" ) ) {
			auto tagName( j["tag_name"].get<std::string>() );
			auto versionNum = ecode::Version::getVersionNumFromTag( tagName );
			if ( versionNum > ecode::Version::getVersionNum() ) {
				auto name( j.value( "name", tagName ) );
				UIMessageBox* msg = UIMessageBox::New(
					UIMessageBox::OK_CANCEL,
					name + i18n( "ecode_updates_available",
								 " is available!\nDo you want to download it now?" )
							   .unescape() );

				auto url( j.value( "html_url", "https://github.com/SpartanJ/ecode/releases/" ) );
				msg->addEventListener( Event::OnConfirm, [&, url, msg]( const Event* ) {
					Engine::instance()->openURI( url );
					msg->closeWindow();
				} );
				msg->setTitle( "ecode" );
				msg->setCloseShortcut( { KEY_ESCAPE, 0 } );
				addStartUpCheckbox( msg );
			} else if ( versionNum < ecode::Version::getVersionNum() ) {
				if ( fromStartup )
					return;
				UIMessageBox* msg = UIMessageBox::New(
					UIMessageBox::OK,
					i18n( "ecode_unreleased_version",
						  "You are running an unreleased version of ecode!\nCurrent version: " )
							.unescape() +
						ecode::Version::getVersionNumString() );
				msg->setTitle( "ecode" );
				msg->setCloseShortcut( { KEY_ESCAPE, 0 } );
				addStartUpCheckbox( msg );
			} else {
				if ( fromStartup )
					return;
				UIMessageBox* msg = UIMessageBox::New(
					UIMessageBox::OK, i18n( "ecode_no_updates_available",
											"There are currently no updates available." ) );
				msg->setTitle( "ecode" );
				msg->setCloseShortcut( { KEY_ESCAPE, 0 } );
				addStartUpCheckbox( msg );
			}
		} else {
			updatesError();
		}
	} catch ( ... ) {
		updatesError();
	}
}

void App::checkForUpdates( bool fromStartup ) {
	Http::getAsync(
		[&, fromStartup]( const Http&, Http::Request&, Http::Response& response ) {
			mUISceneNode->runOnMainThread(
				[&, response]() { checkForUpdatesResponse( response, fromStartup ); } );
		},
		"https://api.github.com/repos/SpartanJ/ecode/releases/latest", Seconds( 30 ) );
}

void App::aboutEcode() {
	String msg( ecode::Version::getVersionFullName() + " (codename: \"" +
				ecode::Version::getCodename() + "\")" );
	UIMessageBox* msgBox = UIMessageBox::New( UIMessageBox::OK, msg );
	UIImage* image = UIImage::New();
	image->setParent( msgBox->getContainer()->getFirstChild() );
	auto tf = TextureFactory::instance();
	Texture* tex = tf->getByName( "ecode-logo" );
	if ( tex == nullptr ) {
		tex = tf->loadFromFile( mResPath + "icon/ecode.png" );
		if ( tex )
			tex->setName( "ecode-logo" );
	}
	image->setDrawable( tex );
	image->setLayoutGravity( UI_NODE_ALIGN_CENTER );
	image->setGravity( UI_NODE_ALIGN_CENTER );
	image->setScaleType( UIScaleType::FitInside );
	image->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	image->setSize( { 128, 128 } );
	image->toBack();
	msgBox->setTitle( i18n( "about_ecode", "About ecode..." ) );
	msgBox->showWhenReady();
}

void App::ecodeSource() {
	Engine::instance()->openURI( "https://github.com/SpartanJ/ecode" );
}

void App::setUIScaleFactor() {
	UIMessageBox* msgBox = UIMessageBox::New(
		UIMessageBox::INPUT,
		i18n( "set_ui_scale_factor", "Set the UI scale factor (pixel density):\nMinimum value is "
									 "1, and maximum 6. Requires restart." ) );
	msgBox->setTitle( mWindowTitle );
	msgBox->getTextInput()->setText( String::format( "%.2f", mConfig.windowState.pixelDensity ) );
	msgBox->setCloseShortcut( { KEY_ESCAPE, 0 } );
	msgBox->showWhenReady();
	msgBox->addEventListener( Event::OnConfirm, [&, msgBox]( const Event* ) {
		msgBox->closeWindow();
		Float val;
		if ( String::fromString( val, msgBox->getTextInput()->getText() ) && val >= 1 &&
			 val <= 6 ) {
			if ( mConfig.windowState.pixelDensity != val ) {
				mConfig.windowState.pixelDensity = val;
				UIMessageBox* msg = UIMessageBox::New(
					UIMessageBox::OK,
					i18n( "new_ui_scale_factor", "New UI scale factor assigned.\nPlease "
												 "restart the application." ) );
				msg->showWhenReady();
				setFocusEditorOnClose( msg );
			} else if ( mSplitter && mSplitter->getCurWidget() ) {
				mSplitter->getCurWidget()->setFocus();
			}
		} else {
			UIMessageBox* msg = UIMessageBox::New( UIMessageBox::OK, "Invalid value!" );
			msg->showWhenReady();
			setFocusEditorOnClose( msg );
		}
	} );
}

PluginManager* App::getPluginManager() const {
	return mPluginManager.get();
}

void App::setEditorFontSize() {
	UIMessageBox* msgBox = UIMessageBox::New(
		UIMessageBox::INPUT, i18n( "set_editor_font_size", "Set the editor font size:" ) );
	msgBox->setTitle( mWindowTitle );
	msgBox->getTextInput()->setText( mConfig.editor.fontSize.toString() );
	msgBox->setCloseShortcut( { KEY_ESCAPE, 0 } );
	msgBox->showWhenReady();
	msgBox->addEventListener( Event::OnConfirm, [&, msgBox]( const Event* ) {
		mConfig.editor.fontSize = StyleSheetLength( msgBox->getTextInput()->getText() );
		mSplitter->forEachEditor( [&]( UICodeEditor* editor ) {
			editor->setFontSize( mConfig.editor.fontSize.asDp( 0, Sizef(), mDisplayDPI ) );
		} );
	} );
	setFocusEditorOnClose( msgBox );
}

void App::setTerminalFontSize() {
	UIMessageBox* msgBox = UIMessageBox::New(
		UIMessageBox::INPUT, i18n( "set_terminal_font_size", "Set the terminal font size:" ) );
	msgBox->setTitle( mWindowTitle );
	msgBox->getTextInput()->setText( mConfig.term.fontSize.toString() );
	msgBox->setCloseShortcut( { KEY_ESCAPE, 0 } );
	msgBox->showWhenReady();
	msgBox->addEventListener( Event::OnConfirm, [&, msgBox]( const Event* ) {
		mConfig.term.fontSize = StyleSheetLength( msgBox->getTextInput()->getText() );
		mSplitter->forEachWidget( [&]( UIWidget* widget ) {
			if ( widget && widget->isType( UI_TYPE_TERMINAL ) )
				widget->asType<UITerminal>()->setFontSize(
					mConfig.term.fontSize.asPixels( 0, Sizef(), mDisplayDPI ) );
		} );
	} );
	setFocusEditorOnClose( msgBox );
}

void App::setUIFontSize() {
	UIMessageBox* msgBox = UIMessageBox::New( UIMessageBox::INPUT,
											  i18n( "set_ui_font_size", "Set the UI font size:" ) );
	msgBox->setTitle( mWindowTitle );
	msgBox->getTextInput()->setText( mConfig.ui.fontSize.toString() );
	msgBox->setCloseShortcut( { KEY_ESCAPE, 0 } );
	msgBox->showWhenReady();
	msgBox->addEventListener( Event::OnConfirm, [&, msgBox]( const Event* ) {
		mConfig.ui.fontSize = StyleSheetLength( msgBox->getTextInput()->getText() );
		Float fontSize = mConfig.ui.fontSize.asDp( 0, Sizef(), mDisplayDPI );
		UIThemeManager* manager = mUISceneNode->getUIThemeManager();
		manager->setDefaultFontSize( fontSize );
		manager->getDefaultTheme()->setDefaultFontSize( fontSize );
		mUISceneNode->forEachNode( [&]( Node* node ) {
			if ( node->isType( UI_TYPE_TEXTVIEW ) ) {
				UITextView* textView = node->asType<UITextView>();
				if ( !textView->getUIStyle()->hasProperty( PropertyId::FontSize ) ) {
					textView->setFontSize(
						mConfig.ui.fontSize.asDp( node->getParent()->getPixelsSize().getWidth(),
												  Sizef(), mUISceneNode->getDPI() ) );
				}
			}
		} );
		msgBox->closeWindow();
	} );
	setFocusEditorOnClose( msgBox );
}

void App::setUIPanelFontSize() {
	UIMessageBox* msgBox = UIMessageBox::New(
		UIMessageBox::INPUT, i18n( "set_side_panel_font_size", "Set side panel font size:" ) );
	msgBox->setTitle( mWindowTitle );
	msgBox->getTextInput()->setText( mConfig.ui.panelFontSize.toString() );
	msgBox->setCloseShortcut( { KEY_ESCAPE, 0 } );
	msgBox->showWhenReady();
	msgBox->addEventListener( Event::OnConfirm, [&, msgBox]( const Event* ) {
		mConfig.ui.panelFontSize = StyleSheetLength( msgBox->getTextInput()->getText() );

		// Update the CSS
		auto selsFound = mUISceneNode->getStyleSheet().findStyleFromSelectorName(
			"#project_view > treeview::row > treeview::cell > treeview::cell::text" );
		if ( !selsFound.empty() ) {
			for ( auto sel : selsFound )
				sel->updatePropertyValue( "font-size", mConfig.ui.panelFontSize.toString() );
			mUISceneNode->getStyleSheet().refreshCacheFromStyles( selsFound );
		}

		UITreeView* treeView = mUISceneNode->find<UITreeView>( "project_view" );
		if ( !treeView ) {
			msgBox->closeWindow();
			return;
		}
		treeView->reloadStyle( true, true, true, true );
		treeView->updateContentSize();
		msgBox->closeWindow();
	} );
	setFocusEditorOnClose( msgBox );
}

void App::setFocusEditorOnClose( UIMessageBox* msgBox ) {
	msgBox->addEventListener( Event::OnClose, [&]( const Event* ) {
		if ( mSplitter && mSplitter->getCurWidget() )
			mSplitter->getCurWidget()->setFocus();
	} );
}

Drawable* App::findIcon( const std::string& name ) {
	return findIcon( name, mMenuIconSize );
}

Drawable* App::findIcon( const std::string& name, const size_t iconSize ) {
	UIIcon* icon = mUISceneNode->findIcon( name );
	if ( icon )
		return icon->getSize( iconSize );
	return nullptr;
}

String App::i18n( const std::string& key, const String& def ) {
	return mUISceneNode->getTranslatorStringFromKey( key, def );
}

std::string App::getCurrentWorkingDir() const {
	if ( !mCurrentProject.empty() )
		return mCurrentProject;

	if ( mSplitter && mSplitter->curEditorIsNotNull() && mSplitter->curEditorExists() &&
		 mSplitter->getCurEditor()->hasDocument() &&
		 mSplitter->getCurEditor()->getDocument().hasFilepath() ) {
		return mSplitter->getCurEditor()->getDocument().getFileInfo().getDirectoryPath();
	}

	return "";
}

std::vector<std::pair<String::StringBaseType, String::StringBaseType>>
App::makeAutoClosePairs( const std::string& strPairs ) {
	auto curPairs = String::split( strPairs, ',' );
	std::vector<std::pair<String::StringBaseType, String::StringBaseType>> pairs;
	for ( auto pair : curPairs ) {
		if ( pair.size() == 2 )
			pairs.emplace_back( std::make_pair( pair[0], pair[1] ) );
	}
	return pairs;
}

ProjectDocumentConfig& App::getProjectDocConfig() {
	return mProjectDocConfig;
}

const std::string& App::getWindowTitle() const {
	return mWindowTitle;
}

void App::setLineBreakingColumn() {
	UIMessageBox* msgBox = UIMessageBox::New(
		UIMessageBox::INPUT,
		i18n( "set_line_breaking_column", "Set Line Breaking Column:\nSet 0 to disable it.\n" )
			.unescape() );
	msgBox->setTitle( mWindowTitle );
	msgBox->setCloseShortcut( { KEY_ESCAPE, 0 } );
	msgBox->getTextInput()->setAllowOnlyNumbers( true, false );
	msgBox->getTextInput()->setText( String::toString( mConfig.doc.lineBreakingColumn ) );
	msgBox->showWhenReady();
	msgBox->addEventListener( Event::OnConfirm, [&, msgBox]( const Event* ) {
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

void App::setLineSpacing() {
	UIMessageBox* msgBox = UIMessageBox::New(
		UIMessageBox::INPUT,
		i18n( "set_line_spacing", "Set Line Spacing:\nSet 0 to disable it.\n" ).unescape() );
	msgBox->setTitle( mWindowTitle );
	msgBox->setCloseShortcut( { KEY_ESCAPE, 0 } );
	msgBox->getTextInput()->setText( mConfig.editor.lineSpacing.toString() );
	msgBox->showWhenReady();
	msgBox->addEventListener( Event::OnConfirm, [&, msgBox]( const Event* ) {
		mConfig.editor.lineSpacing = StyleSheetLength( msgBox->getTextInput()->getText() );
		mSplitter->forEachEditor(
			[&]( UICodeEditor* editor ) { editor->setLineSpacing( mConfig.editor.lineSpacing ); } );
	} );
	setFocusEditorOnClose( msgBox );
}

void App::setCursorBlinkingTime() {
	UIMessageBox* msgBox = UIMessageBox::New(
		UIMessageBox::INPUT,
		i18n( "set_cursor_blinking_time", "Set Cursor Blinking Time:\nSet 0 to disable it.\n" )
			.unescape() );
	msgBox->setTitle( mWindowTitle );
	msgBox->setCloseShortcut( { KEY_ESCAPE, 0 } );
	msgBox->getTextInput()->setText( mConfig.editor.cursorBlinkingTime.toString() );
	msgBox->showWhenReady();
	msgBox->addEventListener( Event::OnConfirm, [&, msgBox]( const Event* ) {
		mConfig.editor.cursorBlinkingTime =
			Time::fromString( msgBox->getTextInput()->getText().toUtf8() );
		mSplitter->forEachEditor( [&]( UICodeEditor* editor ) {
			editor->setCursorBlinkTime( mConfig.editor.cursorBlinkingTime );
		} );
		msgBox->closeWindow();
	} );
	setFocusEditorOnClose( msgBox );
}

void App::loadFileFromPathOrFocus( const std::string& path ) {
	UITab* tab = mSplitter->isDocumentOpen( path );
	if ( !tab ) {
		loadFileFromPath( path );
	} else {
		tab->getTabWidget()->setTabSelected( tab );
	}
}

void App::createPluginManagerUI() {
	UIPluginManager::New( mUISceneNode, mPluginManager.get(), [&]( const std::string& path ) {
		loadFileFromPathOrFocus( path );
	} )->showWhenReady();
}

void App::debugDrawHighlightToggle() {
	mUISceneNode->setHighlightFocus( !mUISceneNode->getHighlightFocus() );
	mUISceneNode->setHighlightOver( !mUISceneNode->getHighlightOver() );
}

void App::debugDrawBoxesToggle() {
	mUISceneNode->setDrawBoxes( !mUISceneNode->getDrawBoxes() );
}

void App::debugDrawData() {
	mUISceneNode->setDrawDebugData( !mUISceneNode->getDrawDebugData() );
}

static void updateKeybindings( IniFile& ini, const std::string& group, Input* input,
							   std::unordered_map<std::string, std::string>& keybindings,
							   const std::unordered_map<std::string, std::string>& defKeybindings,
							   bool forceRebind,
							   const std::map<std::string, std::string>& migrateKeyindings,
							   IniFile& iniState ) {
	KeyBindings bindings( input );
	bool added = false;
	bool migrated = false;

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

	if ( !added && forceRebind ) {
		for ( const auto& migrate : migrateKeyindings ) {
			auto foundCmd = invertedKeybindings.find( migrate.first );
			if ( foundCmd != invertedKeybindings.end() && foundCmd->second == migrate.second ) {
				std::string shortcut;
				for ( const auto& defKb : defKeybindings ) {
					if ( defKb.second == foundCmd->first ) {
						shortcut = defKb.first;
						break;
					}
				}
				if ( !shortcut.empty() &&
					 !iniState.keyValueExists( "migrated_keybindings_" + group, migrate.first ) ) {
					ini.setValue( group, shortcut, foundCmd->first );
					ini.deleteValue( group, migrate.second );
					keybindings.erase( migrate.second );
					invertedKeybindings[foundCmd->first] = shortcut;
					iniState.setValue( "migrated_keybindings_" + group, migrate.first,
									   migrate.second );
					added = true;
					migrated = true;
				}
			}
		}
	}

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

	if ( migrated )
		iniState.writeFile();
	if ( added )
		ini.writeFile();
}

static void updateKeybindings( IniFile& ini, const std::string& group, Input* input,
							   std::unordered_map<std::string, std::string>& keybindings,
							   std::unordered_map<std::string, std::string>& invertedKeybindings,
							   const std::map<KeyBindings::Shortcut, std::string>& defKeybindings,
							   bool forceRebind,
							   const std::map<std::string, std::string>& migrateKeyindings,
							   IniFile& iniState ) {
	KeyBindings bindings( input );
	bool added = false;
	bool migrated = false;

	if ( ini.findKey( group ) != IniFile::noID ) {
		keybindings = ini.getKeyUnorderedMap( group );
	} else {
		for ( const auto& it : defKeybindings )
			ini.setValue( group, bindings.getShortcutString( it.first ), it.second );
		added = true;
	}
	for ( const auto& key : keybindings )
		invertedKeybindings[key.second] = key.first;

	if ( !added && forceRebind ) {
		for ( const auto& migrate : migrateKeyindings ) {
			auto foundCmd = invertedKeybindings.find( migrate.first );
			if ( foundCmd != invertedKeybindings.end() && foundCmd->second == migrate.second ) {
				KeyBindings::Shortcut shortcut;
				for ( const auto& defKb : defKeybindings ) {
					if ( defKb.second == foundCmd->first ) {
						shortcut = defKb.first;
						break;
					}
				}
				if ( !shortcut.empty() &&
					 !iniState.keyValueExists( "migrated_keybindings_" + group, migrate.first ) ) {
					auto newShortcutStr = bindings.getShortcutString( shortcut );
					ini.setValue( group, newShortcutStr, foundCmd->first );
					ini.deleteValue( group, migrate.second );
					keybindings.erase( migrate.second );
					invertedKeybindings[foundCmd->first] = newShortcutStr;
					iniState.setValue( "migrated_keybindings_" + group, migrate.first,
									   migrate.second );
					added = true;
					migrated = true;
				}
			}
		}
	}

	bool keybindingsWereEmpty = keybindings.empty();

	if ( defKeybindings.size() != keybindings.size() || forceRebind ) {
		for ( auto& key : defKeybindings ) {
			auto foundCmd = invertedKeybindings.find( key.second );
			auto shortcutStr = bindings.getShortcutString( key.first );

			if ( ( foundCmd == invertedKeybindings.end() || keybindingsWereEmpty ) &&
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
	if ( migrated )
		iniState.writeFile();
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
						   getDefaultKeybindings(), forceRebind, getMigrateKeybindings(),
						   mConfig.iniState );

		updateKeybindings( ini, "global_search", mWindow->getInput(), mGlobalSearchKeybindings,
						   GlobalSearchController::getDefaultKeybindings(), forceRebind,
						   getMigrateKeybindings(), mConfig.iniState );

		updateKeybindings( ini, "document_search", mWindow->getInput(), mDocumentSearchKeybindings,
						   DocSearchController::getDefaultKeybindings(), forceRebind,
						   getMigrateKeybindings(), mConfig.iniState );
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
			TextDocument::lineEndingToString( doc.getLineEnding() ).c_str() ) );
	}
}

void App::syncProjectTreeWithEditor( UICodeEditor* editor ) {
	if ( mProjectTreeView && mConfig.editor.syncProjectTreeWithEditor && editor != nullptr &&
		 ( editor->getDocument().hasFilepath() ||
		   !editor->getDocument().getLoadingFilePath().empty() ) ) {
		std::string loadingPath( editor->getDocument().getLoadingFilePath() );
		std::string path = !loadingPath.empty() ? loadingPath : editor->getDocument().getFilePath();
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

	mSettings->updateDocumentMenu();
	mSettings->updateTerminalMenu();
	if ( widget && !widget->isType( UI_TYPE_CODEEDITOR ) ) {
		if ( widget->isType( UI_TYPE_TERMINAL ) )
			setAppTitle( widget->asType<UITerminal>()->getTitle() );
		else
			setAppTitle( "" );
	}
}

void App::onCodeEditorFocusChange( UICodeEditor* editor ) {
	updateDocInfo( editor->getDocument() );
	mDocSearchController->onCodeEditorFocusChange( editor );
	mUniversalLocator->onCodeEditorFocusChange( editor );
	syncProjectTreeWithEditor( editor );
}

void App::onColorSchemeChanged( const std::string& ) {
	mSettings->updateColorSchemeMenu();
	mGlobalSearchController->updateColorScheme( mSplitter->getCurrentColorScheme() );

	if ( mStatusBuildOutputController && mStatusBuildOutputController->getContainer() ) {
		mStatusBuildOutputController->getContainer()->setColorScheme(
			mSplitter->getCurrentColorScheme() );
	}

	mNotificationCenter->addNotification(
		String::format( i18n( "color_scheme_set", "Color scheme: %s" ).toUtf8().c_str(),
						mSplitter->getCurrentColorScheme().getName().c_str() ) );
}

void App::cleanUpRecentFiles() {
#if EE_PLATFORM == EE_PLATFORM_WIN
	for ( auto& file : mRecentFiles ) {
		if ( file.size() > 2 && file[1] == ':' )
			file[0] = std::toupper( file[0] );
	}
#endif

	std::vector<std::string> recentFiles;

	for ( const auto& file : mRecentFiles ) {
		if ( std::none_of( recentFiles.begin(), recentFiles.end(),
						   [file]( const auto& other ) { return other == file; } ) )
			recentFiles.emplace_back( file );
	}

	if ( mRecentFolders.size() != recentFiles.size() )
		mRecentFiles = recentFiles;
}

void App::loadFileDelayed() {
	if ( mFileToOpen.empty() )
		return;

	auto fileAndPos = getPathAndPosition( mFileToOpen );
	auto tab = mSplitter->isDocumentOpen( fileAndPos.first, false, true );

	if ( tab ) {
		tab->getTabWidget()->setTabSelected( tab );
		if ( tab->getOwnedWidget()->isType( UI_TYPE_CODEEDITOR ) ) {
			UICodeEditor* editor = tab->getOwnedWidget()->asType<UICodeEditor>();
			if ( editor->getDocument().isLoading() ) {
				Uint32 cb =
					editor->on( Event::OnDocumentLoaded, [fileAndPos]( const Event* event ) {
						if ( event->getNode()->isType( UI_TYPE_CODEEDITOR ) ) {
							UICodeEditor* editor = event->getNode()->asType<UICodeEditor>();
							editor->runOnMainThread(
								[editor, fileAndPos] { editor->goToLine( fileAndPos.second ); } );
						}
						event->getNode()->removeEventListener( event->getCallbackId() );
					} );
				// Don't listen forever if no event is received
				editor->runOnMainThread( [editor, cb]() { editor->removeEventListener( cb ); },
										 Seconds( 4 ) );
			} else {
				editor->runOnMainThread(
					[editor, fileAndPos] { editor->goToLine( fileAndPos.second ); } );
			}
		}
	} else {
		loadFileFromPath( fileAndPos.first, true, nullptr,
						  [fileAndPos]( UICodeEditor* editor, const std::string& ) {
							  editor->runOnMainThread(
								  [editor, fileAndPos] { editor->goToLine( fileAndPos.second ); } );
						  } );
	}

	mFileToOpen.clear();
}

const std::string& App::getThemesPath() const {
	return mThemesPath;
}

std::string App::getThemePath() const {
	if ( mConfig.ui.theme.empty() || "default_theme" == mConfig.ui.theme )
		return getDefaultThemePath();

	auto themePath( mThemesPath + mConfig.ui.theme + ".css" );
	if ( !FileSystem::fileExists( themePath ) )
		return getDefaultThemePath();

	return themePath;
}

std::string App::getDefaultThemePath() const {
	return mResPath + "ui/breeze.css";
}

void App::setTheme( const std::string& path ) {
	UITheme* theme = UITheme::load( "uitheme", "uitheme", "", mFont, path );
	theme->setDefaultFontSize( mConfig.ui.fontSize.asDp( 0, Sizef(), mDisplayDPI ) );

	if ( path != getDefaultThemePath() ) {
		auto style = theme->getStyleSheet().getStyleFromSelector( ":root" );
		if ( style ) {
			auto inheritsBaseTheme = style->getVariableByName( "--inherit-base-theme" );
			if ( !inheritsBaseTheme.isEmpty() ) {
				StyleSheetParser parser;
				parser.loadFromFile( getDefaultThemePath() );
				for ( auto& tstyle : parser.getStyleSheet().getStyles() ) {
					if ( tstyle->getSelector().getName() != ":root" ) {
						theme->getStyleSheet().addStyle( tstyle );
					} else {
						// Add root variables that arent defined in the custom theme
						auto root = theme->getStyleSheet().getStyleFromSelector( ":root" );
						if ( root ) {
							for ( const auto& var : tstyle->getVariables() ) {
								if ( !root->hasVariable( var.second.getName() ) )
									root->setVariable( var.second );
							}
						}
					}
				}
				theme->getStyleSheet().addKeyframes( parser.getStyleSheet().getKeyframes() );
			}
		}
	}

	mUISceneNode->setStyleSheet( theme->getStyleSheet() );
	mUISceneNode
		->getUIThemeManager()
		//->setDefaultEffectsEnabled( true )
		->setDefaultTheme( theme )
		->setDefaultFont( mFont )
		->setDefaultFontSize( mConfig.ui.fontSize.asDp( 0, Sizef(), mDisplayDPI ) )
		->add( theme );

	mUISceneNode->getRoot()->addClass( "appbackground" );

	if ( mTheme )
		mUISceneNode->getUIThemeManager()->remove( mTheme );

	mTheme = theme;

	mUISceneNode->reloadStyle( true, true );
}

void App::onRealDocumentLoaded( UICodeEditor* editor, const std::string& path ) {
	updateEditorTitle( editor );
	if ( mSplitter->curEditorExistsAndFocused() && editor == mSplitter->getCurEditor() )
		mSettings->updateCurrentFileType();
	mSplitter->removeUnusedTab( mSplitter->tabWidgetFromEditor( editor ) );
	auto found = std::find( mRecentFiles.begin(), mRecentFiles.end(), path );
	if ( found != mRecentFiles.end() )
		mRecentFiles.erase( found );
	mRecentFiles.insert( mRecentFiles.begin(), path );
	if ( mRecentFiles.size() > 10 )
		mRecentFiles.resize( 10 );
	cleanUpRecentFiles();
	updateRecentFiles();
	if ( mSplitter->curEditorExistsAndFocused() && mSplitter->getCurEditor() == editor ) {
		mSettings->updateDocumentMenu();
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
		if ( mFileWatcher )
			mFilesFolderWatches[dir] = mFileWatcher->addWatch( dir, mFileSystemListener );
	}
}

void App::onDocumentLoaded( UICodeEditor* editor, const std::string& path ) {
	onRealDocumentLoaded( editor, path );

	// Check if other editor is using the same document and needs to receive the same notification
	const TextDocument* docPtr = &editor->getDocument();
	mSplitter->forEachEditor( [&, docPtr, editor]( UICodeEditor* otherEditor ) {
		if ( otherEditor != editor && docPtr == &otherEditor->getDocument() ) {
			onRealDocumentLoaded( otherEditor, path );
		}
	} );
}

const CodeEditorConfig& App::getCodeEditorConfig() const {
	return mConfig.editor;
}

AppConfig& App::getConfig() {
	return mConfig;
}

std::map<KeyBindings::Shortcut, std::string> App::getDefaultKeybindings() {
	auto bindings = UICodeEditorSplitter::getDefaultKeybindings();
	auto local = getLocalKeybindings();
	auto app = TerminalManager::getTerminalKeybindings();
	local.insert( bindings.begin(), bindings.end() );
	local.insert( app.begin(), app.end() );
	return local;
}

std::map<KeyBindings::Shortcut, std::string> App::getLocalKeybindings() {
	return {
		{ { KEY_RETURN, KEYMOD_LALT | KEYMOD_LCTRL }, "fullscreen-toggle" },
		{ { KEY_F3, KEYMOD_NONE }, "repeat-find" },
		{ { KEY_F3, KEYMOD_SHIFT }, "find-prev" },
		{ { KEY_F12, KEYMOD_NONE }, "console-toggle" },
		{ { KEY_F, KeyMod::getDefaultModifier() }, "find-replace" },
		{ { KEY_Q, KeyMod::getDefaultModifier() | KEYMOD_SHIFT }, "close-app" },
		{ { KEY_O, KeyMod::getDefaultModifier() }, "open-file" },
		{ { KEY_W, KeyMod::getDefaultModifier() | KEYMOD_SHIFT }, "download-file-web" },
		{ { KEY_O, KeyMod::getDefaultModifier() | KEYMOD_SHIFT }, "open-folder" },
		{ { KEY_F11, KEYMOD_NONE }, "debug-widget-tree-view" },
		{ { KEY_K, KeyMod::getDefaultModifier() }, "open-locatebar" },
		{ { KEY_P, KeyMod::getDefaultModifier() }, "open-command-palette" },
		{ { KEY_F, KeyMod::getDefaultModifier() | KEYMOD_SHIFT }, "open-global-search" },
		{ { KEY_L, KeyMod::getDefaultModifier() }, "go-to-line" },
		{ { KEY_M, KeyMod::getDefaultModifier() }, "menu-toggle" },
		{ { KEY_S, KeyMod::getDefaultModifier() | KEYMOD_SHIFT }, "save-all" },
		{ { KEY_F9, KEYMOD_LALT }, "switch-side-panel" },
		{ { KEY_J, KEYMOD_CTRL | KEYMOD_LALT | KEYMOD_SHIFT }, "terminal-split-left" },
		{ { KEY_L, KEYMOD_CTRL | KEYMOD_LALT | KEYMOD_SHIFT }, "terminal-split-right" },
		{ { KEY_I, KEYMOD_CTRL | KEYMOD_LALT | KEYMOD_SHIFT }, "terminal-split-top" },
		{ { KEY_K, KEYMOD_CTRL | KEYMOD_LALT | KEYMOD_SHIFT }, "terminal-split-bottom" },
		{ { KEY_S, KEYMOD_CTRL | KEYMOD_LALT | KEYMOD_SHIFT }, "terminal-split-swap" },
		{ { KEY_T, KEYMOD_CTRL | KEYMOD_LALT | KEYMOD_SHIFT }, "reopen-closed-tab" },
		{ { KEY_1, KEYMOD_LALT }, "toggle-locatebar" },
		{ { KEY_2, KEYMOD_LALT }, "toggle-global-search" },
		{ { KEY_3, KEYMOD_LALT }, "toggle-status-build-output" },
		{ { KEY_4, KEYMOD_LALT }, "toggle-status-terminal" },
	};
}

// Old keybindings will be rebinded to the new keybindings of they are still set to the old
// keybindind
std::map<std::string, std::string> App::getMigrateKeybindings() {
	return {
		{ "fullscreen-toggle", "alt+return" }, { "switch-to-tab-1", "alt+1" },
		{ "switch-to-tab-2", "alt+2" },		   { "switch-to-tab-3", "alt+3" },
		{ "switch-to-tab-4", "alt+4" },		   { "switch-to-tab-5", "alt+5" },
		{ "switch-to-tab-6", "alt+6" },		   { "switch-to-tab-7", "alt+7" },
		{ "switch-to-tab-8", "alt+8" },		   { "switch-to-tab-9", "alt+9" },
		{ "switch-to-last-tab", "alt+0" },
	};
}

std::vector<std::string> App::getUnlockedCommands() {
	return { "create-new",
			 "create-new-terminal",
			 "fullscreen-toggle",
			 "open-file",
			 "open-folder",
			 "reopen-closed-tab",
			 "console-toggle",
			 "close-app",
			 "open-locatebar",
			 "open-command-palette",
			 "open-global-search",
			 "project-build-start",
			 "project-build-cancel",
			 "toggle-locatebar",
			 "toggle-global-search",
			 "toggle-status-build-output",
			 "toggle-status-terminal",
			 "menu-toggle",
			 "switch-side-panel",
			 "toggle-status-bar",
			 "download-file-web",
			 "create-new-terminal",
			 "terminal-split-left",
			 "terminal-split-right",
			 "terminal-split-top",
			 "terminal-split-bottom",
			 "terminal-split-swap",
			 "reopen-closed-tab",
			 "plugin-manager-open",
			 "debug-widget-tree-view",
			 "debug-draw-highlight-toggle",
			 "debug-draw-boxes-toggle",
			 "debug-draw-debug-data",
			 "editor-set-line-breaking-column",
			 "editor-set-line-spacing",
			 "editor-set-cursor-blinking-time",
			 "check-for-updates",
			 "keybindings",
			 "about-ecode",
			 "ecode-source",
			 "ui-scale-factor",
			 "show-side-panel",
			 "editor-font-size",
			 "terminal-font-size",
			 "ui-font-size",
			 "ui-panel-font-size",
			 "serif-font",
			 "monospace-font",
			 "terminal-font",
			 "fallback-font",
			 "tree-view-configure-ignore-files" };
}

bool App::isUnlockedCommand( const std::string& command ) {
	auto cmds = getUnlockedCommands();
	return std::find( cmds.begin(), cmds.end(), command ) != cmds.end();
}

void App::closeEditors() {
	mRecentClosedFiles = {};

	mSplitter->removeTabWithOwnedWidgetId( "welcome_ecode" );
	mStatusBar->setVisible( mConfig.ui.showStatusBar );

	mConfig.saveProject( mCurrentProject, mSplitter, mConfigPath, mProjectDocConfig,
						 mProjectBuildManager ? mProjectBuildManager->getConfig()
											  : ProjectBuildConfiguration() );
	std::vector<UICodeEditor*> editors = mSplitter->getAllEditors();
	while ( !editors.empty() ) {
		UICodeEditor* editor = editors[0];
		UITabWidget* tabWidget = mSplitter->tabWidgetFromEditor( editor );
		tabWidget->removeTab( (UITab*)editor->getData(), true, true );
		editors = mSplitter->getAllEditors();
		if ( editors.size() == 1 && editors[0]->getDocument().isEmpty() )
			break;
	};

	std::vector<UITerminal*> terminals;
	mSplitter->forEachWidgetType( UI_TYPE_TERMINAL, [&terminals]( UIWidget* widget ) {
		terminals.push_back( widget->asType<UITerminal>() );
	} );

	for ( UITerminal* terminal : terminals ) {
		UITabWidget* tabWidget = mSplitter->tabWidgetFromWidget( terminal );
		if ( tabWidget )
			tabWidget->removeTab( (UITab*)terminal->getData(), true, true );
	}

	mSplitter->forEachWidgetClass( "build_settings", []( UIWidget* widget ) {
		widget->asType<UIBuildSettings>()->getTab()->removeTab( true, true );
	} );

	mCurrentProject = "";
	mDirTree = nullptr;
	if ( mFileSystemListener )
		mFileSystemListener->setDirTree( mDirTree );

	mProjectDocConfig = ProjectDocumentConfig( mConfig.doc );
	mSettings->updateProjectSettingsMenu();
	if ( !mSplitter->getTabWidgets().empty() && mSplitter->getTabWidgets()[0]->getTabCount() == 0 )
		mSplitter->createCodeEditorInTabWidget( mSplitter->getTabWidgets()[0] );
}

void App::closeFolder() {
	if ( mCurrentProject.empty() )
		return;

	if ( mProjectBuildManager )
		mProjectBuildManager.reset();

	if ( mSplitter->isAnyEditorDirty() ) {
		UIMessageBox* msgBox = UIMessageBox::New(
			UIMessageBox::OK_CANCEL,
			i18n( "confirm_close_folder",
				  "Do you really want to close the folder?\nSome files haven't been saved." ) );
		msgBox->addEventListener( Event::OnConfirm, [&]( const Event* ) { closeEditors(); } );
		msgBox->setTitle( i18n( "close_folder_question", "Close Folder?" ) );
		msgBox->center();
		msgBox->showWhenReady();
	} else {
		closeEditors();
	}

	mProjectViewEmptyCont->setVisible( true );
	mFileSystemModel->setRootPath( "" );
	updateOpenRecentFolderBtn();
	UIWelcomeScreen::createWelcomeScreen( this );
	mStatusBar->setVisible( false );
}

void App::createDocAlert( UICodeEditor* editor ) {
	UILinearLayout* docAlert = editor->findByClass<UILinearLayout>( "doc_alert" );

	if ( docAlert )
		return;

	const auto msg = R"xml(
	<hbox class="doc_alert" layout_width="wrap_content" layout_height="wrap_content" layout_gravity="top|right" gravity-owner="true">
		<TextView id="doc_alert_text" layout_width="wrap_content" layout_height="wrap_content" margin-right="24dp"
			text='@string(reload_current_file, "The file on the disk is more recent that the current buffer.&#xA;Do you want to reload it?")'
		/>
		<PushButton id="file_reload" layout_width="wrap_content" layout_height="18dp" text='@string("reload", "Reload")' margin-right="4dp"
					tooltip='@string(tooltip_reload_file, "Reload the file from disk. Unsaved changes will be lost.")' />
		<PushButton id="file_overwrite" layout_width="wrap_content" layout_height="18dp" text='@string("overwrite", "Overwrite")' margin-right="4dp"
					tooltip='@string(tooltip_write_local_changes, "Writes the local changes on disk, overwriting the disk changes")' />
		<PushButton id="file_ignore" layout_width="wrap_content" layout_height="18dp" text='@string("ignore", "Ignore")'
					tooltip='@string(tooltip_ignore_file_changes, "Ignores the changes on disk without any action.")' />
	</hbox>
	)xml";
	docAlert = mUISceneNode->loadLayoutFromString( msg, editor )->asType<UILinearLayout>();

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

void App::loadImageFromMedium( const std::string& path, bool isMemory ) {
	UIImage* imageView = mImageLayout->findByType<UIImage>( UI_TYPE_IMAGE );
	UILoader* loaderView = mImageLayout->findByType<UILoader>( UI_TYPE_LOADER );
	if ( imageView ) {
		mImageLayout->setEnabled( true )->setVisible( true );
		loaderView->setVisible( true );
#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN || defined( __EMSCRIPTEN_PTHREADS__ )
		mThreadPool->run( [this, imageView, loaderView, path, isMemory]() {
#endif
			Texture* image =
				isMemory ? TextureFactory::instance()->loadFromMemory(
							   reinterpret_cast<const unsigned char*>( path.c_str() ), path.size() )
						 : TextureFactory::instance()->loadFromFile( path );
			if ( mImageLayout->isVisible() ) {
				imageView->runOnMainThread( [this, imageView, loaderView, image]() {
					mImageLayout->setFocus();
					imageView->setDrawable( image, true );
					loaderView->setVisible( false );
				} );
			} else {
				TextureFactory::instance()->remove( image );
				imageView->setDrawable( nullptr );
				loaderView->setVisible( false );
			}
#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN || defined( __EMSCRIPTEN_PTHREADS__ )
		} );
#endif
	}
}

void App::loadImageFromMemory( const std::string& content ) {
	loadImageFromMedium( content, true );
}

void App::loadImageFromPath( const std::string& path ) {
	loadImageFromMedium( path, false );
}

void App::loadFileFromPath(
	const std::string& path, bool inNewTab, UICodeEditor* codeEditor,
	std::function<void( UICodeEditor* codeEditor, const std::string& path )> onLoaded ) {
	if ( Image::isImageExtension( path ) && Image::isImage( path ) &&
		 FileSystem::fileExtension( path ) != "svg" ) {
		loadImageFromPath( path );
	} else {
		UITab* tab = mSplitter->isDocumentOpen( path );

		if ( tab && tab->getOwnedWidget()->isType( UI_TYPE_CODEEDITOR ) ) {
			UICodeEditor* editor = tab->getOwnedWidget()->asType<UICodeEditor>();
			if ( inNewTab ) {
				auto d = mSplitter->loadDocumentInNewTab( editor->getDocumentRef() );
				updateEditorTitle( d.second );
			} else {
				mSplitter->loadDocument( editor->getDocumentRef(), editor );
				updateEditorTitle( editor );
			}
		} else {
			if ( inNewTab ) {
				mSplitter->loadAsyncFileFromPathInNewTab( path, mThreadPool, onLoaded );
			} else {
				mSplitter->loadAsyncFileFromPath( path, mThreadPool, codeEditor, onLoaded );
			}
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
	mUniversalLocator->hideLocateBar();
}

void App::hideStatusTerminal() {
	mStatusTerminalController->hide();
}

void App::hideStatusBuildOutput() {
	mStatusBuildOutputController->hide();
}

StatusBuildOutputController* App::getStatusBuildOutputController() const {
	return mStatusBuildOutputController.get();
}

bool App::isDirTreeReady() const {
	return mDirTreeReady && mDirTree != nullptr;
}

NotificationCenter* App::getNotificationCenter() const {
	return mNotificationCenter.get();
}

void App::fullscreenToggle() {
	mWindow->toggleFullscreen();
	mSettings->getWindowMenu()
		->find( "fullscreen-toggle" )
		->asType<UIMenuCheckBox>()
		->setActive( !mWindow->isWindowed() );
}

void App::showGlobalSearch( bool searchAndReplace ) {
	mGlobalSearchController->showGlobalSearch( searchAndReplace );
}

void App::showFindView() {
	mDocSearchController->showFindView();
}

void App::createWidgetInspector() {
	UIWidgetInspector::create( mUISceneNode, mMenuIconSize );
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
	editor->setShowLineEndings( config.showLineEndings );
	editor->setShowIndentationGuides( config.showIndentationGuides );
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
	editor->setLineSpacing( config.lineSpacing );
	editor->setCursorBlinkTime( config.cursorBlinkingTime );
	doc.setAutoCloseBrackets( !mConfig.editor.autoCloseBrackets.empty() );
	doc.setAutoCloseBracketsPairs( makeAutoClosePairs( mConfig.editor.autoCloseBrackets ) );
	doc.setLineEnding( docc.lineEndings );
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
	doc.setCommand( "repeat-find", [&] {
		mDocSearchController->findNextText( mDocSearchController->getSearchState() );
	} );
	doc.setCommand( "find-prev", [&] {
		mDocSearchController->findPrevText( mDocSearchController->getSearchState() );
	} );
	doc.setCommand( "close-folder", [&] { closeFolder(); } );
	doc.setCommand( "lock", [&] {
		if ( mSplitter->curEditorExistsAndFocused() ) {
			mSplitter->getCurEditor()->setLocked( true );
			mSettings->updateDocumentMenu();
		}
	} );
	doc.setCommand( "unlock", [&] {
		if ( mSplitter->curEditorExistsAndFocused() ) {
			mSplitter->getCurEditor()->setLocked( false );
			mSettings->updateDocumentMenu();
		}
	} );
	doc.setCommand( "lock-toggle", [&] {
		if ( mSplitter->curEditorExistsAndFocused() ) {
			mSplitter->getCurEditor()->setLocked( !mSplitter->getCurEditor()->isLocked() );
			mSettings->updateDocumentMenu();
		}
	} );
	doc.setCommand( "go-to-line", [&] { mUniversalLocator->goToLine(); } );
	doc.setCommand( "load-current-dir", [&] { loadCurrentDirectory(); } );
	registerUnlockedCommands( doc );

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
		} else if ( mFileSystemMatcher && mFileSystemMatcher->getIgnoreFilePath() ==
											  editor->getDocument().getFilePath() ) {
			loadFileSystemMatcher( mFileSystemMatcher ? mFileSystemMatcher->getPath()
													  : mCurrentProject );
			refreshFolderView();
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
		mRecentClosedFiles.push( docEvent->getDoc()->getFilePath() );
		mSettings->updatedReopenClosedFileState();
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
		editor->getDocument().resetSyntax();
		editor->setSyntaxDefinition( editor->getDocument().getSyntaxDefinition() );
	} );

	auto docChanged = [this]( const Event* event ) {
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
		editor->getDocument().setHAsCpp( mProjectDocConfig.hAsCPP );
	};

	auto docLoaded = [this, editor, docChanged]( const Event* event ) {
		if ( editor->getDocument().getFileInfo().getExtension() == "svg" ) {
			editor->getDocument().setCommand( "show-image-preview", [this, editor]() {
				loadImageFromMemory( editor->getDocument().getText().toUtf8() );
			} );
			editor->addEventListener( Event::OnCreateContextMenu, [this]( const Event* event ) {
				auto cevent = static_cast<const ContextMenuEvent*>( event );
				cevent->getMenu()
					->add( i18n( "show_image_preview", "Show Image Preview" ),
						   findIcon( "filetype-jpg" ) )
					->setId( "show-image-preview" );
			} );
		}

		docChanged( event );
	};

	editor->addEventListener( Event::OnDocumentLoaded, docLoaded );
	editor->addEventListener( Event::OnDocumentChanged, docChanged );
	editor->addEventListener( Event::OnDocumentSave, docChanged );
	editor->addEventListener( Event::OnEditorTabReady, docChanged );

	editor->showMinimap( config.minimap );
	editor->showLinesRelativePosition( config.linesRelativePosition );

	mPluginManager->onNewEditor( editor );
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

GlobalSearchController* App::getGlobalSearchController() const {
	return mGlobalSearchController.get();
}

const std::shared_ptr<FileSystemModel>& App::getFileSystemModel() const {
	return mFileSystemModel;
}

void App::reopenClosedTab() {
	if ( mRecentClosedFiles.empty() )
		return;

	auto prevTabPath = mRecentClosedFiles.top();
	mRecentClosedFiles.pop();

	mSettings->updatedReopenClosedFileState();

	loadFileFromPath( prevTabPath );
}

void App::updateEditorState() {
	if ( mSplitter->curEditorExistsAndFocused() ) {
		updateEditorTitle( mSplitter->getCurEditor() );
		mSettings->updateCurrentFileType();
		mSettings->updateDocumentMenu();
	}
}

void App::removeFolderWatches() {
	std::unordered_set<efsw::WatchID> folderWatches;
	std::unordered_map<std::string, efsw::WatchID> filesFolderWatches;

	Lock l( mWatchesLock );
	if ( !mFileWatcher )
		return;
	folderWatches = mFolderWatches;
	filesFolderWatches = mFilesFolderWatches;
	mFolderWatches.clear();
	mFilesFolderWatches.clear();

	for ( const auto& dir : folderWatches )
		mFileWatcher->removeWatch( dir );

	for ( const auto& fileFolder : filesFolderWatches )
		mFileWatcher->removeWatch( fileFolder.second );
}

void App::loadDirTree( const std::string& path ) {
	Clock* clock = eeNew( Clock, () );
	mDirTreeReady = false;
	mDirTree = std::make_shared<ProjectDirectoryTree>( path, mThreadPool, this );
	Log::info( "Loading DirTree: %s", path.c_str() );
	mDirTree->scan(
		[&, clock]( ProjectDirectoryTree& dirTree ) {
			Log::info( "DirTree read in: %.2fms. Found %ld files.",
					   clock->getElapsedTime().asMilliseconds(), dirTree.getFilesCount() );
			eeDelete( clock );
			mDirTreeReady = true;
			mUISceneNode->runOnMainThread( [&] {
				mUniversalLocator->updateFilesTable();
				if ( mSplitter->curEditorExistsAndFocused() )
					syncProjectTreeWithEditor( mSplitter->getCurEditor() );
			} );
			removeFolderWatches();
			{
				Lock l( mWatchesLock );
				if ( mFileWatcher )
					mFolderWatches.insert(
						mFileWatcher->addWatch( dirTree.getPath(), mFileSystemListener, true ) );
			}
			mFileSystemListener->setDirTree( mDirTree );
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

void App::toggleSettingsMenu() {
	mSettings->toggleSettingsMenu();
}

void App::createNewTerminal() {
	if ( mSplitter->hasSplit() ) {
		if ( mSplitter->getTabWidgets().size() == 2 ) {
			UIOrientation orientation = mSplitter->getMainSplitOrientation();
			if ( mConfig.term.newTerminalOrientation == NewTerminalOrientation::Vertical &&
				 orientation == UIOrientation::Horizontal ) {
				mTerminalManager->createNewTerminal( "", mSplitter->getTabWidgets()[1] );
				return;
			}
			if ( mConfig.term.newTerminalOrientation == NewTerminalOrientation::Horizontal &&
				 orientation == UIOrientation::Vertical ) {
				mTerminalManager->createNewTerminal( "", mSplitter->getTabWidgets()[1] );
				return;
			}
		}
		mTerminalManager->createNewTerminal();
	} else {
		switch ( mConfig.term.newTerminalOrientation ) {
			case NewTerminalOrientation::Vertical: {
				runCommand( "terminal-split-right" );
				break;
			}
			case NewTerminalOrientation::Horizontal: {
				runCommand( "terminal-split-bottom" );
				break;
			}
			case NewTerminalOrientation::Same: {
				mTerminalManager->createNewTerminal();
				break;
			}
		}
	}
}

std::string App::getNewFilePath( const FileInfo& file, UIMessageBox* msgBox, bool keepDir ) {
	auto fileName( msgBox->getTextInput()->getText().toUtf8() );
	auto folderPath( file.getDirectoryPath() );
	if ( file.isDirectory() && !keepDir ) {
		FileSystem::dirRemoveSlashAtEnd( folderPath );
		folderPath = FileSystem::fileRemoveFileName( folderPath );
	}
	FileSystem::dirAddSlashAtEnd( folderPath );
	return folderPath + fileName;
}

const std::stack<std::string>& App::getRecentClosedFiles() const {
	return mRecentClosedFiles;
}

void App::updateTerminalMenu() {
	mSettings->updateTerminalMenu();
}

UIMessageBox* App::newInputMsgBox( const String& title, const String& msg ) {
	UIMessageBox* msgBox = UIMessageBox::New( UIMessageBox::INPUT, msg );
	msgBox->setTitle( title );
	msgBox->setCloseShortcut( { KEY_ESCAPE, 0 } );
	msgBox->showWhenReady();
	return msgBox;
}

static void fsRenameFile( const std::string& fpath, const std::string& newFilePath ) {
#if EE_PLATFORM == EE_PLATFORM_WIN
	fs::rename( String( fpath ).toWideString(), String( newFilePath ).toWideString() );
#else
	fs::rename( fpath, newFilePath );
#endif
}

void App::renameFile( const FileInfo& file ) {
	if ( !file.exists() )
		return;
	UIMessageBox* msgBox =
		newInputMsgBox( i18n( "rename_file", "Rename file" ) + " \"" + file.getFileName() + "\"",
						i18n( "enter_new_file_name", "Enter new file name:" ) );
	msgBox->getTextInput()->setText( file.getFileName() );
	msgBox->addEventListener( Event::OnConfirm, [&, file, msgBox]( const Event* ) {
		auto newFilePath( getNewFilePath( file, msgBox, false ) );
		if ( !FileSystem::fileExists( newFilePath ) ||
			 file.getFileName() != msgBox->getTextInput()->getText() ) {
			try {
				std::string fpath( file.getFilepath() );
				if ( file.isDirectory() )
					FileSystem::dirRemoveSlashAtEnd( fpath );
				fsRenameFile( fpath, newFilePath );
			} catch ( const fs::filesystem_error& err ) {
				errorMsgBox( i18n( "error_renaming_file", "Error renaming file." ) );
			}
			msgBox->closeWindow();
		} else {
			fileAlreadyExistsMsgBox();
		}
	} );
}

void App::toggleHiddenFiles() {
	mFileSystemModel = FileSystemModel::New( mFileSystemModel->getRootPath(),
											 FileSystemModel::Mode::FilesAndDirectories,
											 { true,
											   true,
											   !mFileSystemModel->getDisplayConfig().ignoreHidden,
											   {},
											   [&]( const std::string& filePath ) -> bool {
												   return isFileVisibleInTreeView( filePath );
											   } } );
	if ( mProjectTreeView )
		mProjectTreeView->setModel( mFileSystemModel );
	if ( mFileSystemListener )
		mFileSystemListener->setFileSystemModel( mFileSystemModel );
}

void App::newFile( const FileInfo& file ) {
	UIMessageBox* msgBox = newInputMsgBox( i18n( "create_new_file", "Create new file" ),
										   i18n( "enter_new_file_name", "Enter new file name:" ) );
	msgBox->addEventListener( Event::OnConfirm, [&, file, msgBox]( const Event* ) {
		auto newFilePath( getNewFilePath( file, msgBox ) );
		if ( !FileSystem::fileExists( newFilePath ) ) {
			// Needs to create sub folders?
			if ( msgBox->getTextInput()->getText().find_first_of( "/\\" ) != String::InvalidPos ) {
				auto folderPath( FileSystem::removeLastFolderFromPath( newFilePath ) );
				if ( !FileSystem::makeDir( folderPath, true ) ) {
					errorMsgBox( i18n( "couldnt_create_folder_of_file",
									   "Couldn't create folder of file." ) );
					msgBox->closeWindow();
					return;
				}
			}

			if ( !FileSystem::fileWrite( newFilePath, nullptr, 0 ) ) {
				errorMsgBox( i18n( "couldnt_create_file", "Couldn't create file." ) );
			} else if ( mProjectTreeView ) {
				// We wait 100 ms to get the notification from the file system
				mUISceneNode->runOnMainThread(
					[&, newFilePath] {
						if ( !mFileSystemModel || !mProjectTreeView )
							return;
						std::string nfp( newFilePath );
						FileSystem::filePathRemoveBasePath( mFileSystemModel->getRootPath(), nfp );
						mProjectTreeView->selectRowWithPath( nfp );
					},
					Milliseconds( 100 ) );
			}

			msgBox->closeWindow();
		} else {
			fileAlreadyExistsMsgBox();
		}
	} );
}

void App::newFolder( const FileInfo& file ) {
	UIMessageBox* msgBox =
		newInputMsgBox( i18n( "create_new_folder", "Create new folder" ),
						i18n( "enter_new_folder_name", "Enter new folder name:" ) );
	msgBox->addEventListener( Event::OnConfirm, [&, file, msgBox]( const Event* ) {
		auto newFolderPath( getNewFilePath( file, msgBox ) );
		if ( !FileSystem::fileExists( newFolderPath ) ) {
			if ( !FileSystem::makeDir( newFolderPath ) ) {
				errorMsgBox( i18n( "couldnt_create_directory", "Couldn't create directory." ) );
			} else if ( mProjectTreeView ) {
				// We wait 100 ms to get the notification from the file system
				mUISceneNode->runOnMainThread(
					[&, newFolderPath] {
						if ( !mFileSystemModel || !mProjectTreeView )
							return;
						std::string nfp( newFolderPath );
						FileSystem::filePathRemoveBasePath( mFileSystemModel->getRootPath(), nfp );
						mProjectTreeView->selectRowWithPath( nfp );
					},
					Milliseconds( 100 ) );
			}
			msgBox->closeWindow();
		} else {
			fileAlreadyExistsMsgBox();
		}
	} );
}

void App::createAndShowRecentFolderPopUpMenu( Node* recentFolderBut ) {
	UIPopUpMenu* menu = UIPopUpMenu::New();
	if ( mRecentFolders.empty() )
		return;
	for ( const auto& file : mRecentFolders )
		menu->add( file );
	menu->addEventListener( Event::OnItemClicked, [&]( const Event* event ) {
		if ( !event->getNode()->isType( UI_TYPE_MENUITEM ) )
			return;
		const String& txt = event->getNode()->asType<UIMenuItem>()->getText();
		loadFolder( txt );
	} );
	auto pos = recentFolderBut->getScreenPos();
	pos.y += recentFolderBut->getPixelsSize().getHeight();
	UIMenu::findBestMenuPos( pos, menu );
	menu->setPixelsPosition( pos );
	menu->show();
}

void App::createAndShowRecentFilesPopUpMenu( Node* recentFilesBut ) {
	UIPopUpMenu* menu = UIPopUpMenu::New();
	if ( mRecentFiles.empty() )
		return;
	for ( const auto& file : mRecentFiles )
		menu->add( file );
	menu->addEventListener( Event::OnItemClicked, [&]( const Event* event ) {
		if ( !event->getNode()->isType( UI_TYPE_MENUITEM ) )
			return;
		const String& txt = event->getNode()->asType<UIMenuItem>()->getText();
		loadFileFromPath( txt );
	} );
	auto pos = recentFilesBut->getScreenPos();
	pos.y += recentFilesBut->getPixelsSize().getHeight();
	UIMenu::findBestMenuPos( pos, menu );
	menu->setPixelsPosition( pos );
	menu->show();
}

UISplitter* App::getMainSplitter() const {
	return mMainSplitter;
}

StatusTerminalController* App::getStatusTerminalController() const {
	return mStatusTerminalController.get();
}

void App::updateOpenRecentFolderBtn() {
	if ( mProjectViewEmptyCont ) {
		Node* recentFolderBtn = mProjectViewEmptyCont->find( "open_recent_folder" );
		if ( recentFolderBtn )
			recentFolderBtn->setEnabled( !mRecentFolders.empty() );
	}
}

void App::consoleToggle() {
	mConsole->toggle();
	bool lock = mConsole->isActive();
	mSplitter->forEachEditor( [lock]( UICodeEditor* editor ) { editor->setLocked( lock ); } );
	if ( !lock && mSplitter->getCurWidget() )
		mSplitter->getCurWidget()->setFocus();
}

void App::initProjectTreeView( std::string path ) {
	mProjectViewEmptyCont = mUISceneNode->find<UILinearLayout>( "project_view_empty" );
	mProjectViewEmptyCont->find<UIPushButton>( "open_folder" )
		->addEventListener( Event::MouseClick, [&]( const Event* event ) {
			if ( event->asMouseEvent()->getFlags() & EE_BUTTON_LMASK )
				runCommand( "open-folder" );
		} );
	mProjectViewEmptyCont->find<UIPushButton>( "open_recent_folder" )
		->addEventListener( Event::MouseClick, [&]( const Event* event ) {
			if ( event->asMouseEvent()->getFlags() & EE_BUTTON_LMASK )
				createAndShowRecentFolderPopUpMenu(
					mProjectViewEmptyCont->find<UIPushButton>( "open_recent_folder" ) );
		} );
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
					mSettings->createProjectTreeMenu( FileInfo( path ) );
				}
			}
		}
	} );
	mProjectTreeView->addEventListener( Event::MouseClick, [&]( const Event* event ) {
		const MouseEvent* mouseEvent = static_cast<const MouseEvent*>( event );
		if ( mouseEvent->getFlags() & EE_BUTTON_RMASK )
			mSettings->createProjectTreeMenu();
	} );
	mProjectTreeView->addEventListener( Event::KeyDown, [&]( const Event* event ) {
		if ( !mFileSystemModel )
			return 0;
		const KeyEvent* keyEvent = static_cast<const KeyEvent*>( event );
		if ( keyEvent->getKeyCode() == KEY_F2 || keyEvent->getKeyCode() == KEY_DELETE ) {
			ModelIndex modelIndex = mProjectTreeView->getSelection().first();
			if ( !modelIndex.isValid() )
				return 0;
			Variant vPath( mProjectTreeView->getModel()->data( modelIndex, ModelRole::Custom ) );
			if ( vPath.isValid() && vPath.is( Variant::Type::cstr ) ) {
				FileInfo fileInfo( vPath.asCStr() );
				if ( keyEvent->getKeyCode() == KEY_F2 ) {
					renameFile( fileInfo );
				} else {
					mSettings->deleteFileDialog( fileInfo );
				}
			}
			return 1;
		}

		if ( mSplitter->curEditorExistsAndFocused() ) {
			std::string cmd = mSplitter->getCurEditor()->getKeyBindings().getCommandFromKeyBind(
				{ keyEvent->getKeyCode(), keyEvent->getMod() } );
			if ( !cmd.empty() && mSplitter->getCurEditor()->isUnlockedCommand( cmd ) ) {
				mSplitter->getCurEditor()->getDocument().execute( cmd );
				return 1;
			}
		}

		return 0;
	} );

	bool hasPosition = pathHasPosition( path );
	TextPosition initialPosition;
	if ( hasPosition ) {
		auto pathAndPosition = getPathAndPosition( path );
		path = pathAndPosition.first;
		initialPosition = pathAndPosition.second;
	}

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
				loadFileSystemMatcher( folderPath );

				mFileSystemModel = FileSystemModel::New(
					folderPath, FileSystemModel::Mode::FilesAndDirectories,
					{ true, true, true, {}, [&]( const std::string& filePath ) -> bool {
						 return isFileVisibleInTreeView( filePath );
					 } } );

				mProjectTreeView->setModel( mFileSystemModel );
				mProjectViewEmptyCont->setVisible( false );

				if ( mFileSystemListener )
					mFileSystemListener->setFileSystemModel( mFileSystemModel );

				std::function<void( UICodeEditor * codeEditor, const std::string& path )>
					forcePosition;
				if ( initialPosition.isValid() ) {
					forcePosition = [initialPosition]( UICodeEditor* editor, const auto& ) {
						editor->runOnMainThread(
							[initialPosition, editor] { editor->goToLine( initialPosition ); } );
					};
				}

				if ( FileSystem::fileExists( rpath ) ) {
					loadFileFromPath( rpath, false, nullptr, forcePosition );
				} else if ( FileSystem::fileCanWrite( folderPath ) ) {
					loadFileFromPath( path, false, nullptr, forcePosition );
				}
			}
		}
	} else if ( mConfig.workspace.restoreLastSession && !mRecentFolders.empty() ) {
		loadFolder( mRecentFolders[0] );
	} else {
		updateOpenRecentFolderBtn();

		UIWelcomeScreen::createWelcomeScreen( this );
		mStatusBar->setVisible( false );
	}

	mProjectTreeView->setAutoExpandOnSingleColumn( true );
}

void App::initImageView() {
	mImageLayout->on( Event::MouseClick, [&]( const Event* ) {
		mImageLayout->findByType<UIImage>( UI_TYPE_IMAGE )->setDrawable( nullptr );
		mImageLayout->setEnabled( false )->setVisible( false );
	} );
	mImageLayout->on( Event::KeyDown, [this]( const Event* event ) {
		if ( event->asKeyEvent()->getKeyCode() == KEY_ESCAPE ) {
			mImageLayout->findByType<UIImage>( UI_TYPE_IMAGE )->setDrawable( nullptr );
			mImageLayout->setEnabled( false )->setVisible( false );
			if ( mSplitter->getCurWidget() )
				mSplitter->getCurWidget()->setFocus();
		}
	} );
}

bool App::isFileVisibleInTreeView( const std::string& filePath ) {
	if ( !mFileSystemMatcher || !mFileSystemMatcher->matcherReady() )
		return true;
	auto fpath( filePath );
	FileSystem::filePathRemoveBasePath( mCurrentProject, fpath );
	FileSystem::dirRemoveSlashAtEnd( fpath );
	return !mFileSystemMatcher->match( fpath );
}

void App::treeViewConfigureIgnoreFiles() {
	if ( !mFileSystemModel || mFileSystemModel->getRootPath().empty() ) {
		errorMsgBox( i18n( "open_first_a_folder", "You must first open a folder." ) );
		return;
	}

	std::string ignoreFilePath;

	if ( mFileSystemMatcher && !mFileSystemMatcher->getIgnoreFilePath().empty() ) {
		ignoreFilePath = mFileSystemMatcher->getIgnoreFilePath();
	} else {
		ignoreFilePath = mFileSystemModel->getRootPath();
		FileSystem::dirAddSlashAtEnd( ignoreFilePath );
		ignoreFilePath += ".ecode/.fstreeviewignore";
	}

	if ( !FileSystem::fileExists( ignoreFilePath ) ) {
		bool dirExists = true;
		std::string dirPath( FileSystem::fileRemoveFileName( ignoreFilePath ) );
		if ( !FileSystem::fileExists( dirPath ) )
			dirExists = FileSystem::makeDir( dirPath );

		if ( !dirExists ) {
			errorMsgBox( i18n( "couldnt_create_ecode_dir",
							   "Couldn't create .ecode directory in current folder." ) );
			return;
		}

		if ( !FileSystem::fileWrite( ignoreFilePath, "\n" ) ) {
			errorMsgBox( i18n( "couldnt_write_file", "Couldn't write file on disk." ) );
			return;
		}

		loadFileSystemMatcher( mFileSystemModel->getRootPath() );
	}

	loadFileFromPath( ignoreFilePath );
}

void App::loadFileSystemMatcher( const std::string& folderPath ) {
	if ( folderPath.empty() )
		return;
	mFileSystemMatcher =
		std::make_shared<GitIgnoreMatcher>( folderPath, ".ecode/.fstreeviewignore" );
}

static std::string GetCurrentProcessName() {
#if defined( __APPLE__ ) || defined( __FreeBSD__ )
	return getprogname();
#elif defined( _GNU_SOURCE ) && EE_PLATFORM != EE_PLATFORM_ANDROID && EE_PLATFORM != EE_PLATFORM_IOS
	return program_invocation_name;
#elif defined( _WIN32 )
	return __argv[0];
#else
	return "ecode";
#endif
}

#if EE_PLATFORM == EE_PLATFORM_MACOSX
#include <libproc.h>
#include <unistd.h>
#endif

static std::string getCurrentProcessPath() {
#if EE_PLATFORM != EE_PLATFORM_MACOSX && EE_PLATFORM != EE_PLATFORM_ANDROID
	auto path( Sys::getProcessPath() );
	FileSystem::dirAddSlashAtEnd( path );
	if ( String::startsWith( GetCurrentProcessName(), Sys::getProcessPath() ) ) {
		auto proc( GetCurrentProcessName() );
		FileSystem::filePathRemoveProcessPath( proc );
		path += proc;
	} else {
		path += GetCurrentProcessName();
	}
	return path;
#elif EE_PLATFORM == EE_PLATFORM_ANDROID
	return Sys::getProcessPath();
#else
	char pathbuf[PROC_PIDPATHINFO_MAXSIZE];
	pid_t pid = getpid();
	int ret = proc_pidpath( pid, pathbuf, sizeof( pathbuf ) );
	if ( ret >= 0 )
		return std::string( pathbuf );
	auto path( Sys::getProcessPath() );
	FileSystem::dirAddSlashAtEnd( path );
	return path + GetCurrentProcessName();
#endif
}

void App::checkLanguagesHealth() {
	auto path( getCurrentProcessPath() );
#if EE_PLATFORM == EE_PLATFORM_WIN
	UITerminal* term = mTerminalManager->createNewTerminal(
		"", nullptr, "", Sys::getPlatform() == "Windows" ? "cmd.exe" : "" );
#else
	UITerminal* term = mTerminalManager->createNewTerminal();
#endif
	term->setFocus();
	term->executeBinary( path, "--health" );
}

void App::cleanUpRecentFolders() {
	for ( auto& folder : mRecentFolders ) {
		FileSystem::dirAddSlashAtEnd( folder );
	}

	std::vector<std::string> recentFolders;

	for ( const auto& folder : mRecentFolders ) {
		if ( std::none_of( recentFolders.begin(), recentFolders.end(),
						   [folder]( const auto& other ) { return other == folder; } ) )
			recentFolders.emplace_back( folder );
	}

	if ( mRecentFolders.size() != recentFolders.size() )
		mRecentFolders = recentFolders;
}

void App::loadFolder( const std::string& path ) {
	Clock dirTreeClock;
	if ( !mCurrentProject.empty() ) {
		closeEditors();
	} else {
		mSplitter->removeTabWithOwnedWidgetId( "welcome_ecode" );
		mStatusBar->setVisible( mConfig.ui.showStatusBar );
	}

	mProjectViewEmptyCont->setVisible( false );

	std::string rpath( FileSystem::getRealPath( path ) );
	FileSystem::dirAddSlashAtEnd( rpath );

	mCurrentProject = rpath;
	mPluginManager->setWorkspaceFolder( rpath );

	loadDirTree( rpath );

	Clock projClock;
	mProjectBuildManager =
		std::make_unique<ProjectBuildManager>( rpath, mThreadPool, mSidePanel, this );
	mConfig.loadProject( rpath, mSplitter, mConfigPath, mProjectDocConfig, mThreadPool, this );
	Log::info( "Load project took: %.2f ms", projClock.getElapsedTime().asMilliseconds() );

	loadFileSystemMatcher( rpath );

	mFileSystemModel =
		FileSystemModel::New( rpath, FileSystemModel::Mode::FilesAndDirectories,
							  { true, true, true, {}, [&]( const std::string& filePath ) -> bool {
								   return isFileVisibleInTreeView( filePath );
							   } } );

	if ( mProjectTreeView )
		mProjectTreeView->setModel( mFileSystemModel );

	if ( mFileSystemListener )
		mFileSystemListener->setFileSystemModel( mFileSystemModel );

	auto found = std::find( mRecentFolders.begin(), mRecentFolders.end(), rpath );
	if ( found != mRecentFolders.end() )
		mRecentFolders.erase( found );
	mRecentFolders.insert( mRecentFolders.begin(), rpath );
	if ( mRecentFolders.size() > 10 )
		mRecentFolders.resize( 10 );

	cleanUpRecentFolders();
	updateRecentFolders();
	mSettings->updateProjectSettingsMenu();

	if ( mSplitter->getCurWidget() )
		mSplitter->getCurWidget()->setFocus();
}

FontTrueType* App::loadFont( const std::string& name, std::string fontPath,
							 const std::string& fallback ) {
	if ( FileSystem::isRelativePath( fontPath ) )
		fontPath = mResPath + fontPath;
#if EE_PLATFORM == EE_PLATFORM_ANDROID
	if ( fontPath.empty() ||
		 ( !FileSystem::fileExists( fontPath ) && !PackManager::instance()->exists( fontPath ) ) ) {
#else
	if ( fontPath.empty() || !FileSystem::fileExists( fontPath ) ) {
#endif
		fontPath = fallback;
		if ( !fontPath.empty() && FileSystem::isRelativePath( fontPath ) )
			fontPath = mResPath + fontPath;
	}
	if ( fontPath.empty() )
		return nullptr;
	return FontTrueType::New( name, fontPath );
}

void App::init( const LogLevel& logLevel, std::string file, const Float& pidelDensity,
				const std::string& colorScheme, bool terminal, bool frameBuffer, bool benchmarkMode,
				const std::string& css, bool health, const std::string& healthLang,
				FeaturesHealth::OutputFormat healthFormat, const std::string& fileToOpen,
				bool stdOutLogs, bool disableFileLogs ) {
	DisplayManager* displayManager = Engine::instance()->getDisplayManager();
	Display* currentDisplay = displayManager->getDisplayIndex( 0 );
	mDisplayDPI = currentDisplay->getDPI();
	mUseFrameBuffer = frameBuffer;
	mBenchmarkMode = benchmarkMode;

	mResPath = Sys::getProcessPath();
#if EE_PLATFORM == EE_PLATFORM_LINUX
	if ( String::contains( mResPath, ".mount_" ) )
		FileSystem::dirAddSlashAtEnd( mResPath );
#elif EE_PLATFORM == EE_PLATFORM_EMSCRIPTEN
	mResPath += "ecode/";
#endif
	mResPath += "assets";
	FileSystem::dirAddSlashAtEnd( mResPath );

	loadConfig( logLevel, currentDisplay->getSize(), health, stdOutLogs, disableFileLogs );

	if ( health ) {
		Sys::windowAttachConsole();
		FeaturesHealth::doHealth( mPluginManager.get(), healthLang, healthFormat );
		return;
	}

	currentDisplay = displayManager->getDisplayIndex( mConfig.windowState.displayIndex <
															  displayManager->getDisplayCount()
														  ? mConfig.windowState.displayIndex
														  : 0 );
	mDisplayDPI = currentDisplay->getDPI();

#if EE_PLATFORM == EE_PLATFORM_ANDROID
	mConfig.windowState.pixelDensity =
		pidelDensity > 0
			? pidelDensity
			: ( mConfig.windowState.pixelDensity > 0	? mConfig.windowState.pixelDensity
				: currentDisplay->getPixelDensity() > 2 ? currentDisplay->getPixelDensity() / 2
														: currentDisplay->getPixelDensity() );
#else
	mConfig.windowState.pixelDensity =
		pidelDensity > 0
			? pidelDensity
			: ( mConfig.windowState.pixelDensity > 0 ? mConfig.windowState.pixelDensity
													 : currentDisplay->getPixelDensity() );
#endif

	displayManager->enableScreenSaver();
	displayManager->enableMouseFocusClickThrough();
	displayManager->disableBypassCompositor();

	Engine* engine = Engine::instance();

	WindowSettings winSettings = engine->createWindowSettings( &mConfig.iniState, "window" );
	winSettings.PixelDensity = 1;
	winSettings.Width = mConfig.windowState.size.getWidth();
	winSettings.Height = mConfig.windowState.size.getHeight();
	if ( winSettings.Icon.empty() ) {
		winSettings.Icon = mConfig.windowState.winIcon;
		if ( FileSystem::isRelativePath( winSettings.Icon ) )
			winSettings.Icon = mResPath + winSettings.Icon;
	}

#if EE_PLATFORM == EE_PLATFORM_MACOSX
	mConfig.context = engine->createContextSettings( &mConfig.ini, "window", true );
#else
	mConfig.context = engine->createContextSettings( &mConfig.ini, "window", false );
#endif

	mConfig.context.SharedGLContext = true;

	mWindow = engine->createWindow( winSettings, mConfig.context );
	Log::info( "%s (codename: \"%s\") initializing", ecode::Version::getVersionFullName().c_str(),
			   ecode::Version::getCodename().c_str() );

	if ( mWindow->isOpen() ) {
		// Only verify GPU driver availability on Windows.
		// macOS will have at least a fallback renderer
		// Linux will have at least Mesa drivers with LLVM Pipe
#if EE_PLATFORM == EE_PLATFORM_WIN
		if ( !GLi->shadersSupported() ) {
			mWindow->showMessageBox(
				EE::Window::Window::MessageBoxType::Error, "ecode",
				"ecode detected that there are no GPU drivers available or that the GPU does not "
				"support shaders.\nThis will prevent ecode to properly function.\nPlease check "
				"that your GPU drivers are installed." );
			return;
		}
#endif
#if EE_PLATFORM == EE_PLATFORM_MACOSX
		macOS_CreateApplicationMenus();
#endif

		Log::info( "Window creation took: %.2f ms", globalClock.getElapsedTime().asMilliseconds() );

		mWindow->setFrameRateLimit( mConfig.context.FrameRateLimit );

		if ( mConfig.windowState.position != Vector2i( -1, -1 ) &&
			 mConfig.windowState.displayIndex < displayManager->getDisplayCount() ) {
			mWindow->setPosition( mConfig.windowState.position.x, mConfig.windowState.position.y );
		}

		if ( mWindow->isWindowed() && mWindow->getSize() >= currentDisplay->getSize().asInt() ) {
			mWindow->setPosition( mWindow->getBorderSize().getWidth(),
								  mWindow->getBorderSize().getHeight() );
		}

		loadKeybindings();

		PixelDensity::setPixelDensity( mConfig.windowState.pixelDensity );

		if ( mConfig.windowState.maximized )
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

		PixelDensity::setPixelDensity(
			eemax( mWindow->getScale(), mConfig.windowState.pixelDensity ) );

		mUISceneNode = UISceneNode::New();
		mUISceneNode->setThreadPool( mThreadPool );
		mUIColorScheme = mConfig.ui.colorScheme;
		if ( !colorScheme.empty() ) {
			mUIColorScheme =
				colorScheme == "light" ? ColorSchemePreference::Light : ColorSchemePreference::Dark;
		}
		mUISceneNode->setColorSchemePreference( mUIColorScheme );

		mFont = loadFont( "sans-serif", mConfig.ui.serifFont, "fonts/NotoSans-Regular.ttf" );
		mFontMono = loadFont( "monospace", mConfig.ui.monospaceFont, "fonts/DejaVuSansMono.ttf" );
		if ( mFontMono ) {
			mFontMono->setEnableDynamicMonospace( true );
			mFontMono->setBoldAdvanceSameAsRegular( true );
		}

		loadFont( "NotoEmoji-Regular", "fonts/NotoEmoji-Regular.ttf" );

#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN
		loadFont( "NotoColorEmoji", "fonts/NotoColorEmoji.ttf" );
#endif

		FontTrueType* iconFont = loadFont( "icon", "fonts/remixicon.ttf" );
		FontTrueType* mimeIconFont = loadFont( "nonicons", "fonts/nonicons.ttf" );
		FontTrueType* codIconFont = loadFont( "codicon", "fonts/codicon.ttf" );

		if ( !mFont || !mFontMono || !iconFont || !mimeIconFont || !codIconFont ) {
			printf( "Font not found!" );
			Log::error( "Font not found!" );
			return;
		}

		mTerminalFont = loadFont( "monospace-nerdfont", mConfig.ui.terminalFont,
								  "fonts/DejaVuSansMonoNerdFontComplete.ttf" );

		mFallbackFont = loadFont( "fallback-font", "fonts/DroidSansFallbackFull.ttf" );
		if ( mFallbackFont )
			FontManager::instance()->setFallbackFont( mFallbackFont );

		SceneManager::instance()->add( mUISceneNode );

		setTheme( getThemePath() );

		if ( !css.empty() && FileSystem::fileExists( css ) ) {
			CSS::StyleSheetParser parser;
			if ( parser.loadFromFile( css ) )
				mUISceneNode->combineStyleSheet( parser.getStyleSheet(), false );
		}

		std::string panelUI( String::format( R"css(
		#project_view > treeview::row > treeview::cell > treeview::cell::text {
			font-size: %s;
		}
		)css",
											 mConfig.ui.panelFontSize.toString().c_str() ) );
		mUISceneNode->combineStyleSheet( panelUI, false );

		const auto baseUI = R"html(
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
#panel tab::icon {
	margin-left: 0dp;
	margin-right: 0dp;
}
#panel tab:selected {
	border: none;
	background-color: transparent;
	background-image: rectangle(solid, var(--primary), 0º, 1dp);
	background-size: 80% 1dp;
	background-position-x: center;
	background-position-y: 90%;
	border-left-width: 0dp;
	border-right-width: 0dp;
	border-top-width: 0dp;
	border-top-left-radius: 0dp;
	border-top-right-radius: 0dp;
}
#check_for_updates .check_at_startup {
	margin: 6dp 0dp 6p 0dp;
}
#project_view_empty {
	padding-top: 8dp;
}
#project_view_empty > * {
	margin-left: 8dp;
	margin-right: 8dp;
}
#status_bar {
	background-color: var(--list-back);
	padding-top: 1dp;
	padding-bottom: 1dp;
}
#status_bar > TextView {
	padding-left: 4dp;
	padding-right: 5dp;
	background-color: var(--list-back);
	border-right-color: var(--tab-line);
	font-size: 10dp;
}
#status_bar > TextView:hover {
	background-color: var(--item-hover);
}
#status_bar > TextView.selected {
	background-color: var(--primary);
}
#panel > tabwidget::container > * {
	background-color: var(--list-back);
}
.settings_panel {
	padding: 4dp;
}
.settings_panel .title {
	font-size:17dp;
}
.settings_panel .subtitle {
	font-size:15dp;
	margin-top: 12dp;
}
.settings_panel > .subtitle {
	margin-top: 4dp;
}
.settings_panel .advance_opt {
	font-size:15dp;
}
.settings_panel .separator {
	margin-top: 4dp;
	background-color: var(--font-hint);
}
.settings_panel PushButton {
	padding-top: 2dp;
	padding-bottom: 2dp;
}
.settings_panel .build_step,
.settings_panel .os_select {
	background-color: var(--list-back);
	border-width: 1dp;
	border-color: var(--button-border);
	padding: 4dp;
	margin-top: 4dp;
}
.settings_panel .build_step > LinearLayout > LinearLayout {
	margin-bottom: 3dp;
}
.settings_panel .build_step TextInput {
	padding: 2dp 4dp 2dp 4dp;
}
.settings_panel .add_build_step {
	margin-top: 4dp;
}
.settings_panel .build_steps {
	margin-bottom: 4dp;
}
.settings_panel .build_step:first-child .move_up,
.settings_panel .build_step:first-child .remove_item {
	enabled: false;
}
.settings_panel .build_step:last-child .move_down {
	enabled: false;
}
.settings_panel .build_step > .details {
	margin-top: 2dp;
}
.settings_panel .build_step > .header > PushButton {
	margin-left: 2dp;
}
.settings_panel .build_step > .header > .remove_item:disabled,
.settings_panel .build_step > .header > .move_up:disabled,
.settings_panel .build_step > .header > .move_down:disabled {
	tint: var(--icon);
}
.settings_panel .os_select > CheckBox {
	margin-right: 8dp;
}
.settings_panel .save_cont > PushButton {
	margin: 8dp 0dp 0dp 4dp;
	padding: 4dp 16dp 4dp 16dp;
}
.settings_panel .build_types > DropDownList,
.settings_panel .output_parser > DropDownList,
.settings_panel .span {
	margin-top: 4dp;
}
.settings_panel #build_type_list,
.settings_panel #build_type_add {
	margin-right: 4dp;
}
.settings_panel .build_types_cont {
	row-valign: center;
}
.settings_panel .inner_box {
	visible: false;
}
.settings_panel .advanced_options {
	margin-top: 12dp;
	border-radius: 4dp;
	padding: 4dp;
	background-color: rgba(0,0,0, 0.1);
}
.settings_panel .advanced_options .title {
	padding-bottom: 4dp;
	border-radius: 4dp;
	background-color: transparent;
	transition: background-color 100ms;
}
.settings_panel .advanced_options .title:hover {
	background-color: var(--primary);
}
.settings_panel .output_parser .output_parser_rules {
	background-color: var(--list-back);
}
.settings_panel .output_parser .output_parser_rules {
	margin-top: 4dp;
}
.settings_panel .output_parser .output_parser_rules > TextView {
	padding-left: 4dp;
	min-height: 24dp;
	layout-gravity: center;
}
.settings_panel .output_parser .output_parser_rules > TextView:first-child {
	border-width: 1dp;
	border-right-color: var(--button-border);
}
.settings_panel .output_parser .output_parser_rules > PushButton {
	layout-gravity: center;
}
.settings_panel .details_but {
	icon: icon(arrow-up-s, 10dp);
	inner-widget-orientation: widgettextboxicon;
}
.settings_panel .details_but.contracted {
	icon: icon(arrow-down-s, 10dp);
}
.settings_panel > .advanced_options > .title > Image {
	icon: icon(arrow-down-s, 24dp);
}
.settings_panel > .advanced_options > .title > Image.expanded {
	icon: icon(arrow-up-s, 24dp);
}
.settings_panel > .advanced_options > LinearLayout.inner_box.visible {
	visible: true;
}
.settings_panel .buttons_box {
	margin-left:4dp;
	layout-gravity: center_horizontal|top;
}
.settings_panel .buttons_box > * {
	margin-bottom: 4dp;
}
.settings_panel TableView {
	margin-top: 4dp;
}
.custom_output_parser_cont > * {
	margin-bottom: 4dp;
	padding-top: 4dp;
}
.custom_output_parser_cont > .capture_positions_cont {
	border: 1dp solid var(--tab-line);
	padding: 4dp;
}
.custom_output_parser_cont > .capture_positions_cont > * {
	padding: 2dp;
}
</style>
<MainLayout id="main_layout" lw="mp" lh="mp">
<Splitter id="project_splitter" lw="mp" lh="mp">
	<TabWidget id="panel" tabbar-hide-on-single-tab="true" tabbar-allow-rearrange="true" min-tab-width="32dp" max-tab-width="32dp">
		<RelativeLayout id="project_view_cont" lw="mp" lh="mp">
			<TreeView id="project_view" lw="mp" lh="mp" />
			<vbox id="project_view_empty" lg="top|center_horizontal" lw="mp" lh="wc">
				<TextView text-align="center" lw="mp" lg="center" text='@string(you-have-not-yet-opened-a-folder, "You have not yet opened a folder.")' word-wrap="true"  />
				<PushButton lw="mp" lg="center" id="open_folder" text='@string(open_folder, "Open Folder")' margin-top="4dp" />
				<PushButton lw="mp" lg="center" id="open_recent_folder" text='@string(open-recent-folder, "Open Recent Folder...")' margin-top="4dp" />
			</vbox>
		</RelativeLayout>
		<Tab text='@string("project", "Project")' owns="project_view_cont" text-as-fallback="true" icon="icon(folder-open, 12dp)" />
	</TabWidget>
	<vbox>
		<Splitter id="main_splitter" lw="mp" lh="0" lw8="1" orientation="vertical">
			<RelativeLayout>
				<vbox id="code_container" lw="mp" lh="mp"></vbox>
				<hbox id="doc_info" lw="wc" lh="wc" lg="bottom|right" enabled="false">
					<TextView id="doc_info_text" lw="wc" lh="wc" />
				</hbox>
				<RelativeLayout id="image_container" lw="mp" lh="mp" visible="false" enabled="false">
					<Image lw="mp" lh="mp" scaleType="fit_inside" gravity="center" enabled="false" lg="center" />
					<TextView id="image_close" lw="wc" lh="wc" text="&#xeb99;" lg="top|right" enabled="false" />
					<Loader id="image_loader" lw="64dp" lh="64dp" outline-thickness="6dp" lg="center" visible="false" />
				</RelativeLayout>
				<vbox id="notification_center" lw="256dp" lh="wc"
					  lg="right|bottom" margin-right="22dp" margin-bottom="56dp">
				</vbox>
			</RelativeLayout>
		</Splitter>
		<searchbar id="search_bar" lw="mp" lh="wc">
			<vbox lw="wc" lh="wc" margin-right="4dp">
				<TextView lw="wc" lh="18dp" text='@string("find_text", "Find:")' margin-bottom="2dp" />
				<TextView lw="wc" lh="18dp" text='@string("replace_with_text", "Replace with:")' />
			</vbox>
			<vbox lw="0" lw8="1" lh="wc" margin-right="4dp">
				<TextInput id="search_find" lw="mp" lh="18dp" padding="0" margin-bottom="2dp" />
				<TextInput id="search_replace" lw="mp" lh="18dp" padding="0" />
			</vbox>
			<vbox lw="wc" lh="wc" margin-right="4dp">
				<CheckBox id="case_sensitive" lw="wc" lh="wc" text='@string(case_sensitive, "Case sensitive")' selected="false" />
				<CheckBox id="lua_pattern" lw="wc" lh="wc" text='@string(lua_pattern, "Lua Pattern")' selected="false" />
			</vbox>
			<vbox lw="wc" lh="wc" margin-right="4dp">
				<CheckBox id="whole_word" lw="wc" lh="wc" text='@string(match_whole_word, "Match Whole Word")' selected="false" />
				<CheckBox id="escape_sequence" lw="wc" lh="wc" text='@string(use_escape_sequences, "Use escape sequences")' selected="false" tooltip='@string(escape_sequence_tooltip, "Replace \\, \t, \n, \r and \uXXXX (Unicode characters) with the corresponding control")' />
			</vbox>
			<vbox lw="wc" lh="wc">
				<hbox lw="wc" lh="wc" margin-bottom="2dp">
					<PushButton id="find_prev" lw="wc" lh="18dp" text='@string(previous, "Previous")' margin-right="4dp" />
					<PushButton id="find_next" lw="wc" lh="18dp" text='@string(next, "Next")' margin-right="4dp" />"
					<PushButton id="select_all" lw="wc" lh="18dp" text='@string(select_all, "Select All")' />
					<RelativeLayout lw="0" lw8="1" lh="18dp">
						<Widget id="searchbar_close" class="close_button" lw="wc" lh="wc" lg="center_vertical|right" margin-right="2dp" />
					</RelativeLayout>
				</hbox>
				<hbox lw="wc" lh="wc">
					<PushButton id="replace" lw="wc" lh="18dp" text='@string(replace, "Replace")' margin-right="4dp" />
					<PushButton id="replace_find" lw="wc" lh="18dp" text='@string(replace_and_find, "Replace & Find")' margin-right="4dp" />
					<PushButton id="replace_all" lw="wc" lh="18dp" text='@string(replace_all, "Replace All")' />
				</hbox>
			</vbox>
		</searchbar>
		<locatebar id="locate_bar" lw="mp" lh="wc" visible="false">
			<TextInput id="locate_find" lw="0" lw8="1" lh="18dp" padding="0" margin-bottom="2dp" margin-right="4dp" hint='@string(type_to_locate, "Type to Locate")' />
			<Widget id="locatebar_close" class="close_button" lw="wc" lh="wc" lg="center_vertical|right"/>
		</locatebar>
		<globalsearchbar id="global_search_bar" lw="mp" lh="wc">
			<hbox lw="mp" lh="wc">
				<TextView lw="wc" lh="wc" text='@string(search_for, "Search for:")' margin-right="4dp" />
				<vbox lw="0" lw8="1" lh="wc">
					<TextInput id="global_search_find" lw="mp" lh="wc" lh="18dp" padding="0" margin-bottom="2dp" />
					<hbox lw="mp" lh="wc" margin-bottom="4dp">
						<CheckBox id="case_sensitive" lw="wc" lh="wc" text='@string(case_sensitive, "Case sensitive")' selected="true" />
						<CheckBox id="whole_word" lw="wc" lh="wc" text='@string(match_whole_word, "Match Whole Word")' selected="false" margin-left="8dp" />
						<CheckBox id="lua_pattern" lw="wc" lh="wc" text='@string(lua_pattern, "Lua Pattern")' selected="false" margin-left="8dp" />
						<CheckBox id="escape_sequence" lw="wc" lh="wc" text='@string(use_escape_sequences, "Use escape sequences")' margin-left="8dp" selected="false" tooltip='@string(escape_sequence_tooltip, "Replace \\, \t, \n, \r and \uXXXX (Unicode characters) with the corresponding control")' />
					</hbox>
					<hbox lw="mp" lh="wc">
						<TextView lw="wc" lh="wc" text='@string(history, "History:")' margin-right="4dp" lh="18dp" />
						<DropDownList id="global_search_history" lw="0" lh="18dp" lw8="1" margin-right="4dp" />
						<PushButton id="global_search_clear_history" lw="wc" lh="18dp" text='@string(clear_history, "Clear History")' margin-right="4dp" />
						<PushButton id="global_search" lw="wc" lh="18dp" text='@string(search, "Search")' margin-right="4dp" />
						<PushButton id="global_search_replace" lw="wc" lh="18dp" text='@string(search_and_replace, "Search & Replace")' />
					</hbox>
				</vbox>
				<Widget id="global_searchbar_close" class="close_button" lw="wc" lh="wc" lg="top|right" margin-left="4dp" margin-top="4dp" />
			</hbox>
		</globalsearchbar>
		<statusbar lw="mp" lh="wc" id="status_bar">
			<TextView class="status_but" id="status_locate_bar" text="1 Locate" />
			<TextView class="status_but" id="status_global_search_bar" text="2 Search" />
			<TextView class="status_but" id="status_build_output" text="3 Build Output" />
			<TextView class="status_but" id="status_terminal" text="4 Terminal" />
			<View lw="0" lw8="1" lh="mp" />
		</statusbar>
	</vbox>
</Splitter>
<TextView id="settings" lw="wc" lh="wc" text="&#xf0e9;" lg="top|right" />
</MainLayout>
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
			{ "arrow-down", 0xea4c },
			{ "arrow-up", 0xea76 },
			{ "arrow-down-s", 0xea4e },
			{ "arrow-up-s", 0xea78 },
			{ "arrow-right-s", 0xea6e },
			{ "match-case", 0xed8d },
			{ "palette", 0xefc5 },
			{ "file-code", 0xecd1 },
			{ "cursor-pointer", 0xec09 },
			{ "drive", 0xedf8 },
			{ "refresh", 0xf064 },
			{ "hearth-pulse", 0xee10 },
			{ "add", 0xea12 },
			{ "hammer", 0xedee },
			{ "eraser", 0xec9e } };
		for ( const auto& icon : icons )
			iconTheme->add( UIGlyphIcon::New( icon.first, iconFont, icon.second ) );

		if ( mimeIconFont && mimeIconFont->loaded() ) {
			std::unordered_map<std::string, Uint32> mimeIcons =

				{ { "filetype-lua", 61826 },
				  { "filetype-c", 61718 },
				  { "filetype-h", 61792 },
				  { "filetype-cs", 61720 },
				  { "filetype-cpp", 61719 },
				  { "filetype-hpp", 61719 },
				  { "filetype-css", 61743 },
				  { "filetype-conf", 61781 },
				  { "filetype-cfg", 61781 },
				  { "filetype-desktop", 61781 },
				  { "filetype-service", 61781 },
				  { "filetype-env", 61781 },
				  { "filetype-properties", 61781 },
				  { "filetype-ini", 61781 },
				  { "filetype-dart", 61744 },
				  { "filetype-diff", 61752 },
				  { "filetype-zip", 61775 },
				  { "filetype-go", 61789 },
				  { "filetype-htm", 61799 },
				  { "filetype-html", 61799 },
				  { "filetype-java", 61809 },
				  { "filetype-js", 61810 },
				  { "filetype-json", 61811 },
				  { "filetype-kt", 61814 },
				  { "filetype-md", 61829 },
				  { "filetype-perl", 61853 },
				  { "filetype-php", 61855 },
				  { "filetype-py", 61863 },
				  { "filetype-pyc", 61863 },
				  { "filetype-pyd", 61863 },
				  { "filetype-swift", 61906 },
				  { "filetype-rb", 61880 },
				  { "filetype-rs", 61881 },
				  { "filetype-ts", 61923 },
				  { "filetype-yaml", 61945 },
				  { "filetype-yml", 61945 },
				  { "filetype-jpg", 61801 },
				  { "filetype-png", 61801 },
				  { "filetype-jpeg", 61801 },
				  { "filetype-bmp", 61801 },
				  { "filetype-tga", 61801 },
				  { "filetype-sh", 61911 },
				  { "filetype-bash", 61911 },
				  { "filetype-fish", 61911 },
				  { "filetype-scala", 61882 },
				  { "filetype-r", 61866 },
				  { "filetype-rake", 61880 },
				  { "filetype-rss", 61879 },
				  { "filetype-sql", 61746 },
				  { "filetype-elm", 61763 },
				  { "filetype-ex", 61971 },
				  { "filetype-exs", 61971 },
				  { "filetype-awk", 61971 },
				  { "filetype-nim", 61734 },
				  { "filetype-xml", 61769 },
				  { "filetype-dockerfile", 61758 },
				  { "filetype-ruby", 61880 },
				  { "filetype-scala", 61882 },
				  { "filetype-perl", 61853 },
				  { "file", 61766 },
				  { "file-symlink", 61774 },
				  { "folder", 0xF23B },
				  { "folder-open", 0xF23C },
				  { "tree-expanded", 0xF11E },
				  { "tree-contracted", 0xF120 },
				  { "github", 0xF184 },
				  { "package", 61846 },
				  { "tab-close", 61944 } };

			for ( const auto& icon : mimeIcons )
				iconTheme->add( UIGlyphIcon::New( icon.first, mimeIconFont, icon.second ) );
		}

		if ( codIconFont && codIconFont->loaded() ) {
			std::unordered_map<std::string, Uint32> codIcons = {
				{ "symbol-text", 0xea93 },
				{ "symbol-method", 0xea8c },
				{ "symbol-function", 0xea8c },
				{ "symbol-constructor", 0xea8c },
				{ "symbol-field", 0xeb5f },
				{ "symbol-variable", 0xea88 },
				{ "symbol-class", 0xeb5b },
				{ "symbol-interface", 0xeb61 },
				{ "symbol-module", 0xea8b },
				{ "symbol-property", 0xeb65 },
				{ "symbol-unit", 0xea96 },
				{ "symbol-value", 0xea95 },
				{ "symbol-enum", 0xea95 },
				{ "symbol-keyword", 0xeb62 },
				{ "symbol-snippet", 0xeb66 },
				{ "symbol-color", 0xeb5c },
				{ "symbol-file", 0xeb60 },
				{ "symbol-reference", 0xea94 },
				{ "symbol-folder", 0xea83 },
				{ "symbol-enum-member", 0xeb5e },
				{ "symbol-constant", 0xeb5d },
				{ "symbol-struct", 0xea91 },
				{ "symbol-event", 0xea86 },
				{ "symbol-operator", 0xeb64 },
				{ "symbol-type-parameter", 0xea92 },
				{ "expand-all", 0xeb95 },
				{ "symbol-namespace", 0xea8b },
				{ "symbol-package", 0xea8b },
				{ "symbol-string", 0xeb8d },
				{ "symbol-number", 0xea90 },
				{ "symbol-boolean", 0xea8f },
				{ "symbol-array", 0xea8a },
				{ "symbol-object", 0xea8b },
				{ "symbol-key", 0xea93 },
				{ "symbol-null", 0xea8f },
				{ "collapse-all", 0xeac5 },
				{ "chevron-right", 0xeab6 },
				{ "lightbulb-autofix", 0xeb13 },
				{ "layout-sidebar-left-off", 0xec02 },
				{ "layout-sidebar-left", 0xebf3 } };

			for ( const auto& icon : codIcons )
				iconTheme->add( UIGlyphIcon::New( icon.first, codIconFont, icon.second ) );
		}

		iconTheme->add( UISVGIcon::New(
			"ecode",
			"<svg width='256' height='256' version='1.1' viewBox='0 0 12.7 12.7' "
			"xmlns='http://www.w3.org/2000/svg'><g transform='matrix(.80588 0 0 .80588 1.3641 "
			"1.2538)' fill='none' stroke='#fff' stroke-linecap='round' stroke-width='.8px'><path "
			"transform='matrix(1 0 0 -1 .19808 11.984)' d='m6.0943 5.9899c0.16457 0.1173 0.0098785 "
			"0.35997-0.11145 0.43612-0.40848 0.25638-0.89022-0.11622-1.039-0.4965-0.32983-0.84299 "
			"0.37932-1.6794 1.1646-1.8967 1.4211-0.39338 2.7295 0.77904 3.0093 2.1158 0.36822 "
			"1.7593-0.79537 3.4576-2.3989 4.1096'/><path transform='matrix(1 0 0 -1 .030284 "
			"12.252)' d='m6.3163 6.3608c-0.14689-0.18552 0.10611-0.40806 0.26737-0.45419 "
			"0.54289-0.15532 0.96335 0.42061 1.0068 0.89674 0.096424 1.0555-0.97349 1.7652-1.9275 "
			"1.7539-1.7263-0.020368-2.8161-1.765-2.6954-3.3595 0.15881-2.0986 2.0205-3.6297 "
			"4.0363-3.8406'/></g></svg>" ) );

		mUISceneNode->getUIIconThemeManager()->setCurrentTheme( iconTheme );

		Clock defClock;
		SyntaxDefinitionManager::createSingleton();
		Log::info( "Syntax definitions loaded in %.2f ms.",
				   defClock.getElapsedTimeAndReset().asMilliseconds() );

		UIWidgetCreator::registerWidget( "searchbar", UISearchBar::New );
		UIWidgetCreator::registerWidget( "locatebar", UILocateBar::New );
		UIWidgetCreator::registerWidget( "globalsearchbar", UIGlobalSearchBar::New );
		UIWidgetCreator::registerWidget( "mainlayout", UIMainLayout::New );
		UIWidgetCreator::registerWidget( "statusbar", UIStatusBar::New );
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
		if ( logLevel == LogLevel::Debug )
			mUISceneNode->setVerbose( true );
		mDocInfo->setVisible( mConfig.editor.showDocInfo );

		mProjectSplitter->setSplitPartition(
			StyleSheetLength( mConfig.windowState.panelPartition ) );
		if ( mConfig.ui.panelPosition == PanelPosition::Right )
			mProjectSplitter->swap();

		if ( !mConfig.ui.showSidePanel )
			showSidePanel( mConfig.ui.showSidePanel );

		auto colorSchemes(
			SyntaxColorScheme::loadFromFile( mResPath + "colorschemes/colorschemes.conf" ) );
		if ( FileSystem::isDirectory( mColorSchemesPath ) ) {
			auto colorSchemesFiles = FileSystem::filesGetInPath( mColorSchemesPath );
			for ( const auto& curFile : colorSchemesFiles ) {
				auto colorSchemesInFile =
					SyntaxColorScheme::loadFromFile( mColorSchemesPath + curFile );
				for ( auto& coloScheme : colorSchemesInFile )
					colorSchemes.emplace_back( coloScheme );
			}
		} else {
			FileSystem::makeDir( mColorSchemesPath, true );
		}

		Clock customLangsClock;
		SyntaxDefinitionManager::instance()->loadFromFolder( mLanguagesPath );
		Log::info( "SyntaxDefinitionManager loaded custom languages in: %.2f ms",
				   customLangsClock.getElapsedTime().asMilliseconds() );

		mTerminalManager->loadTerminalColorSchemes();

		mSplitter = UICodeEditorSplitter::New( this, mUISceneNode, colorSchemes, mInitColorScheme );
		mSplitter->setHideTabBarOnSingleTab( mConfig.editor.hideTabBarOnSingleTab );
		mPluginManager->setSplitter( mSplitter );

		Log::info( "Base UI took: %.2f ms", globalClock.getElapsedTime().asMilliseconds() );

		mMainSplitter = mUISceneNode->find<UISplitter>( "main_splitter" );
		mMainSplitter->setSplitPartition(
			StyleSheetLength( mConfig.windowState.statusBarPartition ) );
		mStatusBar = mUISceneNode->find<UIStatusBar>( "status_bar" );
		mStatusBar->setApp( this );

#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN
		mFileWatcher = new efsw::FileWatcher();
		mFileSystemListener = new FileSystemListener( mSplitter, mFileSystemModel );
		mFileWatcher->addWatch( mPluginsPath, mFileSystemListener );
		mFileWatcher->watch();
		mPluginManager->setFileSystemListener( mFileSystemListener );
#endif

		mNotificationCenter = std::make_unique<NotificationCenter>(
			mUISceneNode->find<UILayout>( "notification_center" ), mPluginManager.get() );

		mDocSearchController = std::make_unique<DocSearchController>( mSplitter, this );
		mDocSearchController->initSearchBar( mUISceneNode->find<UISearchBar>( "search_bar" ),
											 mConfig.searchBarConfig, mDocumentSearchKeybindings );

		mGlobalSearchController =
			std::make_unique<GlobalSearchController>( mSplitter, mUISceneNode, this );
		mGlobalSearchController->initGlobalSearchBar(
			mUISceneNode->find<UIGlobalSearchBar>( "global_search_bar" ),
			mConfig.globalSearchBarConfig, mGlobalSearchKeybindings );

		mUniversalLocator = std::make_unique<UniversalLocator>( mSplitter, mUISceneNode, this );
		mUniversalLocator->initLocateBar( mUISceneNode->find<UILocateBar>( "locate_bar" ),
										  mUISceneNode->find<UITextInput>( "locate_find" ) );

		mStatusTerminalController =
			std::make_unique<StatusTerminalController>( mMainSplitter, mUISceneNode, this );

		mStatusBuildOutputController =
			std::make_unique<StatusBuildOutputController>( mMainSplitter, mUISceneNode, this );

		initImageView();

		mSettings = std::make_unique<SettingsMenu>();
		mSettings->createSettingsMenu( this );

		mSplitter->createEditorWithTabWidget( mBaseLayout );

		mConsole = UIConsole::NewOpt( mFontMono, true, true, 1024 * 10 );
		mConsole->setCommand( "hide", [&]( const auto& ) { consoleToggle(); } );
		mConsole->setQuakeMode( true );
		mConsole->setVisible( false );

		registerUnlockedCommands( *mMainLayout );
		mMainLayout->getKeyBindings().addKeybinds( getDefaultKeybindings() );

		Log::instance()->setKeepLog( false );
		Log::info( "Complete UI took: %.2f ms", globalClock.getElapsedTime().asMilliseconds() );

#if EE_PLATFORM == EE_PLATFORM_EMSCRIPTEN
		if ( file == "./this.program" )
			file = "";
#endif

		if ( terminal && file.empty() && fileToOpen.empty() ) {
			showSidePanel( false );
			showStatusBar( false );
			mTerminalManager->createNewTerminal();
		} else {
			initProjectTreeView( file );
		}

		mFileToOpen = fileToOpen;

		Log::info( "Init ProjectTreeView took: %.2f ms",
				   globalClock.getElapsedTime().asMilliseconds() );

#if EE_PLATFORM == EE_PLATFORM_EMSCRIPTEN
		if ( file.empty() )
			downloadFileWeb( "https://raw.githubusercontent.com/SpartanJ/eepp/develop/README.md" );
#endif

		if ( mConfig.workspace.checkForUpdatesAtStartup )
			checkForUpdates( true );

		mWindow->runMainLoop( &appLoop, mBenchmarkMode ? 0 : mConfig.context.FrameRateLimit );
	}
}

static void exportLanguages( const std::string& path, const std::string& langs ) {
	SyntaxDefinitionManager* sdm = SyntaxDefinitionManager::instance();
	std::vector<SyntaxDefinition> defs;

	if ( !langs.empty() ) {
		auto langss = String::split( langs, ',' );
		for ( const auto& l : langss ) {
			const auto& sd = sdm->getByLSPName( l );

			if ( !sd.getLanguageName().empty() ) {
				defs.push_back( sd );
				continue;
			}

			const auto& sd2 = sdm->getByLanguageName( l );

			if ( !sd2.getLanguageName().empty() )
				defs.push_back( sd2 );
		}
	}

	if ( sdm->save( path, defs ) ) {
		std::cout << "Language definitions exported!\n";
	} else {
		std::cout << "Could not write the file\n";
	}
}

} // namespace ecode

using namespace ecode;

EE_MAIN_FUNC int main( int argc, char* argv[] ) {
#ifdef ECODE_USE_BACKWARD
	backward::SignalHandling sh;
#endif
	args::ArgumentParser parser( "ecode" );
	args::HelpFlag help( parser, "help", "Display this help menu", { 'h', '?', "help" } );
	args::Positional<std::string> fileOrFolderPos( parser, "file_or_folder",
												   "The file or folder path to open" );
	args::ValueFlag<std::string> file(
		parser, "file",
		"The file path to open. A file path can also contain the line number and column to "
		"position the cursor when the file is opened. The format is: "
		"$FILEPATH:$LINE_NUMBER:$COLUMN. Both line number and column are optional (line number can "
		"be provided without column too).",
		{ 'f', "file" } );
	args::ValueFlag<std::string> folder( parser, "folder", "The folder path to open",
										 { "folder" } );
	args::ValueFlag<Float> pixelDenstiyConf( parser, "pixel-density",
											 "Set default application pixel density",
											 { 'd', "pixel-density" } );
	args::ValueFlag<std::string> prefersColorScheme(
		parser, "prefers-color-scheme", "Set the preferred color scheme (\"light\" or \"dark\")",
		{ 'c', "prefers-color-scheme" } );
	args::ValueFlag<std::string> css(
		parser, "css",
		"Sets the path for a custom stylesheet to load at the start of the application",
		{ "css" } );
	args::Flag terminal( parser, "terminal", "Open a new terminal / Open ecode in terminal mode",
						 { 't', "terminal" } );
	args::MapFlag<std::string, LogLevel> logLevel(
		parser, "log-level", "The level of details that the application will emmit logs.",
		{ 'l', "log-level" }, Log::getMapFlag(), Log::getDefaultLogLevel() );
	args::Flag fb( parser, "framebuffer", "Use frame buffer (more memory usage, less CPU usage)",
				   { "fb", "framebuffer" } );
	args::Flag benchmarkMode( parser, "benchmark-mode",
							  "Render as much as possible to measure the rendering performance.",
							  { "benchmark-mode" } );
	args::Flag verbose( parser, "verbose", "Redirects all logs to stdout.", { 'v', "verbose" } );
	args::Flag version( parser, "version", "Prints version information", { 'V', "version" } );
	args::ValueFlag<size_t> jobs(
		parser, "jobs",
		"Sets the number of background jobs that the application will spawn "
		"at the start of the application",
		{ 'j', "jobs" }, 0 );
	args::Flag health( parser, "health", "Checks for potential errors in editor setup.",
					   { "health" }, "" );
	args::ValueFlag<std::string> healthLang(
		parser, "health-lang",
		"Checks for potential errors in editor setup for a specific language.", { "health-lang" },
		"" );
	args::MapFlag<std::string, FeaturesHealth::OutputFormat> healthFormat(
		parser, "health-format",
		"Sets the health format report (accepted values: terminal, markdown, ascii, asciidoc)",
		{ "health-format" }, FeaturesHealth::getMapFlag(),
		FeaturesHealth::getDefaultOutputFormat() );
	args::ValueFlag<std::string> exportLangPath(
		parser, "export-lang-path",
		"Export language definitions to the file path. If no \"export-lang\" is defined it will "
		"export all languages available.",
		{ "export-lang-path" }, "" );
	args::ValueFlag<std::string> exportLang(
		parser, "export-lang",
		"Comma separated language names to export its language definitions. \"export-lang-path\" "
		"must be defined. Retrieved with \"--health\" command.",
		{ "export-lang" }, "" );
	args::ValueFlag<std::string> convertLangPath(
		parser, "convert-lang-path",
		"Convert any JSON language definition to CPP syntax definition (development helper)",
		{ "convert-lang-path" }, "" );
	args::ValueFlag<std::string> convertLangOutput(
		parser, "convert-lang-output",
		"Sets the directory output path. If not set it will be printed to stdout",
		{ "convert-lang-output" }, "" );
	args::Flag disableFileLogs( parser, "disable-file-logs", "Disables writing logs to a log file",
								{ "disable-file-logs" } );

	std::vector<std::string> args;
	try {
		args = Sys::parseArguments( argc, argv );
		parser.ParseCLI( args );
	} catch ( const args::Help& ) {
		Sys::windowAttachConsole();
		std::cout << parser;
		return EXIT_SUCCESS;
	} catch ( const args::ParseError& e ) {
		Sys::windowAttachConsole();
		std::cerr << e.what() << std::endl;
		std::cerr << parser;
		return EXIT_FAILURE;
	} catch ( args::ValidationError& e ) {
		Sys::windowAttachConsole();
		std::cerr << e.what() << std::endl;
		std::cerr << parser;
		return EXIT_FAILURE;
	}

	if ( convertLangPath && !convertLangPath.Get().empty() ) {
		Sys::windowAttachConsole();
		IOStreamFile sfile( convertLangPath.Get() );
		if ( sfile.isOpen() ) {
			std::vector<std::string> adedLangs;
			if ( SyntaxDefinitionManager::instance()->loadFromStream( sfile, &adedLangs ) ) {
				for ( const auto& lang : adedLangs ) {
					const auto& def =
						SyntaxDefinitionManager::instance()->getByLanguageName( lang );
					auto code = SyntaxDefinitionManager::toCPP( def );
					if ( convertLangOutput && !convertLangOutput.Get().empty() &&
						 FileSystem::isDirectory( convertLangOutput.Get() ) ) {
						std::string output( convertLangOutput.Get() );
						FileSystem::dirAddSlashAtEnd( output );
						FileSystem::fileWrite( output + String::toLower( def.getLanguageName() ) +
												   ".hpp",
											   code.first );
						FileSystem::fileWrite( output + String::toLower( def.getLanguageName() ) +
												   ".cpp",
											   code.second );
					} else {
						std::cout << code.first << code.second << "\n";
					}
				}
			}
		}
		return EXIT_SUCCESS;
	}

	if ( exportLangPath && !exportLangPath.Get().empty() ) {
		Sys::windowAttachConsole();
		exportLanguages( exportLangPath.Get(), exportLang.Get() );
		return EXIT_SUCCESS;
	}

	if ( version.Get() ) {
		Sys::windowAttachConsole();
		std::cout << ecode::Version::getVersionFullName() << '\n';
		return EXIT_SUCCESS;
	}

	appInstance = eeNew( App, ( jobs, args ) );
	appInstance->init( logLevel.Get(), folder ? folder.Get() : fileOrFolderPos.Get(),
					   pixelDenstiyConf ? pixelDenstiyConf.Get() : 0.f,
					   prefersColorScheme ? prefersColorScheme.Get() : "", terminal.Get(), fb.Get(),
					   benchmarkMode.Get(), css.Get(), health || healthLang, healthLang.Get(),
					   healthFormat.Get(), file.Get(), verbose.Get(), disableFileLogs.Get() );
	eeSAFE_DELETE( appInstance );

	Engine::destroySingleton();
	MemoryManager::showResults();

	return EXIT_SUCCESS;
}
