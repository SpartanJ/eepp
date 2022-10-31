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

	void run( const std::shared_ptr<TextDocument>& doc );

	size_t clientCount() const;

	const std::shared_ptr<ThreadPool>& getThreadPool() const;

	void updateDirty();

	void followSymbolUnderCursor( TextDocument* doc );

  protected:
	friend class LSPClientServer;

	LSPClientPlugin* mPlugin{ nullptr };
	std::shared_ptr<ThreadPool> mThreadPool;
	std::map<String::HashType, std::unique_ptr<LSPClientServer>> mClients;
	std::vector<LSPDefinition> mLSPs;

	std::vector<LSPDefinition> supportsLSP( const std::shared_ptr<TextDocument>& doc );

	std::unique_ptr<LSPClientServer> runLSPServer( const String::HashType& id,
												   const LSPDefinition& lsp,
												   const std::string& rootPath );

	std::string findRootPath( const LSPDefinition& lsp, const std::shared_ptr<TextDocument>& doc );

	void tryRunServer( const std::shared_ptr<TextDocument>& doc );

	void notifyClose( const String::HashType& id );

	void goToLocation( const LSPLocation& loc );
};

} // namespace ecode

#endif // ECODE_LSPCLIENTMANAGER_HPP
