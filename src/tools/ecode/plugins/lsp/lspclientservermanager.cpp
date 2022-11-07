#include "lspclientservermanager.hpp"
#include "lspclientplugin.hpp"
#include <algorithm>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/luapattern.hpp>

namespace ecode {

LSPClientServerManager::LSPClientServerManager() {}

void LSPClientServerManager::load( LSPClientPlugin* plugin, const PluginManager* pluginManager,
								   std::vector<LSPDefinition>&& lsps ) {
	mPlugin = plugin;
	mPluginManager = pluginManager;
	mThreadPool = pluginManager->getThreadPool();
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

std::unique_ptr<LSPClientServer>
LSPClientServerManager::runLSPServer( const String::HashType& id, const LSPDefinition& lsp,
									  const std::string& rootPath ) {
	auto server = std::make_unique<LSPClientServer>( this, id, lsp, rootPath );
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
		Lock l( mClientsMutex );
		auto clientIt = mClients.find( id );
		LSPClientServer* server = nullptr;
		if ( clientIt == mClients.end() ) {
			std::unique_ptr<LSPClientServer> serverUP = runLSPServer( id, lsp, rootPath );
			if ( ( server = serverUP.get() ) )
				mClients[id] = std::move( serverUP );
		} else {
			server = clientIt->second.get();
		}
		if ( server ) {
			if ( !mLSPWorkspaceFolder.uri.empty() )
				server->didChangeWorkspaceFolders( { mLSPWorkspaceFolder }, {} );
			server->registerDoc( doc );
		}
	}
}

void LSPClientServerManager::closeLSPServer( const String::HashType& id ) {
	mThreadPool->run( [this, id]() {
		Lock l( mClientsMutex );
		auto it = mClients.find( id );
		if ( it != mClients.end() ) {
			mClients.erase( it );
		}
	} );
}

void LSPClientServerManager::goToLocation( const LSPLocation& loc ) {
	UICodeEditorSplitter* splitter = mPlugin->getManager()->getSplitter();
	if ( nullptr == splitter )
		return;
	splitter->getUISceneNode()->runOnMainThread( [this, splitter, loc]() {
		UITab* tab = splitter->isDocumentOpen( loc.uri.toString() );
		if ( !tab ) {
			std::string path( loc.uri.getPath() );
			FileInfo fileInfo( path );
			if ( fileInfo.exists() && fileInfo.isRegularFile() ) {
				splitter->loadAsyncFileFromPathInNewTab( path, mThreadPool,
														 [loc]( UICodeEditor* editor, auto ) {
															 editor->goToLine( loc.range.start() );
															 editor->setFocus();
														 } );
			}
		} else {
			tab->getTabWidget()->setTabSelected( tab );
			splitter->editorFromTab( tab )->goToLine( loc.range.start() );
			splitter->editorFromTab( tab )->setFocus();
		}
	} );
}

void LSPClientServerManager::run( const std::shared_ptr<TextDocument>& doc ) {
	mThreadPool->run( [&, doc]() { tryRunServer( doc ); }, []() {} );
}

size_t LSPClientServerManager::clientCount() const {
	return mClients.size();
}

size_t LSPClientServerManager::lspCount() const {
	return mLSPs.size();
}

const std::shared_ptr<ThreadPool>& LSPClientServerManager::getThreadPool() const {
	return mThreadPool;
}

void LSPClientServerManager::updateDirty() {
	{
		Lock l( mClientsMutex );
		for ( auto& server : mClients ) {
			server.second->updateDirty();

			if ( !server.second->hasDocuments() )
				mLSPsToClose.push_back( server.first );
		}
	}
	if ( !mLSPsToClose.empty() )
		for ( const auto& server : mLSPsToClose )
			closeLSPServer( server );
}

void LSPClientServerManager::getAndGoToLocation( const std::shared_ptr<TextDocument>& doc,
												 const std::string& search ) {
	auto* server = getOneLSPClientServer( doc );
	if ( server )
		server->getAndGoToLocation( doc->getURI(), doc->getSelection().start(), search );
}

const PluginManager* LSPClientServerManager::getPluginManager() const {
	return mPluginManager;
}

LSPClientPlugin* LSPClientServerManager::getPlugin() const {
	return mPlugin;
}

void LSPClientServerManager::didChangeWorkspaceFolders( const std::string& folder ) {
	mLSPWorkspaceFolder = { "file://" + folder, FileSystem::fileNameFromPath( folder ) };
	Lock l( mClientsMutex );
	for ( auto& server : mClients )
		server.second->didChangeWorkspaceFolders( { mLSPWorkspaceFolder }, {} );
}

const LSPWorkspaceFolder& LSPClientServerManager::getLSPWorkspaceFolder() const {
	return mLSPWorkspaceFolder;
}

std::vector<LSPClientServer*> LSPClientServerManager::getLSPClientServers( UICodeEditor* editor ) {
	return getLSPClientServers( editor->getDocumentRef() );
}

std::vector<LSPClientServer*>
LSPClientServerManager::getLSPClientServers( const std::shared_ptr<TextDocument>& doc ) {
	std::vector<LSPClientServer*> servers;
	Lock l( mClientsMutex );
	for ( auto& server : mClients ) {
		if ( server.second->hasDocument( doc.get() ) )
			servers.push_back( server.second.get() );
	}
	return servers;
}

LSPClientServer* LSPClientServerManager::getOneLSPClientServer( UICodeEditor* editor ) {
	return getOneLSPClientServer( editor->getDocumentRef() );
}

LSPClientServer*
LSPClientServerManager::getOneLSPClientServer( const std::shared_ptr<TextDocument>& doc ) {
	Lock l( mClientsMutex );
	for ( auto& server : mClients ) {
		if ( server.second->hasDocument( doc.get() ) )
			return server.second.get();
	}
	return nullptr;
}

} // namespace ecode
