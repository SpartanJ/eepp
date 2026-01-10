#ifndef ECODE_HPP
#define ECODE_HPP

#include "appconfig.hpp"
#include "docsearchcontroller.hpp"
#include "featureshealth.hpp"
#include "filesystemlistener.hpp"
#include "globalsearchcontroller.hpp"
#include "notificationcenter.hpp"
#include "plugins/plugincontextprovider.hpp"
#include "plugins/pluginmanager.hpp"
#include "projectbuild.hpp"
#include "projectdirectorytree.hpp"
#include "settingsactions.hpp"
#include "statusappoutputcontroller.hpp"
#include "statusbuildoutputcontroller.hpp"
#include "statusterminalcontroller.hpp"
#include "terminalmanager.hpp"
#include "uistatusbar.hpp"
#include "universallocator.hpp"
#include <eepp/ee.hpp>
#include <efsw/efsw.hpp>
#include <eterm/ui/uiterminal.hpp>
#include <stack>

using namespace eterm::UI;

namespace ecode {

class AutoCompletePlugin;
class LinterPlugin;
class FormatterPlugin;
class SettingsMenu;

class App : public UICodeEditorSplitter::Client, public PluginContextProvider {
  public:
	static App* instance();

	explicit App( const size_t& jobs = 0, const std::vector<std::string>& args = {} );

	~App();

	struct InitParameters {
		LogLevel logLevel{ LogLevel::Info };
		std::string file;
		Float pidelDensity{ 0.f };
		std::string colorScheme;
		bool terminal{ false };
		bool frameBuffer{ false };
		bool benchmarkMode{ false };
		std::string css;
		std::string fileToOpen;
		bool stdOutLogs{ false };
		bool disableFileLogs{ false };
		bool openClean{ false };
		bool portable{ false };
		std::string language;
		bool incognito{ false };
		bool prematureExit{ false };
		std::string profile;
		bool disablePlugins{ false };
		bool redirectToFirstInstance{ false };
	};

	void init( InitParameters& );

	void createWidgetInspector();

	void setAppTitle( const std::string& title );

	void openFileDialog();

	std::string getDefaultFileDialogFolder() const;

	void openFolderDialog();

	void openFontDialog( std::string& fontPath, bool loadingMonoFont, bool terminalFont = false,
						 std::function<void()> onFinish = {} );

	void updateInputFonts();

	void downloadFileWeb( const std::string& url );

	UIFileDialog* saveFileDialog( UICodeEditor* editor, bool focusOnClose = true );

	void closeApp();

	void mainLoop();

	void runCommand( const std::string& command );

	bool commandExists( const std::string& command ) const;

	bool loadConfig( const LogLevel& logLevel, const Sizeu& displaySize, bool sync, bool stdOutLogs,
					 bool disableFileLogs );

	void saveConfig();

	std::string getKeybind( const std::string& command );

	std::vector<std::string> getUnlockedCommands();

	void saveAll();

	ProjectDirectoryTree* getDirTree() const;

	std::shared_ptr<ThreadPool> getThreadPool() const;

	void openFileFromPath( const std::string& path );

	bool loadFileFromPath( std::string path, bool inNewTab = true,
						   UICodeEditor* codeEditor = nullptr,
						   std::function<void( UICodeEditor*, const std::string& )> onLoaded =
							   std::function<void( UICodeEditor*, const std::string& )>(),
						   bool openBinaryAsDocument = false, bool tryFindMimeType = false );

	void hideGlobalSearchBar();

	void hideSearchBar();

	void hideLocateBar();

	bool isDirTreeReady() const;

	NotificationCenter* getNotificationCenter() const;

	void fullscreenToggle();

	void downloadFileWebDialog();

	void showGlobalSearch( bool searchAndReplace, std::optional<std::string> pathFilters = {} );

	void showFindView();

	void toggleHiddenFiles();

	void newFile( const FileInfo& file );

	void newFolder( const FileInfo& file );

	void consoleToggle();

	String i18n( const std::string& key, const String& def );

	UICodeEditorSplitter* getSplitter() const { return mSplitter; }

	TerminalConfig& termConfig() { return mConfig.term; }

	const std::string& resPath() const { return mResPath; }

	Font* getTerminalFont() const { return mTerminalFont; }

	Font* getFontMono() const { return mFontMono; }

	Font* getFallbackFont() const { return mFallbackFont; }

	const Float& getDisplayDPI() const { return mDisplayDPI; }

	const std::string& getCurrentProject() const { return mCurrentProject; }

	std::string getCurrentWorkingDir() const;

	Drawable* findIcon( const std::string& name );

	Drawable* findIcon( const std::string& name, const size_t iconSize );

	const std::map<KeyBindings::Shortcut, std::string>& getRealDefaultKeybindings();

	std::map<KeyBindings::Shortcut, std::string> getDefaultKeybindings();

	std::map<KeyBindings::Shortcut, std::string> getLocalKeybindings();

	std::map<std::string, std::string> getMigrateKeybindings();

	void switchSidePanel();

	void panelPosition( const PanelPosition& panelPosition );

	UniversalLocator* getUniversalLocator() const { return mUniversalLocator.get(); }

	TerminalManager* getTerminalManager() const { return mTerminalManager.get(); }

	UISceneNode* uiSceneNode() const { return mUISceneNode; }

	void reopenClosedTab();

	void createPluginManagerUI();

	void debugDrawHighlightToggle();

	void debugDrawBoxesToggle();

	void debugDrawData();

	void toggleSidePanel();

	UIMainLayout* getMainLayout() const { return mMainLayout; }

	UIMessageBox* errorMsgBox( const String& msg );

	UIMessageBox* fileAlreadyExistsMsgBox();

	void toggleSettingsMenu();

	UIStatusBar* getStatusBar() const { return mStatusBar; }

	void showFolderTreeViewTab();

	void showBuildTab();

	SettingsMenu* getSettingsMenu() const { return mSettings.get(); }

	template <typename T> void registerUnlockedCommands( T& t ) {
		t.setCommand( "keybindings", [this] {
			loadFileFromPath( mKeybindingsPath );

			if ( mNotificationCenter ) {
				mNotificationCenter->addInteractiveNotification(
					i18n( "keybindings_clarification",
						  "More keybindings can be set for each active plugin at the Plugins "
						  "Manager.\nBe aware that many of the core keybindings can be found "
						  "there." ),
					i18n( "plugin_manager_open", "Open Plugins Manager" ),
					[this] { runCommand( "plugin-manager-open" ); }, Seconds( 10 ) );
			}
		} );
		t.setCommand( "debug-draw-boxes-toggle", [this] { debugDrawBoxesToggle(); } );
		t.setCommand( "debug-draw-highlight-toggle", [this] { debugDrawHighlightToggle(); } );
		t.setCommand( "debug-draw-debug-data", [this] { debugDrawData(); } );
		t.setCommand( "debug-widget-tree-view", [this] { createWidgetInspector(); } );
		t.setCommand( "menu-toggle", [this] { toggleSettingsMenu(); } );
		t.setCommand( "switch-side-panel", [this] { switchSidePanel(); } );
		t.setCommand( "download-file-web", [this] { downloadFileWebDialog(); } );
		t.setCommand( "move-panel-left", [this] { panelPosition( PanelPosition::Left ); } );
		t.setCommand( "move-panel-right", [this] { panelPosition( PanelPosition::Right ); } );
		t.setCommand( "create-new-terminal",
					  [this] { mTerminalManager->createTerminalInSplitter(); } );
		t.setCommand( "terminal-split-right", [this] {
			auto cwd = getCurrentWorkingDir();
			mSplitter->split( SplitDirection::Right, mSplitter->getCurWidget(), false );
			mTerminalManager->createNewTerminal( "", nullptr, cwd );
		} );
		t.setCommand( "terminal-split-bottom", [this] {
			auto cwd = getCurrentWorkingDir();
			mSplitter->split( SplitDirection::Bottom, mSplitter->getCurWidget(), false );
			mTerminalManager->createNewTerminal( "", nullptr, cwd );
		} );
		t.setCommand( "terminal-split-left", [this] {
			auto cwd = getCurrentWorkingDir();
			mSplitter->split( SplitDirection::Left, mSplitter->getCurWidget(), false );
			mTerminalManager->createNewTerminal( "", nullptr, cwd );
		} );
		t.setCommand( "terminal-split-top", [this] {
			auto cwd = getCurrentWorkingDir();
			mSplitter->split( SplitDirection::Top, mSplitter->getCurWidget(), false );
			mTerminalManager->createNewTerminal( "", nullptr, cwd );
		} );
		t.setCommand( "reopen-closed-tab", [this] { reopenClosedTab(); } );
		t.setCommand( "plugin-manager-open", [this] { createPluginManagerUI(); } );
		t.setCommand( "close-app", [this] { closeApp(); } );
		t.setCommand( "fullscreen-toggle", [this]() { fullscreenToggle(); } );
		t.setCommand( "open-file", [this] { openFileDialog(); } );
		t.setCommand( "open-folder", [this] { openFolderDialog(); } );
		t.setCommand( "console-toggle", [this] { consoleToggle(); } );
		t.setCommand( "find-replace", [this] { showFindView(); } );
		t.setCommand( "open-global-search", [this] { showGlobalSearch( false ); } );
		t.setCommand( "toggle-status-global-search-bar",
					  [this] { mGlobalSearchController->toggleGlobalSearchBar(); } );
		t.setCommand( "toggle-status-build-output",
					  [this] { mStatusBuildOutputController->toggle(); } );
		t.setCommand( "toggle-status-app-output",
					  [this] { mStatusAppOutputController->toggle(); } );
		t.setCommand( "toggle-status-terminal", [this] { mStatusTerminalController->toggle(); } );
		t.setCommand( "open-locatebar", [this] { mUniversalLocator->showLocateBar(); } );
		t.setCommand( "open-locatebar-glob-search",
					  [this] { mUniversalLocator->showLocateBar( true ); } );
		t.setCommand( "toggle-status-locate-bar",
					  [this] { mUniversalLocator->toggleLocateBar(); } );
		t.setCommand( "open-command-palette", [this] { mUniversalLocator->showCommandPalette(); } );
		t.setCommand( "show-open-documents", [this] { mUniversalLocator->showOpenDocuments(); } );
		t.setCommand( "project-build-start", [this] {
			if ( mProjectBuildManager && mStatusBuildOutputController ) {
				if ( mProjectBuildManager->isBuilding() ) {
					mProjectBuildManager->cancelBuild();
				}
				mProjectBuildManager->buildCurrentConfig( mStatusBuildOutputController.get() );
			}
		} );
		t.setCommand( "project-build-start-cancel", [this] {
			if ( mProjectBuildManager && mStatusBuildOutputController ) {
				if ( mProjectBuildManager->isBuilding() ) {
					mProjectBuildManager->cancelBuild();
				} else {
					mProjectBuildManager->buildCurrentConfig( mStatusBuildOutputController.get() );
				}
			}
		} );
		t.setCommand( "project-build-cancel", [this] {
			if ( mProjectBuildManager && mProjectBuildManager->isBuilding() ) {
				mProjectBuildManager->cancelBuild();
			}
		} );
		t.setCommand( "project-build-clean", [this] {
			if ( mProjectBuildManager && mStatusBuildOutputController )
				mProjectBuildManager->cleanCurrentConfig( mStatusBuildOutputController.get() );
		} );
		t.setCommand( "project-run-executable", [this] {
			if ( mProjectBuildManager && mStatusAppOutputController )
				mProjectBuildManager->runCurrentConfig( mStatusAppOutputController.get(), false );
		} );
		t.setCommand( "project-build-and-run", [this] {
			if ( mProjectBuildManager && mStatusAppOutputController )
				mProjectBuildManager->runCurrentConfig( mStatusAppOutputController.get(), true,
														mStatusBuildOutputController.get() );
		} );
		t.setCommand( "project-stop-executable", [this] {
			if ( mProjectBuildManager && mProjectBuildManager->isRunningApp() )
				mProjectBuildManager->cancelRun();
		} );
		t.setCommand( "show-folder-treeview-tab", [this] { showFolderTreeViewTab(); } );
		t.setCommand( "show-build-tab", [this] { showBuildTab(); } );
		t.setCommand( "open-workspace-symbol-search",
					  [this] { mUniversalLocator->showWorkspaceSymbol(); } );
		t.setCommand( "open-document-symbol-search",
					  [this] { mUniversalLocator->showDocumentSymbol(); } );
		t.setCommand( "editor-set-line-breaking-column",
					  [this] { mSettingsActions->setLineBreakingColumn(); } );
		t.setCommand( "editor-set-line-spacing", [this] { mSettingsActions->setLineSpacing(); } );
		t.setCommand( "editor-set-cursor-blinking-time",
					  [this] { mSettingsActions->setCursorBlinkingTime(); } );
		t.setCommand( "editor-set-indent-tab-character",
					  [this] { mSettingsActions->setIndentTabCharacter(); } );
		t.setCommand( "check-for-updates", [this] { mSettingsActions->checkForUpdates(); } );
		t.setCommand( "about-ecode", [this] { mSettingsActions->aboutEcode(); } );
		t.setCommand( "ecode-source", [this] { mSettingsActions->ecodeSource(); } );
		t.setCommand( "ui-scale-factor", [this] { mSettingsActions->setUIScaleFactor(); } );
		t.setCommand( "show-side-panel", [this] { switchSidePanel(); } );
		t.setCommand( "toggle-status-bar", [this] { switchStatusBar(); } );
		t.setCommand( "toggle-menu-bar", [this] { switchMenuBar(); } );
		t.setCommand( "editor-font-size", [this] { mSettingsActions->setEditorFontSize(); } );
		t.setCommand( "terminal-font-size", [this] { mSettingsActions->setTerminalFontSize(); } );
		t.setCommand( "ui-font-size", [this] { mSettingsActions->setUIFontSize(); } );
		t.setCommand( "ui-panel-font-size", [this] { mSettingsActions->setUIPanelFontSize(); } );
		t.setCommand( "sans-serif-font",
					  [this] { openFontDialog( mConfig.ui.sansSerifFont, false ); } );
		t.setCommand( "monospace-font",
					  [this] { openFontDialog( mConfig.ui.monospaceFont, true ); } );
		t.setCommand( "terminal-font",
					  [this] { openFontDialog( mConfig.ui.terminalFont, true, true ); } );
		t.setCommand( "fallback-font", [this] {
			openFontDialog( mConfig.ui.fallbackFont, false, false, [this] {
				UIMessageBox::New( UIMessageBox::OK,
								   i18n( "new_fallback_font_requires_restart",
										 "New fallback font has been set. Application must be "
										 "restarted in order to see the changes." ) )
					->showWhenReady();
			} );
		} );
		t.setCommand( "tree-view-configure-ignore-files",
					  [this] { treeViewConfigureIgnoreFiles(); } );
		t.setCommand( "check-languages-health", [this] { checkLanguagesHealth(); } );
		t.setCommand( "configure-terminal-shell", [this] {
			if ( mTerminalManager )
				mTerminalManager->configureTerminalShell();
		} );
		t.setCommand( "configure-terminal-scrollback", [this] {
			if ( mTerminalManager )
				mTerminalManager->configureTerminalScrollback();
		} );
		t.setCommand( "check-for-updates", [this] { mSettingsActions->checkForUpdates( false ); } );
		t.setCommand( "create-new-window", [this] { openInNewWindow(); } );
		t.setCommand( "create-new-welcome-tab", [this] { createWelcomeTab(); } );

		mSplitter->registerSplitterCommands( t );

		// Overwrite it
		t.setCommand( "next-tab", [this] {
			UITabWidget* tabWidget =
				getSplitter()->tabWidgetFromWidget( getSplitter()->getCurWidget() );
			if ( tabWidget ) {
				tabWidget->setTabJumpMode( mConfig.editor.tabJumpMode );
				tabWidget->setEnableTabSwitcher( mConfig.editor.tabSwitcher );
				std::vector<Keycode> triggerCodes;
				auto kb = getKeybind( "next-tab" );
				if ( !kb.empty() ) {
					auto shortcut =
						KeyBindings::toShortcut( mWindow->getInput(), getKeybind( "next-tab" ) );
					if ( !shortcut.empty() ) {
						if ( shortcut.mod & KeyMod::getDefaultModifier() ) {
							triggerCodes =
								KeyMod::getKeyCodesFromModifier( KeyMod::getDefaultModifier() );
						}
					}
				}
				tabWidget->focusNextTab( triggerCodes );
			}
		} );

		t.setCommand( "previous-tab", [this] {
			UITabWidget* tabWidget =
				getSplitter()->tabWidgetFromWidget( getSplitter()->getCurWidget() );
			if ( tabWidget ) {
				tabWidget->setTabJumpMode( mConfig.editor.tabJumpMode );
				tabWidget->setEnableTabSwitcher( mConfig.editor.tabSwitcher );
				std::vector<Keycode> triggerCodes;
				auto kb = getKeybind( "next-tab" );
				if ( !kb.empty() ) {
					auto shortcut =
						KeyBindings::toShortcut( mWindow->getInput(), getKeybind( "next-tab" ) );
					if ( !shortcut.empty() ) {
						if ( shortcut.mod & KeyMod::getDefaultModifier() ) {
							triggerCodes =
								KeyMod::getKeyCodesFromModifier( KeyMod::getDefaultModifier() );
						}
					}
				}
				tabWidget->focusPreviousTab( triggerCodes );
			}
		} );

		t.setCommand( "reset-global-language-extensions-priorities", [this] {
			mConfig.languagesExtensions.priorities.clear();
			saveConfig();
			mNotificationCenter->addNotification(
				i18n( "global_language_extensions_priorities_has_been_reset",
					  "Global language extensions priorities has been reset" ) );
		} );

		t.setCommand( "reset-project-language-extensions-priorities", [this] {
			if ( !mCurrentProject.empty() && mCurrentProject != getPlaygroundPath() ) {
				mProjectDocConfig.languagesExtensions.priorities.clear();
				saveProject();
				mNotificationCenter->addNotification(
					i18n( "project_language_extensions_priorities_has_been_reset",
						  "Project language extensions priorities has been reset" ) );
			} else {
				mNotificationCenter->addNotification(
					i18n( "no_project_loaded", "No project loaded" ) );
			}
		} );
	}

	PluginManager* getPluginManager() const;

	void
	loadFileFromPathOrFocus( const std::string& path, bool inNewTab = true,
							 UICodeEditor* codeEditor = nullptr,
							 std::function<void( UICodeEditor*, const std::string& )> onLoaded =
								 std::function<void( UICodeEditor*, const std::string& )>() );

	void focusOrLoadFile( const std::string& path, const TextRange& range = {},
						  bool searchInSameContext = true );

	UISceneNode* getUISceneNode() const { return mUISceneNode; }

	void updateRecentFiles();

	void updateRecentFolders();

	void updateRecentButtons();

	const CodeEditorConfig& getCodeEditorConfig() const;

	AppConfig& getConfig();

	const AppConfig& getConfig() const;

	void updateDocInfo( TextDocument& doc );

	std::vector<std::pair<String::StringBaseType, String::StringBaseType>>
	makeAutoClosePairs( const std::string& strPairs );

	ProjectConfig& getProjectConfig();

	const std::string& getWindowTitle() const;

	void setFocusEditorOnClose( UIMessageBox* msgBox );

	void setUIColorScheme( const ColorSchemeExtPreference& colorScheme );

	void setUIColorSchemeFromUserInteraction( const ColorSchemeExtPreference& colorSchemeExt );

	ColorSchemeExtPreference getUIColorScheme() const;

	EE::Window::Window* getWindow() const;

	UITextView* getDocInfo() const;

	UITreeView* getProjectTreeView() const;

	void loadCurrentDirectory();

	GlobalSearchController* getGlobalSearchController() const;

	const std::shared_ptr<FileSystemModel>& getFileSystemModel() const;

	void renameFile( const FileInfo& file );

	void openAllFilesInFolder( const FileInfo& folder );

	UIMessageBox* newInputMsgBox( const String& title, const String& msg );

	std::string getNewFilePath( const FileInfo& file, UIMessageBox* msgBox, bool keepDir = true );

	const std::stack<std::string>& getRecentClosedFiles() const;

	void refreshFolderView();

	bool isFileVisibleInTreeView( const std::string& filePath );

	void treeViewConfigureIgnoreFiles();

	void loadFileSystemMatcher( const std::string& folderPath );

	void checkLanguagesHealth();

	void loadFileDelayed();

	const std::vector<std::string>& getRecentFolders() const { return mRecentFolders; };

	const std::vector<std::string>& getRecentFiles() const { return mRecentFiles; };

	const std::string& getThemesPath() const;

	const std::string& geti18nPath() const;

	std::string getThemePath() const;

	std::string getDefaultThemePath() const;

	void setTheme( const std::string& path );

	void loadImageFromMedium( const std::string& path, bool isMemory, bool forcePreview = false,
							  bool forceTab = false );

	void loadImageFromPath( const std::string& path );

	void loadImageFromMemory( const std::string& content );

	void loadAudioFromPath( const std::string& path, bool autoPlay = true );

	void createAndShowRecentFolderPopUpMenu( Node* recentFoldersBut );

	void createAndShowRecentFilesPopUpMenu( Node* recentFilesBut );

	UISplitter* getMainSplitter() const;

	StatusTerminalController* getStatusTerminalController() const;

	void hideStatusTerminal();

	void hideStatusBuildOutput();

	void hideStatusAppOutput();

	StatusBuildOutputController* getStatusBuildOutputController() const;

	StatusAppOutputController* getStatusAppOutputController() const;

	void switchStatusBar();

	void showStatusBar( bool show );

	void switchMenuBar();

	ProjectBuildManager* getProjectBuildManager() const;

	UITabWidget* getSidePanel() const;

	const std::map<KeyBindings::Shortcut, std::string>& getRealLocalKeybindings() const;

	const std::map<KeyBindings::Shortcut, std::string>& getRealSplitterKeybindings() const;

	const std::map<KeyBindings::Shortcut, std::string>& getRealTerminalKeybindings() const;

	const std::string& getFileToOpen() const;

	void saveProject( bool onlyIfNeeded = false, bool sessionSnapshotEnabled = true );

	std::pair<bool, std::string> generateConfigPath();

	const std::string getScriptsPath() const { return mScriptsPath; }

	const std::string& getPlaygroundPath() const { return mPlaygroundPath; }

	bool isAnyStatusBarSectionVisible() const;

	void createDocDirtyAlert( UICodeEditor* editor, bool showEnableAutoReload = true );

	void createDocDoesNotExistsInFSAlert( UICodeEditor* editor );

	SettingsActions* getSettingsActions() { return mSettingsActions.get(); }

	const std::string& getLanguagesPath() const { return mLanguagesPath; }

	bool pluginsDisabled() const { return mDisablePlugins; }

	void loadFolder( std::string path, bool forceNewWindow = false );

	const std::unordered_map<std::string, std::string>& getStatusBarKeybindings() const {
		return mStatusBarKeybindings;
	}

	void updateLanguageExtensionsPriorities();

	std::map<std::string, std::string>& getCurrentLanguageExtensionsPriorities();

	bool isDestroyingApp() const { return mDestroyingApp; }

  protected:
	std::vector<std::string> mArgs;
	EE::Window::Window* mWindow{ nullptr };
	UISceneNode* mUISceneNode{ nullptr };
	UIConsole* mConsole{ nullptr };
	std::string mCurWindowTitle;
	std::string mWindowTitle{ "ecode" };
	UIMainLayout* mMainLayout{ nullptr };
	UILayout* mBaseLayout{ nullptr };
	UILayout* mImageLayout{ nullptr };
	UITextView* mDocInfo{ nullptr };
	std::vector<std::string> mRecentFiles;
	std::stack<std::string> mRecentClosedFiles;
	std::vector<std::string> mRecentFolders;
	AppConfig mConfig;
	UISplitter* mProjectSplitter{ nullptr };
	UITabWidget* mSidePanel{ nullptr };
	UICodeEditorSplitter* mSplitter{ nullptr };
	std::string mInitColorScheme;
	std::unordered_map<std::string, std::string> mKeybindings;
	std::unordered_map<std::string, std::string> mKeybindingsInvert;
	std::unordered_map<std::string, std::string> mGlobalSearchKeybindings;
	std::unordered_map<std::string, std::string> mDocumentSearchKeybindings;
	std::unordered_map<std::string, std::string> mStatusBarKeybindings;
	std::map<KeyBindings::Shortcut, std::string> mRealLocalKeybindings;
	std::map<KeyBindings::Shortcut, std::string> mRealSplitterKeybindings;
	std::map<KeyBindings::Shortcut, std::string> mRealTerminalKeybindings;
	std::map<KeyBindings::Shortcut, std::string> mRealDefaultKeybindings;
	std::unordered_map<std::string, std::string> mMousebindings;
	std::unordered_map<std::string, std::string> mMousebindingsInvert;
	std::string mConfigPath;
	std::string mPluginsPath;
	std::string mColorSchemesPath;
	std::string mKeybindingsPath;
	std::string mResPath;
	std::string mLanguagesPath;
	std::string mThemesPath;
	std::string mLogsPath;
	std::string mi18nPath;
	std::string mScriptsPath;
	std::string mPlaygroundPath;
	std::string mIpcPath;
	std::string mPidPath;
	Float mDisplayDPI{ 96 };
	std::shared_ptr<ThreadPool> mThreadPool;
	std::shared_ptr<ProjectDirectoryTree> mDirTree;
	UITreeView* mProjectTreeView{ nullptr };
	UILinearLayout* mProjectViewEmptyCont{ nullptr };
	std::shared_ptr<FileSystemModel> mFileSystemModel;
	std::shared_ptr<GitIgnoreMatcher> mFileSystemMatcher;
	size_t mMenuIconSize{ 16 };
	bool mDirTreeReady{ false };
	bool mUseFrameBuffer{ false };
	bool mBenchmarkMode{ false };
	bool mDisablePlugins{ false };
	bool mRedirectToFirstInstance{ false };
	bool mFirstInstance{ false };
	bool mPortableMode{ false };
	bool mPortableModeFailed{ false };
	bool mDestroyingApp{ false };
	Time mFrameTime{ Time::Zero };
	bool mIncognito{ false };
	Clock mLastRender;
	Clock mSecondsCounter;
	ProjectConfig mProjectDocConfig;
	std::unordered_set<Doc::TextDocument*> mTmpDocs;
	std::string mCurrentProject;
	std::string mCurrentProjectName;
	FontTrueType* mFont{ nullptr };
	FontTrueType* mFontMono{ nullptr };
	FontTrueType* mTerminalFont{ nullptr };
	FontTrueType* mFallbackFont{ nullptr };
	FontTrueType* mUserFallbackFont{ nullptr };
	FontTrueType* mRemixIconFont{ nullptr };
	FontTrueType* mNoniconsFont{ nullptr };
	FontTrueType* mCodIconFont{ nullptr };
	efsw::FileWatcher* mFileWatcher{ nullptr };
	FileSystemListener* mFileSystemListener{ nullptr };
	Mutex mWatchesLock;
	std::unordered_map<std::string, efsw::WatchID> mFolderWatches;
	std::unordered_map<std::string, efsw::WatchID> mFilesFolderWatches;
	std::unique_ptr<GlobalSearchController> mGlobalSearchController;
	std::unique_ptr<DocSearchController> mDocSearchController;
	std::unique_ptr<UniversalLocator> mUniversalLocator;
	std::unique_ptr<NotificationCenter> mNotificationCenter;
	std::unique_ptr<StatusTerminalController> mStatusTerminalController;
	std::unique_ptr<StatusBuildOutputController> mStatusBuildOutputController;
	std::unique_ptr<StatusAppOutputController> mStatusAppOutputController;
	std::unique_ptr<ProjectBuildManager> mProjectBuildManager;
	std::string mLastFileFolder;
	ColorSchemeExtPreference mUIColorScheme;
	std::unique_ptr<TerminalManager> mTerminalManager;
	std::unique_ptr<PluginManager> mPluginManager;
	std::unique_ptr<SettingsMenu> mSettings;
	std::string mFileToOpen;
	UITheme* mTheme{ nullptr };
	UIStatusBar* mStatusBar{ nullptr };
	UISplitter* mMainSplitter{ nullptr };
	StyleSheet mAppStyleSheet;
	UIMessageBox* mCloseMsgBox{ nullptr };
	UIMenuBar* mMenuBar{ nullptr };
	std::unique_ptr<SettingsActions> mSettingsActions;
	std::vector<std::string> mPathsToLoad;
	Uint64 mIpcListenerId{ 0 };
	std::mutex mAsyncResourcesLoadMutex;
	std::condition_variable mAsyncResourcesLoadCond;
	std::vector<SyntaxColorScheme> mColorSchemes;
	bool mAsyncResourcesLoaded{ false };
	bool mTerminalMode{ false };
	bool mTerminalModeSidePanelWasVisible{ false };
	std::string mProfilePath;

	void sortSidePanel();

	void saveSidePanelTabsOrder();

	void saveAllProcess();

	void initLocateBar();

	void initProjectViewEmptyCont();

	void initProjectTreeViewUI();

	void initProjectTreeView( std::string path, bool openClean );

	void initImageView();

	void loadDirTree( const std::string& path );

	void showSidePanel( bool show );

	void onFileDropped( std::string file, bool openBinaryAsDocument );

	void onTextDropped( String text );

	void updateEditorTitle( UICodeEditor* editor );

	void updateEditorTabTitle( UICodeEditor* editor );

	void updateNonUniqueTabTitles();

	std::string titleFromEditor( UICodeEditor* editor );

	bool isAnyTerminalDirty() const;

	bool onCloseRequestCallback( EE::Window::Window* );

	void addRemainingTabWidgets( Node* widget );

	void updateEditorState();

	void saveDoc();

	void loadKeybindings();

	void reloadKeybindings();

	void onDocumentStateChanged( UICodeEditor*, TextDocument& );

	void onDocumentModified( UICodeEditor* editor, TextDocument& );

	void onDocumentUndoRedo( UICodeEditor* editor, TextDocument& );

	void onColorSchemeChanged( const std::string& );

	void onRealDocumentLoaded( UICodeEditor* editor, const std::string& path );

	void onDocumentLoaded( UICodeEditor* editor, const std::string& path );

	void onCodeEditorCreated( UICodeEditor*, TextDocument& doc );

	void onDocumentSelectionChange( UICodeEditor* editor, TextDocument& );

	void onDocumentCursorPosChange( UICodeEditor* editor, TextDocument& );

	void onWidgetFocusChange( UIWidget* widget );

	void onCodeEditorFocusChange( UICodeEditor* editor );

	void onTabCreated( UITab* tab, UIWidget* widget );

	bool trySendUnlockedCmd( const KeyEvent& keyEvent );

	FontTrueType* loadFont( const std::string& name, std::string fontPath,
							const std::string& fallback = "" );

	void closeFolder();

	void closeEditors();

	void removeFolderWatches();

	void createDocManyLangsAlert( UICodeEditor* editor );

	void syncProjectTreeWithEditor( UICodeEditor* editor );

	void initPluginManager();

	void onPluginEnabled( Plugin* plugin );

	std::string getLastUsedFolder() const;

	void insertRecentFolder( const std::string& rpath );

	void cleanUpRecentFolders();

	void cleanUpRecentFiles();

	void updateOpenRecentFolderBtn();

	void updateDocInfoLocation();

	void onReady();

	bool dirInFolderWatches( const std::string& dir );

	void insertRecentFile( const std::string& path );

	void insertRecentFileAndUpdateUI( const std::string& path );

	void createWelcomeTab();

	bool needsRedirectToRunningProcess( std::string file );

	std::function<void( UICodeEditor*, const std::string& )>
	getForcePositionFn( TextPosition initialPosition );

	void openInNewWindow( const std::string& params = "" );

	std::string firstInstanceIndicatorPath() const;

	void tintTitleBar();
};

} // namespace ecode

#endif // ECODE_HPP
