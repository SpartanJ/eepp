#ifndef EE_TOOLS_CODEEDITOR_HPP
#define EE_TOOLS_CODEEDITOR_HPP

#include <eepp/ee.hpp>

class UISearchBar : public UILinearLayout {
  public:
	typedef std::function<void()> CommandCallback;
	static UISearchBar* New() { return eeNew( UISearchBar, () ); }
	UISearchBar() :
		UILinearLayout( "searchbar", UIOrientation::Horizontal ),
		mKeyBindings( getUISceneNode()->getWindow()->getInput() ) {}
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
				return 0;
			}
		}
		return 1;
	}
};

struct UIConfig {
	StyleSheetLength fontSize{12, StyleSheetLength::Dp};
};

struct WindowConfig {
	Float pixelDensity{0};
	Sizei size{1280, 720};
	bool maximized{false};
};

struct AppConfig {
	WindowConfig window;
	UICodeEditorSplitter::CodeEditorConfig editor;
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

class App : public UICodeEditorSplitter::Client {
  public:
	~App();

	void init( const std::string& file, const Float& pidelDensity );

	bool loadFileFromPath( const std::string& path, UICodeEditor* codeEditor = nullptr );

	void setAppTitle( const std::string& title );

	void openFileDialog();

	void saveFileDialog();

	void findPrevText( SearchState& search );

	void findNextText( SearchState& search );

	void closeApp();

	void mainLoop();

	void showFindView();

	void replaceSelection( SearchState& search, const String& replacement );

	void replaceAll( SearchState& search, const String& replace );

	void findAndReplace( SearchState& search, const String& replace );

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
	UILayout* mBaseLayout{nullptr};
	UISearchBar* mSearchBarLayout{nullptr};
	UIPopUpMenu* mSettingsMenu{nullptr};
	UITextView* mSettingsButton{nullptr};
	UIPopUpMenu* mColorSchemeMenu{nullptr};
	UIPopUpMenu* mFiletypeMenu{nullptr};
	IniFile mIni;
	IniFile mIniState;
	std::vector<std::string> mRecentFiles;
	AppConfig mConfig;
	UIPopUpMenu* mDocMenu{nullptr};
	UIPopUpMenu* mViewMenu{nullptr};
	UICodeEditorSplitter* mEditorSplitter{nullptr};
	std::string mInitColorScheme;
	std::map<std::string, std::string> mKeybindings;
	std::map<std::string, std::string> mKeybindingsInvert;
	std::string mConfigPath;
	std::string mKeybindingsPath;
	SearchState mSearchState;
	Float mDisplayDPI;

	void onFileDropped( String file );

	void onTextDropped( String text );

	void updateEditorTitle( UICodeEditor* editor );

	std::string titleFromEditor( UICodeEditor* editor );

	bool tryTabClose( UICodeEditor* editor );

	bool onCloseRequestCallback( EE::Window::Window* );

	void initSearchBar();

	void addRemainingTabWidgets( Node* widget );

	void createSettingsMenu();

	UIMenu* createColorSchemeMenu();

	void updateColorSchemeMenu();

	UIMenu* createFiletypeMenu();

	void updateCurrentFiletype();

	void updateEditorState();

	void saveDoc();

	void updateRecentFiles();

	UIMenu* createViewMenu();

	UIMenu* createEditMenu();

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

	const UICodeEditorSplitter::CodeEditorConfig& getCodeEditorConfig() const;

	void onCodeEditorCreated( UICodeEditor*, TextDocument& doc );

	void onDocumentSelectionChange( UICodeEditor* editor, TextDocument& );

	void onCodeEditorFocusChange( UICodeEditor* editor );
};

#endif // EE_TOOLS_CODEEDITOR_HPP
