#include "lspdocumentclient.hpp"
#include "lspclientplugin.hpp"
#include "lspclientserver.hpp"
#include "lspclientservermanager.hpp"
#include <eepp/system/filesystem.hpp>
#include <eepp/system/iostreamstring.hpp>
#include <eepp/system/log.hpp>
#include <eepp/window/engine.hpp>

using namespace EE::System;

namespace ecode {

LSPDocumentClient::LSPDocumentClient( LSPClientServer* server, TextDocument* doc ) :
	mServer( server ), mDoc( doc ) {
	refreshTag();
	notifyOpen();
	requestSymbolsDelayed();
}

LSPDocumentClient::~LSPDocumentClient() {
	UISceneNode* sceneNode = getUISceneNode();
	if ( nullptr != sceneNode && 0 != mTag )
		sceneNode->removeActionsByTag( mTag );
}

void LSPDocumentClient::onDocumentTextChanged( const DocumentContentChange& change ) {
	++mVersion;
	// If several change event are being fired, the thread pool can't guaranteed that it will be
	// executed in FIFO. Se we accumulate the events in a queue and fire them in correct order.
	mServer->queueDidChange( mDoc->getURI(), mVersion, "", { change } );
	mServer->getThreadPool()->run( [&, change]() { mServer->processDidChangeQueue(); } );
	requestSymbolsDelayed();
}

void LSPDocumentClient::onDocumentUndoRedo( const TextDocument::UndoRedo& /*eventType*/ ) {}

void LSPDocumentClient::onDocumentCursorChange( const TextPosition& ) {}

void LSPDocumentClient::onDocumentSelectionChange( const TextRange& ) {}

void LSPDocumentClient::onDocumentLineCountChange( const size_t& /*lastCount*/,
												   const size_t& /*newCount*/ ) {}

void LSPDocumentClient::onDocumentLineChanged( const Int64& /*lineIndex*/ ) {}

void LSPDocumentClient::onDocumentSaved( TextDocument* ) {
	mServer->getThreadPool()->run( [&]() { mServer->didSave( mDoc ); } );
}

void LSPDocumentClient::onDocumentClosed( TextDocument* ) {
	URI uri = mDoc->getURI();
	LSPClientServer* server = mServer;
	mServer->getThreadPool()->run( [server, uri]() { server->didClose( uri ); } );
	mServer->removeDoc( mDoc );
}

void LSPDocumentClient::onDocumentDirtyOnFileSystem( TextDocument* ) {}

void LSPDocumentClient::onDocumentMoved( TextDocument* ) {
	refreshTag();
}

void LSPDocumentClient::onDocumentReloaded( TextDocument* ) {
	URI uri = mDoc->getURI();
	TextDocument* doc = mDoc;
	LSPClientServer* server = mServer;
	auto version = ++mVersion;
	mServer->getThreadPool()->run( [server, doc, uri, version]() {
		server->didClose( uri );
		server->didOpen( doc, version );
	} );
	refreshTag();
}

TextDocument* LSPDocumentClient::getDoc() const {
	return mDoc;
}

LSPClientServer* LSPDocumentClient::getServer() const {
	return mServer;
}

int LSPDocumentClient::getVersion() const {
	return mVersion;
}

void LSPDocumentClient::onServerInitialized() {
	requestSymbols();
}

void LSPDocumentClient::refreshTag() {
	String::HashType oldTag = mTag;
	mTag = String::hash( mDoc->getURI().toString() );
	UISceneNode* sceneNode = getUISceneNode();
	if ( nullptr != sceneNode && 0 != oldTag )
		sceneNode->removeActionsByTag( oldTag );
}

UISceneNode* LSPDocumentClient::getUISceneNode() {
	LSPClientServer* server = mServer;
	if ( !server || !server->getManager() || !server->getManager()->getPluginManager() ||
		 !server->getManager()->getPluginManager()->getUISceneNode() )
		return nullptr;
	return server->getManager()->getPluginManager()->getUISceneNode();
}

void LSPDocumentClient::notifyOpen() {
	eeASSERT( mDoc );
	if ( Engine::instance()->isMainThread() ) {
		mServer->getThreadPool()->run( [this]() { mServer->didOpen( mDoc, ++mVersion ); } );
	} else {
		mServer->didOpen( mDoc, ++mVersion );
	}
}

void LSPDocumentClient::requestSymbols() {
	eeASSERT( mDoc );
	LSPClientServer* server = mServer;
	if ( !server->getCapabilities().documentSymbolProvider )
		return;
	URI uri = mDoc->getURI();
	auto handler = [uri, server]( const PluginIDType& id, LSPSymbolInformationList&& res ) {
		server->getManager()->getPlugin()->setDocumentSymbolsFromResponse( id, uri,
																		   std::move( res ) );
	};
	if ( Engine::instance()->isMainThread() ) {
		mServer->getThreadPool()->run(
			[server, uri, handler]() { server->documentSymbols( uri, handler ); } );
	} else {
		server->documentSymbols( uri, handler );
	}
}

void LSPDocumentClient::requestSymbolsDelayed() {
	if ( !mServer || !mServer->getCapabilities().documentSymbolProvider )
		return;
	UISceneNode* sceneNode = getUISceneNode();
	if ( sceneNode ) {
		sceneNode->removeActionsByTag( mTag );
		sceneNode->runOnMainThread( [this]() { requestSymbols(); }, Seconds( 1.f ), mTag );
	}
}

} // namespace ecode
