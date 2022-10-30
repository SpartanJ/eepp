#include "lspclientservermanager.hpp"
#include <algorithm>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/luapattern.hpp>

namespace ecode {

LSPClientServerManager::LSPClientServerManager() {}

void LSPClientServerManager::load( const PluginManager* pluginManager,
								   std::vector<LSPDefinition>&& lsps ) {
	mPool = pluginManager->getThreadPool();
	mLSPs = lsps;
}

std::vector<LSPDefinition>
LSPClientServerManager::supportsLSP( const std::shared_ptr<TextDocument>& doc ) {
	if ( !doc->hasFilepath() && doc->getLoadingFilePath().empty() )
		return {};
	std::string fileName( FileSystem::fileNameFromPath(
		doc->getFilePath().empty() ? doc->getLoadingFilePath() : doc->getFilePath() ) );
	const auto& def = doc->getSyntaxDefinition();
	std::vector<LSPDefinition> lsps;

	for ( auto& lsp : mLSPs ) {
		for ( auto& ext : lsp.filePatterns ) {
			if ( LuaPattern::find( fileName, ext ).isValid() ) {
				lsps.push_back( lsp );
				break;
			}
			auto& files = def.getFiles();
			if ( std::find( files.begin(), files.end(), ext ) != files.end() ) {
				lsps.push_back( lsp );
				break;
			}
		}
	}

	return lsps;
}

std::shared_ptr<LSPClientServer>
LSPClientServerManager::runLSPServer( const LSPDefinition& lsp, const std::string& rootPath ) {
	auto server = std::make_shared<LSPClientServer>( lsp, rootPath );
	server->start();
	return server;
}

std::string LSPClientServerManager::findRootPath( const LSPDefinition& lsp,
												  const std::shared_ptr<TextDocument>& doc ) {
	if ( lsp.rootIndicationFileNames.empty() || !doc->hasFilepath() )
		return "";
	std::string rootPath( doc->getFileInfo().getDirectoryPath() );
	std::string lRootPath;
	FileSystem::dirAddSlashAtEnd( rootPath );

	while ( rootPath != lRootPath ) {
		for ( const auto& fileName : lsp.rootIndicationFileNames )
			if ( FileSystem::fileExists( rootPath + fileName ) )
				return rootPath;

		lRootPath = rootPath;
		rootPath = FileSystem::removeLastFolderFromPath( rootPath );
	}

	return "";
}

void LSPClientServerManager::tryRunServer( const std::shared_ptr<TextDocument>& doc ) {
	auto lsps = supportsLSP( doc );
	if ( lsps.empty() )
		return;

	for ( const auto& lsp : lsps ) {
		auto rootPath = findRootPath( lsp, doc );
		auto lspName = lsp.name.empty() ? lsp.command : lsp.name;
		String::HashType id = String::hash( lspName + "|" + lsp.language + "|" + rootPath );
		auto clientIt = mClients.find( id );
		std::shared_ptr<LSPClientServer> server;
		if ( clientIt == mClients.end() ) {
			server = runLSPServer( lsp, rootPath );
			if ( server.use_count() )
				mClients[id] = server;
		} else {
			server = clientIt->second;
		}
		if ( server.use_count() ) {
			server->registerDoc( doc );
		}
	}
}

void LSPClientServerManager::run( const std::shared_ptr<TextDocument>& doc ) {
	mPool->run( [&, doc]() { tryRunServer( doc ); }, []() {} );
}

size_t LSPClientServerManager::clientCount() const {
	return mClients.size();
}

} // namespace ecode
