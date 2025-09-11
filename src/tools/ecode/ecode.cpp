#include "ecode.hpp"
#include "customwidgets.hpp"
#include "featureshealth.hpp"
#include "keybindingshelper.hpp"
#include "pathhelper.hpp"
#include "settingsactions.hpp"
#include "settingsmenu.hpp"
#include "uibuildsettings.hpp"
#include "uitreeviewfs.hpp"
#include "uiwelcomescreen.hpp"
#include "version.hpp"
#include <algorithm>
#include <args/args.hxx>
#include <eepp/graphics/fontfamily.hpp>
#include <eepp/system/iostreammemory.hpp>
#include <eepp/ui/doc/languagessyntaxhighlighting.hpp>
#include <eepp/ui/iconmanager.hpp>
#include <filesystem>
#include <iostream>

//! Plugins
#include "plugins/aiassistant/aiassistantplugin.hpp"
#include "plugins/autocomplete/autocompleteplugin.hpp"
#include "plugins/debugger/debuggerplugin.hpp"
#include "plugins/discordRPC/discordRPCplugin.hpp"
#include "plugins/formatter/formatterplugin.hpp"
#include "plugins/git/gitplugin.hpp"
#include "plugins/linter/linterplugin.hpp"
#include "plugins/lsp/lspclientplugin.hpp"
#include "plugins/spellchecker/spellcheckerplugin.hpp"
#include "plugins/xmltools/xmltoolsplugin.hpp"

#if EE_PLATFORM == EE_PLATFORM_LINUX
// For malloc_trim, which is a GNU extension
extern "C" {
#include <malloc.h>
}
#endif

using namespace std::literals;

namespace fs = std::filesystem;
using json = nlohmann::json;

#if EE_PLATFORM == EE_PLATFORM_MACOS
#include "macos/macos.hpp"
#endif

namespace ecode {

Clock globalClock;
bool firstFrame = true;
bool firstUpdate = true;
App* appInstance = nullptr;

static const Uint32 APP_LAYOUT_STYLE_MARKER = String::hash( "app_layout_style" );
static const auto NOT_UNIQUE_FILENAME = "not_unique";

void appLoop() {
	appInstance->mainLoop();
}

bool App::onCloseRequestCallback( EE::Window::Window* ) {
	if ( mSplitter->isAnyEditorDirty() &&
		 ( !mConfig.workspace.sessionSnapshot || mCurrentProject.empty() ) ) {
		if ( mCloseMsgBox )
			return false;
		mCloseMsgBox = UIMessageBox::New(
			UIMessageBox::OK_CANCEL,
			i18n( "confirm_ecode_exit",
				  "Do you really want to close the code editor?\nAll changes will be lost." )
				.unescape() );
		mCloseMsgBox->on( Event::OnConfirm, [this]( const Event* ) {
			saveProject();
			saveConfig();
			mWindow->close();
		} );
		mCloseMsgBox->on( Event::OnWindowClose, [this]( auto ) { mCloseMsgBox = nullptr; } );
		mCloseMsgBox->setTitle(
			String::format( i18n( "close_title", "Close %s?" ).toUtf8(), mWindowTitle ) );
		mCloseMsgBox->center();
		mCloseMsgBox->showWhenReady();
		return false;
	} else {
		saveProject();
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

	mSplitter->forEachEditorStoppable( [this]( UICodeEditor* editor ) {
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
				dialog->on( Event::SaveFile, [this, editor]( const Event* ) {
					updateEditorTabTitle( editor );
					if ( mSplitter->getCurEditor() == editor )
						updateEditorTitle( editor );
				} );
				dialog->on( Event::OnWindowClose, [this, editor]( const Event* ) {
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
	mTmpDocs.clear();
	mSplitter->forEachEditor( [this]( UICodeEditor* editor ) {
		if ( editor->isDirty() )
			mTmpDocs.insert( &editor->getDocument() );
	} );
	saveAllProcess();
}

std::string App::titleFromEditor( UICodeEditor* editor ) {
	std::string title( editor->getDocument().getFilename() );
	return editor->getDocument().isDirty() ? title + "*" : title;
}

void App::updateNonUniqueTabTitles() {
	if ( mSplitter == nullptr )
		return;
	mSplitter->forEachEditor( [this]( UICodeEditor* editor ) {
		if ( editor->hasClass( NOT_UNIQUE_FILENAME ) )
			updateEditorTabTitle( editor );
	} );
}

void App::updateEditorTabTitle( UICodeEditor* editor ) {
	const auto isUniqueTabTitle = [this]( UITab* tab ) -> bool {
		bool unique = true;
		auto doc = tab->getOwnedWidget()->asType<UICodeEditor>()->getDocumentRef();
		auto fileName = doc->getFilename();
		mSplitter->forEachTabWidgetStoppable( [tab, &unique, &fileName,
											   &doc]( UITabWidget* tabWidget ) {
			for ( size_t i = 0; i < tabWidget->getTabCount(); i++ ) {
				UITab* curTab = tabWidget->getTab( i );
				if ( !curTab->getOwnedWidget()->isType( UI_TYPE_CODEEDITOR ) )
					continue;
				auto curDoc = curTab->getOwnedWidget()->asType<UICodeEditor>()->getDocumentRef();
				if ( curDoc && tab != curTab && fileName == curDoc->getFilename() &&
					 doc->getFilePath() != curDoc->getFilePath() ) {
					unique = false;
					return true;
				}
			}
			return false;
		} );
		return unique;
	};

	const auto getTabWithSameTitle = [this]( UITab* tab ) -> std::vector<UITab*> {
		std::vector<UITab*> tabs{ tab };
		auto doc = tab->getOwnedWidget()->asType<UICodeEditor>()->getDocumentRef();
		const auto& fileName = doc->getFilename();
		mSplitter->forEachTabWidget( [tab, &tabs, &fileName]( UITabWidget* tabWidget ) {
			for ( size_t i = 0; i < tabWidget->getTabCount(); i++ ) {
				UITab* curTab = tabWidget->getTab( i );
				if ( !curTab->getOwnedWidget()->isType( UI_TYPE_CODEEDITOR ) )
					continue;
				auto curDoc = curTab->getOwnedWidget()->asType<UICodeEditor>()->getDocumentRef();
				if ( curDoc && tab != curTab && fileName == curDoc->getFilename() )
					tabs.push_back( curTab );
			}
		} );
		return tabs;
	};

	const auto getUniqueNameForTabs =
		[]( const std::vector<UITab*>& tabs ) -> std::unordered_map<UITab*, String> {
		std::unordered_map<UITab*, String> uniqueTitles;

		size_t subFoldersDisplayed = 1;
		String firstTitle;
		do {
			for ( UITab* tab : tabs ) {
				auto doc = tab->getOwnedWidget()->asType<UICodeEditor>()->getDocumentRef();
				auto path = doc->getFilePath();
				std::string fileName = FileSystem::fileNameFromPath( path );
				std::string containingFolder = FileSystem::fileRemoveFileName( path );
				std::vector<std::string> displayedFolders;

				for ( size_t i = 0; i < subFoldersDisplayed; i++ ) {
					FileSystem::dirRemoveSlashAtEnd( containingFolder );
					displayedFolders.insert( displayedFolders.begin(),
											 FileSystem::fileNameFromPath( containingFolder ) );
					containingFolder = FileSystem::fileRemoveFileName( containingFolder );
				}

				uniqueTitles[tab] = fileName + " - " + String::join( displayedFolders, '/' );
			}

			subFoldersDisplayed++;
			firstTitle = uniqueTitles.begin()->second;
		} while ( std::all_of( ++uniqueTitles.begin(), uniqueTitles.end(),
							   [&firstTitle]( const std::pair<UITab*, String>& title ) {
								   return title.second == firstTitle;
							   } ) );

		return uniqueTitles;
	};

	if ( editor == nullptr )
		return;
	if ( editor->getData() ) {
		UITab* tab = (UITab*)editor->getData();
		tab->ensureMainThread( [editor, isUniqueTabTitle, getTabWithSameTitle, getUniqueNameForTabs,
								tab] {
			auto doc = editor->getDocumentRef();
			std::string fileName( doc->getFilename() );

			if ( fileName != doc->getDefaultFileName() && !isUniqueTabTitle( tab ) ) {
				auto tabsTitles = getUniqueNameForTabs( getTabWithSameTitle( tab ) );
				for ( auto [ntab, title] : tabsTitles ) {
					ntab->setText( title );
					if ( ntab->getOwnedWidget()->isType( UI_TYPE_CODEEDITOR ) )
						ntab->getOwnedWidget()->asType<UICodeEditor>()->addClass(
							NOT_UNIQUE_FILENAME );
				}
			} else if ( tab->getOwnedWidget()->isType( UI_TYPE_CODEEDITOR ) ) {
				tab->getOwnedWidget()->asType<UICodeEditor>()->removeClass( NOT_UNIQUE_FILENAME );
				tab->setText( fileName );
			}

			bool dirty = doc->isDirty();
			tab->removeClass( dirty ? "tab_clear" : "tab_modified" );
			tab->addClass( dirty ? "tab_modified" : "tab_clear" );
		} );
	}
}

void App::updateEditorTitle( UICodeEditor* editor ) {
	if ( editor == nullptr )
		return;
	updateEditorTabTitle( editor );
	setAppTitle( titleFromEditor( editor ) );
}

void App::setAppTitle( const std::string& title ) {
	std::string fullTitle( mWindowTitle );
	if ( !mCurrentProjectName.empty() && mCurrentProject != getPlaygroundPath() )
		fullTitle += " - " + mCurrentProjectName;

	if ( !title.empty() )
		fullTitle += " - " + title;

	if ( mBenchmarkMode )
		fullTitle += " - " + String::toString( mWindow->getFPS() ) + " FPS";

	if ( mCurWindowTitle != fullTitle ) {
		mCurWindowTitle = fullTitle;
		if ( Engine::isMainThread() ) {
			mWindow->setTitle( fullTitle );
		} else {
			mUISceneNode->runOnMainThread( [this, fullTitle] { mWindow->setTitle( fullTitle ); } );
		}
	}
}

void App::onDocumentModified( UICodeEditor* editor, TextDocument& ) {
	bool isDirty = editor->getDocument().isDirty();
	bool wasDirty = !mCurWindowTitle.empty() && mCurWindowTitle[mCurWindowTitle.size() - 1] == '*';

	if ( isDirty != wasDirty )
		setAppTitle( titleFromEditor( editor ) );

	bool tabDirty = ( (UITab*)editor->getData() )->hasClass( "tab_modified" );

	if ( isDirty != tabDirty )
		updateEditorTitle( editor );
}

void App::onDocumentUndoRedo( UICodeEditor* editor, TextDocument& doc ) {
	onDocumentModified( editor, doc );
}

void App::openFileDialog() {
	UIFileDialog* dialog =
		UIFileDialog::New( UIFileDialog::DefaultFlags, "*", getDefaultFileDialogFolder() );
	dialog->setWindowFlags( UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON | UI_WIN_MODAL );
	dialog->setTitle( i18n( "open_file", "Open File" ) );
	dialog->setCloseShortcut( KEY_ESCAPE );
	dialog->setSingleClickNavigation( mConfig.editor.singleClickNavigation );
	dialog->setAllowsMultiFileSelect( true );
	dialog->on( Event::OpenFile, [this]( const Event* event ) {
		auto files = event->getNode()->asType<UIFileDialog>()->getFullPaths();
		for ( const auto& file : files ) {
			mLastFileFolder = FileSystem::fileRemoveFileName( file );
			loadFileFromPath( file );
		}
	} );
	dialog->on( Event::OnWindowClose, [this]( const Event* ) {
		if ( mSplitter && mSplitter->getCurWidget() && !SceneManager::instance()->isShuttingDown() )
			mSplitter->getCurWidget()->setFocus();
	} );
	dialog->center();
	dialog->show();
}

std::string App::getDefaultFileDialogFolder() const {
	return mLastFileFolder.empty() ? getLastUsedFolder() : mLastFileFolder;
}

std::string App::getLastUsedFolder() const {
	if ( !mCurrentProject.empty() && mCurrentProject != getPlaygroundPath() )
		return mCurrentProject;
	if ( !mRecentFolders.empty() )
		return mRecentFolders.front();
	return ".";
}

void App::insertRecentFolder( const std::string& rpath ) {
	if ( mIncognito )
		return;
	auto found = std::find( mRecentFolders.begin(), mRecentFolders.end(), rpath );
	if ( found != mRecentFolders.end() )
		mRecentFolders.erase( found );
	mRecentFolders.insert( mRecentFolders.begin(), rpath );
	if ( mRecentFolders.size() > 10 )
		mRecentFolders.resize( 10 );
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
	dialog->setSingleClickNavigation( mConfig.editor.singleClickNavigation );
	dialog->on( Event::OpenFile, [this]( const Event* event ) {
		String path( event->getNode()->asType<UIFileDialog>()->getFullPath() );
		if ( FileSystem::isDirectory( path ) )
			loadFolder( path );
	} );
	dialog->on( Event::OnWindowClose, [this]( const Event* ) {
		if ( mSplitter && mSplitter->getCurWidget() && !SceneManager::instance()->isShuttingDown() )
			mSplitter->getCurWidget()->setFocus();
	} );
	dialog->center();
	dialog->show();
}

void App::openFontDialog( std::string& fontPath, bool loadingMonoFont, bool terminalFont,
						  std::function<void()> onFinish ) {
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
			[dialog, index]() { dialog->getMultiView()->setSelection( index ); } );
	dialog->setWindowFlags( UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON | UI_WIN_MODAL );
	dialog->setTitle( i18n( "select_font_file", "Select Font File" ) );
	dialog->setCloseShortcut( KEY_ESCAPE );
	dialog->setSingleClickNavigation( mConfig.editor.singleClickNavigation );
	dialog->on( Event::OnWindowClose, [this]( const Event* ) {
		if ( mSplitter && mSplitter->getCurWidget() && !SceneManager::instance()->isShuttingDown() )
			mSplitter->getCurWidget()->setFocus();
	} );
	dialog->on( Event::OpenFile, [this, &fontPath, loadingMonoFont, terminalFont,
								  onFinish]( const Event* event ) {
		auto newPath = event->getNode()->asType<UIFileDialog>()->getFullPath();
		if ( String::startsWith( newPath, mResPath ) )
			newPath = newPath.substr( mResPath.size() );
		if ( fontPath != newPath ) {
			if ( !loadingMonoFont ) {
				fontPath = newPath;
				if ( onFinish )
					onFinish();
				return;
			}
			auto fontName =
				FileSystem::fileRemoveExtension( FileSystem::fileNameFromPath( newPath ) );
			FontTrueType* fontMono = loadFont( fontName, newPath );
			if ( fontMono ) {
				auto loadMonoFont = [this, &fontPath, newPath,
									 terminalFont]( FontTrueType* fontMono ) {
					fontPath = newPath;
					mFontMono = fontMono;
					mFontMono->setEnableDynamicMonospace( true );
					mFontMono->setBoldAdvanceSameAsRegular( true );
					FontFamily::loadFromRegular( mFontMono );
					if ( mSplitter ) {
						if ( terminalFont ) {
							mSplitter->forEachWidgetType(
								UI_TYPE_TERMINAL, [this]( UIWidget* term ) {
									term->asType<UITerminal>()->setFont( mFontMono );
								} );
						} else {
							mSplitter->forEachEditor(
								[this]( UICodeEditor* editor ) { editor->setFont( mFontMono ); } );
						}
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
					msgBox->on( Event::OnConfirm, [loadMonoFont, fontMono]( const Event* ) {
						loadMonoFont( fontMono );
					} );
					msgBox->on( Event::OnCancel, [fontMono]( const Event* ) {
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
	UIMessageBox* msgBox =
		UIMessageBox::New( UIMessageBox::INPUT, i18n( "please_enter_file_url_ellipsis",
													  "Please enter the file URL..." ) );

	msgBox->setTitle( mWindowTitle );
	msgBox->getTextInput()->setHint( i18n( "any_https_or_http_url", "Any https or http URL" ) );
	msgBox->setCloseShortcut( { KEY_ESCAPE, KEYMOD_NONE } );
	msgBox->showWhenReady();
	msgBox->on( Event::OnConfirm, [this, msgBox]( const Event* ) {
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
	std::string filename( editor->getDocument().getFilename() );
	if ( FileSystem::fileExtension( editor->getDocument().getFilename() ).empty() )
		filename += editor->getSyntaxDefinition().getFileExtension();
	std::string folderPath( FileSystem::fileRemoveFileName( editor->getDocument().getFilePath() ) );
	if ( !FileSystem::isDirectory( folderPath ) )
		folderPath = getLastUsedFolder();
	UIFileDialog* dialog =
		UIFileDialog::New( UIFileDialog::DefaultFlags | UIFileDialog::SaveDialog, "*", folderPath );
	dialog->setWindowFlags( UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON | UI_WIN_MODAL );
	dialog->setTitle( i18n( "save_file_as", "Save File As" ) );
	dialog->setCloseShortcut( KEY_ESCAPE );
	dialog->setSingleClickNavigation( mConfig.editor.singleClickNavigation );
	dialog->setFileName( filename );
	dialog->on( Event::SaveFile, [this, editor]( const Event* event ) {
		if ( editor ) {
			std::string path( event->getNode()->asType<UIFileDialog>()->getFullPath() );
			if ( !path.empty() && !FileSystem::isDirectory( path ) &&
				 FileSystem::fileWrite( path, "" ) ) {
				if ( editor->getDocument().save( path ) ) {
					insertRecentFileAndUpdateUI( path );
					updateEditorState();
				} else {
					UIMessageBox* msg =
						UIMessageBox::New( UIMessageBox::OK, i18n( "couldnt_write_the_file",
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
		dialog->on( Event::OnWindowClose, [editor]( const Event* ) {
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
		UICodeEditor* editor = mSplitter->getCurWidget()->asType<UICodeEditor>();
		editor->getDocument().execute( command, editor );
	} else if ( mSplitter->getCurWidget() &&
				mSplitter->getCurWidget()->isType( UI_TYPE_TERMINAL ) ) {
		mSplitter->getCurWidget()->asType<UITerminal>()->execute( command );
	} else {
		mMainLayout->execute( command );
	}
}

bool App::commandExists( const std::string& command ) const {
	if ( mSplitter->getCurWidget() && mSplitter->getCurWidget()->isType( UI_TYPE_CODEEDITOR ) ) {
		UICodeEditor* editor = mSplitter->getCurWidget()->asType<UICodeEditor>();
		if ( editor->getDocument().hasCommand( command ) )
			return true;
	}
	return mMainLayout->getKeyBindings().hasCommand( command );
}

void App::onPluginEnabled( Plugin* plugin ) {
	if ( mSplitter ) {
		mSplitter->forEachEditor(
			[plugin]( UICodeEditor* editor ) { editor->registerPlugin( plugin ); } );
	}

	if ( firstFrame && mConfig.isNewVersion() ) {
		plugin->onVersionUpgrade( mConfig.windowState.lastRunVersion,
								  ecode::Version::getVersionNum() );
	}
}

void App::initPluginManager() {
	mPluginManager = std::make_unique<PluginManager>(
		mResPath, mPluginsPath, mConfigPath, mThreadPool,
		[this]( const std::string& path, const auto& cb ) {
			UITab* tab = mSplitter->isDocumentOpen( path );
			if ( !tab ) {
				loadFileFromPath( path, true, nullptr, cb );
			} else {
				tab->getTabWidget()->setTabSelected( tab );
				cb( tab->getOwnedWidget()->asType<UICodeEditor>(), path );
			}
		},
		this );
	mPluginManager->onPluginEnabled = [this]( Plugin* plugin ) {
		if ( nullptr == mUISceneNode || plugin->isReady() ) {
			onPluginEnabled( plugin );
		} else {
			// If plugin loads asynchronously and is not ready, delay the plugin enabled callback
			plugin->addOnReadyCallback( [this]( UICodeEditorPlugin* plugin, const Uint32& cbId ) {
				mUISceneNode->runOnMainThread(
					[this, plugin]() { onPluginEnabled( static_cast<Plugin*>( plugin ) ); } );
				plugin->removeReadyCallback( cbId );
			} );
		}
	};

	mPluginManager->registerPlugin( DebuggerPlugin::Definition() );
	mPluginManager->registerPlugin( LinterPlugin::Definition() );
	mPluginManager->registerPlugin( FormatterPlugin::Definition() );
	mPluginManager->registerPlugin( AutoCompletePlugin::Definition() );
	mPluginManager->registerPlugin( LSPClientPlugin::Definition() );
	mPluginManager->registerPlugin( XMLToolsPlugin::Definition() );
	mPluginManager->registerPlugin( GitPlugin::Definition() );
	mPluginManager->registerPlugin( AIAssistantPlugin::Definition() );
	mPluginManager->registerPlugin( SpellCheckerPlugin::Definition() );
	mPluginManager->registerPlugin( DiscordRPCplugin::Definition() );
	mPluginManager->setPluginsDisabled( mDisablePlugins );
}

std::pair<bool, std::string> App::generateConfigPath() {
	if ( mPortableMode ) {
		std::string path( mProfilePath.empty() ? ( Sys::getProcessPath() + "config" )
											   : mProfilePath );
		FileSystem::dirAddSlashAtEnd( path );
		bool fileExists = FileSystem::fileExists( path );

		if ( fileExists && !FileSystem::isDirectory( path ) ) {
			if ( FileSystem::fileRemove( path ) )
				fileExists = false;
			else
				return { false, Sys::getConfigPath( "ecode" ) };
		}

		if ( !fileExists && !FileSystem::makeDir( path ) ) {
			path = Sys::getTempPath();
			path += "ecode_portable";
			path += FileSystem::getOSSlash();
			if ( !FileSystem::fileExists( path ) && !FileSystem::makeDir( path ) )
				return { false, Sys::getConfigPath( "ecode" ) };
		}

		return { true, path };
	}

	return { true, Sys::getConfigPath( "ecode" ) };
}

bool App::isAnyStatusBarSectionVisible() const {
	return ( mMainSplitter && mMainSplitter->getLastWidget() != nullptr ) ||
		   mStatusBar->querySelector( ".selected" ) != nullptr;
}

bool App::loadConfig( const LogLevel& logLevel, const Sizeu& displaySize, bool sync,
					  bool stdOutLogs, bool disableFileLogs ) {
	if ( !mPortableMode )
		mPortableMode = FileSystem::fileExists( Sys::getProcessPath() + "portable_mode.txt" );
	auto [ok, configPath] = generateConfigPath();
	mConfigPath = std::move( configPath );
	mPortableModeFailed = !ok;
	bool firstRun = false;
	if ( !FileSystem::fileExists( mConfigPath ) ) {
		FileSystem::makeDir( mConfigPath );
		firstRun = true;
	}
	FileSystem::dirAddSlashAtEnd( mConfigPath );
	mPluginsPath = mConfigPath + "plugins";
	mLanguagesPath = mConfigPath + "languages";
	mThemesPath = mConfigPath + "themes";
	mScriptsPath = mConfigPath + "scripts";
	mPlaygroundPath = mConfigPath + "playground";
	mIpcPath = mConfigPath + "ipc";
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

	if ( !FileSystem::fileExists( mScriptsPath ) )
		FileSystem::makeDir( mScriptsPath );
	FileSystem::dirAddSlashAtEnd( mScriptsPath );

	if ( !FileSystem::fileExists( mPlaygroundPath ) )
		FileSystem::makeDir( mPlaygroundPath );
	FileSystem::dirAddSlashAtEnd( mPlaygroundPath );

	if ( !FileSystem::fileExists( mIpcPath ) )
		FileSystem::makeDir( mIpcPath );
	FileSystem::dirAddSlashAtEnd( mIpcPath );

	Uint64 pid = Sys::getProcessID();
	mPidPath = mIpcPath + String::toString( pid );
	FileSystem::dirAddSlashAtEnd( mPidPath );
	if ( !FileSystem::fileExists( mPidPath ) )
		FileSystem::makeDir( mPidPath );

	mLogsPath = mConfigPath + "ecode.log";

	Log::create( mLogsPath, logLevel, stdOutLogs, !disableFileLogs );

	Log::instance()->setKeepLog( true );

	if ( !mArgs.empty() ) {
		std::string strargs( String::join( mArgs ) );
		Log::info( "ecode starting with these command line arguments: %s", strargs );
	}

	initPluginManager();

	mConfig.load( mConfigPath, mKeybindingsPath, mInitColorScheme, mRecentFiles, mRecentFolders,
				  mResPath, mPluginManager.get(), displaySize.asInt(), sync );

	return firstRun;
}

void App::saveConfig() {
	if ( !mCurrentProject.empty() )
		saveSidePanelTabsOrder();

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
	return mMainLayout->getKeyBindings().getCommandKeybindString( command );
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
			mSplitter->getCurEditor()->getDocument().execute( cmd, mSplitter->getCurEditor() );
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

void App::onReady() {
	// Plugin reload is only available right after we render the first frame and the editor is ready
	// to run.
	mPluginManager->setPluginReloadEnabled( true );
	Log::info( "App Ready" );

	if ( mPortableModeFailed ) {
		UIMessageBox* msg = UIMessageBox::New(
			UIMessageBox::OK,
			i18n( "portable_mode_failed", "Portable Mode failed.\nPlease check that the "
										  "application directory has write permissions." ) );
		msg->setTitle( "Error" );
		msg->setCloseShortcut( { KEY_ESCAPE, 0 } );
		msg->showWhenReady();
	}
}

void App::mainLoop() {
	mWindow->getInput()->update();
	SceneManager::instance()->update();

	if ( unlikely( firstUpdate ) ) {
		Log::info( "First update took: %.2f ms", globalClock.getElapsedTime().asMilliseconds() );
		firstUpdate = false;
	}

	if ( SceneManager::instance()->getUISceneNode()->invalidated() || mBenchmarkMode ) {
		mWindow->clear();
		SceneManager::instance()->draw();
		mWindow->display();
		if ( unlikely( firstFrame ) ) {
			Log::info( "First frame took: %.2f ms", globalClock.getElapsedTime().asMilliseconds() );
			firstFrame = false;
			onReady();
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

void App::onFileDropped( std::string file, bool openBinaryAsDocument ) {
	Vector2f mousePos( mUISceneNode->getEventDispatcher()->getMousePosf() );
	Node* node = mUISceneNode->overFind( mousePos );
	UIWidget* widget = mSplitter->getCurWidget();
	UITab* tab = mSplitter->isDocumentOpen( file );
	UICodeEditor* codeEditor = nullptr;

	if ( tab && tab->getTabWidget() ) {
		tab->getTabWidget()->setTabSelected( tab );
		return;
	}

	if ( !node )
		node = widget;

	bool willLoadAnImage =
		Image::isImageExtension( file ) && FileSystem::fileExtension( file ) != "svg";
	if ( node && node->isType( UI_TYPE_CODEEDITOR ) ) {
		codeEditor = node->asType<UICodeEditor>();
		if ( ( codeEditor->getDocument().isLoading() || codeEditor->getDocument().hasFilepath() ||
			   !codeEditor->getDocument().isEmpty() ) &&
			 !willLoadAnImage ) {
			auto d = mSplitter->createCodeEditorInTabWidget(
				mSplitter->tabWidgetFromEditor( codeEditor ) );
			codeEditor = d.second;
			tab = d.first;
		}
	} else if ( widget && widget->isType( UI_TYPE_TERMINAL ) ) {
		if ( !Image::isImageExtension( file ) && !willLoadAnImage ) {
			auto d =
				mSplitter->createCodeEditorInTabWidget( mSplitter->tabWidgetFromWidget( widget ) );
			codeEditor = d.second;
			tab = d.first;
		}
	} else if ( !willLoadAnImage ) {
		auto d = mSplitter->createEditorInNewTab();
		codeEditor = d.second;
		tab = d.first;
	}

	loadFileFromPath(
		file, false, codeEditor,
		[this, tab]( UICodeEditor* editor, const std::string& ) {
			if ( tab )
				tab->setTabSelected();
			else {
				UITab* tab = mSplitter->tabFromEditor( editor );
				if ( tab )
					tab->setTabSelected();
			}
		},
		openBinaryAsDocument );
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
		ThreadPool::createShared( jobs > 0 ? jobs : eemax<int>( 4, Sys::getCPUCount() ) ) ),
	mSettingsActions( std::make_unique<SettingsActions>( this ) ) {}

static void fsRemoveAll( const std::string& fpath ) {
#if EE_PLATFORM == EE_PLATFORM_WIN
	fs::remove_all( std::filesystem::path( String( fpath ).toWideString() ) );
#else
	fs::remove_all( fpath );
#endif
}

App::~App() {
	mDestroyingApp = true;
	if ( mProjectBuildManager )
		mProjectBuildManager.reset();

	Http::setThreadPool( nullptr );
	mThreadPool.reset();

	if ( mFileWatcher ) {
		Lock l( mWatchesLock );
		delete mFileWatcher;
		mFileWatcher = nullptr;
	}
	if ( mDirTree )
		mDirTree->resetPluginManager();
	mPluginManager.reset();
	eeSAFE_DELETE( mSplitter );

	if ( mFileSystemListener ) {
		if ( mIpcListenerId )
			mFileSystemListener->removeListener( mIpcListenerId );
		delete mFileSystemListener;
		mFileSystemListener = nullptr;
	}
	mDirTree.reset();

	fsRemoveAll( mPidPath );
}

void App::updateRecentButtons() {
	updateOpenRecentFolderBtn();

	if ( mSplitter ) {
		mSplitter->forEachWidgetType(
			static_cast<UINodeType>( CustomWidgets::UI_TYPE_WELCOME_TAB ), []( UIWidget* widget ) {
				UIWelcomeScreen* welcomeTab = static_cast<UIWelcomeScreen*>( widget );
				welcomeTab->refresh();
			} );
	}
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
		for ( const auto& file : mRecentFiles ) {
			if ( ( FileSystem::fileExists( file ) && !FileSystem::isDirectory( file ) ) ||
				 String::startsWith( file, "https://" ) || String::startsWith( file, "http://" ) )
				menu->add( file );
		}
		menu->addSeparator();
		menu->add( i18n( "clear_menu", "Clear Menu" ) )->setId( "clear-menu" );
		menu->on( Event::OnItemClicked, [this]( const Event* event ) {
			if ( !event->getNode()->isType( UI_TYPE_MENUITEM ) )
				return;
			const std::string& id = event->getNode()->asType<UIMenuItem>()->getId();
			if ( id == "reopen-closed-tab" ) {
				reopenClosedTab();
			} else if ( id == "clear-menu" ) {
				mRecentFiles.clear();
				updateRecentFiles();
				updateRecentButtons();
			} else {
				const String& txt = event->getNode()->asType<UIMenuItem>()->getText();
				std::string path( txt.toUtf8() );
				if ( ( FileSystem::fileExists( path ) && !FileSystem::isDirectory( path ) ) ||
					 String::startsWith( path, "https://" ) ||
					 String::startsWith( path, "http://" ) ) {
					loadFileFromPathOrFocus( path );
				} else {
					auto msgBox = UIMessageBox::New(
						UIMessageBox::YES_NO,
						i18n( "file_does_not_exists_anymore_recreate",
							  "File does not exists anymore.\nDo you want to recreate it?" ) );
					msgBox->setTitle( i18n( "file_not_found", "File not found" ) );
					msgBox->on( Event::OnConfirm, [path, this]( auto ) {
						FileSystem::fileWrite( path, "" );
						loadFileFromPathOrFocus( path );
					} );
					msgBox->center();
					msgBox->showWhenReady();
					updateRecentFiles();
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
		for ( const auto& file : mRecentFolders ) {
			if ( FileSystem::fileExists( file ) && FileSystem::isDirectory( file ) )
				menu->add( file );
		}
		menu->addSeparator();
		menu->add( i18n( "clear_menu", "Clear Menu" ) )->setId( "clear-menu" );
		menu->addCheckBox(
				i18n( "restore_last_session_at_startup", "Restart last session at startup" ) )
			->setActive( mConfig.workspace.restoreLastSession )
			->setId( "restore-last-session-at-startup" );
		menu->on( Event::OnItemClicked, [this]( const Event* event ) {
			if ( !event->getNode()->isType( UI_TYPE_MENUITEM ) )
				return;
			const std::string& id = event->getNode()->asType<UIMenuItem>()->getId();
			if ( id == "clear-menu" ) {
				mRecentFolders.clear();
				updateRecentFolders();
				updateRecentButtons();
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
	return mProjectBuildManager ? mProjectBuildManager.get() : nullptr;
}

UITabWidget* App::getSidePanel() const {
	return mSidePanel;
}

const std::map<KeyBindings::Shortcut, std::string>& App::getRealLocalKeybindings() const {
	return mRealLocalKeybindings;
}

const std::map<KeyBindings::Shortcut, std::string>& App::getRealSplitterKeybindings() const {
	return mRealSplitterKeybindings;
}

const std::map<KeyBindings::Shortcut, std::string>& App::getRealTerminalKeybindings() const {
	return mRealTerminalKeybindings;
}

const std::string& App::getFileToOpen() const {
	return mFileToOpen;
}

void App::switchSidePanel() {
	mConfig.ui.showSidePanel = !mConfig.ui.showSidePanel;
	mSettings->updateViewMenu();
	showSidePanel( mConfig.ui.showSidePanel );
}

void App::switchStatusBar() {
	mConfig.ui.showStatusBar = !mConfig.ui.showStatusBar;
	mSettings->updateViewMenu();
	updateDocInfoLocation();
	showStatusBar( mConfig.ui.showStatusBar );
}

void App::switchMenuBar() {
	mConfig.ui.showMenuBar = !mConfig.ui.showMenuBar;
	mSettings->updateMenu();
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

	mSettings->updateViewMenu();
}

void App::setUIColorScheme( const ColorSchemePreference& colorScheme ) {
	if ( colorScheme == mUIColorScheme )
		return;
	mUIColorScheme = mConfig.ui.colorScheme = colorScheme;
	mUISceneNode->setColorSchemePreference( colorScheme );
	if ( !firstFrame )
		mPluginManager->setUIThemeReloaded();
}

ColorSchemePreference App::getUIColorScheme() const {
	return mUIColorScheme;
}

EE::Window::Window* App::getWindow() const {
	return mWindow;
}

UITextView* App::getDocInfo() const {
	return mDocInfo;
}

UITreeView* App::getProjectTreeView() const {
	return mProjectTreeView;
}

PluginManager* App::getPluginManager() const {
	return mPluginManager.get();
}

void App::setFocusEditorOnClose( UIMessageBox* msgBox ) {
	msgBox->on( Event::OnClose, [this]( const Event* ) {
		if ( mSplitter && mSplitter->getCurWidget() )
			mSplitter->getCurWidget()->setFocus();
	} );
}

Drawable* App::findIcon( const std::string& name ) {
	return findIcon( name, mMenuIconSize );
}

Drawable* App::findIcon( const std::string& name, const size_t iconSize ) {
	if ( name.empty() )
		return nullptr;
	UIIcon* icon = mUISceneNode->findIcon( name );
	if ( icon )
		return icon->getSize( iconSize );
	return nullptr;
}

String App::i18n( const std::string& key, const String& def ) {
	return mUISceneNode->getTranslatorStringFromKey( key, def );
}

std::string App::getCurrentWorkingDir() const {
	if ( !mCurrentProject.empty() && mCurrentProject != mPlaygroundPath )
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

void App::loadFileFromPathOrFocus(
	const std::string& path, bool inNewTab, UICodeEditor* codeEditor,
	std::function<void( UICodeEditor*, const std::string& )> onLoaded ) {
	UITab* tab = mSplitter->isDocumentOpen( path );
	if ( !tab ) {
		loadFileFromPath( path, inNewTab, codeEditor, onLoaded );
	} else {
		tab->getTabWidget()->setTabSelected( tab );
	}
}

void App::focusOrLoadFile( const std::string& path, const TextRange& range,
						   bool searchInSameContext ) {
	UITab* tab = mSplitter->isDocumentOpen( path, searchInSameContext );
	if ( !tab ) {
		FileInfo fileInfo( path );
		if ( fileInfo.exists() && fileInfo.isRegularFile() )
			loadFileFromPath( path, true, nullptr, [this, range]( UICodeEditor* editor, auto ) {
				if ( range.isValid() ) {
					editor->goToLine( range.start() );
					mSplitter->addEditorPositionToNavigationHistory( editor );
				}
			} );
	} else {
		tab->getTabWidget()->setTabSelected( tab );
		if ( range.isValid() ) {
			UICodeEditor* editor = tab->getOwnedWidget()->asType<UICodeEditor>();
			editor->goToLine( range.start() );
			mSplitter->addEditorPositionToNavigationHistory( editor );
		}
	}
}

void App::createPluginManagerUI() {
	UIPluginManager::New( mUISceneNode, mPluginManager.get(), [this]( const std::string& path ) {
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

void App::loadKeybindings() {
	if ( !mKeybindings.empty() )
		return;

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

	KeybindingsHelper::updateKeybindings( ini, "editor", mWindow->getInput(), mKeybindings,
										  mKeybindingsInvert, getDefaultKeybindings(), forceRebind,
										  getMigrateKeybindings(), mConfig.iniState );

	KeybindingsHelper::updateKeybindings( ini, "global_search", mWindow->getInput(),
										  mGlobalSearchKeybindings,
										  GlobalSearchController::getDefaultKeybindings(),
										  forceRebind, getMigrateKeybindings(), mConfig.iniState );

	KeybindingsHelper::updateKeybindings( ini, "document_search", mWindow->getInput(),
										  mDocumentSearchKeybindings,
										  DocSearchController::getDefaultKeybindings(), forceRebind,
										  getMigrateKeybindings(), mConfig.iniState );

	KeybindingsHelper::updateKeybindings( ini, "editor_mouse_bindings", mWindow->getInput(),
										  mMousebindings, mMousebindingsInvert,
										  UICodeEditor::getDefaultMousebindings(), forceRebind,
										  getMigrateKeybindings(), mConfig.iniState );

	auto localKeybindings = getLocalKeybindings();
	for ( const auto& kb : localKeybindings ) {
		auto found = mKeybindingsInvert.find( kb.second );
		if ( found != mKeybindingsInvert.end() ) {
			mRealLocalKeybindings[bindings.getShortcutFromString( found->second )] = kb.second;
		} else {
			mRealLocalKeybindings[kb.first] = kb.second;
		}
	}

	auto localSplitterKeybindings = UICodeEditorSplitter::getLocalDefaultKeybindings();
	for ( const auto& kb : localSplitterKeybindings ) {
		auto found = mKeybindingsInvert.find( kb.second );
		if ( found != mKeybindingsInvert.end() ) {
			mRealSplitterKeybindings[bindings.getShortcutFromString( found->second )] = kb.second;
		} else {
			mRealSplitterKeybindings[kb.first] = kb.second;
		}
	}

	auto localTerminalKeybindings = TerminalManager::getTerminalKeybindings();
	for ( const auto& kb : localTerminalKeybindings ) {
		auto found = mKeybindingsInvert.find( kb.second );
		if ( found != mKeybindingsInvert.end() ) {
			mRealTerminalKeybindings[bindings.getShortcutFromString( found->second )] = kb.second;
		} else {
			mRealTerminalKeybindings[kb.first] = kb.second;
		}
	}
}

void App::reloadKeybindings() {
	mKeybindings.clear();
	mKeybindingsInvert.clear();
	mRealLocalKeybindings.clear();
	mRealSplitterKeybindings.clear();
	mRealTerminalKeybindings.clear();
	mRealDefaultKeybindings.clear();
	mMousebindings.clear();
	mMousebindingsInvert.clear();
	loadKeybindings();
	mSplitter->forEachEditor( [this]( UICodeEditor* ed ) {
		ed->getKeyBindings().reset();
		ed->getKeyBindings().addKeybindsStringUnordered( mKeybindings );
		ed->getMouseBindings().reset();
		ed->getMouseBindings().addMousebindsString( mMousebindings );
	} );
	mSplitter->forEachWidgetType( UI_TYPE_TERMINAL, [this]( UIWidget* widget ) {
		mTerminalManager->setKeybindings( widget->asType<UITerminal>() );
	} );
	mMainLayout->getKeyBindings().reset();
	mMainLayout->getKeyBindings().addKeybinds( getRealDefaultKeybindings() );
}

void App::onDocumentStateChanged( UICodeEditor*, TextDocument& ) {
	updateEditorState();
}

void App::onDocumentSelectionChange( UICodeEditor* editor, TextDocument& doc ) {
	onDocumentModified( editor, doc );
}

void App::onDocumentCursorPosChange( UICodeEditor* editor, TextDocument& doc ) {
	if ( mSplitter->curEditorExistsAndFocused() && mSplitter->getCurEditor() == editor )
		updateDocInfo( doc );
}

void App::updateDocInfoLocation() {
	if ( !mDocInfo )
		return;
	if ( mConfig.ui.showStatusBar ) {
		if ( mStatusBar != mDocInfo->getParent() ) {
			mDocInfo->setParent( mStatusBar );
			mDocInfo->setEnabled( true );
		}
	} else if ( mStatusBar == mDocInfo->getParent() ) {
		mDocInfo->setParent( mMainSplitter->find( "main_splitter_cont" ) );
		mDocInfo->setEnabled( false );
	}
}

void App::updateDocInfo( TextDocument& doc ) {
	if ( !doc.isRunningTransaction() && !doc.isLoading() && mConfig.editor.showDocInfo &&
		 mDocInfo && mSplitter->curEditorExistsAndFocused() ) {
		mDocInfo->setVisible( true );
		updateDocInfoLocation();
		String infoStr( String::format(
			"%s: %lld / %zu  %s: %lld    %s    %s%s    %s", i18n( "line_abbr", "line" ).toUtf8(),
			doc.getSelection().start().line() + 1, doc.linesCount(),
			i18n( "col_abbr", "col" ).toUtf8(), mSplitter->getCurEditor()->getCurrentColumnCount(),
			doc.getSyntaxDefinition().getLanguageName(),
			TextFormat::encodingToString( doc.getEncoding() ), doc.isBOM() ? " (with BOM)"sv : ""sv,
			TextFormat::lineEndingToString( doc.getLineEnding() ) ) );
		mDocInfo->debounce( [this, infoStr] { mDocInfo->setText( infoStr ); }, Time::Zero,
							String::hash( "ecode::doc_info::update" ) );
	}
}

void App::syncProjectTreeWithEditor( UICodeEditor* editor ) {
	if ( mProjectTreeView && mConfig.editor.syncProjectTreeWithEditor && editor != nullptr &&
		 ( editor->getDocument().hasFilepath() ||
		   !editor->getDocument().getLoadingFilePath().empty() ) ) {
		std::string loadingPath( editor->getDocument().getLoadingFilePath() );
		std::string path = !loadingPath.empty() ? loadingPath : editor->getDocument().getFilePath();
		mProjectTreeView->setFocusOnSelection( false );
		if ( !mCurrentProject.empty() && String::startsWith( path, mCurrentProject ) ) {
			mProjectTreeView->openRowWithPath( path.substr( mCurrentProject.size() ) );
		} else {
			mProjectTreeView->openRowWithPath( FileSystem::fileNameFromPath( path ) );
		}
		mProjectTreeView->setFocusOnSelection( true );
	}
}

void App::onWidgetFocusChange( UIWidget* widget ) {
	if ( mConfig.editor.showDocInfo && mDocInfo )
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

void App::onTabCreated( UITab* tab, UIWidget* ) {
	tab->on( Event::OnCreateContextMenu, [this]( const Event* event ) {
		if ( !event->getNode()->isType( UI_TYPE_TAB ) )
			return;
		const ContextMenuEvent* menuEvent = static_cast<const ContextMenuEvent*>( event );
		UIPopUpMenu* menu = menuEvent->getMenu();
		if ( nullptr == menu )
			return;
		UITab* tab = event->getNode()->asType<UITab>();
		if ( !tab->getTabWidget() )
			return;
		const auto menuAdd = [menu, this]( const std::string& translateKey,
										   const String& translateString, const std::string& icon,
										   const std::string& cmd ) {
			UIMenuItem* menuItem = menu->add( i18n( translateKey, translateString ),
											  findIcon( icon ), getKeybind( cmd ) );
			menuItem->setId( cmd );
			return menuItem;
		};

		menuAdd( "editor_tab_menu_close_tab", "Close Tab", "document-close", "close-tab" );
		menuAdd( "editor_tab_menu_close_other_tabs", "Close Other Tabs", "", "close-other-tabs" );
		menuAdd( "editor_tab_menu_close_clean_tabs", "Close Clean Tabs", "", "close-clean-tabs" );
		menuAdd( "editor_tab_menu_close_all_tabs", "Close All Tabs", "", "close-all-tabs" );
		menuAdd( "editor_tab_menu_close_tabs_to_the_left", "Close Tabs To The Left", "",
				 "close-tabs-to-the-left" );
		menuAdd( "editor_tab_menu_close_tabs_to_the_right", "Close Tabs To The Right", "",
				 "close-tabs-to-the-right" );

		if ( tab->getOwnedWidget()->isType( UI_TYPE_CODEEDITOR ) ) {
			menu->addSeparator();

			menuAdd( "split_left", "Split Left", "split-horizontal", "split-left" );
			menuAdd( "split_right", "Split Right", "split-horizontal", "split-right" );
			menuAdd( "split_top", "Split Top", "split-vertical", "split-top" );
			menuAdd( "split_bottom", "Split Bottom", "split-vertical", "split-bottom" );

			menuAdd( "open_containing_folder_ellipsis", "Open Containing Folder...", "folder-open",
					 "open-containing-folder" );

			menuAdd( "copy_containing_folder_path_ellipsis", "Copy Containing Folder Path...",
					 "copy", "copy-containing-folder-path" );

			menuAdd( "copy_file_path", "Copy File Path", "copy", "copy-file-path" );

			menuAdd( "copy_file_path_and_position", "Copy File Path and Position", "copy",
					 "copy-file-path-and-position" );

			menu->addSeparator();

			menuAdd( "open_in_new_window", "Open in New Window", "window", "open-in-new-window" );

			menuAdd( "move_to_new_window", "Move to New Window", "window", "move-to-new-window" );
		}

		if ( tab->getOwnedWidget()->isType( UI_TYPE_CODEEDITOR ) ||
			 tab->getOwnedWidget()->isType( UI_TYPE_TERMINAL ) ) {
			menu->addSeparator();

			bool enabled = tab->getTabWidget()->getTabCount() > 1;

			menuAdd( "move_tab_to_start", "Move Tab To Start", "window", "move-tab-to-start" )
				->setEnabled( enabled );

			menuAdd( "move_tab_to_end", "Move Tab To End", "window", "move-tab-to-end" )
				->setEnabled( enabled );
		}

		menu->addEventListener( Event::OnItemClicked, [tab, this]( const Event* event ) {
			if ( !event->getNode()->isType( UI_TYPE_MENUITEM ) )
				return;
			UIMenuItem* item = event->getNode()->asType<UIMenuItem>();
			UICodeEditor* newCe = nullptr;
			UICodeEditor* ce = nullptr;
			UIWidget* ncw = nullptr;
			UIWidget* cw = nullptr;
			if ( tab->getOwnedWidget()->isType( UI_TYPE_CODEEDITOR ) ) {
				newCe = tab->getOwnedWidget()->asType<UICodeEditor>();
				ce = mSplitter->getCurEditor();
				if ( newCe != ce )
					mSplitter->setCurrentEditor( newCe );
			} else {
				ncw = tab->getOwnedWidget()->asType<UIWidget>();
				cw = mSplitter->getCurWidget();
				if ( ncw != cw )
					mSplitter->setCurrentWidget( ncw );
			}
			runCommand( item->getId() );
			if ( tab->getOwnedWidget()->isType( UI_TYPE_CODEEDITOR ) ) {
				if ( newCe != ce && mSplitter->checkEditorExists( ce ) )
					mSplitter->setCurrentEditor( ce );
			} else {
				if ( ncw != cw && mSplitter->checkWidgetExists( cw ) )
					mSplitter->setCurrentWidget( cw );
			}
		} );
	} );
}

void App::onColorSchemeChanged( const std::string& ) {
	mSettings->updateColorSchemeMenu();
	mGlobalSearchController->updateColorScheme( mSplitter->getCurrentColorScheme() );

	if ( mStatusBuildOutputController && mStatusBuildOutputController->getContainer() ) {
		mStatusBuildOutputController->getContainer()->setColorScheme(
			mSplitter->getCurrentColorScheme() );
	}

	if ( mStatusAppOutputController && mStatusAppOutputController->getContainer() ) {
		mStatusAppOutputController->getContainer()->setColorScheme(
			mSplitter->getCurrentColorScheme() );
	}

	mNotificationCenter->addNotification(
		String::format( i18n( "color_scheme_set", "Color scheme: %s" ).toUtf8(),
						mSplitter->getCurrentColorScheme().getName() ) );
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
					editor->on( Event::OnDocumentLoaded, [this, fileAndPos]( const Event* event ) {
						if ( event->getNode()->isType( UI_TYPE_CODEEDITOR ) ) {
							UICodeEditor* editor = event->getNode()->asType<UICodeEditor>();
							editor->runOnMainThread( [this, editor, fileAndPos] {
								editor->goToLine( fileAndPos.second );
								mSplitter->addEditorPositionToNavigationHistory( editor );
							} );
						}
						event->getNode()->removeEventListener( event->getCallbackId() );
					} );
				// Don't listen forever if no event is received
				editor->runOnMainThread( [editor, cb]() { editor->removeEventListener( cb ); },
										 Seconds( 4 ) );
			} else {
				editor->runOnMainThread( [this, editor, fileAndPos] {
					editor->goToLine( fileAndPos.second );
					mSplitter->addEditorPositionToNavigationHistory( editor );
				} );
			}
		}
	} else {
		loadFileFromPath( fileAndPos.first, true, nullptr,
						  [this, fileAndPos]( UICodeEditor* editor, const std::string& ) {
							  editor->runOnMainThread( [this, editor, fileAndPos] {
								  editor->goToLine( fileAndPos.second );
								  mSplitter->addEditorPositionToNavigationHistory( editor );
								  UITab* tab = mSplitter->tabFromEditor( editor );
								  if ( tab )
									  tab->setTabSelected();
								  updateEditorTabTitle( editor );
							  } );
						  } );
	}

	mFileToOpen.clear();
}

const std::string& App::getThemesPath() const {
	return mThemesPath;
}

const std::string& App::geti18nPath() const {
	return mi18nPath;
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
	theme->setDefaultFontSize( mConfig.ui.fontSize.asPixels( 0, Sizef(), mDisplayDPI ) );

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
						auto root = theme->getStyleSheet().getStyleFromSelector( ":root", true );
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

	theme->getStyleSheet().invalidateCache();
	mAppStyleSheet.invalidateCache();

	mUISceneNode->setStyleSheet( theme->getStyleSheet(), false );

	if ( !mAppStyleSheet.isEmpty() )
		mUISceneNode->getStyleSheet().combineStyleSheet( mAppStyleSheet );

	mUISceneNode->getStyleSheet().updateMediaLists( mUISceneNode->getMediaFeatures() );

	mUISceneNode
		->getUIThemeManager()
		//->setDefaultEffectsEnabled( true )
		->setDefaultTheme( theme )
		->setDefaultFont( mFont )
		->setDefaultFontSize( mConfig.ui.fontSize.asPixels( 0, Sizef(), mDisplayDPI ) )
		->add( theme );

	mUISceneNode->setTheme( theme );

	mUISceneNode->getRoot()->addClass( "appbackground" );

	if ( mTheme )
		mUISceneNode->getUIThemeManager()->remove( mTheme );

	mTheme = theme;

	mUISceneNode->reloadStyle( true, true, true );

	if ( !firstFrame )
		mPluginManager->setUIThemeReloaded();
}

bool App::dirInFolderWatches( const std::string& dir ) {
	Lock l( mWatchesLock );
	for ( const auto& watch : mFolderWatches )
		if ( String::startsWith( dir, watch.first ) )
			return true;
	return false;
}

void App::insertRecentFile( const std::string& path ) {
	if ( mIncognito )
		return;
	auto found = std::find( mRecentFiles.begin(), mRecentFiles.end(), path );
	if ( found != mRecentFiles.end() )
		mRecentFiles.erase( found );
	mRecentFiles.insert( mRecentFiles.begin(), path );
	if ( mRecentFiles.size() > 10 )
		mRecentFiles.resize( 10 );
}

void App::insertRecentFileAndUpdateUI( const std::string& path ) {
	insertRecentFile( path );
	cleanUpRecentFiles();
	auto urfId = String::hash( "updateRecentFiles" );
	mUISceneNode->debounce( [this] { updateRecentFiles(); }, Seconds( 0.5f ), urfId );
}

void App::onRealDocumentLoaded( UICodeEditor* editor, const std::string& path ) {
	updateEditorTitle( editor );
	if ( mSplitter->curEditorExistsAndFocused() && editor == mSplitter->getCurEditor() )
		mSettings->updateCurrentFileType();
	mSplitter->removeUnusedTab( mSplitter->tabWidgetFromEditor( editor ) );
	insertRecentFileAndUpdateUI( path );
	if ( mSplitter->curEditorExistsAndFocused() && mSplitter->getCurEditor() == editor ) {
		mSettings->updateDocumentMenu();
		updateDocInfo( editor->getDocument() );
	}

	if ( !path.empty() ) {
		UITab* tab = reinterpret_cast<UITab*>( editor->getData() );
		tab->setTooltipText( path );
	}

	TextDocument& doc = editor->getDocument();
	std::string filePath =
		doc.hasFilepath() ? doc.getFilePath()
						  : ( !doc.getLoadingFilePath().empty() ? doc.getLoadingFilePath() : "" );

	if ( mFileWatcher && !filePath.empty() &&
		 ( !mDirTree || !mDirTree->isDirInTree( filePath ) ) ) {
		std::string dir( FileSystem::fileRemoveFileName( filePath ) );
		mThreadPool->run( [this, dir] {
			if ( mFileWatcher && !dirInFolderWatches( dir ) ) {
				auto watchId = mFileWatcher->addWatch( dir, mFileSystemListener );
				Lock l( mWatchesLock );
				mFilesFolderWatches[dir] = watchId;
			}
		} );
	}
}

void App::onDocumentLoaded( UICodeEditor* editor, const std::string& path ) {
	if ( editor->getData() == 0 )
		return;

	onRealDocumentLoaded( editor, path );

	// Check if other editor is using the same document and needs to receive the same notification
	const TextDocument* docPtr = &editor->getDocument();
	mSplitter->forEachEditor( [this, &path, docPtr, editor]( UICodeEditor* otherEditor ) {
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

const AppConfig& App::getConfig() const {
	return mConfig;
}

const std::map<KeyBindings::Shortcut, std::string>& App::getRealDefaultKeybindings() {
	if ( mRealDefaultKeybindings.empty() ) {
		mRealDefaultKeybindings.insert( mRealLocalKeybindings.begin(),
										mRealLocalKeybindings.end() );
		mRealDefaultKeybindings.insert( mRealSplitterKeybindings.begin(),
										mRealSplitterKeybindings.end() );
		mRealDefaultKeybindings.insert( mRealTerminalKeybindings.begin(),
										mRealTerminalKeybindings.end() );
	}
	return mRealDefaultKeybindings;
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
		{ { KEY_F11, KeyMod::getDefaultModifier() | KEYMOD_SHIFT }, "debug-widget-tree-view" },
		{ { KEY_K, KeyMod::getDefaultModifier() }, "open-locatebar" },
		{ { KEY_P, KeyMod::getDefaultModifier() }, "open-command-palette" },
		{ { KEY_F, KeyMod::getDefaultModifier() | KEYMOD_SHIFT }, "open-global-search" },
		{ { KEY_L, KeyMod::getDefaultModifier() }, "go-to-line" },
#if EE_PLATFORM == EE_PLATFORM_MACOS
		{ { KEY_M, KeyMod::getDefaultModifier() | KEYMOD_SHIFT }, "menu-toggle" },
#else
		{ { KEY_M, KeyMod::getDefaultModifier() }, "menu-toggle" },
#endif
		{ { KEY_S, KeyMod::getDefaultModifier() | KEYMOD_SHIFT }, "save-all" },
		{ { KEY_F9, KEYMOD_LALT }, "switch-side-panel" },
		{ { KEY_J, KeyMod::getDefaultModifier() | KEYMOD_LALT | KEYMOD_SHIFT },
		  "terminal-split-left" },
		{ { KEY_L, KeyMod::getDefaultModifier() | KEYMOD_LALT | KEYMOD_SHIFT },
		  "terminal-split-right" },
		{ { KEY_I, KeyMod::getDefaultModifier() | KEYMOD_LALT | KEYMOD_SHIFT },
		  "terminal-split-top" },
		{ { KEY_K, KeyMod::getDefaultModifier() | KEYMOD_LALT | KEYMOD_SHIFT },
		  "terminal-split-bottom" },
		{ { KEY_S, KeyMod::getDefaultModifier() | KEYMOD_LALT | KEYMOD_SHIFT },
		  "terminal-split-swap" },
		{ { KEY_T, KeyMod::getDefaultModifier() | KEYMOD_LALT | KEYMOD_SHIFT },
		  "reopen-closed-tab" },
		{ { KEY_1, KEYMOD_LALT }, "toggle-status-locate-bar" },
		{ { KEY_2, KEYMOD_LALT }, "toggle-status-global-search-bar" },
		{ { KEY_3, KEYMOD_LALT }, "toggle-status-terminal" },
		{ { KEY_4, KEYMOD_LALT }, "toggle-status-build-output" },
		{ { KEY_5, KEYMOD_LALT }, "toggle-status-app-output" },
		{ { KEY_B, KeyMod::getDefaultModifier() | KEYMOD_SHIFT }, "project-build-start-cancel" },
		{ { KEY_C, KeyMod::getDefaultModifier() | KEYMOD_SHIFT }, "project-build-cancel" },
		{ { KEY_R, KeyMod::getDefaultModifier() }, "project-build-and-run" },
		{ { KEY_O, KEYMOD_LALT | KEYMOD_SHIFT }, "show-open-documents" },
		{ { KEY_K, KeyMod::getDefaultModifier() | KEYMOD_SHIFT }, "open-workspace-symbol-search" },
		{ { KEY_P, KeyMod::getDefaultModifier() | KEYMOD_SHIFT }, "open-document-symbol-search" },
		{ { KEY_N, KEYMOD_SHIFT | KEYMOD_LALT }, "create-new-window" },
	};
}

// Old keybindings will be rebinded to the new keybindings when they are still set to the old
// keybindind
std::map<std::string, std::string> App::getMigrateKeybindings() {
	return { { "fullscreen-toggle", "alt+return" }, { "switch-to-tab-1", "alt+1" },
			 { "switch-to-tab-2", "alt+2" },		{ "switch-to-tab-3", "alt+3" },
			 { "switch-to-tab-4", "alt+4" },		{ "switch-to-tab-5", "alt+5" },
			 { "switch-to-tab-6", "alt+6" },		{ "switch-to-tab-7", "alt+7" },
			 { "switch-to-tab-8", "alt+8" },		{ "switch-to-tab-9", "alt+9" },
			 { "switch-to-last-tab", "alt+0" },
#if EE_PLATFORM == EE_PLATFORM_MACOS
			 { "menu-toggle", "mod+shift+m" },
#endif
			 { "lock-toggle", "mod+shift+l" },		{ "debug-widget-tree-view", "f11" },
			 { "project-build-and-run", "f5" },		{ "project-build-start", "mod+shift+b" } };
}

std::vector<std::string> App::getUnlockedCommands() {
	return { "create-new",
			 "create-new-terminal",
			 "create-new-welcome-tab",
			 "fullscreen-toggle",
			 "open-file",
			 "open-folder",
			 "reopen-closed-tab",
			 "console-toggle",
			 "close-app",
			 "open-locatebar",
			 "open-locatebar-glob-search",
			 "open-command-palette",
			 "open-global-search",
			 "project-build-start",
			 "project-build-start-cancel",
			 "project-build-cancel",
			 "project-build-clean",
			 "project-build-and-run",
			 "project-run-executable",
			 "toggle-status-locate-bar",
			 "toggle-status-global-search-bar",
			 "toggle-status-build-output",
			 "toggle-status-terminal",
			 "toggle-status-app-output",
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
			 "editor-set-indent-tab-character",
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
			 "tree-view-configure-ignore-files",
			 "show-open-documents",
			 "open-workspace-symbol-search",
			 "open-document-symbol-search",
			 "show-folder-treeview-tab",
			 "show-build-tab",
			 "create-new-window" };
}

void App::saveProject( bool onlyIfNeeded, bool sessionSnapshotEnabled ) {
	if ( !mCurrentProject.empty() ) {
		mConfig.saveProject(
			mCurrentProject, mSplitter, mConfigPath, mProjectDocConfig,
			mProjectBuildManager ? mProjectBuildManager->getConfig() : ProjectBuildConfiguration(),
			onlyIfNeeded, sessionSnapshotEnabled && mConfig.workspace.sessionSnapshot,
			mPluginManager.get() );
	}
}

void App::closeEditors() {
	mRecentClosedFiles = {};

	mSplitter->removeTabWithOwnedWidgetId( "welcome_ecode" );
	mSplitter->clearNavigationHistory();
	mStatusBar->setVisible( mConfig.ui.showStatusBar );

	saveProject();

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

	mSplitter->forEachTab( []( UITab* tab ) { tab->removeTab( true, true ); } );

	mCurrentProject = "";
	mCurrentProjectName = "";
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

	saveSidePanelTabsOrder();

	saveProject( true );

	if ( mProjectBuildManager )
		mProjectBuildManager.reset();

	if ( mSplitter->isAnyEditorDirty() && !mConfig.workspace.sessionSnapshot ) {
		UIMessageBox* msgBox = UIMessageBox::New(
			UIMessageBox::OK_CANCEL,
			i18n( "confirm_close_folder",
				  "Do you really want to close the folder?\nSome files haven't been saved." ) );
		msgBox->on( Event::OnConfirm, [this]( const Event* ) { closeEditors(); } );
		msgBox->setTitle( i18n( "close_folder_question", "Close Folder?" ) );
		msgBox->center();
		msgBox->showWhenReady();
	} else {
		closeEditors();
	}

	mProjectViewEmptyCont->setVisible( true );
	mFileSystemModel->setRootPath( "" );
	mPluginManager->setWorkspaceFolder( "" );
	updateOpenRecentFolderBtn();
	if ( mUniversalLocator )
		mUniversalLocator->updateFilesTable();
	if ( getConfig().ui.welcomeScreen ) {
		createWelcomeTab();
		mStatusBar->setVisible( false );
	}
}

void App::createWelcomeTab() {
	UIWelcomeScreen::createWelcomeScreen( this );
}

void App::createDocDirtyAlert( UICodeEditor* editor, bool showEnableAutoReload ) {
	UILinearLayout* docAlert = editor->findByClass<UILinearLayout>( "doc_alert" );

	if ( docAlert )
		return;

	const auto msg = R"xml(
	<hbox class="doc_alert" layout_width="wrap_content" layout_height="wrap_content" layout_gravity="top|right" gravity-owner="true">
		<TextView id="doc_alert_text" layout_width="wrap_content" layout_height="wrap_content" margin-right="24dp"
			text='@string(reload_current_file, "The file on the disk is more recent that the current buffer.&#xA;Do you want to reload it?")'
		/>
		<PushButton id="file_autoreload" layout_width="wrap_content" layout_height="18dp" text='@string("enable_autoreload", "Enable Auto-Reload")' margin-right="4dp"
					tooltip='@string(tooltip_autoreload_file, "Will never again warn about on disk changes but always reload unless there are local changes.")' />
		<PushButton id="file_reload" layout_width="wrap_content" layout_height="18dp" text='@string("reload", "Reload")' margin-right="4dp"
					tooltip='@string(tooltip_reload_file, "Reload the file from disk. Unsaved changes will be lost.")' />
		<PushButton id="file_overwrite" layout_width="wrap_content" layout_height="18dp" text='@string("overwrite", "Overwrite")' margin-right="4dp"
					tooltip='@string(tooltip_write_local_changes, "Writes the local changes on disk, overwriting the disk changes")' />
		<PushButton id="file_ignore" layout_width="wrap_content" layout_height="18dp" text='@string("ignore", "Ignore")'
					tooltip='@string(tooltip_ignore_file_changes, "Ignores the changes on disk without any action.")' />
	</hbox>
	)xml";
	docAlert = mUISceneNode->loadLayoutFromString( msg, editor )->asType<UILinearLayout>();

	editor->enableReportSizeChangeToChildren();

	docAlert->find( "file_autoreload" )
		->setVisible( showEnableAutoReload ? !editor->getDocument().isDirty() : false )
		->onClick( [editor, docAlert, this]( const MouseEvent* ) {
			editor->getDocument().reload();
			editor->disableReportSizeChangeToChildren();
			docAlert->close();
			editor->setFocus();
			mConfig.editor.autoReloadOnDiskChange = true;
			mSettings->updateGlobalDocumentSettingsMenu();
		} );

	docAlert->find( "file_reload" )->onClick( [editor, docAlert]( const MouseEvent* ) {
		editor->getDocument().reload();
		editor->disableReportSizeChangeToChildren();
		docAlert->close();
		editor->setFocus();
	} );

	docAlert->find( "file_overwrite" )->onClick( [editor, docAlert]( const MouseEvent* ) {
		editor->getDocument().save();
		editor->disableReportSizeChangeToChildren();
		docAlert->close();
		editor->setFocus();
	} );

	docAlert->find( "file_ignore" )->onClick( [docAlert, editor]( const MouseEvent* ) {
		editor->disableReportSizeChangeToChildren();
		docAlert->close();
		editor->setFocus();
	} );

	docAlert->runOnMainThread(
		[docAlert, editor] {
			editor->disableReportSizeChangeToChildren();
			docAlert->close();
			editor->setFocus();
		},
		Seconds( 30.f ) );
}

void App::createDocDoesNotExistsInFSAlert( UICodeEditor* editor ) {
	UILinearLayout* docAlert = editor->findByClass<UILinearLayout>( "doc_alert" );

	if ( docAlert )
		return;

	const auto msg = R"xml(
	<hbox class="doc_alert" layout_width="wrap_content" layout_height="wrap_content" layout_gravity="top|right" gravity-owner="true">
		<TextView id="doc_alert_text" layout_width="wrap_content" layout_height="wrap_content" margin-right="24dp"
			text='@string(reload_current_file, "The file does not exists anymore on the disk. You are editing a non-existent file.&#xA;Do you want to continue editing the saved buffer?")'
		/>
		<PushButton id="file_continue_editing" layout_width="wrap_content" layout_height="18dp" text='@string("continue_editing", "Continue Editing")' margin-right="4dp"
					tooltip='@string(tooltip_continue_editing_file, "Continue editing the text document buffer.")' />
		<PushButton id="file_close" layout_width="wrap_content" layout_height="18dp" text='@string("close_file", "Close File")'
					tooltip='@string(tooltip_close_unexistent_file, "Closes the edited buffer")' />
	</hbox>
	)xml";
	docAlert = mUISceneNode->loadLayoutFromString( msg, editor )->asType<UILinearLayout>();

	editor->enableReportSizeChangeToChildren();

	docAlert->find( "file_continue_editing" )->onClick( [docAlert, editor]( const MouseEvent* ) {
		editor->disableReportSizeChangeToChildren();
		docAlert->close();
		editor->setFocus();
	} );

	docAlert->find( "file_close" )->onClick( [this, editor, docAlert]( const MouseEvent* ) {
		editor->disableReportSizeChangeToChildren();
		docAlert->close();
		mSplitter->closeTab( editor, UITabWidget::FocusTabBehavior::Default );
	} );

	docAlert->runOnMainThread(
		[docAlert, editor] {
			editor->disableReportSizeChangeToChildren();
			docAlert->close();
			editor->setFocus();
		},
		Seconds( 30.f ) );
}

void App::createDocManyLangsAlert( UICodeEditor* editor ) {
	UILinearLayout* docAlert = editor->findByClass<UILinearLayout>( "doc_alert_manylangs" );

	if ( docAlert )
		return;

	auto ext = editor->getDocument().getFileInfo().getExtension();
	auto langs = SyntaxDefinitionManager::instance()->languagesThatSupportExtension( ext );

	if ( langs.size() <= 1 )
		return;

	const auto msg = R"xml(
	<vbox class="doc_alert doc_alert_manylangs" layout_width="wrap_content" layout_height="wrap_content" layout_gravity="top|right" gravity-owner="true">
		<TextView id="doc_alert_text" layout_width="wrap_content" layout_height="wrap_content" margin-right="24dp"
			text='@string(reload_current_file, "The current document uses an extension that can be interpreted as more than one languages.&#xA;Which language is this document?")'
		/>
		<StackLayout class="languages" layout_width="200dp" layout_height="wrap_content" margin-right="24dp" margin-top="8dp"></StackLayout>
		<TextView font-size="9dp" text='@string(lang_selected_default, The language selected will be set as the default language for this file extension.)' margin-top="8dp" />
	</vbox>
	)xml";
	docAlert = mUISceneNode->loadLayoutFromString( msg, editor )->asType<UILinearLayout>();

	UIStackLayout* stack = docAlert->findByClass<UIStackLayout>( "languages" );

	if ( !stack ) {
		docAlert->close();
		return;
	}

	for ( const auto& lang : langs ) {
		UIPushButton* btn = UIPushButton::New();
		btn->setParent( stack );
		btn->setText( lang->getLanguageName() );
		btn->setLayoutMarginRight( PixelDensity::dpToPx( 8 ) );
		btn->onClick( [this, editor, lang, docAlert, ext]( auto ) {
			editor->setSyntaxDefinition( *lang );
			editor->disableReportSizeChangeToChildren();
			docAlert->close();
			editor->setFocus();
			mConfig.languagesExtensions.priorities[ext] = lang->getLSPName();
			SyntaxDefinitionManager::instance()->setLanguageExtensionsPriority(
				mConfig.languagesExtensions.priorities );
		} );
	}

	editor->enableReportSizeChangeToChildren();

	docAlert->runOnMainThread(
		[docAlert, editor] {
			editor->disableReportSizeChangeToChildren();
			docAlert->close();
			editor->setFocus();
		},
		Seconds( 30.f ) );
}

void App::loadImageFromMedium( const std::string& path, bool isMemory ) {
	UIImage* imageView = mImageLayout->findByType<UIImage>( UI_TYPE_IMAGE );
	UILoader* loaderView = mImageLayout->findByType<UILoader>( UI_TYPE_LOADER );
	if ( imageView ) {
		mImageLayout->setEnabled( true )->setVisible( true );
		loaderView->setVisible( true );
		mThreadPool->run( [this, imageView, loaderView, path, isMemory]() {
			Image::Format format =
				isMemory ? Image::getFormat( reinterpret_cast<const unsigned char*>( path.c_str() ),
											 path.size() )
						 : Image::getFormat( path );

			if ( format == Image::Format::Unknown )
				return;

			Drawable* image = nullptr;

			if ( format != Image::Format::GIF ) {
				image = isMemory ? TextureFactory::instance()->loadFromMemory(
									   reinterpret_cast<const unsigned char*>( path.c_str() ),
									   path.size() )
								 : TextureFactory::instance()->loadFromFile( path );
			} else {
				IOStream* stream = isMemory
									   ? (IOStream*)new IOStreamMemory( path.c_str(), path.size() )
									   : (IOStream*)new IOStreamFile( path );
				Sprite* sprite = Sprite::fromGif( *stream );
				sprite->setAutoAnimate( false );
				image = sprite;
				delete stream;
			}

			if ( mImageLayout->isVisible() ) {
				imageView->runOnMainThread( [this, imageView, loaderView, image]() {
					mImageLayout->setFocus();
					imageView->setDrawable( image, true );
					loaderView->setVisible( false );
				} );
			} else {
				eeSAFE_DELETE( image );
				imageView->setDrawable( nullptr );
				loaderView->setVisible( false );
			}
		} );
	}
}

void App::loadImageFromMemory( const std::string& content ) {
	loadImageFromMedium( content, true );
}

void App::loadImageFromPath( const std::string& path ) {
	loadImageFromMedium( path, false );
}

void App::openFileFromPath( const std::string& path ) {
	std::string ext = FileSystem::fileExtension( path );
	if ( !Image::isImageExtension( path ) && !PathHelper::isOpenExternalExtension( ext ) &&
		 TextDocument::fileMightBeBinary( path ) ) {
		auto msgBox = UIMessageBox::New(
			UIMessageBox::YES_NO,
			i18n( "open_binary_file_warning",
				  "File looks like a binary file. Are you sure "
				  "you want to open it as a document?\nOtherwise it will try to open the "
				  "file with an external application." ) );
		msgBox->getButtonOK()->setText( i18n( "open_as_document", "Open as document" ) );
		msgBox->getButtonOK()->getIcon()->setVisible( false );
		msgBox->getButtonCancel()->setText( i18n( "open_externally", "Open externally" ) );
		msgBox->getButtonCancel()->getIcon()->setVisible( false );
		msgBox->on( Event::OnConfirm, [this, path]( auto ) { loadFileFromPath( path ); } );
		msgBox->on( Event::OnCancel, [path]( auto ) {
			FileInfo f( path );
			if ( f.isExecutable() )
				Sys::execute( path );
			else
				Engine::instance()->openURI( path );
		} );
		msgBox->showWhenReady();
	} else {
		loadFileFromPath( path );
	}
}

bool App::loadFileFromPath(
	std::string path, bool inNewTab, UICodeEditor* codeEditor,
	std::function<void( UICodeEditor* codeEditor, const std::string& path )> onLoaded,
	bool openBinaryAsDocument ) {
	std::string ext = FileSystem::fileExtension( path );

	if ( ext == "lnk" ) {
		auto target = Sys::getShortcutTarget( path );
		if ( !target.empty() ) {
			if ( FileSystem::fileExists( target ) )
				path = target;
			else
				return false;
		} else if ( !FileSystem::fileExists( path ) )
			return false;
	}

	if ( Image::isImageExtension( path ) && Image::isImage( path ) && ext != "svg" ) {
		loadImageFromPath( path );
	} else if ( !openBinaryAsDocument && PathHelper::isOpenExternalExtension( ext ) ) {
		Engine::instance()->openURI( path );
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
				mSplitter->loadAsyncFileFromPathInNewTab( path, onLoaded );
			} else {
				mSplitter->loadAsyncFileFromPath( path, codeEditor, onLoaded );
			}
		}
	}

	return true;
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

void App::hideStatusAppOutput() {
	mStatusAppOutputController->hide();
}

StatusBuildOutputController* App::getStatusBuildOutputController() const {
	return mStatusBuildOutputController.get();
}

StatusAppOutputController* App::getStatusAppOutputController() const {
	return mStatusAppOutputController.get();
}

bool App::isDirTreeReady() const {
	return mDirTreeReady && mDirTree != nullptr;
}

NotificationCenter* App::getNotificationCenter() const {
	return mNotificationCenter.get();
}

void App::fullscreenToggle() {
	mWindow->toggleFullscreen();
	mSettings->updateViewMenu();
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
	editor->setFontSize( config.fontSize.asPixels( 0, Sizef(), mUISceneNode->getDPI() ) );
	editor->setEnableColorPickerOnSelection( true );
	editor->setDisplayLockedIcon( true );
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
	editor->setLineWrapKeepIndentation( config.wrapKeepIndentation );
	editor->setLineWrapMode( config.wrapMode );
	editor->setLineWrapType( config.wrapType );
	editor->setFoldDrawable( findIcon( "chevron-down", PixelDensity::dpToPxI( 12 ) ) );
	editor->setFoldedDrawable( findIcon( "chevron-right", PixelDensity::dpToPxI( 12 ) ) );
	editor->setTabStops( mConfig.doc.tabStops );

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

	doc.getFoldRangeService().setEnabled( config.codeFoldingEnabled );
	editor->setFoldsAlwaysVisible( config.codeFoldingAlwaysVisible );
	editor->setFoldsRefreshTime( config.codeFoldingRefreshFreq );

	if ( !config.tabIndentCharacter.empty() ) {
		String indentChar( config.tabIndentCharacter );
		if ( indentChar.size() == 1 )
			editor->setTabIndentCharacter( indentChar[0] );
	}
	editor->setTabIndentAlignment( config.tabIndentAlignment );

	editor->addKeyBinds( getLocalKeybindings() );
	editor->addUnlockedCommands( getUnlockedCommands() );
	doc.setCommand( "save-doc", [this] { saveDoc(); } );
	doc.setCommand( "save-as-doc", [this] {
		if ( mSplitter->curEditorExistsAndFocused() )
			saveFileDialog( mSplitter->getCurEditor() );
	} );
	doc.setCommand( "save-all", [this] { saveAll(); } );
	doc.setCommand( "repeat-find", [this] {
		mDocSearchController->findNextText( mDocSearchController->getSearchState() );
	} );
	doc.setCommand( "find-prev", [this] {
		mDocSearchController->findPrevText( mDocSearchController->getSearchState() );
	} );
	doc.setCommand( "close-folder", [this] { closeFolder(); } );
	doc.setCommand( "lock", [this] {
		if ( mSplitter->curEditorExistsAndFocused() ) {
			mSplitter->getCurEditor()->setLocked( true );
			mSettings->updateDocumentMenu();
		}
	} );
	doc.setCommand( "unlock", [this] {
		if ( mSplitter->curEditorExistsAndFocused() ) {
			mSplitter->getCurEditor()->setLocked( false );
			mSettings->updateDocumentMenu();
		}
	} );
	doc.setCommand( "lock-toggle", [this] {
		if ( mSplitter->curEditorExistsAndFocused() ) {
			mSplitter->getCurEditor()->setLocked( !mSplitter->getCurEditor()->isLocked() );
			mSettings->updateDocumentMenu();
		}
	} );
	doc.setCommand( "go-to-line", [this] { mUniversalLocator->goToLine(); } );
	doc.setCommand( "load-current-dir", [this] { loadCurrentDirectory(); } );
	doc.setCommand( "open-in-new-window", [this] {
		auto editor = mSplitter->getCurEditor();
		if ( editor == nullptr || editor->getDocumentRef() == nullptr ||
			 editor->getDocumentRef()->getFilePath().empty() )
			return;
		std::string processPath = Sys::getProcessFilePath();
		if ( !processPath.empty() ) {
			auto cmd( processPath + " -x \"" + editor->getDocumentRef()->getFilePath() + "\"" );
			Sys::execute( cmd );
		}
	} );
	doc.setCommand( "move-to-new-window", [this] {
		auto editor = mSplitter->getCurEditor();
		if ( editor == nullptr || editor->getDocumentRef() == nullptr ||
			 editor->getDocumentRef()->getFilePath().empty() )
			return;
		UITabWidget* tabWidget = mSplitter->tabWidgetFromEditor( editor );
		if ( tabWidget )
			tabWidget->removeTab( (UITab*)editor->getData() );
		std::string processPath = Sys::getProcessFilePath();
		if ( !processPath.empty() ) {
			auto cmd( processPath + " -x \"" + editor->getDocumentRef()->getFilePath() + "\"" );
			Sys::execute( cmd );
		}
	} );
	doc.setCommand( "move-tab-to-start", [this] {
		auto widget = mSplitter->getCurWidget();
		if ( widget == nullptr || widget->getData() == 0 )
			return;
		UITabWidget* tabWidget = mSplitter->tabWidgetFromWidget( widget );
		if ( tabWidget ) {
			UITab* tab = (UITab*)widget->getData();
			tabWidget->moveTab( tab, 0 );
		}
	} );
	doc.setCommand( "move-tab-to-end", [this] {
		auto widget = mSplitter->getCurWidget();
		if ( widget == nullptr || widget->getData() == 0 )
			return;
		UITabWidget* tabWidget = mSplitter->tabWidgetFromWidget( widget );
		if ( tabWidget ) {
			UITab* tab = (UITab*)widget->getData();
			tabWidget->moveTab( tab, tabWidget->getTabCount() );
		}
	} );
	registerUnlockedCommands( doc );

	editor->on( Event::OnDocumentSave, [this]( const Event* event ) {
		UICodeEditor* editor = event->getNode()->asType<UICodeEditor>();
		updateEditorTabTitle( editor );
		if ( mSplitter->curEditorExistsAndFocused() && mSplitter->getCurEditor() == editor )
			editor->setFocus();
		if ( editor->getDocument().getFilePath() == mKeybindingsPath ) {
			reloadKeybindings();
		} else if ( mFileSystemMatcher && mFileSystemMatcher->getIgnoreFilePath() ==
											  editor->getDocument().getFilePath() ) {
			loadFileSystemMatcher( mFileSystemMatcher ? mFileSystemMatcher->getPath()
													  : mCurrentProject );
			refreshFolderView();
		}
		if ( !editor->getDocument().hasSyntaxDefinition() ) {
			editor->setSyntaxDefinition( editor->getDocument().guessSyntax() );
		}
		if ( editor->getData() ) {
			UITab* tab = (UITab*)editor->getData();
			tab->removeClass( "tab_file_deleted" );
		}
	} );

	editor->on( Event::OnDocumentDirtyOnFileSystem, [this, editor]( const Event* event ) {
		const DocEvent* docEvent = static_cast<const DocEvent*>( event );
		FileInfo file( docEvent->getDoc()->getFileInfo().getFilepath() );
		TextDocument* doc = docEvent->getDoc();
		if ( FileSystem::fileExists( doc->getFileInfo().getFilepath() ) ) { // File modified!
			if ( doc->getFileInfo() != file ) {
				if ( !mConfig.editor.autoReloadOnDiskChange || doc->isDirty() ) {
					editor->runOnMainThread( [this, editor]() { createDocDirtyAlert( editor ); } );
				} else {
					auto hash = String::hash( "OnDocumentDirtyOnFileSystem-" +
											  docEvent->getDoc()->getFilePath() );
					editor->removeActionsByTag( hash );
					editor->runOnMainThread(
						[doc, this]() {
							auto docRef = mSplitter->getTextDocumentRef( doc );
							if ( docRef )
								docRef->reload();
						},
						Seconds( 0.5f ), hash );
				}
			}
		} else { // File deleted!
			if ( editor->getData() ) {
				UITab* tab = (UITab*)editor->getData();
				tab->ensureMainThread( [this, tab, doc] {
					tab->addClass( "tab_file_deleted" );
					auto docRef = mSplitter->getTextDocumentRef( doc );
					if ( docRef )
						docRef->setDirtyUntilSave();
				} );
			}
		}
	} );

	if ( !mKeybindings.empty() ) {
		editor->getKeyBindings().reset();
		editor->getKeyBindings().addKeybindsStringUnordered( mKeybindings );
	}

	if ( !mMousebindings.empty() ) {
		editor->getMouseBindings().reset();
		editor->getMouseBindings().addMousebindsString( mMousebindings );
	}

	editor->on( Event::OnClose, [this, editor]( auto ) {
		if ( SceneManager::existsSingleton() && !SceneManager::instance()->isShuttingDown() &&
			 editor->hasClass( NOT_UNIQUE_FILENAME ) )
			updateNonUniqueTabTitles();
	} );

	editor->on( Event::OnDocumentClosed, [this]( const Event* event ) {
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
			if ( !mDirTree || !mDirTree->isDirInTree( dir ) )
				mFileWatcher->removeWatch( itWatch->second );
			mFilesFolderWatches.erase( itWatch );
		}
	} );

	editor->on( Event::OnDocumentMoved, [this]( const Event* event ) {
		if ( !appInstance )
			return;
		UICodeEditor* editor = event->getNode()->asType<UICodeEditor>();
		editor->runOnMainThread( [this, editor] {
			updateEditorTabTitle( editor );

			UITab* tab = reinterpret_cast<UITab*>( editor->getData() );
			tab->setTooltipText(
				editor->getDocument().hasFilepath() ? editor->getDocument().getFilePath() : "" );

			editor->setSyntaxDefinition( editor->getDocument().guessSyntax() );
		} );
	} );

	auto docChanged = [this]( const Event* event ) {
		if ( !Engine::isMainThread() )
			return;
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
		editor->getDocument().setHExtLanguageType( mProjectDocConfig.hExtLanguageType );

		auto ext = editor->getDocument().getFileInfo().getExtension();
		if ( SyntaxDefinitionManager::instance()->extensionCanRepresentManyLanguages( ext ) ) {
			auto hasConfig = mConfig.languagesExtensions.priorities.find( ext );
			const SyntaxDefinition* def = nullptr;
			if ( hasConfig != mConfig.languagesExtensions.priorities.end() &&
				 ( def = SyntaxDefinitionManager::instance()->getPtrByLSPName(
					   hasConfig->second ) ) ) {
				editor->setSyntaxDefinition( *def );
			} else {
				createDocManyLangsAlert( editor );
			}
		}
	};

	auto docLoaded = [this, editor, docChanged]( const Event* event ) {
		if ( editor->getDocument().getFileInfo().getExtension() == "svg" ) {
			editor->getDocument().setCommand(
				"show-image-preview", [this]( TextDocument::Client* client ) {
					loadImageFromMemory(
						static_cast<UICodeEditor*>( client )->getDocument().getText().toUtf8() );
				} );
			editor->on( Event::OnCreateContextMenu, [this]( const Event* event ) {
				auto cevent = static_cast<const ContextMenuEvent*>( event );
				cevent->getMenu()
					->add( i18n( "show_image_preview", "Show Image Preview" ),
						   findIcon( "filetype-jpg" ) )
					->setId( "show-image-preview" );
			} );
		}

		docChanged( event );
	};

	editor->on( Event::OnDocumentLoaded, docLoaded );
	editor->on( Event::OnDocumentChanged, docChanged );
	editor->on( Event::OnDocumentSave, docChanged );
	editor->on( Event::OnEditorTabReady, docChanged );
	editor->on( Event::OnDocumentReloaded, [editor]( const Event* event ) {
		if ( editor->getData() ) {
			UITab* tab = (UITab*)editor->getData();
			tab->removeClass( "tab_file_deleted" );
		}
		editor->getDocument().resetUndoRedo();
	} );

	editor->on( Event::OnCursorPosChangeInteresting, [this, editor]( auto ) {
		mSplitter->addEditorPositionToNavigationHistory( editor );
	} );

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
	std::unordered_map<std::string, efsw::WatchID> folderWatches;
	std::unordered_map<std::string, efsw::WatchID> filesFolderWatches;

	if ( !mFileWatcher )
		return;

	{
		Lock l( mWatchesLock );
		folderWatches = mFolderWatches;
		filesFolderWatches = mFilesFolderWatches;
		mFolderWatches.clear();
		mFilesFolderWatches.clear();
	}

	for ( const auto& dir : folderWatches )
		mFileWatcher->removeWatch( dir.first );

	for ( const auto& fileFolder : filesFolderWatches )
		mFileWatcher->removeWatch( fileFolder.second );
}

void App::loadDirTree( const std::string& path ) {
	Clock clock;
	mDirTreeReady = false;
	mDirTree = std::make_shared<ProjectDirectoryTree>(
		path, mThreadPool, mPluginManager.get(),
		[this]( auto path ) { loadFileFromPathOrFocus( path ); } );
	Log::info( "Loading DirTree: %s", path );
	std::vector<std::string> supportedExts(
		SyntaxDefinitionManager::instance()->getExtensionsPatternsSupported() );
	auto imgExts( Image::getImageExtensionsSupported() );
	supportedExts.reserve( supportedExts.size() + imgExts.size() );
	for ( auto& ext : imgExts ) {
		ext.insert( 0, "%." );
		ext += "$";
		supportedExts.push_back( ext );
	}
	mDirTree->scan(
		[this, clock]( ProjectDirectoryTree& dirTree ) {
			Log::info( "DirTree read in: %s. Found %ld files.", clock.getElapsedTime().toString(),
					   dirTree.getFilesCount() );
			mDirTreeReady = true;
			mUISceneNode->runOnMainThread( [this] {
				mUniversalLocator->updateFilesTable();
				if ( mSplitter->curEditorExistsAndFocused() )
					syncProjectTreeWithEditor( mSplitter->getCurEditor() );
			} );
			removeFolderWatches();
			if ( mFileWatcher ) {
				{
					Lock l( mWatchesLock );
					mFolderWatches.insert( { dirTree.getPath(), 0 } );
				}
				mFolderWatches[dirTree.getPath()] =
					mFileWatcher->addWatch( dirTree.getPath(), mFileSystemListener, true );
			}
			mFileSystemListener->setDirTree( mDirTree );
		},
		supportedExts );
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

void App::showFolderTreeViewTab() {
	UITab* tab = mSidePanel->find<UITab>( "treeview_tab" );
	if ( tab )
		tab->setTabSelected();
}

void App::showBuildTab() {
	UITab* tab = mSidePanel->find<UITab>( "build_tab" );
	if ( tab )
		tab->setTabSelected();
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
	msgBox->on( Event::OnConfirm, [this, file, msgBox]( const Event* ) {
		auto newFilePath( getNewFilePath( file, msgBox, false ) );
		if ( !FileSystem::fileExists( newFilePath ) ||
			 file.getFileName() != msgBox->getTextInput()->getText() ) {
			try {
				std::string fpath( file.getFilepath() );
				if ( file.isDirectory() )
					FileSystem::dirRemoveSlashAtEnd( fpath );
				fsRenameFile( fpath, newFilePath );
			} catch ( const fs::filesystem_error& ) {
				errorMsgBox( i18n( "error_renaming_file", "Error renaming file." ) );
			}
			msgBox->closeWindow();
		} else {
			fileAlreadyExistsMsgBox();
		}
	} );
}

void App::openAllFilesInFolder( const FileInfo& folder ) {
	auto files =
		FileSystem::filesInfoGetInPath( folder.getDirectoryPath(), true, false, true,
										getFileSystemModel()->getDisplayConfig().ignoreHidden );

	std::vector<std::string> supportedExts(
		SyntaxDefinitionManager::instance()->getExtensionsPatternsSupported() );
	std::vector<LuaPatternStorage> acceptedPatterns;
	acceptedPatterns.reserve( supportedExts.size() );
	for ( const auto& strPattern : supportedExts )
		acceptedPatterns.emplace_back( std::string{ strPattern } );

	for ( const auto& file : files ) {
		if ( file.isRegularFile() ) {
			bool foundPattern = acceptedPatterns.empty();
			for ( auto& pattern : acceptedPatterns ) {
				if ( pattern.matches( file.getFilepath() ) ) {
					foundPattern = true;
					break;
				}
			}
			if ( foundPattern )
				loadFileFromPath( file.getFilepath() );
		}
	}
}

void App::toggleHiddenFiles() {
	mFileSystemModel = FileSystemModel::New( mFileSystemModel->getRootPath(),
											 FileSystemModel::Mode::FilesAndDirectories,
											 { true,
											   true,
											   !mFileSystemModel->getDisplayConfig().ignoreHidden,
											   {},
											   [this]( const std::string& filePath ) -> bool {
												   return isFileVisibleInTreeView( filePath );
											   } },
											 &mUISceneNode->getTranslator(), mThreadPool );
	if ( mProjectTreeView )
		mProjectTreeView->setModel( mFileSystemModel );
	if ( mFileSystemListener )
		mFileSystemListener->setFileSystemModel( mFileSystemModel );
}

void App::newFile( const FileInfo& file ) {
	UIMessageBox* msgBox = newInputMsgBox( i18n( "create_new_file", "Create new file" ),
										   i18n( "enter_new_file_name", "Enter new file name:" ) );
	msgBox->on( Event::OnConfirm, [this, file, msgBox]( const Event* ) {
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
					[this, newFilePath] {
						if ( !mFileSystemModel || !mProjectTreeView )
							return;
						loadFileFromPathOrFocus( newFilePath );
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
	msgBox->on( Event::OnConfirm, [this, file, msgBox]( const Event* ) {
		auto newFolderPath( getNewFilePath( file, msgBox ) );
		if ( !FileSystem::fileExists( newFolderPath ) ) {
			if ( !FileSystem::makeDir( newFolderPath ) ) {
				errorMsgBox( i18n( "couldnt_create_directory", "Couldn't create directory." ) );
			} else if ( mProjectTreeView ) {
				// We wait 100 ms to get the notification from the file system
				mUISceneNode->runOnMainThread(
					[this, newFolderPath] {
						if ( !mFileSystemModel || !mProjectTreeView )
							return;
						std::string nfp( newFolderPath );
						FileSystem::filePathRemoveBasePath( mFileSystemModel->getRootPath(), nfp );
						mProjectTreeView->openRowWithPath( nfp );
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
	menu->on( Event::OnItemClicked, [this]( const Event* event ) {
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
	menu->on( Event::OnItemClicked, [this]( const Event* event ) {
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

std::function<void( UICodeEditor* codeEditor, const std::string& path )>
App::getForcePositionFn( TextPosition initialPosition ) {
	std::function<void( UICodeEditor * codeEditor, const std::string& path )> forcePosition;
	if ( initialPosition.isValid() ) {
		forcePosition = [this, initialPosition]( UICodeEditor* editor, const auto& ) {
			editor->runOnMainThread( [this, initialPosition, editor] {
				editor->goToLine( initialPosition );
				mSplitter->addEditorPositionToNavigationHistory( editor );
			} );
		};
	}
	return forcePosition;
}

void App::initProjectTreeViewUI() {
	mProjectViewEmptyCont = mUISceneNode->find<UILinearLayout>( "project_view_empty" );
	mProjectViewEmptyCont->find<UIPushButton>( "open_folder" )
		->on( Event::MouseClick, [this]( const Event* event ) {
			if ( event->asMouseEvent()->getFlags() & EE_BUTTON_LMASK )
				runCommand( "open-folder" );
		} );
	mProjectViewEmptyCont->find<UIPushButton>( "open_recent_folder" )
		->on( Event::MouseClick, [this]( const Event* event ) {
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
	mProjectTreeView->setSingleClickNavigation( mConfig.editor.singleClickNavigation );
	mProjectTreeView->setScrollViewType( UIScrollableWidget::Inclusive );
	mProjectTreeView->on( Event::OnModelEvent, [this]( const Event* event ) {
		const ModelEvent* modelEvent = static_cast<const ModelEvent*>( event );
		ModelEventType type = modelEvent->getModelEventType();
		if ( type == ModelEventType::Open || type == ModelEventType::OpenMenu ) {
			Variant vPath(
				modelEvent->getModel()->data( modelEvent->getModelIndex(), ModelRole::Custom ) );
			if ( vPath.isValid() && vPath.isString() ) {
				std::string path( vPath.toString() );
				if ( type == ModelEventType::Open ) {
					UITab* tab = mSplitter->isDocumentOpen( path );
					if ( !tab ) {
						FileInfo fileInfo( path );
						if ( fileInfo.exists() && fileInfo.isRegularFile() )
							openFileFromPath( path );
					} else {
						tab->getTabWidget()->setTabSelected( tab );
					}
				} else { // ModelEventType::OpenMenu
					mSettings->createProjectTreeMenu( FileInfo( path ) );
				}
			}
		}
	} );
	mProjectTreeView->on( Event::MouseClick, [this]( const Event* event ) {
		const MouseEvent* mouseEvent = static_cast<const MouseEvent*>( event );
		if ( mouseEvent->getFlags() & EE_BUTTON_RMASK )
			mSettings->createProjectTreeMenu();
	} );
	mProjectTreeView->on( Event::KeyDown, [this]( const Event* event ) {
		if ( !mFileSystemModel )
			return;
		const KeyEvent* keyEvent = static_cast<const KeyEvent*>( event );
		if ( keyEvent->getKeyCode() == KEY_F2 || keyEvent->getKeyCode() == KEY_DELETE ) {
			ModelIndex modelIndex = mProjectTreeView->getSelection().first();
			if ( !modelIndex.isValid() )
				return;
			Variant vPath( mProjectTreeView->getModel()->data( modelIndex, ModelRole::Custom ) );
			if ( vPath.isValid() && vPath.isString() ) {
				FileInfo fileInfo( vPath.toString() );
				if ( keyEvent->getKeyCode() == KEY_F2 ) {
					renameFile( fileInfo );
				} else {
					mSettings->deleteFileDialog( fileInfo );
				}
			}
		}
	} );
	mProjectTreeView->setDisableCellClipping( true );
	mProjectTreeView->setAutoExpandOnSingleColumn( true );
}

void App::initProjectTreeView( std::string path, bool openClean ) {
	initProjectTreeViewUI();

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

				mFileSystemModel =
					FileSystemModel::New( folderPath, FileSystemModel::Mode::FilesAndDirectories,
										  { true,
											true,
											true,
											{},
											[this]( const std::string& filePath ) -> bool {
												return isFileVisibleInTreeView( filePath );
											} },
										  &mUISceneNode->getTranslator(), mThreadPool );

				mProjectTreeView->setModel( mFileSystemModel );
				mProjectViewEmptyCont->setVisible( false );

				if ( mFileSystemListener )
					mFileSystemListener->setFileSystemModel( mFileSystemModel );

				auto forcePosition = getForcePositionFn( initialPosition );
				auto onLoaded = [this, forcePosition]( UICodeEditor* codeEditor,
													   const std::string& path ) {
					if ( forcePosition )
						forcePosition( codeEditor, path );
					syncProjectTreeWithEditor( mSplitter->getCurEditor() );
				};

				if ( FileSystem::fileExists( rpath ) ) {
					loadFileFromPath( rpath, false, nullptr, onLoaded );
				} else if ( FileSystem::fileCanWrite( path ) ) {
					loadFileFromPath( path, false, nullptr, onLoaded );
				}

				mSettings->updateProjectSettingsMenu();
			}
		}
	} else if ( mConfig.workspace.restoreLastSession && !mRecentFolders.empty() && !openClean ) {
		loadFolder( mRecentFolders[0] );
	} else {
		if ( mConfig.workspace.sessionSnapshot && !openClean )
			loadFolder( getPlaygroundPath() );

		updateOpenRecentFolderBtn();

		if ( getConfig().ui.welcomeScreen && mSplitter->allEditorsEmpty() ) {
			createWelcomeTab();
			mStatusBar->setVisible( false );
		}
	}
}

void App::initImageView() {
	mImageLayout->on( Event::MouseClick, [this]( const Event* ) {
		mImageLayout->findByType<UIImage>( UI_TYPE_IMAGE )->setDrawable( nullptr );
		mImageLayout->setEnabled( false )->setVisible( false );
		if ( mSplitter->getCurWidget() )
			mSplitter->getCurWidget()->setFocus();
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
	if ( !appInstance || mDestroyingApp || !mFileSystemMatcher ||
		 !mFileSystemMatcher->matcherReady() )
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

void App::checkLanguagesHealth() {
	FeaturesHealth::displayHealth( mPluginManager.get(), mUISceneNode );
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

void App::saveSidePanelTabsOrder() {
	mConfig.windowState.sidePanelTabsOrder.clear();
	mConfig.windowState.sidePanelTabsOrder.reserve( mSidePanel->getTabCount() );
	for ( Uint32 i = 0; i < mSidePanel->getTabCount(); i++ )
		mConfig.windowState.sidePanelTabsOrder.emplace_back( mSidePanel->getTab( i )->getId() );
}

void App::loadFolder( std::string path ) {
	if ( !FileSystem::fileExists( path ) || !FileSystem::isDirectory( path ) ) {
		auto msgBox = UIMessageBox::New(
			UIMessageBox::OK, i18n( "directory_does_not_exist",
									"The directory does not exists and cannot be opened" ) );
		msgBox->setTitle( i18n( "invalid_directory", "Invalid Directory" ) );
		msgBox->center();
		msgBox->showWhenReady();
		updateRecentFolders();
		return;
	}

	Clock dirTreeClock;

	if ( FileSystem::fileExtension( path ) == "lnk" ) {
		auto target = Sys::getShortcutTarget( path );
		if ( !target.empty() ) {
			if ( FileSystem::fileExists( target ) )
				path = target;
			else
				return;
		} else if ( !FileSystem::fileExists( path ) )
			return;
	}

	bool projectWasLoaded = !mCurrentProject.empty();
	if ( projectWasLoaded ) {
		saveSidePanelTabsOrder();

		closeEditors();
	} else {
		mSplitter->removeTabWithOwnedWidgetId( "welcome_ecode" );
		mStatusBar->setVisible( mConfig.ui.showStatusBar );
	}

	if ( !mProjectTreeView ) {
		showSidePanel( mConfig.ui.showSidePanel );
		showStatusBar( mConfig.ui.showStatusBar );
		initProjectTreeViewUI();
	}

	mProjectViewEmptyCont->setVisible( path == getPlaygroundPath() );
	mProjectTreeView->setVisible( !mProjectViewEmptyCont->isVisible() );

	std::string rpath( FileSystem::getRealPath( path ) );
	FileSystem::dirAddSlashAtEnd( rpath );

	mCurrentProject = rpath;
	mCurrentProjectName = FileSystem::fileNameFromPath( mCurrentProject );

	loadDirTree( rpath );

	Clock projClock;
	if ( mProjectBuildManager )
		mProjectBuildManager.reset();

	mProjectBuildManager =
		std::make_unique<ProjectBuildManager>( rpath, mThreadPool, mSidePanel, this );
	mConfig.loadProject( rpath, mSplitter, mConfigPath, mProjectDocConfig, this,
						 mConfig.workspace.sessionSnapshot, mPluginManager.get() );
	Log::info( "Load project took: %.2f ms", projClock.getElapsedTime().asMilliseconds() );

	loadFileSystemMatcher( rpath );

	mFileSystemModel = FileSystemModel::New( rpath, FileSystemModel::Mode::FilesAndDirectories,
											 { true,
											   true,
											   true,
											   {},
											   [this]( const std::string& filePath ) -> bool {
												   return isFileVisibleInTreeView( filePath );
											   } },
											 &mUISceneNode->getTranslator(), mThreadPool );

	if ( mProjectTreeView )
		mProjectTreeView->setModel( mFileSystemModel );

	if ( mFileSystemListener )
		mFileSystemListener->setFileSystemModel( mFileSystemModel );

	if ( rpath != getPlaygroundPath() ) {
		insertRecentFolder( rpath );
		cleanUpRecentFolders();
		updateRecentFolders();
	}
	mSettings->updateProjectSettingsMenu();

	if ( mSplitter->getCurWidget() )
		mSplitter->getCurWidget()->setFocus();

	if ( mSplitter->getCurEditor() )
		setAppTitle( titleFromEditor( mSplitter->getCurEditor() ) );

	mPluginManager->setWorkspaceFolder( rpath );

	saveProject( true, false );

	if ( projectWasLoaded || !mConfig.windowState.sidePanelTabsOrder.empty() )
		mSidePanel->runOnMainThread( [this] { sortSidePanel(); } );
}

void App::sortSidePanel() {
	mConfig.windowState.sidePanelTabsOrder.erase(
		std::remove_if(
			mConfig.windowState.sidePanelTabsOrder.begin(),
			mConfig.windowState.sidePanelTabsOrder.end(),
			[this]( const std::string& id ) { return mSidePanel->getTabById( id ) == nullptr; } ),
		mConfig.windowState.sidePanelTabsOrder.end() );

	for ( size_t i = 0; i < mConfig.windowState.sidePanelTabsOrder.size(); ++i ) {
		UITab* targetTab = mSidePanel->getTabById( mConfig.windowState.sidePanelTabsOrder[i] );
		UITab* currentTab = mSidePanel->getTab( i );
		if ( targetTab && currentTab != targetTab )
			mSidePanel->swapTabs( currentTab, targetTab );
	}
}

#if EE_PLATFORM == EE_PLATFORM_MACOS || EE_PLATFORM == EE_PLATFORM_LINUX || \
	EE_PLATFORM == EE_PLATFORM_BSD
static std::string getDefaultShell() {
	std::string shell = Sys::getEnv( "SHELL" );
	if ( !shell.empty() )
		return shell;
#if EE_PLATFORM == EE_PLATFORM_WIN
	return "cmd";
#elif EE_PLATFORM == EE_PLATFORM_MACOS
	return "zsh";
#else
	return "bash";
#endif
}

static std::string getShellEnv( const std::string& env, const std::string& defShell = "" ) {
	std::string shell = defShell.empty() ? Sys::which( getDefaultShell() ) : defShell;
	Process process;
	if ( process.create( shell, "-l -c env", Process::getDefaultOptions() ) ) {
		size_t envLen = env.size();
		std::string buf( 32 * 1024, '\0' );
		process.readAllStdOut( buf );
		auto pathPos = buf.find( "\n" + env + "=" );
		bool startsWithPath = false;
		if ( pathPos == std::string::npos && buf.substr( 0, envLen + 1 ) == env + "=" )
			startsWithPath = true;
		if ( pathPos != std::string::npos || startsWithPath ) {
			pathPos += startsWithPath ? envLen + 1 : envLen + 2; // Remove \n + env + "="
			auto endPathPos = buf.find_first_of( "\r\n", pathPos );
			if ( endPathPos != std::string::npos ) {
				size_t len = endPathPos - pathPos;
				return buf.substr( pathPos, len );
			} else {
				return buf.substr( pathPos );
			}
		}
	}
	return "";
}
#endif

FontTrueType* App::loadFont( const std::string& name, std::string fontPath,
							 const std::string& fallback ) {
	bool wasFallback = false;
	if ( FileSystem::isRelativePath( fontPath ) )
		fontPath = mResPath + fontPath;
#if EE_PLATFORM == EE_PLATFORM_ANDROID
	if ( fontPath.empty() ||
		 ( !FileSystem::fileExists( fontPath ) && !PackManager::instance()->exists( fontPath ) ) ) {
#else
	if ( fontPath.empty() || !FileSystem::fileExists( fontPath ) ) {
#endif
		fontPath = fallback;
		wasFallback = true;
		if ( !fontPath.empty() && FileSystem::isRelativePath( fontPath ) )
			fontPath = mResPath + fontPath;
	}
	if ( fontPath.empty() )
		return nullptr;
	FontTrueType* font = FontTrueType::New( name );
	if ( font->loadFromFile( fontPath ) ) {
		font->setHinting( mConfig.ui.fontHinting );
		font->setAntialiasing( mConfig.ui.fontAntialiasing );
		return font;
	}
	eeSAFE_DELETE( font );
	// Failed to load original font? Try to fallback
	if ( !fallback.empty() && !wasFallback ) {
		if ( !fontPath.empty() && FileSystem::isRelativePath( fontPath ) )
			fontPath = mResPath + fontPath;
		font = FontTrueType::New( name );
		if ( font->loadFromFile( fontPath ) ) {
			font->setHinting( mConfig.ui.fontHinting );
			font->setAntialiasing( mConfig.ui.fontAntialiasing );
			return font;
		}
		eeSAFE_DELETE( font );
	}
	return nullptr;
}

bool App::needsRedirectToRunningProcess( std::string file ) {
	if ( mConfig.ui.openFilesInNewWindow || file.empty() )
		return false;

	bool hasPosition = pathHasPosition( file );
	TextPosition position;
	if ( hasPosition ) {
		auto pathAndPosition = getPathAndPosition( file );
		file = pathAndPosition.first;
		position = pathAndPosition.second;
	}

	std::string rpath( FileSystem::getRealPath( file ) );
	FileInfo finfo( rpath );

	if ( !finfo.exists() || finfo.isDirectory() )
		return false;

	std::string processName( FileSystem::fileNameFromPath( Sys::getProcessFilePath() ) );
	auto pids = Sys::pidof( processName );
	if ( pids.size() <= 1 )
		return false;

	Uint64 processPid = Sys::getProcessID();
	Uint64 latestPid = processPid;
	Uint64 lastCreationTime = 0;

	for ( const auto pid : pids ) {
		if ( pid != Sys::getProcessID() ) {
			Uint64 creationTime = Sys::getProcessCreationTime( pid );
			if ( creationTime >= lastCreationTime ) {
				latestPid = pid;
				lastCreationTime = creationTime;
			}
		}
	}

	if ( latestPid == processPid )
		return false;

	std::string pidPath = mIpcPath + String::toString( latestPid );
	if ( !FileSystem::isDirectory( pidPath ) )
		return false;
	FileSystem::dirAddSlashAtEnd( pidPath );
	FileSystem::fileWrite( pidPath + MD5::fromString( finfo.getFilepath() ).toHexString(),
						   finfo.getFilepath() +
							   ( position.isValid() ? position.toPositionString() : "" ) );
	return true;
}

void App::init( InitParameters& params ) {
	Http::setThreadPool( mThreadPool );
	DisplayManager* displayManager = Engine::instance()->getDisplayManager();
	Display* currentDisplay = displayManager->getDisplayIndex( 0 );

	if ( currentDisplay == nullptr ) {
		std::cerr << "Display not found, exiting" << std::endl;
		return;
	}

	mIncognito = params.incognito;
	mPortableMode = params.portable || !params.profile.empty();
	mProfilePath = params.profile;
	mDisplayDPI = currentDisplay->getDPI();
	mUseFrameBuffer = params.frameBuffer;
	mBenchmarkMode = params.benchmarkMode;
	mDisablePlugins = params.disablePlugins;

	mResPath = Sys::getProcessPath();
#if EE_PLATFORM == EE_PLATFORM_LINUX
	if ( String::contains( mResPath, ".mount_" ) )
		FileSystem::dirAddSlashAtEnd( mResPath );
#elif EE_PLATFORM == EE_PLATFORM_EMSCRIPTEN
	mResPath += "ecode/";
#endif
	mResPath += "assets";
	FileSystem::dirAddSlashAtEnd( mResPath );

	bool firstRun = loadConfig( params.logLevel, currentDisplay->getSize(), params.prematureExit,
								params.stdOutLogs, params.disableFileLogs );

	if ( params.prematureExit )
		return;

	if ( !params.openClean && needsRedirectToRunningProcess( params.file ) )
		return;

	currentDisplay = displayManager->getDisplayIndex( mConfig.windowState.displayIndex <
															  displayManager->getDisplayCount()
														  ? mConfig.windowState.displayIndex
														  : 0 );
	mDisplayDPI = currentDisplay->getDPI();

#if EE_PLATFORM == EE_PLATFORM_ANDROID
	mConfig.windowState.pixelDensity =
		params.pidelDensity > 0
			? params.pidelDensity
			: ( mConfig.windowState.pixelDensity > 0	? mConfig.windowState.pixelDensity
				: currentDisplay->getPixelDensity() > 2 ? currentDisplay->getPixelDensity() / 2
														: currentDisplay->getPixelDensity() );
#else
	mConfig.windowState.pixelDensity =
		params.pidelDensity > 0
			? params.pidelDensity
			: ( mConfig.windowState.pixelDensity > 0 ? mConfig.windowState.pixelDensity
													 : currentDisplay->getPixelDensity() );
#endif

	displayManager->enableScreenSaver();
	displayManager->enableMouseFocusClickThrough();
	displayManager->disableBypassCompositor();

	Engine* engine = Engine::instance();

	WindowSettings winSettings = engine->createWindowSettings( &mConfig.iniState, "window" );
	winSettings.Title = "ecode";
	winSettings.PixelDensity = 1;
	winSettings.Width = mConfig.windowState.size.getWidth();
	winSettings.Height = mConfig.windowState.size.getHeight();
	if ( winSettings.Icon.empty() ) {
		winSettings.Icon = mConfig.windowState.winIcon;
		if ( FileSystem::isRelativePath( winSettings.Icon ) )
			winSettings.Icon = mResPath + winSettings.Icon;
	}

#if EE_PLATFORM == EE_PLATFORM_MACOS
	mConfig.context = engine->createContextSettings( &mConfig.ini, "window", true );
#else
	mConfig.context = engine->createContextSettings( &mConfig.ini, "window", false );
#endif

	if ( firstRun )
		mConfig.context.FrameRateLimit = currentDisplay->getRefreshRate();

	mConfig.context.SharedGLContext = true;

	mThreadPool->run( [this] {
		// Load language definitions
		Clock defClock;
		SyntaxDefinitionManager::createSingleton();
		Language::LanguagesSyntaxHighlighting::load();
		SyntaxDefinitionManager::instance()->setLanguageExtensionsPriority(
			mConfig.languagesExtensions.priorities );
		Log::info( "Syntax definitions loaded in %s.",
				   defClock.getElapsedTimeAndReset().toString() );

		// Load user defined languages definitions
		Clock customLangsClock;
		SyntaxDefinitionManager::instance()->loadFromFolder( mLanguagesPath );
		Log::info( "SyntaxDefinitionManager loaded custom languages in: %s.",
				   customLangsClock.getElapsedTime().toString() );

		// Load editor color schemes
		mColorSchemes =
			( SyntaxColorScheme::loadFromFile( mResPath + "colorschemes/colorschemes.conf" ) );
		if ( FileSystem::isDirectory( mColorSchemesPath ) ) {
			auto colorSchemesFiles = FileSystem::filesGetInPath( mColorSchemesPath );
			for ( const auto& curFile : colorSchemesFiles ) {
				auto colorSchemesInFile =
					SyntaxColorScheme::loadFromFile( mColorSchemesPath + curFile );
				for ( auto& colorScheme : colorSchemesInFile )
					mColorSchemes.emplace_back( colorScheme );
			}
		} else {
			FileSystem::makeDir( mColorSchemesPath, true );
		}

		// Load terminal color schemes
		mTerminalManager->loadTerminalColorSchemes();

		// Load fonts
		Clock fontsClock;

		mFont = loadFont( "sans-serif", mConfig.ui.serifFont, "fonts/NotoSans-Regular.ttf" );
		FontFamily::loadFromRegular( mFont );

		mFontMono = loadFont( "monospace", mConfig.ui.monospaceFont, "fonts/DejaVuSansMono.ttf" );
		if ( mFontMono ) {
			mFontMono->setEnableDynamicMonospace( true );
			mFontMono->setBoldAdvanceSameAsRegular( true );
			FontFamily::loadFromRegular( mFontMono );
		}

		loadFont( "NotoEmoji-Regular", "fonts/NotoEmoji-Regular.ttf" );

#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN
		loadFont( "NotoColorEmoji", "fonts/NotoColorEmoji.ttf" );
#endif

		mRemixIconFont = loadFont( "icon", "fonts/remixicon.ttf" );
		mNoniconsFont = loadFont( "nonicons", "fonts/nonicons.ttf" );
		mCodIconFont = loadFont( "codicon", "fonts/codicon.ttf" );

		mTerminalFont = loadFont( "monospace-nerdfont", mConfig.ui.terminalFont,
								  "fonts/DejaVuSansMonoNerdFontComplete.ttf" );

		if ( ( nullptr != mTerminalFont && mTerminalFont->getInfo().family == "DejaVuSansMono NF" &&
			   mFontMono->getInfo().family == "DejaVu Sans Mono" ) ||
			 ( nullptr != mTerminalFont &&
			   mTerminalFont->getInfo().family == mFontMono->getInfo().family ) ) {
			mTerminalFont->setBoldFont( mFontMono->getBoldFont() );
			mTerminalFont->setItalicFont( mFontMono->getItalicFont() );
			mTerminalFont->setBoldItalicFont( mFontMono->getBoldItalicFont() );
		} else {
			FontFamily::loadFromRegular( mTerminalFont );
		}

		mFallbackFont = loadFont( "fallback-font", "fonts/DroidSansFallbackFull.ttf" );
		if ( mFallbackFont )
			FontManager::instance()->addFallbackFont( mFallbackFont );

		if ( mConfig.ui.fallbackFont != "fonts/DroidSansFallbackFull.ttf" ) {
			mUserFallbackFont = loadFont( "fallback-font", mConfig.ui.fallbackFont );
			if ( mUserFallbackFont )
				FontManager::instance()->addFallbackFont( mUserFallbackFont );
		}

		Log::info( "Fonts loaded in: %s", fontsClock.getElapsedTime().toString() );

		mAsyncResourcesLoaded = true;
		mAsyncResourcesLoadCond.notify_all();
	} );

	mWindow = engine->createWindow( winSettings, mConfig.context );
	Log::info( "%s (codename: \"%s\") initializing", ecode::Version::getVersionFullName(),
			   ecode::Version::getCodename() );

	if ( mWindow && mWindow->isOpen() ) {
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
#if EE_PLATFORM == EE_PLATFORM_MACOS || EE_PLATFORM == EE_PLATFORM_LINUX || \
	EE_PLATFORM == EE_PLATFORM_BSD

#if EE_PLATFORM == EE_PLATFORM_MACOS
		macOS_createApplicationMenus();
		macOS_enableScrollMomentum();
		macOS_removeTitleBarSeparator( mWindow->getWindowHandler() );
#endif

		mThreadPool->run( [this]() {
			// Checks if the default shell path contains more paths
			// than the current environment, and adds them to ensure
			// that the environment is more friendly for any new user
			std::string path( Sys::getEnv( "PATH" ) );
			std::string shellPath( getShellEnv( "PATH", mConfig.term.shell ) );

#if EE_PLATFORM == EE_PLATFORM_LINUX || EE_PLATFORM == EE_PLATFORM_BSD
			if ( path == shellPath )
				return;
#endif

			std::vector<std::string> paths;
			auto pathSpl = String::split( path, ':' );

			for ( auto& path : pathSpl )
				paths.emplace_back( std::move( path ) );

			if ( !shellPath.empty() && String::hash( path ) != String::hash( shellPath ) ) {
				auto shellPathSpl = String::split( shellPath, ':' );
				for ( auto& shellPath : shellPathSpl ) {
					if ( std::find( paths.begin(), paths.end(), shellPath ) == paths.end() )
						paths.emplace_back( std::move( shellPath ) );
				}
			}

#if EE_PLATFORM == EE_PLATFORM_MACOS
			// Small hack to also add the xcode binaries path (provides git, lldb-dap and more)
			{
				std::string lldbPath;
				Process p;
				if ( p.create( "xcrun -f lldb" ) ) {
					p.readAllStdOut( lldbPath, Seconds( 5 ) );
					int retCode = -1;
					p.join( &retCode );
					if ( retCode == 0 && !lldbPath.empty() ) {
						String::trimInPlace( lldbPath, '\n' );
						lldbPath = FileSystem::removeLastFolderFromPath( lldbPath );
						if ( std::find( paths.begin(), paths.end(), lldbPath ) == paths.end() )
							paths.emplace_back( lldbPath );
					}
				}
			}
#endif

			if ( paths.size() > pathSpl.size() ) {
				std::string newPath = String::join( paths, ':' );
				setenv( "PATH", newPath.c_str(), 1 );
				Log::info( "New PATH env has been set (inherited+shell PATH env): %s", newPath );
			}
		} );
#endif

		Log::info( "Window creation took: %s", globalClock.getElapsedTime().toString() );

		mWindow->setFrameRateLimit( mConfig.context.FrameRateLimit );

		if ( mConfig.windowState.position != Vector2i( -1, -1 ) &&
			 mConfig.windowState.displayIndex < displayManager->getDisplayCount() ) {
			// 1 px offset to avoid a bug in SDL2 2.28 when maximizing windows
			mWindow->setPosition( mConfig.windowState.position.x +
									  ( mConfig.windowState.maximized ? -1 : 0 ),
								  mConfig.windowState.position.y );
		}

		if ( mWindow->isWindowed() && mWindow->getSize() >= currentDisplay->getSize().asInt() ) {
			auto borderSize( mWindow->getBorderSize() );
			mWindow->setPosition( borderSize.getWidth(), borderSize.getHeight() );
		}

		loadKeybindings();

		PixelDensity::setPixelDensity( mConfig.windowState.pixelDensity );

#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN
		if ( mConfig.windowState.maximized ) {
#if EE_PLATFORM == EE_PLATFORM_LINUX
			mThreadPool->run( [this] { mWindow->maximize(); } );
#elif EE_PLATFORM == EE_PLATFORM_MACOS
			// Do no maximize since the result is not exactly maximized
#else
			mWindow->maximize();
#endif
		}
#endif

		mWindow->setCloseRequestCallback(
			[this]( EE::Window::Window* win ) -> bool { return onCloseRequestCallback( win ); } );

		mWindow->setQuitCallback( [this]( EE::Window::Window* win ) {
			if ( win->isOpen() )
				closeApp();
		} );

		mWindow->getInput()->pushCallback( [this]( InputEvent* event ) {
			switch ( event->Type ) {
				case InputEvent::FileDropped: {
					std::string file( event->file.file );
					if ( FileSystem::fileExtension( file ) == "lnk" ) {
						auto target = Sys::getShortcutTarget( file );
						if ( !target.empty() ) {
							if ( FileSystem::fileExists( target ) )
								file = target;
							else
								return;
						}
					}
					mPathsToLoad.emplace_back( std::move( file ) );
					break;
				}
				case InputEvent::TextDropped: {
					onTextDropped( event->textdrop.text );
					break;
				}
				case InputEvent::EventsSent: {
					if ( mPathsToLoad.empty() )
						break;

					std::size_t dirCount = 0;
					std::size_t lastDirIdx = std::numeric_limits<std::size_t>::max();
					std::size_t idx = 0;
					for ( const auto& file : mPathsToLoad ) {
						if ( FileSystem::isDirectory( file ) ) {
							dirCount++;
							lastDirIdx = idx;
						}
						idx++;
					}

					bool onlyDirectories = dirCount == mPathsToLoad.size();

					// If only directories has been dropped, just load the last directory dropped
					if ( onlyDirectories ) {
						loadFolder( mPathsToLoad[lastDirIdx] );
					} else {
						int mightBeBinaryCount = 0;
						for ( const auto& file : mPathsToLoad ) {
							if ( !FileSystem::isDirectory( file ) &&
								 !Image::isImageExtension( file ) &&
								 ( TextDocument::fileMightBeBinary( file ) ||
								   PathHelper::isOpenExternalExtension(
									   FileSystem::fileExtension( file ) ) ) ) {
								mightBeBinaryCount++;
								break;
							}
						}

						if ( mightBeBinaryCount ) {
							UIMessageBox* msgBox = UIMessageBox::New(
								UIMessageBox::OK_CANCEL,
								i18n( "dropped_binary_files_open_warn",
									  "Some of the files seem to be binary.\nDo you want to "
									  "open them as document? Otherwise they will be ignored "
									  "for loading." ) );
							msgBox->getButtonOK()->setText( i18n( "open", "Open" ) );
							msgBox->getButtonOK()->getIcon()->setVisible( false );
							msgBox->getButtonCancel()->setText( i18n( "ignore", "Ignore" ) );
							msgBox->getButtonCancel()->getIcon()->setVisible( false );
							msgBox->on( Event::OnConfirm,
										[this, pathsToLoad = mPathsToLoad]( const Event* ) {
											for ( const auto& file : pathsToLoad ) {
												if ( !FileSystem::isDirectory( file ) )
													onFileDropped( file, true );
											}
										} );
							msgBox->on( Event::OnCancel,
										[this, pathsToLoad = mPathsToLoad]( const Event* ) {
											for ( const auto& file : pathsToLoad ) {
												if ( !FileSystem::isDirectory( file ) &&
													 !( TextDocument::fileMightBeBinary( file ) ||
														PathHelper::isOpenExternalExtension(
															FileSystem::fileExtension( file ) ) ) )
													onFileDropped( file, true );
											}
										} );
							msgBox->setTitle( i18n( "warning", "Warning" ) );
							msgBox->center();
							msgBox->showWhenReady();
						} else {
							// Load only files even if there are directories
							for ( const auto& file : mPathsToLoad ) {
								if ( !FileSystem::isDirectory( file ) )
									onFileDropped( file, false );
							}
						}
					}

					mPathsToLoad.clear();
					break;
				}
				default:
					break;
			}
		} );

		PixelDensity::setPixelDensity(
			eemax( mWindow->getScale(), mConfig.windowState.pixelDensity ) );

		mUISceneNode = UISceneNode::New();
		mUISceneNode->setThreadPool( mThreadPool );
		mUIColorScheme = mConfig.ui.colorScheme;

		if ( params.language.empty() )
			params.language = mConfig.ui.language;
		if ( !params.language.empty() )
			mUISceneNode->getTranslator().setCurrentLanguage( params.language );
		std::string currentLanguage( mUISceneNode->getTranslator().getCurrentLanguage() );
		mi18nPath = mResPath + "i18n" + FileSystem::getOSSlash();
		std::string langPath( mi18nPath + currentLanguage + ".xml" );
		if ( currentLanguage != "en" && FileSystem::fileExists( langPath ) )
			mUISceneNode->getTranslator().loadFromFile( langPath );

		if ( !params.colorScheme.empty() ) {
			mUIColorScheme = params.colorScheme == "light" ? ColorSchemePreference::Light
														   : ColorSchemePreference::Dark;
		}
		mUISceneNode->setColorSchemePreference( mUIColorScheme );

		if ( !mAsyncResourcesLoaded ) {
			std::unique_lock<std::mutex> syntaxLanguagesLock( mAsyncResourcesLoadMutex );
			mAsyncResourcesLoadCond.wait( syntaxLanguagesLock,
										  [this]() { return mAsyncResourcesLoaded; } );
		}

		if ( !mFont || !mFontMono || !mRemixIconFont || !mNoniconsFont || !mCodIconFont ) {
			printf( "Font not found!" );
			Log::error( "Font not found!" );
			return;
		}

		SceneManager::instance()->add( mUISceneNode );

		setTheme( getThemePath() );

		if ( params.css.empty() )
			params.css = mConfigPath + "style.css";

		if ( FileSystem::fileExists( params.css ) ) {
			CSS::StyleSheetParser parser;
			if ( parser.loadFromFile( params.css ) )
				mUISceneNode->combineStyleSheet( parser.getStyleSheet(), false );
		}

#if EE_PLATFORM == EE_PLATFORM_MACOS
		if ( ( macOS_isDarkModeEnabled() &&
			   mConfig.ui.colorScheme == ColorSchemePreference::Dark ) ||
			 ( !macOS_isDarkModeEnabled() &&
			   mConfig.ui.colorScheme == ColorSchemePreference::Light ) ) {
			auto backVar = mUISceneNode->getStyleSheet()
							   .getStyleFromSelector( ":root", true )
							   ->getVariableByName( "--back" );
			if ( !backVar.isEmpty() ) {
				auto backColor( Color::fromString( backVar.getValue() ) );
				macOS_changeTitleBarColor( mWindow->getWindowHandler(), backColor.r / 255.f,
										   backColor.g / 255.f, backColor.b / 255.f );
			}
		}
#endif

		std::string panelUI( String::format( R"css(
		#panel treeview > treeview::row > treeview::cell > treeview::cell::text,
		#panel treeview > treeview::row > table::cell > table::cell::text {
			font-size: %s;
		}
		)css",
											 mConfig.ui.panelFontSize.toString() ) );
		mUISceneNode->combineStyleSheet( panelUI, false, APP_LAYOUT_STYLE_MARKER );

		const auto baseUI = (
#include "applayout.xml.hpp"
		);

		mMenuIconSize = mConfig.ui.fontSize.asPixels( 0, Sizef(), mDisplayDPI );
		mUISceneNode->getUIIconThemeManager()->setCurrentTheme(
			IconManager::init( "ecode", mRemixIconFont, mNoniconsFont, mCodIconFont ) );

		UIWidgetCreator::registerWidget( "searchbar", UISearchBar::New );
		UIWidgetCreator::registerWidget( "locatebar", UILocateBar::New );
		UIWidgetCreator::registerWidget( "globalsearchbar", UIGlobalSearchBar::New );
		UIWidgetCreator::registerWidget( "mainlayout", UIMainLayout::New );
		UIWidgetCreator::registerWidget( "statusbar", UIStatusBar::New );
		UIWidgetCreator::registerWidget( "rellayce", UIRelativeLayoutCommandExecuter::New );
		UIWidgetCreator::registerWidget( "hboxce", UIHLinearLayoutCommandExecuter::New );
		UIWidgetCreator::registerWidget( "vboxce", UIVLinearLayoutCommandExecuter::New );
		UIWidgetCreator::registerWidget( "treeviewfs", UITreeViewFS::New );

		mUISceneNode->loadLayoutFromString( baseUI, nullptr, APP_LAYOUT_STYLE_MARKER );
		mAppStyleSheet = mUISceneNode->getStyleSheet().getAllWithMarker( APP_LAYOUT_STYLE_MARKER );
		mUISceneNode->bind( "main_layout", mMainLayout );
		mUISceneNode->bind( "code_container", mBaseLayout );
		mUISceneNode->bind( "image_container", mImageLayout );
		mUISceneNode->bind( "doc_info", mDocInfo );
		mUISceneNode->bind( "panel", mSidePanel );
		mUISceneNode->bind( "project_splitter", mProjectSplitter );
		mUISceneNode->bind( "main_menubar", mMenuBar );
		mUISceneNode->on( Event::KeyDown, [this]( const Event* event ) {
			trySendUnlockedCmd( *static_cast<const KeyEvent*>( event ) );
		} );
		if ( params.logLevel == LogLevel::Debug )
			mUISceneNode->setVerbose( true );
		mDocInfo->setVisible( mConfig.editor.showDocInfo );

		mProjectSplitter->setSplitPartition(
			StyleSheetLength( mConfig.windowState.panelPartition ) );
		if ( mConfig.ui.panelPosition == PanelPosition::Right )
			mProjectSplitter->swap();

		if ( !mConfig.ui.showSidePanel )
			showSidePanel( mConfig.ui.showSidePanel );

		mSplitter = UICodeEditorSplitter::New( this, mUISceneNode, mThreadPool, mColorSchemes,
											   mInitColorScheme );
		mSplitter->setHideTabBarOnSingleTab( mConfig.editor.hideTabBarOnSingleTab );
		mSplitter->setHideTabBar( mConfig.editor.hideTabBar );
		mSplitter->setOnTabWidgetCreateCb( [this]( UITabWidget* tabWidget ) {
			tabWidget->getTabBar()->onDoubleClick(
				[this]( const MouseEvent* ) { mSplitter->createEditorInNewTab(); } );
		} );
		mPluginManager->setSplitter( mSplitter );

		Log::info( "Base UI took: %.2f ms", globalClock.getElapsedTime().asMilliseconds() );

		mMainSplitter = mUISceneNode->find<UISplitter>( "main_splitter" );
		mMainSplitter->setSplitPartition(
			StyleSheetLength( mConfig.windowState.statusBarPartition ) );
		mStatusBar = mUISceneNode->find<UIStatusBar>( "status_bar" );
		mPluginManager->setMainSplitter( mMainSplitter );

#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN
		mFileWatcher = new efsw::FileWatcher();
		mFileSystemListener = new FileSystemListener( mSplitter, mFileSystemModel, { mLogsPath } );
		mFileWatcher->addWatch( mPluginsPath, mFileSystemListener );
		mFileWatcher->addWatch( mPidPath, mFileSystemListener );
		mFileWatcher->watch();
		mPluginManager->setFileSystemListener( mFileSystemListener );
		mIpcListenerId =
			mFileSystemListener->addListener( [this]( const FileEvent& fe, const FileInfo& fi ) {
				if ( !( ( fe.type == FileSystemEventType::Add ||
						  fe.type == FileSystemEventType::Modified ) &&
						fe.directory == mPidPath ) )
					return;
				std::string path;
				FileSystem::fileGet( fi.getFilepath(), path );
				String::trimInPlace( path, ' ' );
				String::trimInPlace( path, '\n' );

				bool hasPosition = pathHasPosition( path );
				TextPosition initialPosition;
				if ( hasPosition ) {
					auto pathAndPosition = getPathAndPosition( path );
					path = pathAndPosition.first;
					initialPosition = pathAndPosition.second;
				}

				if ( FileSystem::fileExists( path ) ) {
					mUISceneNode->runOnMainThread( [path, initialPosition, this] {
						loadFileFromPathOrFocus( path, true, nullptr,
												 getForcePositionFn( initialPosition ) );

						if ( !mWindow->hasFocus() ) {
							if ( mWindow->isMinimized() )
								mWindow->restore();
							mWindow->raise();
						}
					} );
				}
				FileSystem::fileRemove( fi.getFilepath() );
			} );
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

		mStatusAppOutputController =
			std::make_unique<StatusAppOutputController>( mMainSplitter, mUISceneNode, this );

		initImageView();

		mStatusBar->setPluginContextProvider( this );

		mSettings = std::make_unique<SettingsMenu>();
		mSettings->createSettingsMenu( this, mMenuBar );

		mSplitter->createEditorWithTabWidget( mBaseLayout );

		mConsole = UIConsole::NewOpt( mFontMono, true, true, 1024 * 10 );
		mConsole->setCommand( "hide", [this]( const auto& ) { consoleToggle(); } );
		mConsole->setQuakeMode( true );
		mConsole->setVisible( false );

		registerUnlockedCommands( *mMainLayout );
		mMainLayout->getKeyBindings().addKeybinds( getRealDefaultKeybindings() );

		Log::instance()->setKeepLog( false );
		Log::info( "Complete UI took: %.2f ms", globalClock.getElapsedTime().asMilliseconds() );

#if EE_PLATFORM == EE_PLATFORM_EMSCRIPTEN
		if ( file == "./this.program" )
			file = "";
#endif

		if ( params.terminal && params.file.empty() && params.fileToOpen.empty() ) {
			showSidePanel( false );
			showStatusBar( false );
			mTerminalManager->createNewTerminal();
		} else {
			initProjectTreeView( params.file, params.openClean );
		}

		mFileToOpen = params.fileToOpen;

		Log::info( "Init ProjectTreeView took: %.2f ms",
				   globalClock.getElapsedTime().asMilliseconds() );

#if EE_PLATFORM == EE_PLATFORM_EMSCRIPTEN
		if ( file.empty() || !mFileToOpen.empty() )
			downloadFileWeb(
				mFileToOpen.empty()
					? "https://raw.githubusercontent.com/SpartanJ/eepp/develop/README.md"
					: mFileToOpen );
#endif

		if ( mConfig.workspace.checkForUpdatesAtStartup )
			mSettingsActions->checkForUpdates( true );

		mUISceneNode->setInterval(
			[this] {
				if ( mWindow && mThreadPool &&
					 mWindow->getInput()->getElapsedSinceLastKeyboardOrMouseEvent().asSeconds() <
						 60.f ) {
					saveProject( true );
#if EE_PLATFORM == EE_PLATFORM_LINUX && defined( __GLIBC__ )
					mThreadPool->run( [] { malloc_trim( 0 ); } );
#endif
				}
			},
			Seconds( 60.f ) );

		mPluginManager->setUIReady();

		mWindow->runMainLoop( &appLoop, mBenchmarkMode ? 0 : mConfig.context.FrameRateLimit );
	}
}

static void exportLanguages( const std::string& path, const std::string& langs,
							 const std::string& langsPath ) {
	Language::LanguagesSyntaxHighlighting::load();
	SyntaxDefinitionManager* sdm = SyntaxDefinitionManager::instance();
	SyntaxDefinitionManager::instance()->loadFromFolder( langsPath );
	std::vector<SyntaxDefinition> defs;

	for ( auto& preDef : sdm->getPreDefinitions() )
		preDef.load();

	if ( !langs.empty() ) {
		if ( langs == "all" ) {
			for ( const auto& def : sdm->getDefinitions() )
				defs.push_back( *def.get() );
		} else {
			auto langss = String::split( langs, ',' );
			for ( const auto& l : langss ) {
				const auto& sd = sdm->getByLSPName( l );

				if ( !sd.getPatterns().empty() ) {
					defs.push_back( sd );
					continue;
				}

				const auto& sd2 = sdm->getByLanguageNameInsensitive( l );

				if ( !sd2.getPatterns().empty() )
					defs.push_back( sd2 );
			}
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
	args::ValueFlag<Float> pixelDensityConf( parser, "pixel-density",
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
		parser, "log-level", "The level of details that the application will emit logs.",
		{ 'l', "log-level" }, Log::getMapFlag(), Log::getDefaultLogLevel() );
	args::Flag fb( parser, "framebuffer", "Use frame buffer (more memory usage, less CPU usage)",
				   { "fb", "framebuffer" } );
	args::Flag benchmarkMode( parser, "benchmark-mode",
							  "Render as much as possible to measure the rendering performance.",
							  { "benchmark-mode" } );
	args::Flag verbose( parser, "verbose", "Redirects all logs to stdout.", { 'v', "verbose" } );
	args::Flag version( parser, "version", "Prints version information", { 'V', "version" } );
	args::Flag portable(
		parser, "portable",
		"Portable Mode (it will save the configuration files within the ecode main folder)",
		{ 'p', "portable" } );
	args::Flag incognito(
		parser, "incognito",
		"It will stop keeping track of the opened files or folders during the session",
		{ 'i', "incognito" } );
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
	args::Flag openClean( parser, "open-clean",
						  "Open a new instance of ecode without recovering the last session",
						  { "open-clean", 'x' } );
	args::ValueFlag<std::string> language(
		parser, "language",
		"Try to set the default language the editor will be loaded. The language must be supported "
		"in order to this option do something.",
		{ "language" }, "" );
	args::ValueFlag<std::string> profile( parser, "profile", "Start with profile at <path>",
										  { "profile" }, "" );
	args::Flag disablePlugins( parser, "disable-plugins",
							   "Disable all plugins. This option is not persisted and is effective "
							   "only when the command opens a new window.",
							   { "disable-plugins" } );

#ifdef EE_TEXT_SHAPER_ENABLED
	args::Flag textShaper( parser, "text-shaper", "Enables text-shaping capabilities",
						   { "text-shaper" } );
#endif

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
		if ( !sfile.isOpen() )
			return EXIT_FAILURE;
		std::vector<std::string> addedLangs;
		if ( SyntaxDefinitionManager::instance()->loadFromStream( sfile, &addedLangs ) ) {
			for ( const auto& lang : addedLangs ) {
				const auto& def = SyntaxDefinitionManager::instance()->getByLanguageName( lang );
				auto code = SyntaxDefinitionManager::toCPP( def );
				if ( convertLangOutput && !convertLangOutput.Get().empty() &&
					 FileSystem::isDirectory( convertLangOutput.Get() ) ) {
					std::string output( convertLangOutput.Get() );
					FileSystem::dirAddSlashAtEnd( output );
					std::string fileName( def.getLanguageNameForFileSystem() );
					FileSystem::fileWrite( output + fileName + ".hpp", code.first );
					FileSystem::fileWrite( output + fileName + ".cpp", code.second );
				} else {
					std::cout << code.first << code.second << "\n";
				}
			}
		}
		return EXIT_SUCCESS;
	}

	if ( version.Get() ) {
		Sys::windowAttachConsole();
		std::cout << ecode::Version::getVersionFullName() << '\n';
		return EXIT_SUCCESS;
	}

#ifdef EE_TEXT_SHAPER_ENABLED
	if ( textShaper.Get() )
		Text::TextShaperEnabled = true;
#endif

	App::InitParameters params;
	params.logLevel = logLevel.Get();
	params.file = folder ? folder.Get() : fileOrFolderPos.Get();
	params.pidelDensity = pixelDensityConf ? pixelDensityConf.Get() : 0.f;
	params.colorScheme = prefersColorScheme ? prefersColorScheme.Get() : "";
	params.terminal = terminal.Get();
	params.frameBuffer = fb.Get();
	params.benchmarkMode = benchmarkMode.Get();
	params.css = css.Get();
	params.fileToOpen = file.Get();
	params.stdOutLogs = verbose.Get();
	params.disableFileLogs = disableFileLogs.Get();
	params.openClean = openClean.Get();
	params.portable = portable.Get();
	params.language = language.Get();
	params.incognito = incognito.Get();
	params.prematureExit = health || ( healthLang && !healthLang.Get().empty() ) ||
						   ( exportLangPath && !exportLangPath.Get().empty() );
	params.profile = profile.Get();
	params.disablePlugins = disablePlugins.Get();

	appInstance = eeNew( App, ( jobs, args ) );
	appInstance->init( params );

	if ( exportLangPath && !exportLangPath.Get().empty() ) {
		Sys::windowAttachConsole();
		exportLanguages( exportLangPath.Get(), exportLang.Get(), appInstance->getLanguagesPath() );
		return EXIT_SUCCESS;
	}

	if ( health || ( healthLang && !healthLang.Get().empty() ) ) {
		Sys::windowAttachConsole();
		Language::LanguagesSyntaxHighlighting::load();
		SyntaxDefinitionManager::instance()->loadFromFolder( appInstance->getLanguagesPath() );
		FeaturesHealth::doHealth( appInstance->getPluginManager(), healthLang.Get(),
								  healthFormat.Get() );
		return EXIT_SUCCESS;
	}

	eeSAFE_DELETE( appInstance );

	Engine::destroySingleton();
	MemoryManager::showResults();

	return EXIT_SUCCESS;
}
