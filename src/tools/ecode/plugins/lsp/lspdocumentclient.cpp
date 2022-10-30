#include "lspdocumentclient.hpp"
#include "lspclientserver.hpp"
#include <eepp/system/filesystem.hpp>
#include <eepp/system/iostreamstring.hpp>

using namespace EE::System;

namespace ecode {

LSPDocumentClient::LSPDocumentClient( LSPClientServer* server, TextDocument* doc ) :
	mServer( server ), mDoc( doc ) {
	notifyOpen();
	mServer->documentSymbols(
		mDoc->getURI(),
		[&]( const json& ) {

		},
		[&]( const json& ) {

		} );
}

void LSPDocumentClient::onDocumentTextChanged() {}

void LSPDocumentClient::onDocumentUndoRedo( const TextDocument::UndoRedo& /*eventType*/ ) {}

void LSPDocumentClient::onDocumentCursorChange( const TextPosition& ) {}

void LSPDocumentClient::onDocumentSelectionChange( const TextRange& ) {}

void LSPDocumentClient::onDocumentLineCountChange( const size_t& /*lastCount*/,
												   const size_t& /*newCount*/ ) {}

void LSPDocumentClient::onDocumentLineChanged( const Int64& /*lineIndex*/ ) {}

void LSPDocumentClient::onDocumentSaved( TextDocument* ) {}

void LSPDocumentClient::onDocumentClosed( TextDocument* ) {}

void LSPDocumentClient::onDocumentDirtyOnFileSystem( TextDocument* ) {}

void LSPDocumentClient::onDocumentMoved( TextDocument* ) {}

void LSPDocumentClient::notifyOpen() {
	if ( mDoc->isDirty() ) {
		IOStreamString text;
		mDoc->save( text, true );
		mServer->didOpen( mDoc->getURI(), text.getStream(), mVersion );
	} else {
		std::string text;
		FileSystem::fileGet( mDoc->getFilePath(), text );
		mServer->didOpen( mDoc->getURI(), text, mVersion );
	}
}

} // namespace ecode
