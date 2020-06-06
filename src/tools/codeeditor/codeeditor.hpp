#ifndef EE_TOOLS_CODEEDITOR_HPP
#define EE_TOOLS_CODEEDITOR_HPP

#include <eepp/ee.hpp>

class App {
  public:
	enum class SplitDirection { Left, Right, Top, Bottom };

	~App();

	void init( const std::string& file = "" );

	void loadFileFromPath( const std::string& path, UICodeEditor* codeEditor = NULL );

	void setAppTitle( const std::string& title );

	void openFileDialog();

	void findText( String text = "" );

	void closeApp();

	void mainLoop();

	void findTextMessageBox();

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

  protected:
	EE::Window::Window* mWindow{NULL};
	UISceneNode* mUISceneNode{NULL};
	UICodeEditor* mCurEditor{NULL};
	Console* mConsole{NULL};
	std::string mWindowTitle{"eepp - Code Editor"};
	UIMessageBox* mMsgBox{NULL};
	String mLastSearch;
	UILayout* mBaseLayout{NULL};
	std::vector<UITabWidget*> mTabWidgets;
	std::map<std::string, SyntaxColorScheme> mColorSchemes;
	std::string mCurrentColorScheme;

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
};

#endif // EE_TOOLS_CODEEDITOR_HPP
