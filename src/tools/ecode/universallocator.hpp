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
	struct LocatorProvider {
		LocatorProvider(
			String&& symbol, String&& description, std::function<bool( const String& )> switchFn,
			std::function<void( const Variant& var, const ModelEvent* modelEvent )> openFn,
			std::function<bool( const String& )> pressEnterFn = nullptr,
			bool projectNeeded = true ) :
			symbol( std::move( symbol ) ),
			symbolTrigger( this->symbol + " " ),
			description( std::move( description ) ),
			switchFn( std::move( switchFn ) ),
			openFn( std::move( openFn ) ),
			pressEnterFn( std::move( pressEnterFn ) ),
			projectNeeded( projectNeeded ) {}

		String symbol;
		String symbolTrigger;
		String description;
		std::function<bool( const String& )> switchFn;
		std::function<void( const Variant&, const ModelEvent* )> openFn;
		std::function<bool( const String& )> pressEnterFn{ nullptr };
		bool projectNeeded{ true };
	};

	UniversalLocator( UICodeEditorSplitter* editorSplitter, UISceneNode* sceneNode, App* app );

	void initLocateBar( UILocateBar* locateBar, UITextInput* locateInput );

	void showLocateBar( bool useGlob = false );

	void hideLocateBar();

	void toggleLocateBar();

	void showCommandPalette();

	void goToLine();

	void updateFilesTable( bool useGlob = false );

	void updateCommandPaletteTable();

	void showLocateTable();

	void showLocateTableGlob();

	void showWorkspaceSymbol();

	void showDocumentSymbol();

	void onCodeEditorFocusChange( UICodeEditor* editor );

	void showOpenDocuments();

	void showSwitchBuild();

	void showSwitchBuildType();

	void showSwitchRunTarget();

	void showSwitchFileType();

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
	std::vector<LocatorProvider> mLocatorProviders;

	void updateLocateBar();

	void updateLocateBarSync();

	void showBar();

	PluginRequestHandle processResponse( const PluginMessage& msg );

	void updateWorkspaceSymbol( const LSPSymbolInformationList& info );

	void updateDocumentSymbol( const LSPSymbolInformationList& info );

	void updateOpenDocumentsTable();

	void updateSwitchBuildTable();

	void updateSwitchBuildTypeTable();

	void updateSwitchRunTargetTable();

	void updateSwitchFileTypeTable();

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

	std::shared_ptr<ItemListOwnerModel<std::string>> openRunTargetModel( const std::string& match );

	std::shared_ptr<ItemListOwnerModel<std::string>> openFileTypeModel( const std::string& match );

	bool findCapability( PluginCapability );

	String getDefQueryText( PluginCapability );

	nlohmann::json pluginID( const PluginIDType& id );

	bool isCommand( const std::string& filename );

	std::optional<LocatorProvider> getLocator( const String& txt );

	bool isLocator( const String& txt );

	bool tryLocator( const String& txt );

	bool openLocator( const String& txt, const Variant& vName, const ModelEvent* modelEvent );

	bool pressEnterLocator( const String& txt );
};

} // namespace ecode

#endif // ECODE_UNIVERSALLOCATOR_HPP
