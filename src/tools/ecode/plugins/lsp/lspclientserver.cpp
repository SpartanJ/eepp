#include "lspclientserver.hpp"
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
// static const char* MEMBER_ERROR = "error";
// static const char* MEMBER_CODE = "code";
// static const char* MEMBER_MESSAGE = "message";
// static const char* MEMBER_RESULT = "result";
// static const char* MEMBER_START = "start";
// static const char* MEMBER_END = "end";
// static const char* MEMBER_POSITION = "position";
// static const char* MEMBER_POSITIONS = "positions";
// static const char* MEMBER_LOCATION = "location";
// static const char* MEMBER_RANGE = "range";
// static const char* MEMBER_LINE = "line";
// static const char* MEMBER_CHARACTER = "character";
// static const char* MEMBER_KIND = "kind";
// static const char* MEMBER_LABEL = "label";
// static const char* MEMBER_DOCUMENTATION = "documentation";
// static const char* MEMBER_DETAIL = "detail";
// static const char* MEMBER_COMMAND = "command";
// static const char* MEMBER_EDIT = "edit";
// static const char* MEMBER_TITLE = "title";
// static const char* MEMBER_ARGUMENTS = "arguments";
// static const char* MEMBER_DIAGNOSTICS = "diagnostics";
// static const char* MEMBER_TARGET_URI = "targetUri";
// static const char* MEMBER_TARGET_RANGE = "targetRange";
// static const char* MEMBER_TARGET_SELECTION_RANGE = "targetSelectionRange";
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

LSPClientServer::LSPClientServer( const LSPDefinition& lsp, const std::string& rootPath ) :
	mLSP( lsp ), mRootPath( rootPath ) {}

LSPClientServer::~LSPClientServer() {
	for ( const auto& client : mClients ) {
		client.first->unregisterClient( client.second.get() );
	}
}

bool LSPClientServer::start() {
	bool ret = mProcess.create( mLSP.command, Process::getDefaultOptions(), {}, mRootPath );
	if ( ret ) {
		mProcess.startAsyncRead(
			[this]( const char* bytes, size_t n ) { readStdOut( bytes, n ); },
			[this]( const char* bytes, size_t n ) { readStdErr( bytes, n ); } );

		initialize();
	}
	return ret;
}

bool LSPClientServer::registerDoc( const std::shared_ptr<TextDocument>& doc ) {
	for ( auto& cdoc : mDocs ) {
		if ( cdoc.get() == doc.get() ) {
			if ( mClients.find( doc.get() ) == mClients.end() ) {
				mClients[doc.get()] = std::make_unique<LSPDocumentClient>( this, doc.get() );
				return true;
			}
			return false;
		}
	}

	mClients[doc.get()] = std::make_unique<LSPDocumentClient>( this, doc.get() );
	mDocs.emplace_back( doc );
	doc->registerClient( mClients[doc.get()].get() );
	return true;
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

LSPClientServer::RequestHandle LSPClientServer::documentSymbols( const URI& document,
																 const GenericReplyHandler& h,
																 const GenericReplyHandler& eh ) {
	auto params = textDocumentParams( document );
	return send( newRequest( "textDocument/documentSymbol", params ), h, eh );
}

void LSPClientServer::readStdOut( const char* bytes, size_t /*n*/ ) {
	const char* skipLength = strstr( bytes, "\r\n\r\n" );
	if ( nullptr != skipLength ) {
		try {
			auto j = json::parse( skipLength + 4 );
			Log::debug( "LSP Server %s said: \n%s", mLSP.name.c_str(), j.dump( 2, ' ' ).c_str() );
			return;
		} catch ( const json::exception& e ) {
			Log::debug( "LSP Server %s said: Coudln't parse json err: %s", mLSP.name.c_str(),
						e.what() );
		}
	}
	Log::debug( "LSP Server %s said: \n%s", mLSP.name.c_str(), bytes );
}

void LSPClientServer::readStdErr( const char* bytes, size_t /*n*/ ) {
	Log::debug( "LSP Server %s err said: \n%s", mLSP.name.c_str(), bytes );
}

static std::vector<std::string> supportedSemanticTokenTypes() {
	return { ( "namespace" ), ( "type" ),	  ( "class" ),		   ( "enum" ),
			 ( "interface" ), ( "struct" ),	  ( "typeParameter" ), ( "parameter" ),
			 ( "variable" ),  ( "property" ), ( "enumMember" ),	   ( "event" ),
			 ( "function" ),  ( "method" ),	  ( "macro" ),		   ( "keyword" ),
			 ( "modifier" ),  ( "comment" ),  ( "string" ),		   ( "number" ),
			 ( "regexp" ),	  ( "operator" ) };
}

void LSPClientServer::initialize() {
	json codeAction{ { ( "codeActionLiteralSupport" ),
					   json{ { ( "codeActionKind" ), json{ { ( "valueSet" ), {} } } } } } };

	json semanticTokens{
		{ ( "requests" ),
		  json{ { ( "range" ), true }, { ( "full" ), json{ { ( "delta" ), true } } } } },
		{ ( "tokenTypes" ), supportedSemanticTokenTypes() },
		{ ( "tokenModifiers" ), {} },
		{ ( "formats" ), { ( "relative" ) } },
	};

	json capabilities{
		{
			( "textDocument" ),
			json{
				{ ( "documentSymbol" ), json{ { ( "hierarchicalDocumentSymbolSupport" ), true } } },
				{ ( "publishDiagnostics" ), json{ { ( "relatedInformation" ), true } } },
				{ ( "codeAction" ), codeAction },
				{ ( "semanticTokens" ), semanticTokens },
				{ ( "synchronization" ), json{ { ( "didSave" ), true } } },
				{ ( "selectionRange" ), json{ { ( "dynamicRegistration" ), false } } },
				{ ( "hover" ),
				  json{ { ( "contentFormat" ), { ( "markdown" ), ( "plaintext" ) } } } } },
		},
		{ ( "window" ), json{ { ( "workDoneProgress" ), true } } } };

	json params{ { ( "processId" ), Sys::getProcessID() },
				 { ( "rootPath" ), !mRootPath.empty() ? mRootPath : "" },
				 { ( "rootUri" ), !mRootPath.empty() ? "file://" + mRootPath : "" },
				 { ( "capabilities" ), capabilities },
				 { ( "initializationOptions" ), {} } };

	write(
		newRequest( ( "initialize" ), params ),
		[&]( const json& ) {

		},
		[&]( const json& ) {

		} );
}

} // namespace ecode
