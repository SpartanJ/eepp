#ifndef ECODE_HPP
#define ECODE_HPP

#include "appconfig.hpp"
#include "docsearchcontroller.hpp"
#include "filelocator.hpp"
#include "filesystemlistener.hpp"
#include "globalsearchcontroller.hpp"
#include "notificationcenter.hpp"
#include "projectdirectorytree.hpp"
#include "projectsearch.hpp"
#include "uitreeviewglobalsearch.hpp"
#include "widgetcommandexecuter.hpp"
#include <eepp/ee.hpp>
#include <efsw/efsw.hpp>
#include <eterm/ui/uiterminal.hpp>

using namespace eterm::UI;

namespace ecode {

class AutoCompletePlugin;
class LinterPlugin;
class FormatterPlugin;

class App : public UICodeEditorSplitter::Client {
  public:
	App();

	~App();

	void createWidgetTreeView();

	void init( std::string file, const Float& pidelDensity, const std::string& colorScheme );

	void setAppTitle( const std::string& title );

	void openFileDialog();

	void openFolderDialog();

	void openFontDialog( std::string& fontPath, bool loadingMonoFont );

	void downloadFileWeb( const std::string& url );

	UIFileDialog* saveFileDialog( UICodeEditor* editor, bool focusOnClose = true );

	void closeApp();

	void mainLoop();

	void runCommand( const std::string& command );

	void loadConfig();

	void saveConfig();

	std::string getKeybind( const std::string& command );

	std::vector<std::string> getUnlockedCommands();

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

	void createNewTerminal();

	std::map<KeyBindings::Shortcut, std::string> getAppKeybindings();

	void fullscreenToggle();

  protected:
	EE::Window::Window* mWindow{ nullptr };
	UISceneNode* mUISceneNode{ nullptr };
	Console* mConsole{ nullptr };
	std::string mWindowTitle{ "ecode" };
	UILayout* mMainLayout{ nullptr };
	UILayout* mBaseLayout{ nullptr };
	UILayout* mImageLayout{ nullptr };
	UIPopUpMenu* mSettingsMenu{ nullptr };
	UITextView* mSettingsButton{ nullptr };
	std::vector<UIPopUpMenu*> mColorSchemeMenues;
	Float mColorSchemeMenuesCreatedWithHeight{ 0 };
	std::vector<UIPopUpMenu*> mFileTypeMenues;
	Float mFileTypeMenuesCreatedWithHeight{ 0 };
	UILinearLayout* mDocInfo{ nullptr };
	UITextView* mDocInfoText{ nullptr };
	std::vector<std::string> mRecentFiles;
	std::vector<std::string> mRecentFolders;
	AppConfig mConfig;
	UIPopUpMenu* mDocMenu{ nullptr };
	UIPopUpMenu* mViewMenu{ nullptr };
	UIPopUpMenu* mWindowMenu{ nullptr };
	UIPopUpMenu* mToolsMenu{ nullptr };
	UIPopUpMenu* mProjectTreeMenu{ nullptr };
	UIPopUpMenu* mProjectMenu{ nullptr };
	UISplitter* mProjectSplitter{ nullptr };
	UITabWidget* mSidePanel{ nullptr };
	UICodeEditorSplitter* mEditorSplitter{ nullptr };
	std::string mInitColorScheme;
	std::unordered_map<std::string, std::string> mKeybindings;
	std::unordered_map<std::string, std::string> mKeybindingsInvert;
	std::unordered_map<std::string, std::string> mGlobalSearchKeybindings;
	std::unordered_map<std::string, std::string> mDocumentSearchKeybindings;
	std::string mConfigPath;
	std::string mPluginsPath;
	std::string mKeybindingsPath;
	Float mDisplayDPI{ 96 };
	std::string mResPath;
	AutoCompletePlugin* mAutoCompletePlugin{ nullptr };
	LinterPlugin* mLinterPlugin{ nullptr };
	FormatterPlugin* mFormatterPlugin{ nullptr };
	std::shared_ptr<ThreadPool> mThreadPool;
	std::shared_ptr<ProjectDirectoryTree> mDirTree;
	UITreeView* mProjectTreeView{ nullptr };
	std::shared_ptr<FileSystemModel> mFileSystemModel;
	size_t mMenuIconSize;
	bool mDirTreeReady{ false };
	bool mIsBundledApp{ false };
	ProjectDocumentConfig mProjectDocConfig;
	std::unordered_set<Doc::TextDocument*> mTmpDocs;
	std::string mCurrentProject;
	FontTrueType* mFont{ nullptr };
	FontTrueType* mFontMono{ nullptr };
	FontTrueType* mFontMonoNerdFont{ nullptr };
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

	void createSettingsMenu();

	UIMenu* createColorSchemeMenu();

	void updateColorSchemeMenu();

	UIMenu* createFileTypeMenu();

	void updateCurrentFileType();

	void updateEditorState();

	void saveDoc();

	void updateRecentFiles();

	void updateRecentFolders();

	void loadFolder( const std::string& path );

	UIMenu* createViewMenu();

	UIMenu* createEditMenu();

	UIMenu* createWindowMenu();

	UIMenu* createHelpMenu();

	Drawable* findIcon( const std::string& name );

	String i18n( const std::string& key, const String& def );

	void updateProjectSettingsMenu();

	UIMenu* createDocumentMenu();

	void updateDocumentMenu();

	void loadKeybindings();

	std::map<KeyBindings::Shortcut, std::string> getDefaultKeybindings();

	std::map<KeyBindings::Shortcut, std::string> getLocalKeybindings();

	void onDocumentStateChanged( UICodeEditor*, TextDocument& );

	void onDocumentModified( UICodeEditor* editor, TextDocument& );

	void onColorSchemeChanged( const std::string& );

	void onDocumentLoaded( UICodeEditor* editor, const std::string& path );

	const CodeEditorConfig& getCodeEditorConfig() const;

	void onCodeEditorCreated( UICodeEditor*, TextDocument& doc );

	void onDocumentSelectionChange( UICodeEditor* editor, TextDocument& );

	void onDocumentCursorPosChange( UICodeEditor* editor, TextDocument& );

	void onWidgetFocusChange( UIWidget* widget );

	void onCodeEditorFocusChange( UICodeEditor* editor );

	bool setAutoComplete( bool enable );

	bool setLinter( bool enable );

	bool setFormatter( bool enable );

	void updateDocInfo( TextDocument& doc );

	void setFocusEditorOnClose( UIMessageBox* msgBox );

	UIPopUpMenu* createToolsMenu();

	bool trySendUnlockedCmd( const KeyEvent& keyEvent );

	void loadCurrentDirectory();

	void toggleSettingsMenu();

	FontTrueType* loadFont( const std::string& name, std::string fontPath,
							const std::string& fallback = "" );

	void closeFolder();

	void closeEditors();

	void switchSidePanel();

	void panelPosition( const PanelPosition& panelPosition );

	void removeFolderWatches();

	void createDocAlert( UICodeEditor* editor );

	void syncProjectTreeWithEditor( UICodeEditor* editor );

	void createProjectTreeMenu( const FileInfo& file );

	void setUIColorScheme( const ColorSchemePreference& colorScheme );
};

} // namespace ecode

#endif // ECODE_HPP
