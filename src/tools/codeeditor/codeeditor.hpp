#ifndef EE_TOOLS_CODEEDITOR_HPP
#define EE_TOOLS_CODEEDITOR_HPP

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

class AutoCompleteModule;
class LinterModule;
class FormatterModule;

class App : public UICodeEditorSplitter::Client {
  public:
	App();

	~App();

	void init( const std::string& file, const Float& pidelDensity );

	void setAppTitle( const std::string& title );

	void openFileDialog();

	void openFolderDialog();

	void openFontDialog( std::string& fontPath );

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
						   UICodeEditor* codeEditor = nullptr );

	void hideGlobalSearchBar();

	void hideSearchBar();

	void hideLocateBar();

	bool isDirTreeReady() const;

	NotificationCenter* getNotificationCenter() const;

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
	UIPopUpMenu* mColorSchemeMenu{ nullptr };
	UIPopUpMenu* mFiletypeMenu{ nullptr };
	UILinearLayout* mDocInfo{ nullptr };
	UITextView* mDocInfoText{ nullptr };
	std::vector<std::string> mRecentFiles;
	std::vector<std::string> mRecentFolders;
	AppConfig mConfig;
	UIPopUpMenu* mDocMenu{ nullptr };
	UIPopUpMenu* mViewMenu{ nullptr };
	UIPopUpMenu* mWindowMenu{ nullptr };
	UIPopUpMenu* mToolsMenu{ nullptr };
	UISplitter* mProjectSplitter{ nullptr };
	UITabWidget* mSidePanel{ nullptr };
	UICodeEditorSplitter* mEditorSplitter{ nullptr };
	std::string mInitColorScheme;
	std::map<std::string, std::string> mKeybindings;
	std::map<std::string, std::string> mKeybindingsInvert;
	std::string mConfigPath;
	std::string mKeybindingsPath;
	Float mDisplayDPI;
	std::string mResPath;
	AutoCompleteModule* mAutoCompleteModule{ nullptr };
	LinterModule* mLinterModule{ nullptr };
	FormatterModule* mFormatterModule{ nullptr };
	std::shared_ptr<ThreadPool> mThreadPool;
	std::unique_ptr<ProjectDirectoryTree> mDirTree;
	UITreeView* mProjectTreeView{ nullptr };
	std::shared_ptr<FileSystemModel> mFileSystemModel;
	size_t mMenuIconSize;
	bool mDirTreeReady{ false };
	std::unordered_set<Doc::TextDocument*> mTmpDocs;
	std::string mCurrentProject;
	FontTrueType* mFont{ nullptr };
	FontTrueType* mFontMono{ nullptr };
	efsw::FileWatcher* mFileWatcher{ nullptr };
	FileSystemListener* mFileSystemListener{ nullptr };
	std::unordered_set<efsw::WatchID> mFolderWatches;
	std::unordered_map<std::string, efsw::WatchID> mFilesFolderWatches;
	std::unique_ptr<GlobalSearchController> mGlobalSearchController;
	std::unique_ptr<DocSearchController> mDocSearchController;
	std::unique_ptr<FileLocator> mFileLocator;
	std::unique_ptr<NotificationCenter> mNotificationCenter;

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

	bool tryTabClose( UICodeEditor* editor );

	bool onCloseRequestCallback( EE::Window::Window* );

	void addRemainingTabWidgets( Node* widget );

	void createSettingsMenu();

	UIMenu* createColorSchemeMenu();

	void updateColorSchemeMenu();

	UIMenu* createFiletypeMenu();

	void updateCurrentFiletype();

	void updateEditorState();

	void saveDoc();

	void updateRecentFiles();

	void updateRecentFolders();

	void loadFolder( const std::string& path );

	UIMenu* createViewMenu();

	UIMenu* createEditMenu();

	UIMenu* createWindowMenu();

	Drawable* findIcon( const std::string& name );

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

	void removeFolderWatches();

	void createDocAlert( UICodeEditor* editor );
};

#endif // EE_TOOLS_CODEEDITOR_HPP
