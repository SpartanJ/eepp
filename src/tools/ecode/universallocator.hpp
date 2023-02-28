#ifndef ECODE_UNIVERSALLOCATOR_HPP
#define ECODE_UNIVERSALLOCATOR_HPP

#include "commandpalette.hpp"
#include "plugins/pluginmanager.hpp"
#include "projectdirectorytree.hpp"
#include "widgetcommandexecuter.hpp"
#include <eepp/ee.hpp>

namespace ecode {

class App;
class LSPSymbolInfoModel;

class UniversalLocator {
  public:
	UniversalLocator( UICodeEditorSplitter* editorSplitter, UISceneNode* sceneNode, App* app );

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

	std::vector<ProjectDirectoryTree::CommandInfo> getLocatorCommands() const;
};

} // namespace ecode

#endif // ECODE_UNIVERSALLOCATOR_HPP
