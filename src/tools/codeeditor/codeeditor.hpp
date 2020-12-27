#ifndef EE_TOOLS_CODEEDITOR_HPP
#define EE_TOOLS_CODEEDITOR_HPP

#include "appconfig.hpp"
#include "filesystemlistener.hpp"
#include "projectdirectorytree.hpp"
#include "projectsearch.hpp"
#include "uitreeviewglobalsearch.hpp"
#include <eepp/ee.hpp>
#include <efsw/efsw.hpp>

class WidgetCommandExecuter {
  public:
	typedef std::function<void()> CommandCallback;

	WidgetCommandExecuter( const KeyBindings& keybindings ) : mKeyBindings( keybindings ) {}

	void addCommand( const std::string& name, const CommandCallback& cb ) { mCommands[name] = cb; }

	void execute( const std::string& command ) {
		auto cmdIt = mCommands.find( command );
		if ( cmdIt != mCommands.end() )
			cmdIt->second();
	}

	KeyBindings& getKeyBindings() { return mKeyBindings; }

  protected:
	KeyBindings mKeyBindings;
	std::unordered_map<std::string, std::function<void()>> mCommands;

	Uint32 onKeyDown( const KeyEvent& event ) {
		std::string cmd =
			mKeyBindings.getCommandFromKeyBind( { event.getKeyCode(), event.getMod() } );
		if ( !cmd.empty() ) {
			auto cmdIt = mCommands.find( cmd );
			if ( cmdIt != mCommands.end() ) {
				cmdIt->second();
				return 1;
			}
		}
		return 0;
	}
};

class UISearchBar : public UILinearLayout, public WidgetCommandExecuter {
  public:
	static UISearchBar* New() { return eeNew( UISearchBar, () ); }

	UISearchBar() :
		UILinearLayout( "searchbar", UIOrientation::Horizontal ),
		WidgetCommandExecuter( getUISceneNode()->getWindow()->getInput() ) {}

	virtual Uint32 onKeyDown( const KeyEvent& event ) {
		return WidgetCommandExecuter::onKeyDown( event );
	}
};

class UILocateBar : public UILinearLayout, public WidgetCommandExecuter {
  public:
	static UILocateBar* New() { return eeNew( UILocateBar, () ); }
	UILocateBar() :
		UILinearLayout( "locatebar", UIOrientation::Horizontal ),
		WidgetCommandExecuter( getUISceneNode()->getWindow()->getInput() ) {}

	virtual Uint32 onKeyDown( const KeyEvent& event ) {
		return WidgetCommandExecuter::onKeyDown( event );
	}
};

class UIGlobalSearchBar : public UILinearLayout, public WidgetCommandExecuter {
  public:
	static UIGlobalSearchBar* New() { return eeNew( UIGlobalSearchBar, () ); }

	UIGlobalSearchBar() :
		UILinearLayout( "globalsearchbar", UIOrientation::Vertical ),
		WidgetCommandExecuter( getUISceneNode()->getWindow()->getInput() ) {}

	virtual Uint32 onKeyDown( const KeyEvent& event ) {
		return WidgetCommandExecuter::onKeyDown( event );
	}
};

struct SearchState {
	UICodeEditor* editor{ nullptr };
	String text;
	TextRange range = TextRange();
	bool caseSensitive{ false };
	bool wholeWord{ false };
	TextDocument::FindReplaceType type{ TextDocument::FindReplaceType::Normal };
	void reset() {
		editor = nullptr;
		range = TextRange();
		text = "";
	}
};

class AutoCompleteModule;
class LinterModule;

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

	bool findPrevText( SearchState& search );

	bool findNextText( SearchState& search );

	void closeApp();

	void mainLoop();

	void showFindView();

	void showGlobalSearch();

	void showLocateBar();

	bool replaceSelection( SearchState& search, const String& replacement );

	int replaceAll( SearchState& search, const String& replace );

	bool findAndReplace( SearchState& search, const String& replace );

	void runCommand( const std::string& command );

	void loadConfig();

	void saveConfig();

	std::string getKeybind( const std::string& command );

	std::vector<std::string> getUnlockedCommands();

	void saveAll();

  protected:
	EE::Window::Window* mWindow{ nullptr };
	UISceneNode* mUISceneNode{ nullptr };
	Console* mConsole{ nullptr };
	std::string mWindowTitle{ "ecode" };
	String mLastSearch;
	UILayout* mMainLayout{ nullptr };
	UILayout* mBaseLayout{ nullptr };
	UISearchBar* mSearchBarLayout{ nullptr };
	UILocateBar* mLocateBarLayout{ nullptr };
	UILocateBar* mGlobalSearchBarLayout{ nullptr };
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
	SearchState mSearchState;
	Float mDisplayDPI;
	std::string mResPath;
	AutoCompleteModule* mAutoCompleteModule{ nullptr };
	LinterModule* mLinterModule{ nullptr };
	std::shared_ptr<ThreadPool> mThreadPool;
	std::unique_ptr<ProjectDirectoryTree> mDirTree;
	UITreeView* mProjectTreeView{ nullptr };
	std::shared_ptr<FileSystemModel> mFileSystemModel;
	UITableView* mLocateTable{ nullptr };
	UITextInput* mLocateInput{ nullptr };
	UITreeViewGlobalSearch* mGlobalSearchTree{ nullptr };
	UITextInput* mGlobalSearchInput{ nullptr };
	UIDropDownList* mGlobalSearchHistoryList{ nullptr };
	Uint32 mGlobalSearchHistoryOnItemSelectedCb{ 0 };
	std::deque<std::pair<std::string, std::shared_ptr<ProjectSearch::ResultModel>>>
		mGlobalSearchHistory;
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

	void saveAllProcess();

	void initLocateBar();

	void initProjectTreeView( const std::string& path );

	void loadDirTree( const std::string& path );

	void showSidePanel( bool show );

	void onFileDropped( String file );

	void onTextDropped( String text );

	void updateEditorTitle( UICodeEditor* editor );

	void updateEditorTabTitle( UICodeEditor* editor );

	std::string titleFromEditor( UICodeEditor* editor );

	bool tryTabClose( UICodeEditor* editor );

	bool onCloseRequestCallback( EE::Window::Window* );

	void initSearchBar();

	void initGlobalSearchBar();

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

	void updateDocInfo( TextDocument& doc );

	void setFocusEditorOnClose( UIMessageBox* msgBox );

	void updateLocateBar();

	void updateLocateTable();

	void updateGlobalSearchBar();

	UIPopUpMenu* createToolsMenu();

	void hideGlobalSearchBar();

	void hideSearchBar();

	void hideLocateBar();

	bool trySendUnlockedCmd( const KeyEvent& keyEvent );

	void goToLine();

	void loadCurrentDirectory();

	void toggleSettingsMenu();

	FontTrueType* loadFont( const std::string& name, std::string fontPath,
							const std::string& fallback = "" );

	void closeFolder();

	void closeEditors();

	void updateGlobalSearchBarResults( const std::string& search,
									   std::shared_ptr<ProjectSearch::ResultModel> model );

	void switchSidePanel();

	void removeFolderWatches();

	void createDocAlert( UICodeEditor* editor );
};

#endif // EE_TOOLS_CODEEDITOR_HPP
