#ifndef ECODE_HPP
#define ECODE_HPP

#include "appconfig.hpp"
#include "docsearchcontroller.hpp"
#include "filelocator.hpp"
#include "filesystemlistener.hpp"
#include "globalsearchcontroller.hpp"
#include "notificationcenter.hpp"
#include "plugins/pluginmanager.hpp"
#include "projectdirectorytree.hpp"
#include "terminalmanager.hpp"
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
	App();

	~App();

	void init( const LogLevel& logLevel, std::string file, const Float& pidelDensity,
			   const std::string& colorScheme, bool terminal, bool frameBuffer, bool benchmarkMode,
			   const std::string& css );

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

	void loadConfig( const LogLevel& logLevel, const Sizeu& displaySize );

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

	std::map<KeyBindings::Shortcut, std::string> getDefaultKeybindings();

	std::map<KeyBindings::Shortcut, std::string> getLocalKeybindings();

	void switchSidePanel();

	void panelPosition( const PanelPosition& panelPosition );

	FileLocator* getFileLocator() const { return mFileLocator.get(); }

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

	template <typename T> void registerUnlockedCommands( T& t ) {
		t.setCommand( "keybindings", [&] { loadFileFromPath( mKeybindingsPath ); } );
		t.setCommand( "debug-draw-boxes-toggle", [&] { debugDrawBoxesToggle(); } );
		t.setCommand( "debug-draw-highlight-toggle", [&] { debugDrawHighlightToggle(); } );
		t.setCommand( "debug-draw-debug-data", [&] { debugDrawData(); } );
		t.setCommand( "debug-widget-tree-view", [&] { createWidgetInspector(); } );
		t.setCommand( "menu-toggle", [&] { toggleSettingsMenu(); } );
		t.setCommand( "switch-side-panel", [&] { switchSidePanel(); } );
		t.setCommand( "download-file-web", [&] { downloadFileWebDialog(); } );
		t.setCommand( "move-panel-left", [&] { panelPosition( PanelPosition::Left ); } );
		t.setCommand( "move-panel-right", [&] { panelPosition( PanelPosition::Right ); } );
		t.setCommand( "create-new-terminal", [&] { mTerminalManager->createNewTerminal(); } );
		t.setCommand( "terminal-split-right", [&] {
			auto cwd = getCurrentWorkingDir();
			mSplitter->split( UICodeEditorSplitter::SplitDirection::Right,
							  mSplitter->getCurWidget(), false );
			mTerminalManager->createNewTerminal( "", nullptr, cwd );
		} );
		t.setCommand( "terminal-split-bottom", [&] {
			auto cwd = getCurrentWorkingDir();
			mSplitter->split( UICodeEditorSplitter::SplitDirection::Bottom,
							  mSplitter->getCurWidget(), false );
			mTerminalManager->createNewTerminal( "", nullptr, cwd );
		} );
		t.setCommand( "terminal-split-left", [&] {
			auto cwd = getCurrentWorkingDir();
			mSplitter->split( UICodeEditorSplitter::SplitDirection::Left, mSplitter->getCurWidget(),
							  false );
			mTerminalManager->createNewTerminal( "", nullptr, cwd );
		} );
		t.setCommand( "terminal-split-top", [&] {
			auto cwd = getCurrentWorkingDir();
			mSplitter->split( UICodeEditorSplitter::SplitDirection::Top, mSplitter->getCurWidget(),
							  false );
			mTerminalManager->createNewTerminal( "", nullptr, cwd );
		} );
		t.setCommand( "reopen-closed-tab", [&] { reopenClosedTab(); } );
		t.setCommand( "plugin-manager-open", [&] { createPluginManagerUI(); } );
		t.setCommand( "close-app", [&] { closeApp(); } );
		t.setCommand( "fullscreen-toggle", [&]() { fullscreenToggle(); } );
		t.setCommand( "open-file", [&] { openFileDialog(); } );
		t.setCommand( "open-folder", [&] { openFolderDialog(); } );
		t.setCommand( "console-toggle", [&] { consoleToggle(); } );
		t.setCommand( "find-replace", [&] { showFindView(); } );
		t.setCommand( "open-global-search", [&] { showGlobalSearch( false ); } );
		t.setCommand( "open-locatebar", [&] { mFileLocator->showLocateBar(); } );
		t.setCommand( "editor-set-line-breaking-column", [&] { setLineBreakingColumn(); } );
		t.setCommand( "editor-set-line-spacing", [&] { setLineSpacing(); } );
		t.setCommand( "editor-set-cursor-blinking-time", [&] { setCursorBlinkingTime(); } );
		t.setCommand( "check-for-updates", [&] { checkForUpdates(); } );
		t.setCommand( "about-ecode", [&] { aboutEcode(); } );
		t.setCommand( "ecode-source", [&] { ecodeSource(); } );
		t.setCommand( "ui-scale-factor", [&] { setUIScaleFactor(); } );
		t.setCommand( "show-side-panel", [&] { switchSidePanel(); } );
		t.setCommand( "editor-font-size", [&] { setEditorFontSize(); } );
		t.setCommand( "terminal-font-size", [&] { setTerminalFontSize(); } );
		t.setCommand( "ui-font-size", [&] { setUIFontSize(); } );
		t.setCommand( "ui-panel-font-size", [&] { setUIPanelFontSize(); } );
		t.setCommand( "serif-font", [&] { openFontDialog( mConfig.ui.serifFont, false ); } );
		t.setCommand( "monospace-font", [&] { openFontDialog( mConfig.ui.monospaceFont, true ); } );
		t.setCommand( "terminal-font", [&] { openFontDialog( mConfig.ui.terminalFont, false ); } );
		t.setCommand( "fallback-font", [&] { openFontDialog( mConfig.ui.fallbackFont, false ); } );
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

	UILinearLayout* getDocInfo() const;

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

  protected:
	EE::Window::Window* mWindow{ nullptr };
	UISceneNode* mUISceneNode{ nullptr };
	UIConsole* mConsole{ nullptr };
	std::string mWindowTitle{ "ecode" };
	UIMainLayout* mMainLayout{ nullptr };
	UILayout* mBaseLayout{ nullptr };
	UILayout* mImageLayout{ nullptr };
	UILinearLayout* mDocInfo{ nullptr };
	UITextView* mDocInfoText{ nullptr };
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
	std::string mConfigPath;
	std::string mPluginsPath;
	std::string mColorSchemesPath;
	std::string mKeybindingsPath;
	std::string mResPath;
	Float mDisplayDPI{ 96 };
	std::shared_ptr<ThreadPool> mThreadPool;
	std::shared_ptr<ProjectDirectoryTree> mDirTree;
	UITreeView* mProjectTreeView{ nullptr };
	std::shared_ptr<FileSystemModel> mFileSystemModel;
	size_t mMenuIconSize{ 16 };
	bool mDirTreeReady{ false };
	bool mIsBundledApp{ false };
	bool mUseFrameBuffer{ false };
	bool mBenchmarkMode{ false };
	Time mFrameTime{ Time::Zero };
	Clock mLastRender;
	Clock mSecondsCounter;
	ProjectDocumentConfig mProjectDocConfig;
	std::unordered_set<Doc::TextDocument*> mTmpDocs;
	std::string mCurrentProject;
	FontTrueType* mFont{ nullptr };
	FontTrueType* mFontMono{ nullptr };
	FontTrueType* mTerminalFont{ nullptr };
	FontTrueType* mFallbackFont{ nullptr };
	efsw::FileWatcher* mFileWatcher{ nullptr };
	FileSystemListener* mFileSystemListener{ nullptr };
	Mutex mWatchesLock;
	std::unordered_set<efsw::WatchID> mFolderWatches;
	std::unordered_map<std::string, efsw::WatchID> mFilesFolderWatches;
	std::unique_ptr<GlobalSearchController> mGlobalSearchController;
	std::unique_ptr<DocSearchController> mDocSearchController;
	std::unique_ptr<FileLocator> mFileLocator;
	std::unique_ptr<NotificationCenter> mNotificationCenter;
	std::string mLastFileFolder;
	ColorSchemePreference mUIColorScheme;
	std::unique_ptr<TerminalManager> mTerminalManager;
	std::unique_ptr<PluginManager> mPluginManager;
	std::unique_ptr<SettingsMenu> mSettings;

	void saveAllProcess();

	void initLocateBar();

	void initProjectTreeView( const std::string& path );

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

	void createDocAlert( UICodeEditor* editor );

	void syncProjectTreeWithEditor( UICodeEditor* editor );

	void initPluginManager();

	void onPluginEnabled( UICodeEditorPlugin* plugin );

	void checkForUpdatesResponse( Http::Response response, bool fromStartup );
};

} // namespace ecode

#endif // ECODE_HPP
