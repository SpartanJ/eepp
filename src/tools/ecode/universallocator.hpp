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
class OpenDocumentsModel;

class UniversalLocator {
  public:
	UniversalLocator( UICodeEditorSplitter* editorSplitter, UISceneNode* sceneNode, App* app );

	void initLocateBar( UILocateBar* locateBar, UITextInput* locateInput );

	void showLocateBar();

	void hideLocateBar();

	void toggleLocateBar();

	void showCommandPalette();

	void goToLine();

	void updateFilesTable();

	void updateCommandPaletteTable();

	void showLocateTable();

	void showWorkspaceSymbol();

	void showDocumentSymbol();

	void onCodeEditorFocusChange( UICodeEditor* editor );

	void showOpenDocuments();

	void showSwitchBuild();

	void showSwitchBuildType();

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
	std::string mCurDocURI;
	std::string mCurDocQuery;
	std::shared_ptr<LSPSymbolInfoModel> mTextDocumentSymbolModel{ nullptr };
	std::shared_ptr<OpenDocumentsModel> mOpenDocumentsModel{ nullptr };
	PluginIDType mQueryWorkspaceLastId;

	void updateLocateBar();

	void updateLocateBarSync();

	void showBar();

	PluginRequestHandle processResponse( const PluginMessage& msg );

	void updateWorkspaceSymbol( const LSPSymbolInformationList& info );

	void updateDocumentSymbol( const LSPSymbolInformationList& info );

	void updateOpenDocumentsTable();

	void updateSwitchBuildTable();

	void updateSwitchBuildTypeTable();

	void requestWorkspaceSymbol();

	void requestDocumentSymbol();

	std::string getCurDocURI();

	std::vector<ProjectDirectoryTree::CommandInfo> getLocatorCommands() const;

	std::shared_ptr<LSPSymbolInfoModel> emptyModel( const String& defTxt,
													const std::string& query = "" );

	void asyncFuzzyMatchTextDocumentSymbol(
		const LSPSymbolInformationList& list, const std::string& query, const size_t& limit,
		std::function<void( std::shared_ptr<LSPSymbolInfoModel> )> cb );

	std::shared_ptr<FileListModel> openDocumentsModel( const std::string& match );

	void focusOrLoadFile( const std::string& path, const TextRange& range = {} );

	std::shared_ptr<ItemListOwnerModel<std::string>> openBuildModel( const std::string& match );

	std::shared_ptr<ItemListOwnerModel<std::string>> openBuildTypeModel( const std::string& match );

	bool findCapability( PluginCapability );

	String getDefQueryText( PluginCapability );

	nlohmann::json pluginID( const PluginIDType& id );
};

} // namespace ecode

#endif // ECODE_UNIVERSALLOCATOR_HPP
