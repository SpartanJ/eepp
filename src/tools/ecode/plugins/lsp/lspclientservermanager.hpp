#ifndef ECODE_LSPCLIENTMANAGER_HPP
#define ECODE_LSPCLIENTMANAGER_HPP

#include "../pluginmanager.hpp"
#include "lspclientserver.hpp"
#include "lspdefinition.hpp"
#include <eepp/core.hpp>

using namespace EE;

namespace ecode {

class LSPClientPlugin;

class LSPClientServerManager {
  public:
	LSPClientServerManager();

	void load( LSPClientPlugin*, PluginManager* pluginManager, std::vector<LSPDefinition>&& lsps );

	// async
	void run( const std::shared_ptr<TextDocument>& doc );

	// sync
	void tryRunServer( const std::shared_ptr<TextDocument>& doc );

	size_t clientCount() const;

	size_t lspCount() const;

	const std::shared_ptr<ThreadPool>& getThreadPool() const;

	void updateDirty();

	void didChangeWorkspaceFolders( const std::string& folder );

	const LSPWorkspaceFolder& getLSPWorkspaceFolder() const;

	std::vector<LSPClientServer*> getLSPClientServers( UICodeEditor* editor );

	std::vector<LSPClientServer*> getLSPClientServers( const std::shared_ptr<TextDocument>& doc );

	std::vector<LSPClientServer*> getLSPClientServers( const URI& uri );

	std::vector<LSPClientServer*> getLSPClientServers( const std::string& language );

	std::vector<LSPClientServer*> getAllRunningServers();

	std::vector<LSPClientServer*>
	getFilteredServers( const std::function<bool( LSPClientServer* )>& filter );

	LSPClientServer* getOneLSPClientServer( UICodeEditor* editor );

	LSPClientServer* getOneLSPClientServer( const std::shared_ptr<TextDocument>& doc );

	LSPClientServer* getOneLSPClientServer( const URI& uri );

	LSPClientServer* getOneLSPClientServer( const std::string& language );

	void getAndGoToLocation( const std::shared_ptr<TextDocument>& doc, const std::string& search );

	PluginManager* getPluginManager() const;

	LSPClientPlugin* getPlugin() const;

	void findAndOpenClosestURI( const std::vector<URI>& uris );

	const Time& getLSPDecayTime() const;

	void setLSPDecayTime( const Time& lSPDecayTime );

	void getSymbolReferences( std::shared_ptr<TextDocument> doc );

	void codeAction( std::shared_ptr<TextDocument> doc, const nlohmann::json& diagnostics,
					 const LSPClientServer::CodeActionHandler& h );

	void memoryUsage( std::shared_ptr<TextDocument> doc );

	const std::vector<LSPDefinition>& getLSPs() const;

	LSPDefinition getLSPForLang( const std::string& lang ) const;

	void getAndGoToLocation( const std::shared_ptr<TextDocument>& doc, const std::string& search,
							 const LSPClientServer::LocationHandler& h );

	void goToLocation( const LSPLocation& loc );

	void executeCommand( const std::shared_ptr<TextDocument>& doc, const LSPCommand& cmd );

	void applyWorkspaceEdit(
		const LSPWorkspaceEdit& edit,
		const std::function<void( const LSPApplyWorkspaceEditResponse& res )>& resCb );

	void renameSymbol( const URI& uri, const TextPosition& pos, const std::string& newName );

	bool isServerRunning( const LSPClientServer* server );

	void requestSymanticHighlighting( std::shared_ptr<TextDocument> doc );

	void rangeFormatting( std::shared_ptr<TextDocument> doc );

  protected:
	friend class LSPClientServer;
	PluginManager* mPluginManager{ nullptr };
	LSPClientPlugin* mPlugin{ nullptr };
	std::shared_ptr<ThreadPool> mThreadPool;
	std::map<String::HashType, std::unique_ptr<LSPClientServer>> mClients;
	std::set<String::HashType> mErasingClients;
	std::vector<LSPDefinition> mLSPs;
	std::map<String::HashType, std::unique_ptr<Clock>> mLSPsToClose;
	LSPWorkspaceFolder mLSPWorkspaceFolder;
	Clock mUpdateClock;
	Mutex mClientsMutex;
	Time mLSPDecayTime{ Minutes( 1 ) };

	std::vector<LSPDefinition> supportsLSP( const std::shared_ptr<TextDocument>& doc );

	std::unique_ptr<LSPClientServer> runLSPServer( const String::HashType& id,
												   const LSPDefinition& lsp,
												   const std::string& rootPath,
												   std::vector<std::string> languagesSupported );

	std::string findRootPath( const LSPDefinition& lsp, const std::shared_ptr<TextDocument>& doc );

	void closeLSPServer( const String::HashType& id );

	void sendSymbolReferenceBroadcast( const std::vector<LSPLocation>& resp );
};

} // namespace ecode

#endif // ECODE_LSPCLIENTMANAGER_HPP
