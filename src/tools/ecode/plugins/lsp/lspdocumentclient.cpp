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
	mServer( server ), mServerManager( server->getManager() ), mDoc( doc ) {
	refreshTag();
	notifyOpen();
	requestSymbolsDelayed();
	requestSemanticHighlightingDelayed();
}

LSPDocumentClient::~LSPDocumentClient() {
	UISceneNode* sceneNode = getUISceneNode();
	if ( nullptr != sceneNode && 0 != mTag )
		sceneNode->removeActionsByTag( mTag );
	if ( nullptr != sceneNode && 0 != mTagSemanticTokens )
		sceneNode->removeActionsByTag( mTagSemanticTokens );
	mShutdown = true;
	while ( mRunningSemanticTokens )
		Sys::sleep( Milliseconds( 0.1f ) );
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
	mServer->getThreadPool()->run( [this]() { mServer->didSave( mDoc ); } );
}

void LSPDocumentClient::onDocumentClosed( TextDocument* ) {
	URI uri = mDoc->getURI();
	LSPClientServer* server = mServer;
	LSPClientServerManager* manager = mServerManager;
	mServer->getThreadPool()->run( [server, manager, uri]() {
		if ( manager->isServerRunning( server ) )
			server->didClose( uri );
	} );
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

void LSPDocumentClient::requestSemanticHighlighting( bool reqFull ) {
	if ( !mServer || !mServer->getManager()->getPlugin()->semanticHighlightingEnabled() ||
		 !mServer->getManager()->getPlugin()->langSupportsSemanticHighlighting(
			 mServer->getDefinition().language ) )
		return;
	const auto& cap = mServer->getCapabilities();
	if ( !cap.semanticTokenProvider.full && !cap.semanticTokenProvider.fullDelta &&
		 !cap.semanticTokenProvider.range )
		return;

	TextRange range;
	std::string reqId;
	bool delta = false;
	if ( cap.semanticTokenProvider.range && !mFirstHighlight && !reqFull ) {
		range = mDoc->getActiveClientVisibleRange();
	} else if ( mFirstHighlight || ( reqFull && !cap.semanticTokenProvider.fullDelta ) ) {
		mFirstHighlight = false;
	} else if ( cap.semanticTokenProvider.fullDelta ) {
		delta = true;
		reqId = reqFull ? "" : mSemanticeResultId;
	}

	LSPDocumentClient* docClient = this;
	URI uri = mDoc->getURI();
	LSPClientServer* server = mServer;
	Uint64 docModId = mDoc->getModificationId();
	mServer->documentSemanticTokensFull(
		mDoc->getURI(), delta, reqId, range,
		[docClient, uri, server, docModId]( const auto&, const LSPSemanticTokensDelta& deltas ) {
			if ( server->hasDocument( uri ) )
				docClient->processTokens( deltas, docModId );
		} );
}

void LSPDocumentClient::requestSemanticHighlightingDelayed( bool reqFull ) {
	if ( !mServer || !mServer->getManager()->getPlugin()->semanticHighlightingEnabled() ||
		 !mServer->getManager()->getPlugin()->langSupportsSemanticHighlighting(
			 mServer->getDefinition().language ) )
		return;
	const auto& cap = mServer->getCapabilities();
	if ( !cap.semanticTokenProvider.full && !cap.semanticTokenProvider.fullDelta &&
		 !cap.semanticTokenProvider.range )
		return;
	UISceneNode* sceneNode = getUISceneNode();
	if ( sceneNode ) {
		sceneNode->removeActionsByTag( mTagSemanticTokens );
		sceneNode->runOnMainThread( [this, reqFull]() { requestSemanticHighlighting( reqFull ); },
									Seconds( 0.5f ), mTagSemanticTokens );
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
												  const SyntaxDefinition& ) {
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
			return "keyword3";
		case SemanticTokenTypes::Variable:
			return "symbol";
		case SemanticTokenTypes::Property:
			return "symbol";
		case SemanticTokenTypes::EnumMember:
		case SemanticTokenTypes::Event:
			return "keyword2";
		case SemanticTokenTypes::Function:
		case SemanticTokenTypes::Method:
		case SemanticTokenTypes::Member:
			return "function";
		case SemanticTokenTypes::Macro:
			return "keyword2";
		case SemanticTokenTypes::Keyword:
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

void LSPDocumentClient::processTokens( const LSPSemanticTokensDelta& tokens,
									   const Uint64& docModificationId ) {
	// If the document has already being modified after requesting the semantic highlighting,
	// re-request the changes
	if ( docModificationId != mDoc->getModificationId() )
		return requestSemanticHighlightingDelayed();

	mRunningSemanticTokens = true;

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

	mRunningSemanticTokens = false;
}

void LSPDocumentClient::highlight() {
	if ( mShutdown )
		return;
	const auto& data = mSemanticTokens.data;

	if ( data.size() % 5 != 0 ) {
		Log::warning( "LSPDocumentClient::highlight bad data format for doc: %s",
					  mDoc->getURI().toString().c_str() );
		return;
	}
	Clock clock;
	const auto& caps = mServer->getCapabilities().semanticTokenProvider;
	Uint32 currentLine = 0;
	Uint32 start = 0;
	std::unordered_map<size_t, TokenizedLine> tokenizerLines;
	Int64 lastLine = 0;
	TokenizedLine* lastLinePtr = nullptr;
	Time diff;

	for ( size_t i = 0; i < data.size(); i += 5 ) {
		if ( mShutdown )
			return;
		const Uint32 deltaLine = data[i];
		const Uint32 deltaStart = data[i + 1];
		const Uint32 len = data[i + 2];
		const int type = data[i + 3];
		// const Uint32 mod = data[i + 4];
		currentLine += deltaLine;

		if ( deltaLine == 0 ) {
			start += deltaStart;
		} else {
			start = deltaStart;
		}

		auto* line = &tokenizerLines[currentLine];
		if ( type >= 0 && type < (int)caps.legend.tokenTypes.size() ) {
			const auto& ltype = caps.legend.tokenTypes[type];
			line->tokens.push_back(
				{ semanticTokenTypeToSyntaxType( ltype, mDoc->getSyntaxDefinition() ), start,
				  len } );
		} else {
			line->tokens.push_back( { "normal", start, len } );
		}
		line->hash = mDoc->line( currentLine ).getHash();
		line->updateSignature();

		auto curSignature = mDoc->getHighlighter()->getTokenizedLineSignature( lastLine );
		if ( lastLine != currentLine && lastLinePtr && lastLinePtr->signature == curSignature ) {
			tokenizerLines.erase( lastLine );
		}

		lastLine = currentLine;
		lastLinePtr = line;
	}

	diff = clock.getElapsedTime();

	for ( auto& tline : tokenizerLines ) {
		if ( mShutdown )
			return;

		mDoc->getHighlighter()->mergeLine( tline.first, tline.second );
	}

	Log::debug( "LSPDocumentClient::highlight took: %.2f ms. Diff analysis took: %.2f ms. Updated "
				"%lld elements",
				clock.getElapsedTime().asMilliseconds(), diff.asMilliseconds(),
				tokenizerLines.size() );
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
	if ( Engine::instance()->isMainThread() ) {
		mServer->getThreadPool()->run(
			[server, uri]() { server->documentSymbolsBroadcast( uri ); } );
	} else {
		server->documentSymbolsBroadcast( uri );
	}
}

void LSPDocumentClient::requestSymbolsDelayed() {
	if ( !mServer || !mServer->getCapabilities().documentSymbolProvider )
		return;
	UISceneNode* sceneNode = getUISceneNode();
	if ( sceneNode ) {
		sceneNode->removeActionsByTag( mTag );
		LSPDocumentClient* docClient = this;
		URI uri = mDoc->getURI();
		LSPClientServer* server = mServer;
		sceneNode->runOnMainThread(
			[docClient, server, uri]() {
				if ( server->hasDocument( uri ) )
					docClient->requestSymbols();
			},
			Seconds( 1.f ), mTag );
	}
}

} // namespace ecode
