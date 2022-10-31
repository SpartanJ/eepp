#include "lspclientserver.hpp"
#include "lspclientservermanager.hpp"
#include <algorithm>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/iostreamstring.hpp>
#include <eepp/system/lock.hpp>
#include <eepp/system/log.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/ui/doc/textdocument.hpp>

namespace ecode {

static const char* MEMBER_ID = "id";
static const char* MEMBER_METHOD = "method";
static const char* MEMBER_PARAMS = "params";
static const char* MEMBER_URI = "uri";
static const char* MEMBER_VERSION = "version";
static const char* MEMBER_TEXT = "text";
static const char* MEMBER_LANGID = "languageId";
static const char* MEMBER_ERROR = "error";
// static const char* MEMBER_CODE = "code";
// static const char* MEMBER_MESSAGE = "message";
static const char* MEMBER_RESULT = "result";
static const char* MEMBER_START = "start";
static const char* MEMBER_END = "end";
static const char* MEMBER_POSITION = "position";
// static const char* MEMBER_POSITIONS = "positions";
// static const char* MEMBER_LOCATION = "location";
static const char* MEMBER_RANGE = "range";
static const char* MEMBER_LINE = "line";
static const char* MEMBER_CHARACTER = "character";
// static const char* MEMBER_KIND = "kind";
// static const char* MEMBER_LABEL = "label";
// static const char* MEMBER_DOCUMENTATION = "documentation";
// static const char* MEMBER_DETAIL = "detail";
// static const char* MEMBER_COMMAND = "command";
// static const char* MEMBER_EDIT = "edit";
// static const char* MEMBER_TITLE = "title";
// static const char* MEMBER_ARGUMENTS = "arguments";
// static const char* MEMBER_DIAGNOSTICS = "diagnostics";
static const char* MEMBER_TARGET_URI = "targetUri";
static const char* MEMBER_TARGET_RANGE = "targetRange";
static const char* MEMBER_TARGET_SELECTION_RANGE = "targetSelectionRange";
// static const char* MEMBER_PREVIOUS_RESULT_ID = "previousResultId";
// static const char* MEMBER_QUERY = "query";

static json newRequest( const std::string& method, const json& params ) {
	json j;
	j[MEMBER_METHOD] = method;
	j[MEMBER_PARAMS] = params;
	return j;
}

static json versionedTextDocumentIdentifier( const URI& document, int version = -1 ) {
	json map{ { MEMBER_URI, document.toString() } };
	if ( version >= 0 )
		map[MEMBER_VERSION] = version;
	return map;
}

static json textDocumentItem( const URI& document, const std::string& lang, const std::string& text,
							  int version ) {
	auto map = versionedTextDocumentIdentifier( document, version );
	map[MEMBER_TEXT] = text;
	map[MEMBER_LANGID] = lang;
	return map;
}

static json textDocumentParams( const json& m ) {
	return json{ { "textDocument", m } };
}

static json textDocumentParams( const URI& document, int version = -1 ) {
	return textDocumentParams( versionedTextDocumentIdentifier( document, version ) );
}
static json to_json( const TextPosition& pos ) {
	return json{ { MEMBER_LINE, pos.line() }, { MEMBER_CHARACTER, pos.column() } };
}

static json textDocumentPositionParams( const URI& document, TextPosition pos ) {
	auto params = textDocumentParams( document );
	params[MEMBER_POSITION] = to_json( pos );
	return params;
}

LSPClientServer::LSPClientServer( LSPClientServerManager* manager, const String::HashType& id,
								  const LSPDefinition& lsp, const std::string& rootPath ) :
	mManager( manager ), mId( id ), mLSP( lsp ), mRootPath( rootPath ) {}

LSPClientServer::~LSPClientServer() {
	Lock l( mClientsMutex );
	for ( const auto& client : mClients )
		client.first->unregisterClient( client.second.get() );
}

bool LSPClientServer::start() {
	bool ret = mProcess.create(
		mLSP.command, Process::getDefaultOptions() | (Uint32)Process::Options::CombinedStdoutStderr,
		{}, mRootPath );
	if ( ret ) {
		mProcess.startAsyncRead( [this]( const char* bytes, size_t n ) { readStdOut( bytes, n ); },
								 []( const char*, size_t ) {} );

		initialize();
	}
	return ret;
}

bool LSPClientServer::registerDoc( const std::shared_ptr<TextDocument>& doc ) {
	Lock l( mClientsMutex );
	for ( TextDocument* cdoc : mDocs ) {
		if ( cdoc == doc.get() ) {
			if ( mClients.find( doc.get() ) == mClients.end() ) {
				mClients[doc.get()] = std::make_unique<LSPDocumentClient>( this, doc.get() );
				return true;
			}
			return false;
		}
	}

	mClients[doc.get()] = std::make_unique<LSPDocumentClient>( this, doc.get() );
	mDocs.emplace_back( doc.get() );
	doc->registerClient( mClients[doc.get()].get() );
	return true;
}

LSPClientServer::RequestHandle LSPClientServer::cancel( int reqid ) {
	if ( mHandlers.erase( reqid ) > 0 ) {
		auto params = json{ MEMBER_ID, reqid };
		return write( newRequest( "$/cancelRequest", params ) );
	}
	return RequestHandle();
}

LSPClientServer::RequestHandle LSPClientServer::write( const json& msg,
													   const GenericReplyHandler& h,
													   const GenericReplyHandler& eh,
													   const int id ) {
	RequestHandle ret;
	ret.mServer = this;

	if ( !mProcess.isAlive() )
		return ret;

	auto ob = msg;
	ob["jsonrpc"] = "2.0";

	// notification == no handler
	if ( h ) {
		ob[MEMBER_ID] = ++mLastMsgId;
		ret.mId = mLastMsgId;
		mHandlers[mLastMsgId] = { h, eh };
	} else if ( id ) {
		ob[MEMBER_ID] = id;
	}

	std::string sjson = ob.dump();
	sjson = String::format( "Content-Length: %lu\r\n\r\n%s", sjson.length(), sjson.c_str() );

	Log::info( "LSPClient calling %s", msg["method"].get<std::string>().c_str() );
	Log::debug( "LSPClient sending message:\n%s", sjson.c_str() );

	mProcess.write( sjson );

	return ret;
}

LSPClientServer::RequestHandle LSPClientServer::send( const json& msg, const GenericReplyHandler& h,
													  const GenericReplyHandler& eh ) {
	if ( mProcess.isAlive() ) {
		return write( msg, h, eh );
	} else {
		Log::error( "LSPClientServer - Send for non-running server: %s - %s", mLSP.name.c_str(),
					mLSP.language.c_str() );
	}
	return RequestHandle();
}

LSPClientServer::RequestHandle LSPClientServer::didOpen( const URI& document,
														 const std::string& text, int version ) {
	auto params = textDocumentParams( textDocumentItem( document, mLSP.language, text, version ) );
	return send( newRequest( "textDocument/didOpen", params ) );
}

LSPClientServer::RequestHandle LSPClientServer::didOpen( TextDocument* doc, int version ) {
	if ( doc->isDirty() ) {
		IOStreamString text;
		doc->save( text, true );
		return didOpen( doc->getURI(), text.getStream(), version );
	} else {
		std::string text;
		FileSystem::fileGet( doc->getFilePath(), text );
		return didOpen( doc->getURI(), text, version );
	}
}

LSPClientServer::RequestHandle LSPClientServer::didSave( const URI& document,
														 const std::string& text ) {
	auto params = textDocumentParams( document );
	if ( !text.empty() )
		params["text"] = text;
	return send( newRequest( "textDocument/didSave", params ) );
}

LSPClientServer::RequestHandle LSPClientServer::didSave( TextDocument* doc ) {
	return didSave( doc->getURI(), doc->getText( doc->getDocRange() ).toUtf8() );
}

LSPClientServer::RequestHandle LSPClientServer::didChange( const URI& document, int version,
														   const std::string& text ) {
	auto params = textDocumentParams( document, version );
	if ( !text.empty() )
		params["contentChanges"] = { json{ MEMBER_TEXT, text } };
	return send( newRequest( "textDocument/didChange", params ) );
}

LSPClientServer::RequestHandle LSPClientServer::didChange( TextDocument* doc ) {
	Lock l( mClientsMutex );
	auto it = mClients.find( doc );
	if ( it != mClients.end() )
		return didChange( doc->getURI(), it->second->getVersion(),
						  doc->getText( doc->getDocRange() ).toUtf8() );
	return RequestHandle();
}

void LSPClientServer::updateDirty() {
	Lock l( mClientsMutex );
	for ( auto& client : mClients ) {
		if ( client.second->isDirty() ) {
			TextDocument* doc = client.first;
			mManager->getThreadPool()->run( [this, doc]() { didChange( doc ); } );
			client.second->resetDirty();
		}
	}
}

bool LSPClientServer::hasDocument( TextDocument* doc ) const {
	return std::find( mDocs.begin(), mDocs.end(), doc ) != mDocs.end();
}

LSPClientServer::RequestHandle LSPClientServer::didClose( const URI& document ) {
	auto params = textDocumentParams( document );
	return send( newRequest( "textDocument/didClose", params ) );
}

LSPClientServer::RequestHandle LSPClientServer::didClose( TextDocument* doc ) {
	auto ret = didClose( doc->getURI() );
	Lock l( mClientsMutex );
	if ( mClients.erase( doc ) > 0 ) {
		auto it = std::find( mDocs.begin(), mDocs.end(), doc );
		if ( it != mDocs.end() )
			mDocs.erase( it );
		// No more docs are being used, close the LSP
		if ( mDocs.empty() )
			mManager->notifyClose( mId );
	}
	return ret;
}

LSPClientServerManager* LSPClientServer::getManager() const {
	return mManager;
}

const std::shared_ptr<ThreadPool>& LSPClientServer::getThreadPool() const {
	return mManager->getThreadPool();
}

LSPClientServer::RequestHandle LSPClientServer::documentSymbols( const URI& document,
																 const GenericReplyHandler& h,
																 const GenericReplyHandler& eh ) {
	auto params = textDocumentParams( document );
	return send( newRequest( "textDocument/documentSymbol", params ), h, eh );
}

void LSPClientServer::processNotification( const json& msg ) {
	auto method = msg[MEMBER_METHOD].get<std::string>();
	if ( method == "textDocument/publishDiagnostics" ) {
		// publishDiagnostics( msg );
	} else if ( method == ( "window/showMessage" ) ) {
		// showMessage( msg );
	} else if ( method == ( "window/logMessage" ) ) {
		// logMessage( msg );
	} else if ( method == ( "$/progress" ) ) {
		// workDoneProgress( msg );
	} else {
		Log::warning( "LSPClientServer::processNotification msg discarded: %s",
					  msg.dump( 2, ' ' ) );
	}
}
void LSPClientServer::processRequest( const json& /*msg*/ ) {
	//	auto method = msg[MEMBER_METHOD].get<std::string>();
	//	auto msgid = msg[MEMBER_ID];
	//	auto params = msg[MEMBER_PARAMS];
	//	bool handled = false;
}

void LSPClientServer::readStdOut( const char* bytes, size_t /*n*/ ) {
	const char* skipLength = strstr( bytes, "\r\n\r\n" );
	if ( nullptr != skipLength ) {
		try {
			auto res = json::parse( skipLength + 4 );
			Log::debug( "LSP Server %s said: \n%s", mLSP.name.c_str(), res.dump( 2, ' ' ).c_str() );

			int msgid = -1;
			if ( res.contains( MEMBER_ID ) ) {
				msgid = res[MEMBER_ID].get<int>();
			} else {
				processNotification( res );
				return;
			}

			if ( res.contains( MEMBER_METHOD ) ) {
				processRequest( res );
				return;
			}

			auto it = mHandlers.find( msgid );
			if ( it != mHandlers.end() ) {
				const auto handler = *it;
				mHandlers.erase( it );
				auto& h = handler.second.first;
				auto& eh = handler.second.second;
				if ( res.contains( MEMBER_ERROR ) && eh ) {
					eh( res[MEMBER_ERROR] );
				} else {
					h( res[MEMBER_RESULT] );
				}
			} else {
				Log::debug( "LSPClientServer::readStdOut unexpected reply id: %d", msgid );
			}

			return;
		} catch ( const json::exception& e ) {
			Log::debug( "LSP Server %s said: Coudln't parse json err: %s", mLSP.name.c_str(),
						e.what() );
		}
	}
	Log::debug( "LSP Server %s said: \n%s", mLSP.name.c_str(), bytes );
}

static std::vector<std::string> supportedSemanticTokenTypes() {
	return { "namespace",	  "type",	   "class",	   "enum",	   "interface",	 "struct",
			 "typeParameter", "parameter", "variable", "property", "enumMember", "event",
			 "function",	  "method",	   "macro",	   "keyword",  "modifier",	 "comment",
			 "string",		  "number",	   "regexp",   "operator" };
}

void LSPClientServer::initialize() {
	json codeAction{
		{ "codeActionLiteralSupport", json{ { "codeActionKind", json{ { "valueSet", {} } } } } } };

	json semanticTokens{
		{ "requests", json{ { "range", true }, { "full", json{ { "delta", true } } } } },
		{ "tokenTypes", supportedSemanticTokenTypes() },
		{ "tokenModifiers", {} },
		{ "formats", { "relative" } },
	};

	json capabilities{
		{
			"textDocument",
			json{ { "documentSymbol", json{ { "hierarchicalDocumentSymbolSupport", true } } },
				  { "publishDiagnostics", json{ { "relatedInformation", true } } },
				  { "codeAction", codeAction },
				  { "semanticTokens", semanticTokens },
				  { "synchronization", json{ { "didSave", true } } },
				  { "selectionRange", json{ { "dynamicRegistration", false } } },
				  { "hover", json{ { "contentFormat", { "markdown", "plaintext" } } } } },
		},
		{ "window", json{ { "workDoneProgress", true } } } };

	json params{ { "processId", Sys::getProcessID() },
				 { "rootPath", !mRootPath.empty() ? mRootPath : "" },
				 { "rootUri", !mRootPath.empty() ? "file://" + mRootPath : "" },
				 { "capabilities", capabilities },
				 { "initializationOptions", {} } };

	write(
		newRequest( "initialize", params ),
		[&]( const json& ) {

		},
		[&]( const json& ) {

		} );
}

static TextPosition parsePosition( const json& m ) {
	auto line = m[MEMBER_LINE].get<int>();
	auto column = m[MEMBER_CHARACTER].get<int>();
	return { line, column };
}

static TextRange parseRange( const json& range ) {
	auto startpos = parsePosition( range[MEMBER_START] );
	auto endpos = parsePosition( range[MEMBER_END] );
	return { startpos, endpos };
}

static LSPLocation parseLocation( const json& loc ) {
	auto uri = URI( loc[MEMBER_URI].get<std::string>() );
	auto range = parseRange( loc[MEMBER_RANGE] );
	return { uri, range };
}

static LSPLocation parseLocationLink( const json& loc ) {
	auto uri = URI( loc[MEMBER_TARGET_URI].get<std::string>() );
	auto vrange = loc[MEMBER_TARGET_SELECTION_RANGE];
	if ( vrange.is_null() )
		vrange = loc[MEMBER_TARGET_RANGE];
	auto range = parseRange( vrange );
	return { uri, range };
}

static std::vector<LSPLocation> parseDocumentLocation( const json& result ) {
	std::vector<LSPLocation> ret;
	if ( result.is_array() ) {
		const auto& locs = result;
		for ( const auto& def : locs ) {
			const auto& ob = def;
			ret.push_back( parseLocation( ob ) );
			if ( ret.back().uri.empty() )
				ret.back() = parseLocationLink( ob );
		}
	} else if ( result.is_object() ) {
		ret.push_back( parseLocation( result ) );
	}
	return ret;
}

void LSPClientServer::goToLocation( const json& res ) {
	auto locs = parseDocumentLocation( res );
	if ( !locs.empty() )
		mManager->goToLocation( locs.front() );
}

LSPClientServer::RequestHandle LSPClientServer::getAndGoToLocation( const URI& document,
																	const TextPosition& pos,
																	const std::string& search ) {
	auto params = textDocumentPositionParams( document, pos );
	return send( newRequest( search, params ), [this]( json res ) { goToLocation( res ); } );
}

LSPClientServer::RequestHandle LSPClientServer::documentDefinition( const URI& document,
																	const TextPosition& pos ) {
	return getAndGoToLocation( document, pos, "textDocument/definition" );
}

LSPClientServer::RequestHandle LSPClientServer::documentDeclaration( const URI& document,
																	 const TextPosition& pos ) {
	return getAndGoToLocation( document, pos, "textDocument/declaration" );
}

LSPClientServer::RequestHandle LSPClientServer::documentTypeDefinition( const URI& document,
																		const TextPosition& pos ) {
	return getAndGoToLocation( document, pos, "textDocument/typeDefinition" );
}

LSPClientServer::RequestHandle LSPClientServer::documentImplementation( const URI& document,
																		const TextPosition& pos ) {
	return getAndGoToLocation( document, pos, "textDocument/implementation" );
}

} // namespace ecode
