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

	void load( LSPClientPlugin*, const PluginManager* pluginManager,
			   std::vector<LSPDefinition>&& lsps );

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

	LSPClientServer* getOneLSPClientServer( UICodeEditor* editor );

	LSPClientServer* getOneLSPClientServer( const std::shared_ptr<TextDocument>& doc );

	void getAndGoToLocation( const std::shared_ptr<TextDocument>& doc, const std::string& search );

  protected:
	friend class LSPClientServer;

	LSPClientPlugin* mPlugin{ nullptr };
	std::shared_ptr<ThreadPool> mThreadPool;
	std::map<String::HashType, std::unique_ptr<LSPClientServer>> mClients;
	std::vector<LSPDefinition> mLSPs;
	std::vector<String::HashType> mLSPsToClose;
	LSPWorkspaceFolder mLSPWorkspaceFolder;

	std::vector<LSPDefinition> supportsLSP( const std::shared_ptr<TextDocument>& doc );

	std::unique_ptr<LSPClientServer> runLSPServer( const String::HashType& id,
												   const LSPDefinition& lsp,
												   const std::string& rootPath );

	std::string findRootPath( const LSPDefinition& lsp, const std::shared_ptr<TextDocument>& doc );

	void closeLSPServer( const String::HashType& id );

	void goToLocation( const LSPLocation& loc );
};

} // namespace ecode

#endif // ECODE_LSPCLIENTMANAGER_HPP
