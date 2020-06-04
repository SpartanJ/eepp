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

  protected:
	EE::Window::Window* mWindow{NULL};
	UISceneNode* mUISceneNode{NULL};
	UICodeEditor* mCurEditor{NULL};
	Console* mConsole{NULL};
	std::string mWindowTitle{"eepp - Code Editor"};
	UIMessageBox* mMsgBox{NULL};
	String mLastSearch;
	UILayout* mBaseLayout{NULL};

	void onFileDropped( String file );

	void onTextDropped( String text );

	void updateEditorTitle( UICodeEditor* editor );

	std::string titleFromEditor( UICodeEditor* editor );

	bool onTabCloseRequestCallback( EE::Window::Window* );

	bool onCloseRequestCallback( EE::Window::Window* );

	void closeCurrrentTab();

	void onTabClosed( const TabEvent* tabEvent );
};

#endif // EE_TOOLS_CODEEDITOR_HPP
