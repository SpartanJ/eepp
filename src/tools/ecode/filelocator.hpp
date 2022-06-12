#ifndef ECODE_FILELOCATOR_HPP
#define ECODE_FILELOCATOR_HPP

#include "widgetcommandexecuter.hpp"
#include <eepp/ee.hpp>

namespace ecode {

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

} // namespace ecode

#endif // ECODE_FILELOCATOR_HPP
