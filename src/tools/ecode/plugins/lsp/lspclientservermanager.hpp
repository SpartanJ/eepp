#ifndef ECODE_LSPCLIENTMANAGER_HPP
#define ECODE_LSPCLIENTMANAGER_HPP

#include "../pluginmanager.hpp"
#include "lspclientserver.hpp"
#include "lspdefinition.hpp"
#include <eepp/core.hpp>

using namespace EE;

namespace ecode {

class LSPClientServerManager {
  public:
	LSPClientServerManager();

	void load( const PluginManager* pluginManager, std::vector<LSPDefinition>&& lsps );

	void run( const std::shared_ptr<TextDocument>& doc );

	size_t clientCount() const;

  protected:
	std::shared_ptr<ThreadPool> mPool;
	std::map<String::HashType, std::shared_ptr<LSPClientServer>> mClients;
	std::vector<LSPDefinition> mLSPs;

	std::vector<LSPDefinition> supportsLSP( const std::shared_ptr<TextDocument>& doc );

	std::shared_ptr<LSPClientServer> runLSPServer( const LSPDefinition& lsp,
												   const std::string& rootPath );

	std::string findRootPath( const LSPDefinition& lsp, const std::shared_ptr<TextDocument>& doc );

	void tryRunServer( const std::shared_ptr<TextDocument>& doc );
};

} // namespace ecode

#endif // ECODE_LSPCLIENTMANAGER_HPP
