#include "lspdocumentclient.hpp"
#include "lspclientserver.hpp"
#include <eepp/system/filesystem.hpp>
#include <eepp/system/iostreamstring.hpp>
#include <eepp/system/log.hpp>
#include <eepp/window/engine.hpp>

using namespace EE::System;

namespace ecode {

LSPDocumentClient::LSPDocumentClient( LSPClientServer* server, TextDocument* doc ) :
	mServer( server ), mDoc( doc ) {
	notifyOpen();
}

LSPDocumentClient::~LSPDocumentClient() {}

void LSPDocumentClient::onDocumentTextChanged( const DocumentContentChange& change ) {
	++mVersion;
	// If several change event are being fired, the thread pool can't guaranteed that it will be
	// executed in FIFO. Se we accumulate the events in a queue and fire them in correct order.
	mServer->queueDidChange( mDoc->getURI(), mVersion, "", { change } );
	mServer->getThreadPool()->run( [&, change]() { mServer->processDidChangeQueue(); } );
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

void LSPDocumentClient::onDocumentMoved( TextDocument* ) {}

void LSPDocumentClient::onDocumentReloaded( TextDocument* ) {
	URI uri = mDoc->getURI();
	TextDocument* doc = mDoc;
	LSPClientServer* server = mServer;
	auto version = ++mVersion;
	mServer->getThreadPool()->run( [server, doc, uri, version]() {
		server->didClose( uri );
		server->removeDoc( doc );
		server->didOpen( doc, version );
	} );
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

void LSPDocumentClient::notifyOpen() {
	eeASSERT( mDoc );
	if ( Engine::instance()->isMainThread() ) {
		mServer->getThreadPool()->run( [this]() { mServer->didOpen( mDoc, ++mVersion ); } );
	} else {
		mServer->didOpen( mDoc, ++mVersion );
	}
}

} // namespace ecode
