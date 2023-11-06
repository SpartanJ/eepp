#ifndef ECODE_HPP
#define ECODE_HPP

#include "appconfig.hpp"
#include "docsearchcontroller.hpp"
#include "featureshealth.hpp"
#include "filesystemlistener.hpp"
#include "globalsearchcontroller.hpp"
#include "notificationcenter.hpp"
#include "plugins/pluginmanager.hpp"
#include "projectbuild.hpp"
#include "projectdirectorytree.hpp"
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

class App : public UICodeEditorSplitter::Client {
  public:
	explicit App( const size_t& jobs = 0, const std::vector<std::string>& args = {} );

	~App();

	void init( const LogLevel& logLevel, std::string file, const Float& pidelDensity,
			   const std::string& colorScheme, bool terminal, bool frameBuffer, bool benchmarkMode,
			   const std::string& css, bool health, const std::string& healthLang,
			   ecode::FeaturesHealth::OutputFormat healthFormat, const std::string& fileToOpen,
			   bool stdOutLogs, bool disableFileLogs, bool openClean );

	void createWidgetInspector();

	void setAppTitle( const std::string& title );

	void openFileDialog();

	void openFolderDialog();

	void openFontDialog( std::string& fontPath, bool loadingMonoFont );

	void downloadFileWeb( const std::string& url );

	UIFileDialog* saveFileDialog( UICodeEditor* editor, bool focusOnClose = true );

	void closeApp();

	void mainLoop();

	void runCommand( const std::string& command );

	bool loadConfig( const LogLevel& logLevel, const Sizeu& displaySize, bool sync, bool stdOutLogs,
					 bool disableFileLogs );

	void saveConfig();

	std::string getKeybind( const std::string& command );

	std::vector<std::string> getUnlockedCommands();

	bool isUnlockedCommand( const std::string& command );

	void saveAll();

	ProjectDirectoryTree* getDirTree() const;

	std::shared_ptr<ThreadPool> getThreadPool() const;

	void loadFileFromPath( const std::string& path, bool inNewTab = true,
						   UICodeEditor* codeEditor = nullptr,
						   std::function<void( UICodeEditor*, const std::string& )> onLoaded =
							   std::function<void( UICodeEditor*, const std::string& )>() );

	void hideGlobalSearchBar();

	void hideSearchBar();

	void hideLocateBar();

	bool isDirTreeReady() const;

	NotificationCenter* getNotificationCenter() const;

	void fullscreenToggle();

	void downloadFileWebDialog();

	void showGlobalSearch( bool searchAndReplace );

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

	void setUIFontSize();

	void setEditorFontSize();

	void setTerminalFontSize();

	void setUIScaleFactor();

	void toggleSidePanel();

	UIMainLayout* getMainLayout() const { return mMainLayout; }

	UIMessageBox* errorMsgBox( const String& msg );

	UIMessageBox* fileAlreadyExistsMsgBox();

	void toggleSettingsMenu();

	void createNewTerminal();

	UIStatusBar* getStatusBar() const { return mStatusBar; }

	void showFolderTreeViewTab();

	void showBuildTab();

	template <typename T> void registerUnlockedCommands( T& t ) {
		t.setCommand( "keybindings", [this] { loadFileFromPath( mKeybindingsPath ); } );
		t.setCommand( "debug-draw-boxes-toggle", [this] { debugDrawBoxesToggle(); } );
		t.setCommand( "debug-draw-highlight-toggle", [this] { debugDrawHighlightToggle(); } );
		t.setCommand( "debug-draw-debug-data", [this] { debugDrawData(); } );
		t.setCommand( "debug-widget-tree-view", [this] { createWidgetInspector(); } );
		t.setCommand( "menu-toggle", [this] { toggleSettingsMenu(); } );
		t.setCommand( "switch-side-panel", [this] { switchSidePanel(); } );
		t.setCommand( "download-file-web", [this] { downloadFileWebDialog(); } );
		t.setCommand( "move-panel-left", [this] { panelPosition( PanelPosition::Left ); } );
		t.setCommand( "move-panel-right", [this] { panelPosition( PanelPosition::Right ); } );
		t.setCommand( "create-new-terminal", [this] { createNewTerminal(); } );
		t.setCommand( "terminal-split-right", [this] {
			auto cwd = getCurrentWorkingDir();
			mSplitter->split( UICodeEditorSplitter::SplitDirection::Right,
							  mSplitter->getCurWidget(), false );
			mTerminalManager->createNewTerminal( "", nullptr, cwd );
		} );
		t.setCommand( "terminal-split-bottom", [this] {
			auto cwd = getCurrentWorkingDir();
			mSplitter->split( UICodeEditorSplitter::SplitDirection::Bottom,
							  mSplitter->getCurWidget(), false );
			mTerminalManager->createNewTerminal( "", nullptr, cwd );
		} );
		t.setCommand( "terminal-split-left", [this] {
			auto cwd = getCurrentWorkingDir();
			mSplitter->split( UICodeEditorSplitter::SplitDirection::Left, mSplitter->getCurWidget(),
							  false );
			mTerminalManager->createNewTerminal( "", nullptr, cwd );
		} );
		t.setCommand( "terminal-split-top", [this] {
			auto cwd = getCurrentWorkingDir();
			mSplitter->split( UICodeEditorSplitter::SplitDirection::Top, mSplitter->getCurWidget(),
							  false );
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
		t.setCommand( "toggle-status-terminal", [this] { mStatusTerminalController->toggle(); } );
		t.setCommand( "open-locatebar", [this] { mUniversalLocator->showLocateBar(); } );
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
		t.setCommand( "project-build-cancel", [this] {
			if ( mProjectBuildManager && mProjectBuildManager->isBuilding() ) {
				mProjectBuildManager->cancelBuild();
			}
		} );
		t.setCommand( "show-folder-treeview-tab", [this] { showFolderTreeViewTab(); } );
		t.setCommand( "show-build-tab", [this] { showBuildTab(); } );
		t.setCommand( "open-workspace-symbol-search",
					  [this] { mUniversalLocator->showWorkspaceSymbol(); } );
		t.setCommand( "open-document-symbol-search",
					  [this] { mUniversalLocator->showDocumentSymbol(); } );
		t.setCommand( "editor-set-line-breaking-column", [this] { setLineBreakingColumn(); } );
		t.setCommand( "editor-set-line-spacing", [this] { setLineSpacing(); } );
		t.setCommand( "editor-set-cursor-blinking-time", [this] { setCursorBlinkingTime(); } );
		t.setCommand( "check-for-updates", [this] { checkForUpdates(); } );
		t.setCommand( "about-ecode", [this] { aboutEcode(); } );
		t.setCommand( "ecode-source", [this] { ecodeSource(); } );
		t.setCommand( "ui-scale-factor", [this] { setUIScaleFactor(); } );
		t.setCommand( "show-side-panel", [this] { switchSidePanel(); } );
		t.setCommand( "toggle-status-bar", [this] { switchStatusBar(); } );
		t.setCommand( "editor-font-size", [this] { setEditorFontSize(); } );
		t.setCommand( "terminal-font-size", [this] { setTerminalFontSize(); } );
		t.setCommand( "ui-font-size", [this] { setUIFontSize(); } );
		t.setCommand( "ui-panel-font-size", [this] { setUIPanelFontSize(); } );
		t.setCommand( "serif-font", [this] { openFontDialog( mConfig.ui.serifFont, false ); } );
		t.setCommand( "monospace-font",
					  [this] { openFontDialog( mConfig.ui.monospaceFont, true ); } );
		t.setCommand( "terminal-font",
					  [this] { openFontDialog( mConfig.ui.terminalFont, false ); } );
		t.setCommand( "fallback-font",
					  [this] { openFontDialog( mConfig.ui.fallbackFont, false ); } );
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
		t.setCommand( "check-for-updates", [this] { checkForUpdates( false ); } );
		t.setCommand( "create-new-window", [] {
			std::string processPath = Sys::getProcessFilePath();
			if ( !processPath.empty() ) {
				std::string cmd( processPath + " -x" );
				Sys::execute( cmd );
			}
		} );
		mSplitter->registerSplitterCommands( t );
	}

	PluginManager* getPluginManager() const;

	void loadFileFromPathOrFocus( const std::string& path );

	UISceneNode* getUISceneNode() const { return mUISceneNode; }

	void setLineBreakingColumn();

	void setLineSpacing();

	void setCursorBlinkingTime();

	void checkForUpdates( bool fromStartup = false );

	void aboutEcode();

	void updateRecentFiles();

	void updateRecentFolders();

	const CodeEditorConfig& getCodeEditorConfig() const;

	AppConfig& getConfig();

	void updateDocInfo( TextDocument& doc );

	std::vector<std::pair<String::StringBaseType, String::StringBaseType>>
	makeAutoClosePairs( const std::string& strPairs );

	ProjectDocumentConfig& getProjectDocConfig();

	const std::string& getWindowTitle() const;

	void setFocusEditorOnClose( UIMessageBox* msgBox );

	void setUIColorScheme( const ColorSchemePreference& colorScheme );

	ColorSchemePreference getUIColorScheme() const;

	EE::Window::Window* getWindow() const;

	UITextView* getDocInfo() const;

	UITreeView* getProjectTreeView() const;

	void loadCurrentDirectory();

	GlobalSearchController* getGlobalSearchController() const;

	const std::shared_ptr<FileSystemModel>& getFileSystemModel() const;

	void renameFile( const FileInfo& file );

	UIMessageBox* newInputMsgBox( const String& title, const String& msg );

	std::string getNewFilePath( const FileInfo& file, UIMessageBox* msgBox, bool keepDir = true );

	const std::stack<std::string>& getRecentClosedFiles() const;

	void updateTerminalMenu();

	void ecodeSource();

	void setUIPanelFontSize();

	void refreshFolderView();

	bool isFileVisibleInTreeView( const std::string& filePath );

	void treeViewConfigureIgnoreFiles();

	void loadFileSystemMatcher( const std::string& folderPath );

	void checkLanguagesHealth();

	void loadFileDelayed();

	const std::vector<std::string>& getRecentFolders() const { return mRecentFolders; };

	const std::vector<std::string>& getRecentFiles() const { return mRecentFiles; };

	const std::string& getThemesPath() const;

	std::string getThemePath() const;

	std::string getDefaultThemePath() const;

	void setTheme( const std::string& path );

	void loadImageFromMedium( const std::string& path, bool isMemory );

	void loadImageFromPath( const std::string& path );

	void loadImageFromMemory( const std::string& content );

	void createAndShowRecentFolderPopUpMenu( Node* recentFoldersBut );

	void createAndShowRecentFilesPopUpMenu( Node* recentFilesBut );

	UISplitter* getMainSplitter() const;

	StatusTerminalController* getStatusTerminalController() const;

	void hideStatusTerminal();

	void hideStatusBuildOutput();

	StatusBuildOutputController* getStatusBuildOutputController() const;

	void switchStatusBar();

	void showStatusBar( bool show );

	ProjectBuildManager* getProjectBuildManager() const;

	UITabWidget* getSidePanel() const;

	const std::map<KeyBindings::Shortcut, std::string>& getRealLocalKeybindings() const;

	const std::map<KeyBindings::Shortcut, std::string>& getRealSplitterKeybindings() const;

	const std::map<KeyBindings::Shortcut, std::string>& getRealTerminalKeybindings() const;

	const std::string& getFileToOpen() const;

	void saveProject();

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
	std::map<KeyBindings::Shortcut, std::string> mRealLocalKeybindings;
	std::map<KeyBindings::Shortcut, std::string> mRealSplitterKeybindings;
	std::map<KeyBindings::Shortcut, std::string> mRealTerminalKeybindings;
	std::map<KeyBindings::Shortcut, std::string> mRealDefaultKeybindings;
	std::string mConfigPath;
	std::string mPluginsPath;
	std::string mColorSchemesPath;
	std::string mKeybindingsPath;
	std::string mResPath;
	std::string mLanguagesPath;
	std::string mThemesPath;
	std::string mLogsPath;
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
	Time mFrameTime{ Time::Zero };
	Clock mLastRender;
	Clock mSecondsCounter;
	ProjectDocumentConfig mProjectDocConfig;
	std::unordered_set<Doc::TextDocument*> mTmpDocs;
	std::string mCurrentProject;
	std::string mCurrentProjectName;
	FontTrueType* mFont{ nullptr };
	FontTrueType* mFontMono{ nullptr };
	FontTrueType* mTerminalFont{ nullptr };
	FontTrueType* mFallbackFont{ nullptr };
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
	std::unique_ptr<ProjectBuildManager> mProjectBuildManager;
	std::string mLastFileFolder;
	ColorSchemePreference mUIColorScheme;
	std::unique_ptr<TerminalManager> mTerminalManager;
	std::unique_ptr<PluginManager> mPluginManager;
	std::unique_ptr<SettingsMenu> mSettings;
	std::string mFileToOpen;
	UITheme* mTheme{ nullptr };
	UIStatusBar* mStatusBar{ nullptr };
	UISplitter* mMainSplitter{ nullptr };

	void saveAllProcess();

	void initLocateBar();

	void initProjectTreeView( std::string path, bool openClean );

	void initImageView();

	void loadDirTree( const std::string& path );

	void showSidePanel( bool show );

	void onFileDropped( String file );

	void onTextDropped( String text );

	void updateEditorTitle( UICodeEditor* editor );

	void updateEditorTabTitle( UICodeEditor* editor );

	std::string titleFromEditor( UICodeEditor* editor );

	bool onCloseRequestCallback( EE::Window::Window* );

	void addRemainingTabWidgets( Node* widget );

	void updateEditorState();

	void saveDoc();

	void loadFolder( const std::string& path );

	void loadKeybindings();

	void reloadKeybindings();

	void onDocumentStateChanged( UICodeEditor*, TextDocument& );

	void onDocumentModified( UICodeEditor* editor, TextDocument& );

	void onColorSchemeChanged( const std::string& );

	void onRealDocumentLoaded( UICodeEditor* editor, const std::string& path );

	void onDocumentLoaded( UICodeEditor* editor, const std::string& path );

	void onCodeEditorCreated( UICodeEditor*, TextDocument& doc );

	void onDocumentSelectionChange( UICodeEditor* editor, TextDocument& );

	void onDocumentCursorPosChange( UICodeEditor* editor, TextDocument& );

	void onWidgetFocusChange( UIWidget* widget );

	void onCodeEditorFocusChange( UICodeEditor* editor );

	bool trySendUnlockedCmd( const KeyEvent& keyEvent );

	FontTrueType* loadFont( const std::string& name, std::string fontPath,
							const std::string& fallback = "" );

	void closeFolder();

	void closeEditors();

	void removeFolderWatches();

	void createDocDirtyAlert( UICodeEditor* editor );

	void createDocManyLangsAlert( UICodeEditor* editor );

	void syncProjectTreeWithEditor( UICodeEditor* editor );

	void initPluginManager();

	void onPluginEnabled( UICodeEditorPlugin* plugin );

	void checkForUpdatesResponse( Http::Response response, bool fromStartup );

	std::string getLastUsedFolder();

	void insertRecentFolder( const std::string& rpath );

	void cleanUpRecentFolders();

	void cleanUpRecentFiles();

	void updateOpenRecentFolderBtn();

	void updateDocInfoLocation();

	void onReady();

	bool dirInFolderWatches( const std::string& dir );
};

} // namespace ecode

#endif // ECODE_HPP
