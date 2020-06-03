#ifndef EE_TOOLS_CODEEDITOR_HPP
#define EE_TOOLS_CODEEDITOR_HPP

#include <eepp/ee.hpp>

class App {
  public:
	~App();

	void init( const std::string& file = "" );

	void loadFileFromPath( const std::string& path, UICodeEditor* codeEditor = NULL );

	void setAppTitle( const std::string& title );

	void openFileDialog();

	void findText( String text = "" );

	void closeApp();

	void mainLoop();

	void findTextMessageBox();

	bool onCloseRequestCallback( EE::Window::Window* );

	UICodeEditor* createCodeEditor();

	UISplitter* createEditorSplitter( Node* parent );

	UITabWidget* tabWidgetFromEditor( UICodeEditor* editor );

	std::pair<UITab*, UICodeEditor*> createCodeEditorInTabWidget( UITabWidget* tabWidget );

	UITabWidget* createEditorWithTabWidget( UISplitter* splitter );

	void splitEditor( const UIOrientation& orientation );

  protected:
	EE::Window::Window* mWindow = NULL;
	UISceneNode* mUISceneNode = NULL;
	UICodeEditor* mCurEditor = NULL;
	Console* mConsole = NULL;
	std::string mWindowTitle = "eepp - Code Editor";
	bool mDocDirtyState = false;
	UIMessageBox* mMsgBox = NULL;
	String mLastSearch;
	UILayout* mBaseLayout;
};

#endif // EE_TOOLS_CODEEDITOR_HPP
