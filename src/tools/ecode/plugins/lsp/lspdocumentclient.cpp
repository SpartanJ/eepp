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
	mModified = true;
	++mVersion;
	if ( !change.text.empty() || change.range.start() != change.range.end() )
		mDocChange.push_back( change );
	mLastModified.restart();
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
	mServer->didClose( mDoc );
}

void LSPDocumentClient::onDocumentDirtyOnFileSystem( TextDocument* ) {}

void LSPDocumentClient::onDocumentMoved( TextDocument* ) {}

bool LSPDocumentClient::isDirty() const {
	return mModified && mLastModified.getElapsedTime() > Milliseconds( 250.f );
}

void LSPDocumentClient::resetDirty() {
	mModified = false;
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

const std::vector<DocumentContentChange>& LSPDocumentClient::getDocChange() const {
	return mDocChange;
}

void LSPDocumentClient::clearDocChange() {
	mDocChange.clear();
}

void LSPDocumentClient::notifyOpen() {
	eeASSERT( mDoc );
	mServer->didOpen( mDoc, ++mVersion );
}

} // namespace ecode
