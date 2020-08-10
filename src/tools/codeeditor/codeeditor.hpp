#ifndef EE_TOOLS_CODEEDITOR_HPP
#define EE_TOOLS_CODEEDITOR_HPP

#include "projectdirectorytree.hpp"
#include <eepp/ee.hpp>

class UISearchBar : public UILinearLayout {
  public:
	typedef std::function<void()> CommandCallback;
	static UISearchBar* New() { return eeNew( UISearchBar, () ); }
	UISearchBar() :
		UILinearLayout( "searchbar", UIOrientation::Horizontal ),
		mKeyBindings( getUISceneNode()->getWindow()->getInput() ) {}

  public:
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
			mKeyBindings.getCommandFromKeyBind( {event.getKeyCode(), event.getMod()} );
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

class UILocateBar : public UILinearLayout {
  public:
	typedef std::function<void()> CommandCallback;
	static UILocateBar* New() { return eeNew( UILocateBar, () ); }
	UILocateBar() :
		UILinearLayout( "locatebar", UIOrientation::Horizontal ),
		mKeyBindings( getUISceneNode()->getWindow()->getInput() ) {}

  public:
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
			mKeyBindings.getCommandFromKeyBind( {event.getKeyCode(), event.getMod()} );
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

class UIGlobalSearchBar : public UILinearLayout {
  public:
	typedef std::function<void()> CommandCallback;
	static UIGlobalSearchBar* New() { return eeNew( UIGlobalSearchBar, () ); }
	UIGlobalSearchBar() :
		UILinearLayout( "globalsearchbar", UIOrientation::Vertical ),
		mKeyBindings( getUISceneNode()->getWindow()->getInput() ) {}

  public:
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
			mKeyBindings.getCommandFromKeyBind( {event.getKeyCode(), event.getMod()} );
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

struct UIConfig {
	StyleSheetLength fontSize{12, StyleSheetLength::Dp};
	bool showSidePanel{true};
};

struct WindowConfig {
	Float pixelDensity{0};
	Sizei size{1280, 720};
	std::string winIcon;
	bool maximized{false};
	std::string panelPartition;
};

struct CodeEditorConfig {
	std::string colorScheme{"lite"};
	StyleSheetLength fontSize{12, StyleSheetLength::Dp};
	bool showLineNumbers{true};
	bool showWhiteSpaces{true};
	bool highlightMatchingBracket{true};
	bool horizontalScrollbar{false};
	bool highlightCurrentLine{true};
	bool trimTrailingWhitespaces{false};
	bool forceNewLineAtEndOfFile{false};
	bool autoDetectIndentType{true};
	bool writeUnicodeBOM{false};
	bool indentSpaces{false};
	bool windowsLineEndings{false};
	bool highlightSelectionMatch{true};
	bool colorPickerSelection{false};
	bool colorPreview{false};
	bool autoComplete{true};
	bool showDocInfo{true};
	int indentWidth{4};
	int tabWidth{4};
	int lineBreakingColumn{100};
};

struct AppConfig {
	WindowConfig window;
	CodeEditorConfig editor;
	UIConfig ui;
};

struct SearchState {
	UICodeEditor* editor{nullptr};
	String text;
	TextRange range = TextRange();
	bool caseSensitive{false};
	void reset() {
		editor = nullptr;
		range = TextRange();
		text = "";
	}
};

class AutoCompleteModule;

class App : public UICodeEditorSplitter::Client {
  public:
	App();

	~App();

	void init( const std::string& file, const Float& pidelDensity );

	void setAppTitle( const std::string& title );

	void openFileDialog();

	void openFolderDialog();

	void saveFileDialog();

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

  protected:
	EE::Window::Window* mWindow{nullptr};
	UISceneNode* mUISceneNode{nullptr};
	Console* mConsole{nullptr};
	std::string mWindowTitle{"ecode"};
	String mLastSearch;
	UILayout* mMainLayout{nullptr};
	UILayout* mBaseLayout{nullptr};
	UISearchBar* mSearchBarLayout{nullptr};
	UILocateBar* mLocateBarLayout{nullptr};
	UILocateBar* mGlobalSearchBarLayout{nullptr};
	UIPopUpMenu* mSettingsMenu{nullptr};
	UITextView* mSettingsButton{nullptr};
	UIPopUpMenu* mColorSchemeMenu{nullptr};
	UIPopUpMenu* mFiletypeMenu{nullptr};
	UILinearLayout* mDocInfo{nullptr};
	UITextView* mDocInfoText{nullptr};
	IniFile mIni;
	IniFile mIniState;
	std::vector<std::string> mRecentFiles;
	std::vector<std::string> mRecentFolders;
	AppConfig mConfig;
	UIPopUpMenu* mDocMenu{nullptr};
	UIPopUpMenu* mViewMenu{nullptr};
	UIPopUpMenu* mWindowMenu{nullptr};
	UIPopUpMenu* mToolsMenu{nullptr};
	UISplitter* mProjectSplitter{nullptr};
	UITabWidget* mSidePanel{nullptr};
	UICodeEditorSplitter* mEditorSplitter{nullptr};
	std::string mInitColorScheme;
	std::map<std::string, std::string> mKeybindings;
	std::map<std::string, std::string> mKeybindingsInvert;
	std::string mConfigPath;
	std::string mKeybindingsPath;
	SearchState mSearchState;
	Float mDisplayDPI;
	std::string mResPath;
	AutoCompleteModule* mAutoCompleteModule{nullptr};
	std::shared_ptr<ThreadPool> mThreadPool;
	std::unique_ptr<ProjectDirectoryTree> mDirTree;
	UITreeView* mProjectTreeView{nullptr};
	UITableView* mLocateTable{nullptr};
	UITextInput* mLocateInput{nullptr};
	UITreeView* mGlobalSearchTree{nullptr};
	UITextInput* mGlobalSearchInput;
	size_t mMenuIconSize;
	bool mDirTreeReady{false};

	void initLocateBar();

	void initProjectTreeView( const std::string& path );

	void loadDirTree( const std::string& path );

	void showSidePanel( bool show );

	void onFileDropped( String file );

	void onTextDropped( String text );

	void updateEditorTitle( UICodeEditor* editor );

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

	void onDocumentLoaded( UICodeEditor* codeEditor, const std::string& path );

	const CodeEditorConfig& getCodeEditorConfig() const;

	void onCodeEditorCreated( UICodeEditor*, TextDocument& doc );

	void onDocumentSelectionChange( UICodeEditor* editor, TextDocument& );

	void onDocumentCursorPosChange( UICodeEditor* editor, TextDocument& );

	void onCodeEditorFocusChange( UICodeEditor* editor );

	bool setAutoComplete( bool enable );

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
};

#endif // EE_TOOLS_CODEEDITOR_HPP
