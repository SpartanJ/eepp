#include "lspdocumentclient.hpp"
#include "lspclientserver.hpp"
#include <eepp/system/filesystem.hpp>
#include <eepp/system/iostreamstring.hpp>
#include <eepp/system/log.hpp>

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
	mServer->getThreadPool()->run( [&]() { mServer->didClose( mDoc ); } );
}

void LSPDocumentClient::onDocumentDirtyOnFileSystem( TextDocument* ) {}

void LSPDocumentClient::onDocumentMoved( TextDocument* ) {}

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
	mServer->didOpen( mDoc, ++mVersion );
}

} // namespace ecode
