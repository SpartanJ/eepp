#include "lspdocumentclient.hpp"
#include "lspclientplugin.hpp"
#include "lspclientserver.hpp"
#include "lspclientservermanager.hpp"
#include <eepp/system/filesystem.hpp>
#include <eepp/system/iostreamstring.hpp>
#include <eepp/system/log.hpp>
#include <eepp/system/scopedop.hpp>
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
	if ( mServer->isReady() )
		setupFoldRangeService();
}

void LSPDocumentClient::onServerInitialized() {
	requestSymbols();
	requestSemanticHighlighting();
	setupFoldRangeService();
	// requestCodeLens();
}

void LSPDocumentClient::setupFoldRangeService() {
	mDoc->getFoldRangeService().setProvider( this );
	if ( mDoc->getFoldRangeService().isEnabled() )
		tryRequestFoldRanges( true );
}

LSPDocumentClient::~LSPDocumentClient() {
	mDoc->getFoldRangeService().setProvider( nullptr );
	mDoc = nullptr;
	UISceneNode* sceneNode = getUISceneNode();
	if ( nullptr != sceneNode && 0 != mTag )
		sceneNode->removeActionsByTag( mTag );
	if ( nullptr != sceneNode && 0 != mTagSemanticTokens )
		sceneNode->removeActionsByTag( mTagSemanticTokens );
	mShutdown = true;
	while ( mRunningSemanticTokens || mProcessingSemanticTokensResponse )
		Sys::sleep( Milliseconds( 0.1f ) );
}

bool LSPDocumentClient::tryRequestFoldRanges( bool requestFolds ) {
	bool ret = mServer->getCapabilities().foldingRangeProvider;
	if ( ret && requestFolds )
		requestFoldRange();
	return ret;
}

bool LSPDocumentClient::foldingRangeProvider() const {
	return mServer->getCapabilities().foldingRangeProvider;
}

void LSPDocumentClient::onDocumentLoaded( TextDocument* ) {
	refreshTag();
	requestSemanticHighlightingDelayed();
	// requestCodeLens();
}

void LSPDocumentClient::onDocumentTextChanged( const DocumentContentChange& change ) {
	++mVersion;
	// If several change event are being fired, the thread pool can't guaranteed that it will be
	// executed in FIFO. Se we accumulate the events in a queue and fire them in correct order.
	mServer->queueDidChange( mDoc->getURI(), mVersion, "", { change } );
	mServer->getThreadPool()->run( [this, change]() { mServer->processDidChangeQueue(); } );
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
	mServer->getThreadPool()->run( [this, server, doc, uri, version]() {
		server->didClose( uri );
		server->didOpen( doc, version );
		requestSemanticHighlighting( true );
	} );
	refreshTag();
}

void LSPDocumentClient::onDocumentReset( TextDocument* doc ) {
	onDocumentReloaded( doc );
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

	mWaitingSemanticTokensResponse = true;
	TextRange range;
	std::string reqId;
	bool delta = false;
	if ( cap.semanticTokenProvider.fullDelta ) {
		delta = true;
		reqId = reqFull ? "" : mSemanticeResultId;
	} else if ( cap.semanticTokenProvider.range && !mFirstHighlight && !reqFull ) {
		range = mDoc->getActiveClientVisibleRange();
	} else if ( mFirstHighlight || reqFull ) {
		mFirstHighlight = false;
	}

	LSPDocumentClient* docClient = this;
	URI uri = mDoc->getURI();
	LSPClientServer* server = mServer;
	Uint64 docModId = mDoc->getModificationId();
	mServer->documentSemanticTokensFull(
		mDoc->getURI(), delta, reqId, range,
		[docClient, uri, server, docModId, this]( const auto&, LSPSemanticTokensDelta&& deltas ) {
			BoolScopedOp op( mProcessingSemanticTokensResponse, true );
			if ( server->hasDocumentClient( docClient ) && server->hasDocument( uri ) ) {
				docClient->mWaitingSemanticTokensResponse = false;
				docClient->processTokens( std::move( deltas ), docModId );
			}
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
		mWaitingSemanticTokensResponse = true;
		sceneNode->debounce( [this, reqFull]() { requestSemanticHighlighting( reqFull ); },
							 Seconds( 0.5f ), mTagSemanticTokens );
	}
}

bool LSPDocumentClient::isRunningSemanticTokens() const {
	return mRunningSemanticTokens;
}

bool LSPDocumentClient::isWaitingSemanticTokensResponse() const {
	return mWaitingSemanticTokensResponse;
}

void LSPDocumentClient::requestCodeLens() {
	if ( !mServer || !mServer->getCapabilities().codeLensProvider.supported )
		return;

	URI uri = mDoc->getURI();
	LSPClientServer* server = mServer;
	LSPDocumentClient* docClient = this;
	mServer->documentCodeLens(
		uri, [uri, server, docClient]( const auto&, const std::vector<LSPCodeLens>& codeLens ) {
			if ( server->hasDocument( uri ) ) {
				docClient->mCodeLens = codeLens;
			}
		} );
}

UISceneNode* LSPDocumentClient::getUISceneNode() {
	LSPClientServer* server = mServer;
	if ( !server || !server->getManager() || !server->getManager()->getPluginManager() ||
		 !server->getManager()->getPluginManager()->getUISceneNode() )
		return nullptr;
	return server->getManager()->getPluginManager()->getUISceneNode();
}

static SyntaxStyleType semanticTokenTypeToSyntaxType( const std::string& type ) {
	switch ( String::hash( type ) ) {
		case SemanticTokenTypes::Namespace:
		case SemanticTokenTypes::Type:
		case SemanticTokenTypes::Class:
		case SemanticTokenTypes::Enum:
		case SemanticTokenTypes::Interface:
		case SemanticTokenTypes::Struct:
		case SemanticTokenTypes::TypeParameter:
			return "type"_sst;
		case SemanticTokenTypes::Parameter:
			return "parameter"_sst;
		case SemanticTokenTypes::Variable:
			return "symbol"_sst;
		case SemanticTokenTypes::Property:
			return "symbol"_sst;
		case SemanticTokenTypes::EnumMember:
		case SemanticTokenTypes::Event:
			return "type"_sst;
		case SemanticTokenTypes::Function:
		case SemanticTokenTypes::Method:
		case SemanticTokenTypes::Member:
			return "function"_sst;
		case SemanticTokenTypes::Macro:
			return "type"_sst;
		case SemanticTokenTypes::Keyword:
		case SemanticTokenTypes::Modifier:
			return "keyword"_sst;
		case SemanticTokenTypes::Comment:
			return "comment"_sst;
		case SemanticTokenTypes::Str:
			return "string"_sst;
		case SemanticTokenTypes::Number:
		case SemanticTokenTypes::Regexp:
			return "number"_sst;
		case SemanticTokenTypes::Operator:
			return "operator"_sst;
		case SemanticTokenTypes::Decorator:
			return "literal"_sst;
		case SemanticTokenTypes::Unknown:
			break;
	};
	return SyntaxStyleTypes::Normal;
}

void LSPDocumentClient::processTokens( LSPSemanticTokensDelta&& tokens,
									   const Uint64& docModificationId ) {
	if ( mDoc == nullptr || mServer == nullptr )
		return;

	// If the document has already being modified after requesting the semantic highlighting,
	// re-request the changes
	if ( docModificationId != mDoc->getModificationId() )
		return requestSemanticHighlightingDelayed();

	BoolScopedOp op( mRunningSemanticTokens, true );

	if ( !tokens.resultId.empty() )
		mSemanticeResultId = tokens.resultId;

	for ( const auto& edit : tokens.edits ) {
		auto& curTokens = mSemanticTokens.data;
		if ( edit.deleteCount > 0 ) {
			curTokens.erase( curTokens.begin() + edit.start,
							 curTokens.begin() + edit.start + edit.deleteCount );
		}
		curTokens.insert( curTokens.begin() + edit.start, edit.data.begin(), edit.data.end() );

		if ( mShutdown )
			return;
	}

	if ( !tokens.data.empty() ) {
		mSemanticTokens = std::move( tokens );
	}

	highlight();
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
	Int64 firstLine = !data.empty() ? data[0] : -1;
	Int64 currentLine = 0;
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
			line->tokens.push_back( { semanticTokenTypeToSyntaxType( ltype ), start, len } );
		} else {
			line->tokens.push_back( { SyntaxStyleTypes::Normal, start, len } );
		}
		line->hash = mDoc->getLineHash( currentLine );
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

	if ( firstLine != -1 && lastLine >= firstLine && mDoc ) {
		getServer()->getManager()->getPlugin()->getManager()->getSplitter()->forEachEditor(
			[this, firstLine, lastLine]( UICodeEditor* editor ) {
				if ( editor->isVisible() && &editor->getDocument() == mDoc &&
					 editor->getVisibleRange().intersectsLineRange( firstLine, lastLine ) ) {
					editor->runOnMainThread( [editor] { editor->invalidateDraw(); } );
				}
			} );
	}

	if ( !mServer->isSilent() ) {
		Log::debug(
			"LSPDocumentClient::highlight took: %.2f ms. Diff analysis took: %.2f ms. Updated "
			"%lld elements",
			clock.getElapsedTime().asMilliseconds(), diff.asMilliseconds(), tokenizerLines.size() );
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
	if ( Engine::instance()->isMainThread() ) {
		mServer->getThreadPool()->run(
			[server, uri]() { server->documentSymbolsBroadcast( uri ); } );
	} else {
		server->documentSymbolsBroadcast( uri );
	}
}

void LSPDocumentClient::requestFoldRange() {
	eeASSERT( mDoc );
	LSPClientServer* server = mServer;
	if ( !server->getCapabilities().foldingRangeProvider )
		return;
	URI uri = mDoc->getURI();
	TextDocument* doc = mDoc;
	auto handler = [uri, server, doc]( const PluginIDType&,
									   const std::vector<LSPFoldingRange>& res ) {
		if ( !server->hasDocument( uri ) )
			return;
		std::vector<TextRange> regions;
		regions.reserve( res.size() );
		for ( const auto& region : res ) {
			if ( region.endLine - region.startLine > 1 )
				regions.push_back( { { region.startLine, 0 }, { region.endLine, 0 } } );
		}
		doc->getFoldRangeService().setFoldingRegions( regions );
	};

	if ( Engine::instance()->isMainThread() ) {
		server->getThreadPool()->run(
			[server, uri, handler]() { server->documentFoldingRange( uri, handler ); } );
	} else {
		server->documentFoldingRange( uri, handler );
	}
}

void LSPDocumentClient::requestSymbolsDelayed() {
	if ( !mServer || !mServer->getCapabilities().documentSymbolProvider )
		return;
	UISceneNode* sceneNode = getUISceneNode();
	if ( sceneNode ) {
		LSPDocumentClient* docClient = this;
		URI uri = mDoc->getURI();
		LSPClientServer* server = mServer;
		sceneNode->debounce(
			[docClient, server, uri]() {
				if ( server->hasDocument( uri ) )
					docClient->requestSymbols();
			},
			Seconds( 1.f ), mTag );
	}
}

} // namespace ecode
