#include "lspdocumentclient.hpp"
#include "lspclientplugin.hpp"
#include "lspclientserver.hpp"
#include "lspclientservermanager.hpp"
#include <eepp/system/filesystem.hpp>
#include <eepp/system/iostreamstring.hpp>
#include <eepp/system/log.hpp>
#include <eepp/ui/doc/syntaxhighlighter.hpp>
#include <eepp/window/engine.hpp>

using namespace EE::System;

namespace ecode {

LSPDocumentClient::LSPDocumentClient( LSPClientServer* server, TextDocument* doc ) :
	mServer( server ), mDoc( doc ) {
	refreshTag();
	notifyOpen();
	requestSymbolsDelayed();
	requestSemanticHighlightingDelayed();
}

LSPDocumentClient::~LSPDocumentClient() {
	UISceneNode* sceneNode = getUISceneNode();
	if ( nullptr != sceneNode && 0 != mTag )
		sceneNode->removeActionsByTag( mTag );
}

void LSPDocumentClient::onDocumentLoaded( TextDocument* ) {
	requestSemanticHighlightingDelayed();
}

void LSPDocumentClient::onDocumentTextChanged( const DocumentContentChange& change ) {
	++mVersion;
	// If several change event are being fired, the thread pool can't guaranteed that it will be
	// executed in FIFO. Se we accumulate the events in a queue and fire them in correct order.
	mServer->queueDidChange( mDoc->getURI(), mVersion, "", { change } );
	mServer->getThreadPool()->run( [&, change]() { mServer->processDidChangeQueue(); } );
	requestSymbolsDelayed();
	requestSemanticHighlightingDelayed();
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
	requestSemanticHighlighting();
}

void LSPDocumentClient::refreshTag() {
	String::HashType oldTag = mTag;
	mTag = String::hash( mDoc->getURI().toString() );
	mTagSemanticTokens = String::hash( mDoc->getURI().toString() + ":semantictokens" );
	UISceneNode* sceneNode = getUISceneNode();
	if ( nullptr != sceneNode && 0 != oldTag )
		sceneNode->removeActionsByTag( oldTag );
}

void LSPDocumentClient::requestSemanticHighlighting() {
	if ( !mServer || !mServer->getManager()->getPlugin()->semanticHighlightingEnabled() )
		return;
	const auto& cap = mServer->getCapabilities();
	if ( !cap.semanticTokenProvider.full && !cap.semanticTokenProvider.fullDelta /*&&
		 !cap.semanticTokenProvider.range*/ )
		return;

	TextRange range;
	std::string reqId;
	bool delta = false;
	/*if ( cap.semanticTokenProvider.range ) {
		range = mDoc->getDocRange();
	} else */
	if ( cap.semanticTokenProvider.fullDelta ) {
		delta = true;
		reqId = mSemanticeResultId;
	}

	mServer->documentSemanticTokensFull(
		mDoc->getURI(), delta, reqId, range,
		[this]( const auto&, const LSPSemanticTokensDelta& deltas ) { processTokens( deltas ); } );
}

void LSPDocumentClient::requestSemanticHighlightingDelayed() {
	if ( !mServer || !mServer->getManager()->getPlugin()->semanticHighlightingEnabled() )
		return;
	const auto& cap = mServer->getCapabilities();
	if ( !cap.semanticTokenProvider.full && !cap.semanticTokenProvider.fullDelta /*&&
		 !cap.semanticTokenProvider.range*/ )
		return;
	UISceneNode* sceneNode = getUISceneNode();
	if ( sceneNode ) {
		sceneNode->removeActionsByTag( mTagSemanticTokens );
		sceneNode->runOnMainThread( [this]() { requestSemanticHighlighting(); }, Seconds( 0.1f ),
									mTagSemanticTokens );
	}
}

UISceneNode* LSPDocumentClient::getUISceneNode() {
	LSPClientServer* server = mServer;
	if ( !server || !server->getManager() || !server->getManager()->getPluginManager() ||
		 !server->getManager()->getPluginManager()->getUISceneNode() )
		return nullptr;
	return server->getManager()->getPluginManager()->getUISceneNode();
}

static std::string semanticTokenTypeToSyntaxType( const std::string& type,
												  const SyntaxDefinition& syn ) {
	switch ( String::hash( type ) ) {
		case SemanticTokenTypes::Namespace:
		case SemanticTokenTypes::Type:
		case SemanticTokenTypes::Class:
		case SemanticTokenTypes::Enum:
		case SemanticTokenTypes::Interface:
		case SemanticTokenTypes::Struct:
		case SemanticTokenTypes::TypeParameter:
			return "keyword2";
		case SemanticTokenTypes::Parameter:
		case SemanticTokenTypes::Variable:
			return "symbol";
		case SemanticTokenTypes::Property:
			if ( syn.getLSPName() == "typescript" || syn.getLSPName() == "javascript" )
				return "function";
			return "symbol";
		case SemanticTokenTypes::EnumMember:
		case SemanticTokenTypes::Event:
			return "keyword2";
		case SemanticTokenTypes::Function:
		case SemanticTokenTypes::Method:
		case SemanticTokenTypes::Member:
			return "function";
		case SemanticTokenTypes::Macro:
		case SemanticTokenTypes::Keyword:
			return "keyword2";
		case SemanticTokenTypes::Modifier:
			return "keyword";
		case SemanticTokenTypes::Comment:
			return "comment";
		case SemanticTokenTypes::Str:
			return "string";
		case SemanticTokenTypes::Number:
		case SemanticTokenTypes::Regexp:
			return "number";
		case SemanticTokenTypes::Operator:
			return "operator";
		case SemanticTokenTypes::Decorator:
			return "literal";
		case SemanticTokenTypes::Unknown:
			break;
	};
	return "normal";
}

void LSPDocumentClient::processTokens( const LSPSemanticTokensDelta& tokens ) {
	if ( !tokens.resultId.empty() )
		mSemanticeResultId = tokens.resultId;

	for ( const auto& edit : tokens.edits ) {
		auto& curTokens = mSemanticTokens.data;
		if ( edit.deleteCount > 0 ) {
			curTokens.erase( curTokens.begin() + edit.start,
							 curTokens.begin() + edit.start + edit.deleteCount );
		}
		curTokens.insert( curTokens.begin() + edit.start, edit.data.begin(), edit.data.end() );
	}

	if ( !tokens.data.empty() ) {
		mSemanticTokens = tokens;
	}

	highlight();
}

void LSPDocumentClient::highlight() {
	const auto& data = mSemanticTokens.data;

	if ( data.size() % 5 != 0 ) {
		Log::warning( "LSPDocumentClient::highlight bad data format for doc: %s",
					  mDoc->getURI().toString().c_str() );
		return;
	}

	const auto& caps = mServer->getCapabilities().semanticTokenProvider;
	Uint32 currentLine = 0;
	Uint32 start = 0;
	std::map<size_t, TokenizedLine> tokenizedLines;
	for ( size_t i = 0; i < data.size(); i += 5 ) {
		const Uint32 deltaLine = data[i];
		const Uint32 deltaStart = data[i + 1];
		const Uint32 len = data[i + 2];
		const Uint32 type = data[i + 3];
		// const Uint32 mod = data[i + 4];
		currentLine += deltaLine;

		if ( deltaLine == 0 ) {
			start += deltaStart;
		} else {
			start = deltaStart;
		}

		auto& line = tokenizedLines[currentLine];
		const auto& ltype = caps.legend.tokenTypes[type];
		line.tokens.push_back(
			{ semanticTokenTypeToSyntaxType( ltype, mDoc->getSyntaxDefinition() ), start, len } );
		line.hash = mDoc->line( currentLine ).getHash();
	}

	for ( const auto& tline : tokenizedLines ) {
		mDoc->getHighlighter()->mergeLine( tline.first, tline.second );
	}
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
