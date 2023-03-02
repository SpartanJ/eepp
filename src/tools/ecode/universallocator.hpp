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

	void showDocumentSymbol();

	void onCodeEditorFocusChange( UICodeEditor* editor );

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

	void updateLocateBar();

	void showBar();

	PluginRequestHandle processResponse( const PluginMessage& msg );

	void requestWorkspaceSymbol();

	void updateWorkspaceSymbol( const LSPSymbolInformationList& info );

	void requestDocumentSymbol();

	void updateDocumentSymbol( const LSPSymbolInformationList& info );

	std::string getCurDocURI();

	std::vector<ProjectDirectoryTree::CommandInfo> getLocatorCommands() const;

	std::shared_ptr<LSPSymbolInfoModel> emptyModel( const String& defTxt,
													const std::string& query = "" );

	void asyncFuzzyMatchTextDocumentSymbol(
		const LSPSymbolInformationList& list, const std::string& query, const size_t& limit,
		std::function<void( std::shared_ptr<LSPSymbolInfoModel> )> cb );
};

} // namespace ecode

#endif // ECODE_UNIVERSALLOCATOR_HPP
