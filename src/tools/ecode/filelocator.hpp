#ifndef ECODE_FILELOCATOR_HPP
#define ECODE_FILELOCATOR_HPP

#include "commandpalette.hpp"
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

	void showCommandPalette();

	void goToLine();

	void updateFilesTable();

	void updateCommandPaletteTable();

	void showLocateTable();

  protected:
	UILocateBar* mLocateBarLayout{ nullptr };
	UITableView* mLocateTable{ nullptr };
	UITextInput* mLocateInput{ nullptr };
	UICodeEditorSplitter* mSplitter{ nullptr };
	UISceneNode* mUISceneNode{ nullptr };
	App* mApp{ nullptr };
	CommandPalette mCommandPalette;

	void updateLocateBar();

	void showBar();
};

} // namespace ecode

#endif // ECODE_FILELOCATOR_HPP
