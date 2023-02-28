#ifndef ECODE_FILELOCATOR_HPP
#define ECODE_FILELOCATOR_HPP

#include "commandpalette.hpp"
#include "plugins/pluginmanager.hpp"
#include "widgetcommandexecuter.hpp"
#include <eepp/ee.hpp>

namespace ecode {

class App;
class LSPSymbolInfoModel;

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

	void showWorkspaceSymbol();

  protected:
	UILocateBar* mLocateBarLayout{ nullptr };
	UITableView* mLocateTable{ nullptr };
	UITextInput* mLocateInput{ nullptr };
	UICodeEditorSplitter* mSplitter{ nullptr };
	UISceneNode* mUISceneNode{ nullptr };
	std::shared_ptr<LSPSymbolInfoModel> mWorkspaceSymbolModel{ nullptr };
	std::string mWorkspaceSymbolQuery;
	App* mApp{ nullptr };
	CommandPalette mCommandPalette;

	void updateLocateBar();

	void showBar();

	PluginRequestHandle processResponse( const PluginMessage& msg );

	void requestWorkspaceSymbol();

	void updateWorkspaceSymbol( const std::vector<LSPSymbolInformation>& info );
};

} // namespace ecode

#endif // ECODE_FILELOCATOR_HPP
