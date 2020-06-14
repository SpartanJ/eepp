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
	enum class SplitDirection { Left, Right, Top, Bottom };

	~App();

	void init( const std::string& file = "" );

	bool loadFileFromPath( const std::string& path, UICodeEditor* codeEditor = NULL );

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

	void focusSomeEditor( Node* searchFrom = NULL );

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

  protected:
	EE::Window::Window* mWindow{NULL};
	UISceneNode* mUISceneNode{NULL};
	UICodeEditor* mCurEditor{NULL};
	Console* mConsole{NULL};
	std::string mWindowTitle{"ecode"};
	UIMessageBox* mMsgBox{NULL};
	String mLastSearch;
	UILayout* mBaseLayout{NULL};
	UISearchBar* mSearchBarLayout{NULL};
	std::vector<UITabWidget*> mTabWidgets;
	std::map<std::string, SyntaxColorScheme> mColorSchemes;
	std::string mCurrentColorScheme;
	UIPopUpMenu* mSettingsMenu;
	UITextView* mSettingsButton;
	UIPopUpMenu* mColorSchemeMenu;
	UIPopUpMenu* mFiletypeMenu;
	UITheme* mTheme;

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
};

#endif // EE_TOOLS_CODEEDITOR_HPP
