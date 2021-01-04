#ifndef FILELOCATOR_HPP
#define FILELOCATOR_HPP

#include "widgetcommandexecuter.hpp"
#include <eepp/ee.hpp>

class App;

class FileLocator {
  public:
	FileLocator( UICodeEditorSplitter* editorSplitter, UISceneNode* sceneNode, App* app );

	void initLocateBar( UILocateBar* locateBar, UITextInput* locateInput );

	void showLocateBar();

	void hideLocateBar();

	void goToLine();

	void updateLocateTable();

  protected:
	UILocateBar* mLocateBarLayout{ nullptr };
	UITableView* mLocateTable{ nullptr };
	UITextInput* mLocateInput{ nullptr };
	UICodeEditorSplitter* mEditorSplitter{ nullptr };
	UISceneNode* mUISceneNode{ nullptr };
	App* mApp{ nullptr };

	void updateLocateBar();
};

#endif // FILELOCATOR_HPP
