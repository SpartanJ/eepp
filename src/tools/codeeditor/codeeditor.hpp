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

class App {
  public:
	struct Config {
		struct {
			Float pixelDensity{0};
			Sizei size{1280, 720};
			bool maximized{false};
		} window;
		struct {
			std::string colorScheme{"lite"};
			Float fontSize{11};
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
			int indentWidth{4};
			int tabWidth{4};
			int lineBreakingColumn{100};
		} editor;
		struct {
			Float fontSize{11};
		} ui;
	};

	enum class SplitDirection { Left, Right, Top, Bottom };

	~App();

	void init( const std::string& file, const Float& pidelDensity );

	bool loadFileFromPath( const std::string& path, UICodeEditor* codeEditor = nullptr );

	void setAppTitle( const std::string& title );

	void openFileDialog();

	void saveFileDialog();

	void findPrevText( String text = "", const bool& caseSensitive = true );

	void findNextText( String text = "", const bool& caseSensitive = true );

	void closeApp();

	void mainLoop();

	void showFindView();

	UICodeEditor* createCodeEditor();

	UITabWidget* tabWidgetFromEditor( UICodeEditor* editor );

	UISplitter* splitterFromEditor( UICodeEditor* editor );

	std::pair<UITab*, UICodeEditor*> createCodeEditorInTabWidget( UITabWidget* tabWidget );

	UITabWidget* createEditorWithTabWidget( Node* parent );

	void splitEditor( const SplitDirection& direction, UICodeEditor* editor );

	void focusSomeEditor( Node* searchFrom = nullptr );

	void switchToTab( Int32 index );

	UITabWidget* findPreviousSplit( UICodeEditor* editor );

	void switchPreviousSplit( UICodeEditor* editor );

	UITabWidget* findNextSplit( UICodeEditor* editor );

	void switchNextSplit( UICodeEditor* editor );

	void applyColorScheme( const SyntaxColorScheme& colorScheme );

	void replaceSelection( const String& replacement );

	void replaceAll( String find, const String& replace, const bool& caseSensitive );

	void findAndReplace( String find, String replace, const bool& caseSensitive );

	void runCommand( const std::string& command );

	void loadConfig();

	void saveConfig();

	void loadFileFromPathInNewTab( const std::string& path );

	void setCurrentEditor( UICodeEditor* editor );

  protected:
	EE::Window::Window* mWindow{nullptr};
	UISceneNode* mUISceneNode{nullptr};
	UICodeEditor* mCurEditor{nullptr};
	Console* mConsole{nullptr};
	std::string mWindowTitle{"ecode"};
	String mLastSearch;
	UILayout* mBaseLayout{nullptr};
	UISearchBar* mSearchBarLayout{nullptr};
	std::vector<UITabWidget*> mTabWidgets;
	std::map<std::string, SyntaxColorScheme> mColorSchemes;
	std::string mCurrentColorScheme;
	UIPopUpMenu* mSettingsMenu{nullptr};
	UITextView* mSettingsButton{nullptr};
	UIPopUpMenu* mColorSchemeMenu{nullptr};
	UIPopUpMenu* mFiletypeMenu{nullptr};
	IniFile mIni;
	IniFile mIniState;
	std::vector<std::string> mRecentFiles;
	Config mConfig;
	UIPopUpMenu* mDocMenu{nullptr};
	UIPopUpMenu* mViewMenu{nullptr};

	void onFileDropped( String file );

	void onTextDropped( String text );

	void updateEditorTitle( UICodeEditor* editor );

	std::string titleFromEditor( UICodeEditor* editor );

	bool tryTabClose( UICodeEditor* editor );

	bool onCloseRequestCallback( EE::Window::Window* );

	void closeEditorTab( UICodeEditor* editor );

	void onTabClosed( const TabEvent* tabEvent );

	void closeSplitter( UISplitter* splitter );

	void closeTabWidgets( UISplitter* splitter );

	void initSearchBar();

	void addRemainingTabWidgets( Node* widget );

	void createSettingsMenu();

	UIMenu* createColorSchemeMenu();

	void updateColorSchemeMenu();

	void setColorScheme( const std::string& name );

	UIMenu* createFiletypeMenu();

	void updateCurrentFiletype();

	void updateEditorState();

	void saveDoc();

	void removeUnusedTab( UITabWidget* tabWidget );

	void updateRecentFiles();

	void forEachEditor( std::function<void( UICodeEditor* )> run );

	UIMenu* createViewMenu();

	UIMenu* createEditMenu();

	Drawable* findIcon( const std::string& name );

	UIMenu* createDocumentMenu();

	void updateDocumentMenu();

	void zoomIn();

	void zoomOut();

	void zoomReset();
};

#endif // EE_TOOLS_CODEEDITOR_HPP
