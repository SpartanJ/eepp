#include "lspclientservermanager.hpp"
#include "../../projectsearch.hpp"
#include "lspclientplugin.hpp"
#include <algorithm>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/luapattern.hpp>

namespace ecode {

LSPClientServerManager::LSPClientServerManager() {}

void LSPClientServerManager::load( LSPClientPlugin* plugin, PluginManager* pluginManager,
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
			if ( LuaPattern::hasMatches( fileName, ext ) ) {
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
									  const std::string& rootPath,
									  std::vector<std::string> languagesSupported ) {
	auto server = std::make_unique<LSPClientServer>( this, id, lsp, rootPath, languagesSupported );
	server->start();
	return server;
}

std::string LSPClientServerManager::findRootPath( const LSPDefinition& lsp,
												  const std::shared_ptr<TextDocument>& doc ) {
	if ( lsp.rootIndicationFileNames.empty() || !doc->hasFilepath() )
		return "";
	std::string oriRootPath( doc->getFileInfo().getDirectoryPath() );
	std::string rootPath( oriRootPath );
	std::string lRootPath;
	FileSystem::dirAddSlashAtEnd( rootPath );

	while ( rootPath != lRootPath ) {
		for ( const auto& fileName : lsp.rootIndicationFileNames ) {
			if ( FileSystem::fileExists( rootPath + fileName ) )
				return rootPath;

			if ( fileName.find_first_of( "$%" ) != std::string::npos &&
				 !LuaPattern::matchesAny( FileSystem::filesGetInPath( rootPath ), fileName )
					  .empty() )
				return rootPath;
		}
		lRootPath = rootPath;
		rootPath = FileSystem::removeLastFolderFromPath( rootPath );
	}

	return oriRootPath;
}

void LSPClientServerManager::tryRunServer( const std::shared_ptr<TextDocument>& doc ) {
	auto lsps = supportsLSP( doc );
	if ( lsps.empty() )
		return;

	for ( const auto& lsp : lsps ) {
		if ( !lsp.commandAvailable() && lsp.host.empty() )
			continue;
		std::string rootPath = mLSPWorkspaceFolder.isEmpty() ? findRootPath( lsp, doc )
															 : mLSPWorkspaceFolder.uri.getFSPath();
		auto lspName = lsp.name.empty() ? lsp.command : lsp.name;
		String::HashType id = lsp.shareProcessWithOtherDefinition
								  ? String::hash( lspName + "|" + rootPath )
								  : String::hash( lspName + "|" + lsp.language + "|" + rootPath );
		LSPClientServer* server = nullptr;
		{
			Lock l( mClientsMutex );
			auto clientIt = mClients.find( id );
			if ( clientIt == mClients.end() ) {
				auto rlsp = lsp;
				if ( !lsp.usesLSP.empty() ) {
					for ( const auto& flsp : mLSPs ) {
						if ( flsp.name == lsp.usesLSP ) {
							rlsp = flsp;
							break;
						}
					}
				}
				std::vector<std::string> languagesSupported;
				languagesSupported.push_back( rlsp.language );
				for ( const auto& flsp : mLSPs )
					if ( flsp.usesLSP == rlsp.name )
						languagesSupported.push_back( flsp.language );
				std::unique_ptr<LSPClientServer> serverUP =
					runLSPServer( id, rlsp, rootPath, languagesSupported );
				if ( ( server = serverUP.get() ) ) {
					mClients[id] = std::move( serverUP );
					if ( server->isRunning() ) {
						if ( !mLSPWorkspaceFolder.uri.empty() )
							server->didChangeWorkspaceFolders( { mLSPWorkspaceFolder }, {}, true );
					} else {
						Log::debug( "LSP Server: %s failed to initialize.",
									server->getDefinition().name );
					}
				}
			} else {
				server = clientIt->second.get();
			}
		}
		if ( server )
			server->registerDoc( doc );
	}
}

void LSPClientServerManager::closeLSPServer( const String::HashType& id ) {
	if ( mErasingClients.find( id ) != mErasingClients.end() )
		return;
	mErasingClients.insert( id );
	mThreadPool->run( [this, id]() {
		Lock l( mClientsMutex );
		auto it = mClients.find( id );
		if ( it != mClients.end() ) {
			const auto& def = it->second->getDefinition();
			Log::debug( "Closing LSP server: %s for language %s", def.name.c_str(),
						def.language.c_str() );
			mClients.erase( it );
			mLSPsToClose.erase( id );
			mErasingClients.erase( id );
		}
	} );
}

void LSPClientServerManager::goToLocation( const LSPLocation& loc ) {
	UICodeEditorSplitter* splitter = mPlugin->getManager()->getSplitter();
	if ( nullptr == splitter )
		return;
	splitter->getUISceneNode()->runOnMainThread( [splitter, loc]() {
		UITab* tab = splitter->isDocumentOpen( loc.uri );
		if ( !tab ) {
			std::string path( loc.uri.getFSPath() );
			FileInfo fileInfo( path );
			if ( fileInfo.exists() && fileInfo.isRegularFile() ) {
				splitter->addCurrentPositionToNavigationHistory();
				splitter->loadAsyncFileFromPathInNewTab(
					path, [loc, splitter]( UICodeEditor* editor, auto ) {
						if ( loc.range.isValid() ) {
							editor->goToLine( loc.range.start() );
							splitter->addEditorPositionToNavigationHistory( editor );
						}
						editor->setFocus();
					} );
			}
		} else {
			if ( loc.range.isValid() )
				splitter->addCurrentPositionToNavigationHistory();
			tab->getTabWidget()->setTabSelected( tab );
			if ( loc.range.isValid() ) {
				splitter->editorFromTab( tab )->goToLine( loc.range.start() );
				splitter->addCurrentPositionToNavigationHistory();
			}
			splitter->editorFromTab( tab )->setFocus();
		}
	} );
}

void LSPClientServerManager::executeCommand( const std::shared_ptr<TextDocument>& doc,
											 const LSPCommand& cmd ) {
	if ( cmd.command.empty() )
		return;
	auto* server = getOneLSPClientServer( doc );
	if ( server )
		server->executeCommand( cmd.command, cmd.arguments );
}

void LSPClientServerManager::applyWorkspaceEdit(
	const LSPWorkspaceEdit& edit,
	const std::function<void( const LSPApplyWorkspaceEditResponse& res )>& resCb ) {
	mPluginManager->getSplitter()->getUISceneNode()->runOnMainThread( [this, edit, resCb] {
		bool allDone = true;

		for ( const auto& ed : edit.changes ) {
			if ( !mPlugin->processDocumentFormattingResponse( ed.first, ed.second ) ) {
				allDone = false;
			}
		}

		for ( const auto& edc : edit.documentChanges ) {
			if ( !mPlugin->processDocumentFormattingResponse( edc.textDocument.uri, edc.edits ) ) {
				allDone = false;
			}
		}

		if ( resCb ) {
			LSPApplyWorkspaceEditResponse res;
			res.applied = allDone;
			resCb( res );
		}
	} );
}

void LSPClientServerManager::renameSymbol( const URI& uri, const TextPosition& pos,
										   const std::string& newName ) {
	auto* server = getOneLSPClientServer( uri );
	if ( !server )
		return;
	server->documentRename( uri, pos, newName,
							[this]( const PluginIDType&, const LSPWorkspaceEdit& edit ) {
								applyWorkspaceEdit( edit, []( const auto& ) {} );
							} );
}

bool LSPClientServerManager::isServerRunning( const LSPClientServer* server ) {
	for ( const auto& svr : mClients ) {
		if ( server == svr.second.get() ) {
			if ( mErasingClients.find( svr.first ) != mErasingClients.end() )
				return false;
			return true;
		}
	}
	return false;
}

void LSPClientServerManager::requestSymanticHighlighting( std::shared_ptr<TextDocument> doc ) {
	auto* server = getOneLSPClientServer( doc );
	if ( server ) {
		LSPDocumentClient* client = server->getLSPDocumentClient( doc.get() );
		if ( client )
			client->requestSemanticHighlighting( true );
	}
}

static json getURIJSON( std::shared_ptr<TextDocument> doc ) {
	json data;
	json docUri;
	json options;
	docUri["uri"] = doc->getURI().toString();
	data["textDocument"] = docUri;
	options["tabSize"] = doc->getIndentWidth();
	options["insertSpaces"] = doc->getIndentType() == TextDocument::IndentType::IndentSpaces;
	data["options"] = options;
	return data;
}

void LSPClientServerManager::rangeFormatting( std::shared_ptr<TextDocument> doc ) {
	if ( !doc->getSelection().hasSelection() )
		return;
	mThreadPool->run( [this, doc] {
		auto* server = getOneLSPClientServer( doc );
		if ( server ) {
			URI uri( doc->getURI() );
			server->documentRangeFormatting(
				uri, doc->getSelection(), getURIJSON( doc ),
				[this, uri]( const PluginIDType&, const std::vector<LSPTextEdit>& edits ) {
					mPluginManager->getSplitter()->getUISceneNode()->runOnMainThread(
						[this, edits, uri] {
							mPlugin->processDocumentFormattingResponse( uri, edits );
						} );
				} );
		}
	} );
}

void LSPClientServerManager::run( const std::shared_ptr<TextDocument>& doc ) {
	mThreadPool->run( [this, doc]() { tryRunServer( doc ); } );
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
	// Run the check only once per second
	if ( mUpdateClock.getElapsedTime() < Seconds( 1 ) )
		return;

	mUpdateClock.restart();

	{
		Lock l( mClientsMutex );
		for ( auto& server : mClients ) {
			if ( server.second && !server.second->hasDocuments() &&
				 mLSPsToClose.find( server.first ) == mLSPsToClose.end() )
				mLSPsToClose.insert( { server.first, std::make_unique<Clock>() } );
		}
	}

	if ( !mLSPsToClose.empty() ) {
		std::vector<String::HashType> removed;
		std::vector<String::HashType> invalidatedClose;

		for ( const auto& server : mLSPsToClose ) {
			// Kill server only after N seconds of inactivity
			if ( server.second->getElapsedTime() > mLSPDecayTime ) {
				// If a document was opened while waiting, remove the server from the queue
				auto clientServer = mClients.find( server.first );
				if ( clientServer != mClients.end() && clientServer->second->hasDocuments() ) {
					invalidatedClose.push_back( server.first );
				} else {
					removed.push_back( server.first );
				}
			}
		}
		if ( !invalidatedClose.empty() ) {
			for ( auto invalided : invalidatedClose )
				mLSPsToClose.erase( invalided );
		}
		if ( !removed.empty() ) {
			for ( const auto& remove : removed )
				closeLSPServer( remove );
		}
	}
}

void LSPClientServerManager::getAndGoToLocation( const std::shared_ptr<TextDocument>& doc,
												 const std::string& search ) {
	auto* server = getOneLSPClientServer( doc );
	if ( server )
		server->getAndGoToLocation( doc->getURI(), doc->getSelection().start(), search );
}

void LSPClientServerManager::getAndGoToLocation( const std::shared_ptr<TextDocument>& doc,
												 const std::string& search,
												 const LSPClientServer::LocationHandler& h ) {
	auto* server = getOneLSPClientServer( doc );
	if ( server )
		server->getAndGoToLocation( doc->getURI(), doc->getSelection().start(), search, h );
}

PluginManager* LSPClientServerManager::getPluginManager() const {
	return mPluginManager;
}

LSPClientPlugin* LSPClientServerManager::getPlugin() const {
	return mPlugin;
}

void LSPClientServerManager::findAndOpenClosestURI( const std::vector<URI>& uris ) {
	json data;
	data["uri"] = json::array();
	for ( const auto& uri : uris )
		data["uri"].push_back( uri.toString() );
	mPluginManager->sendRequest( mPlugin, PluginMessageType::FindAndOpenClosestURI,
								 PluginMessageFormat::JSON, &data );
}

const Time& LSPClientServerManager::getLSPDecayTime() const {
	return mLSPDecayTime;
}

void LSPClientServerManager::setLSPDecayTime( const Time& lSPDecayTime ) {
	mLSPDecayTime = lSPDecayTime;
}

void LSPClientServerManager::sendSymbolReferenceBroadcast( const std::vector<LSPLocation>& resp ) {
	if ( resp.empty() )
		return;

	std::map<std::string, ProjectSearch::ResultData> res;
	std::unordered_map<std::string, std::unique_ptr<TextDocument>> tmpDocs;

	for ( auto& r : resp ) {
		std::string fspath( r.uri.getFSPath() );
		auto& rd = res[fspath];
		if ( rd.file.empty() )
			rd.file = fspath;

		auto curDoc = mPluginManager->getSplitter()->findDocFromURI( r.uri );
		if ( curDoc ) {
			ProjectSearch::ResultData::Result rs( curDoc->line( r.range.start().line() ).getText(),
												  r.range, -1, -1 );

			rd.results.emplace_back( std::move( rs ) );
		} else {
			String lineText;
			auto foundDoc = tmpDocs.find( fspath );
			if ( foundDoc == tmpDocs.end() ) {
				std::unique_ptr<TextDocument> doc = std::make_unique<TextDocument>();
				if ( TextDocument::LoadStatus::Loaded != doc->loadFromFile( fspath ) )
					continue;

				lineText = doc->line( r.range.start().line() ).getText();

				tmpDocs.insert( { std::move( fspath ), std::move( doc ) } );
			} else {
				lineText = foundDoc->second->line( r.range.start().line() ).getText();
			}

			ProjectSearch::ResultData::Result rs( lineText, r.range, -1, -1 );

			rd.results.emplace_back( std::move( rs ) );
		}
	}

	ProjectSearch::Result result;
	for ( auto& r : res ) {
		if ( !r.second.results.empty() )
			result.emplace_back( std::move( r.second ) );
	}

	mPluginManager->sendBroadcast( PluginMessageType::SymbolReference,
								   PluginMessageFormat::ProjectSearchResult, &result );
}

void LSPClientServerManager::getSymbolReferences( std::shared_ptr<TextDocument> doc ) {
	auto* server = getOneLSPClientServer( doc );
	if ( !server )
		return;
	server->documentReferences(
		doc->getURI(), doc->getSelection().start(), true,
		[this]( const PluginIDType&, const std::vector<LSPLocation>& resp ) {
			sendSymbolReferenceBroadcast( resp );
		} );
}

void LSPClientServerManager::codeAction( std::shared_ptr<TextDocument> doc,
										 const nlohmann::json& diagnostics,
										 const LSPClientServer::CodeActionHandler& h ) {
	auto* server = getOneLSPClientServer( doc );
	if ( !server )
		return;

	auto range = doc->getSelection();
	if ( !diagnostics.empty() && diagnostics.contains( "diagnostics" ) &&
		 diagnostics["diagnostics"].is_array() && !diagnostics["diagnostics"].empty() &&
		 diagnostics["diagnostics"][0].contains( "range" ) ) {
		range = LSPConverter::parseRange( diagnostics["diagnostics"][0]["range"] );
	} else if ( !doc->hasSelection() ) {
		range = { doc->startOfLine( range.start() ),
				  doc->positionOffset( doc->endOfLine( range.end() ), 1 ) };
	}

	server->documentCodeAction( doc->getURI(), range, {}, diagnostics, h );
}

void LSPClientServerManager::memoryUsage( std::shared_ptr<TextDocument> doc ) {
	auto* server = getOneLSPClientServer( doc );
	if ( !server )
		return;
	server->memoryUsage();
}

const std::vector<LSPDefinition>& LSPClientServerManager::getLSPs() const {
	return mLSPs;
}

LSPDefinition LSPClientServerManager::getLSPForLang( const std::string& lang ) const {
	for ( const auto& lsp : mLSPs ) {
		if ( lsp.language == lang )
			return lsp;
	}
	return {};
}

void LSPClientServerManager::didChangeWorkspaceFolders( const std::string& folder ) {
	std::vector<LSPWorkspaceFolder> oldLSPWorkspaceFolder;
	if ( !mLSPWorkspaceFolder.isEmpty() )
		oldLSPWorkspaceFolder.emplace_back( mLSPWorkspaceFolder );
	mLSPWorkspaceFolder = { "file://" + folder, FileSystem::fileNameFromPath( folder ) };
	std::vector<LSPWorkspaceFolder> newWorkspaceFolder = { mLSPWorkspaceFolder };
	Lock l( mClientsMutex );
	for ( auto& server : mClients ) {
		server.second->didChangeWorkspaceFolders( newWorkspaceFolder, oldLSPWorkspaceFolder, true );
		if ( server.second->getCapabilities().diagnosticProvider.workspaceDiagnostics ) {
			mPlugin->getManager()->sendRequest( PluginMessageType::WorkspaceDiagnostic,
												PluginMessageFormat::LSPClientServer,
												server.second.get() );
		}
		// If there's a workspace folder change, but the server don't support it, we need to close
		// the server because the current workspace will be broken if we don't reopen the server in
		// the correct rootUri/rootPath
		if ( !server.second->getCapabilities().workspaceFolders.supported )
			closeLSPServer( server.first );
	}
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
		if ( server.second && server.second->hasDocument( doc.get() ) )
			servers.push_back( server.second.get() );
	}
	return servers;
}

std::vector<LSPClientServer*> LSPClientServerManager::getLSPClientServers( const URI& uri ) {
	std::vector<LSPClientServer*> servers;
	Lock l( mClientsMutex );
	for ( auto& server : mClients ) {
		if ( server.second && server.second->hasDocument( uri ) )
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
		if ( server.second && server.second->hasDocument( doc.get() ) )
			return server.second.get();
	}
	return nullptr;
}

LSPClientServer* LSPClientServerManager::getOneLSPClientServer( const URI& uri ) {
	Lock l( mClientsMutex );
	for ( auto& server : mClients ) {
		if ( server.second && server.second->hasDocument( uri ) )
			return server.second.get();
	}
	return nullptr;
}

LSPClientServer* LSPClientServerManager::getOneLSPClientServer( const std::string& language ) {
	Lock l( mClientsMutex );
	for ( auto& server : mClients ) {
		if ( server.second && server.second->supportsLanguage( language ) )
			return server.second.get();
	}
	return nullptr;
}

std::vector<LSPClientServer*>
LSPClientServerManager::getLSPClientServers( const std::string& language ) {
	std::vector<LSPClientServer*> servers;
	Lock l( mClientsMutex );
	for ( auto& server : mClients ) {
		if ( server.second->supportsLanguage( language ) )
			servers.push_back( server.second.get() );
	}
	return servers;
}

std::vector<LSPClientServer*> LSPClientServerManager::getAllRunningServers() {
	std::vector<LSPClientServer*> servers;
	Lock l( mClientsMutex );
	for ( auto& server : mClients ) {
		if ( server.second->isRunning() )
			servers.push_back( server.second.get() );
	}
	return servers;
}

std::vector<LSPClientServer*> LSPClientServerManager::getFilteredServers(
	const std::function<bool( LSPClientServer* )>& filter ) {
	std::vector<LSPClientServer*> servers;
	Lock l( mClientsMutex );
	for ( auto& server : mClients ) {
		if ( server.second->isRunning() && filter( server.second.get() ) )
			servers.push_back( server.second.get() );
	}
	return servers;
}

} // namespace ecode
