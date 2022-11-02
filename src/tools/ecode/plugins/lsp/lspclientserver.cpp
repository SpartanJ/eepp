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

#define CONTENT_LENGTH "Content-Length"
#define CONTENT_LENGTH_HEADER "Content-Length:"

static const char* MEMBER_ID = "id";
static const char* MEMBER_METHOD = "method";
static const char* MEMBER_PARAMS = "params";
static const char* MEMBER_URI = "uri";
static const char* MEMBER_VERSION = "version";
static const char* MEMBER_TEXT = "text";
static const char* MEMBER_LANGID = "languageId";
static const char* MEMBER_ERROR = "error";
static const char* MEMBER_CODE = "code";
static const char* MEMBER_MESSAGE = "message";
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

static json newRequest( const std::string& method, const json& params = {} ) {
	json j;
	j[MEMBER_METHOD] = method;
	j[MEMBER_PARAMS] = params.empty() ? json() : params;
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
	map[MEMBER_LANGID] = lang;
	map[MEMBER_TEXT] = text;
	return map;
}

static json textDocumentParams( const json& m ) {
	return json{ { "textDocument", m } };
}

static json textDocumentParams( const URI& document, int version = -1 ) {
	return textDocumentParams( versionedTextDocumentIdentifier( document, version ) );
}

static json toJson( const TextPosition& pos ) {
	return json{ { MEMBER_LINE, pos.line() }, { MEMBER_CHARACTER, pos.column() } };
}

static json workspaceFolder( const LSPWorkspaceFolder& response ) {
	return json{ { MEMBER_URI, response.uri.toString() }, { "name", response.name } };
}

static json toJson( const std::vector<LSPWorkspaceFolder>& l ) {
	if ( l.empty() )
		return json::array();
	json result;
	for ( const auto& e : l )
		result.push_back( workspaceFolder( e ) );
	return result;
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

static json changeWorkspaceFoldersParams( const std::vector<LSPWorkspaceFolder>& added,
										  const std::vector<LSPWorkspaceFolder>& removed ) {
	json event;
	event["added"] = toJson( added );
	event["removed"] = toJson( removed );
	return json{ { "event", event } };
}

static json textDocumentPositionParams( const URI& document, TextPosition pos ) {
	auto params = textDocumentParams( document );
	params[MEMBER_POSITION] = toJson( pos );
	return params;
}

static void fromJson( std::vector<char>& trigger, const json& json ) {
	if ( !json.empty() ) {
		const auto triggersArray = json;
		for ( const auto& t : triggersArray ) {
			auto st = t.get<std::string>();
			if ( st.length() )
				trigger.push_back( st.at( 0 ) );
		}
	}
}

static void fromJson( LSPCompletionOptions& options, const json& json ) {
	if ( !json.empty() && json.is_object() ) {
		auto ob = json;
		options.provider = true;
		options.resolveProvider = ob["resolveProvider"].get<bool>();
		fromJson( options.triggerCharacters, ob["triggerCharacters"] );
	}
}

static void fromJson( LSPSignatureHelpOptions& options, const json& json ) {
	if ( !json.empty() && json.is_object() ) {
		auto ob = json;
		options.provider = true;
		fromJson( options.triggerCharacters, ob["triggerCharacters"] );
	}
}

static void fromJson( LSPDocumentOnTypeFormattingOptions& options, const json& json ) {
	if ( !json.empty() && json.is_object() ) {
		auto ob = json;
		options.provider = true;
		fromJson( options.triggerCharacters, ob["moreTriggerCharacter"] );
		auto trigger = ob["firstTriggerCharacter"].get<std::string>();
		if ( trigger.size() )
			options.triggerCharacters.push_back( trigger.at( 0 ) );
	}
}

static void fromJson( LSPWorkspaceFoldersServerCapabilities& options, const json& json ) {
	if ( json.is_object() ) {
		auto ob = json;
		options.supported = ob["supported"].get<bool>();
		auto notify = ob["changeNotifications"].get<bool>();
		options.changeNotifications = notify;
	}
}

static void fromJson( LSPServerCapabilities& caps, const json& json ) {
	// in older protocol versions a support option is simply a boolean
	// in newer version it may be an object instead;
	// it should not be sent unless such support is announced, but let's handle it anyway
	// so consider an object there as a (good?) sign that the server is suitably capable
	auto toBoolOrObject = []( const nlohmann::json& value, const std::string& valueName ) {
		return value.contains( "valueName" ) &&
			   ( value[valueName].get<bool>() || value[valueName].is_object() );
	};

	auto sync = json["textDocumentSync"];
	caps.textDocumentSync.change = static_cast<LSPDocumentSyncKind>(
		( sync.is_object() ? sync["change"].get<int>() : sync.get<bool>() ) );
	if ( sync.is_object() ) {
		auto syncObject = sync;
		auto save = syncObject["save"];
		if ( !save.empty() && ( save.is_object() || save.get<bool>() ) ) {
			caps.textDocumentSync.save = { save["includeText"].get<bool>() };
		}
	}

	caps.hoverProvider = toBoolOrObject( json, "hoverProvider" );
	fromJson( caps.completionProvider, json["completionProvider"] );
	fromJson( caps.signatureHelpProvider, json["signatureHelpProvider"] );
	caps.definitionProvider = toBoolOrObject( json, "definitionProvider" );
	caps.declarationProvider = toBoolOrObject( json, "declarationProvider" );
	caps.typeDefinitionProvider = toBoolOrObject( json, "typeDefinitionProvider" );
	caps.referencesProvider = toBoolOrObject( json, "referencesProvider" );
	caps.implementationProvider = toBoolOrObject( json, "implementationProvider" );
	caps.documentSymbolProvider = toBoolOrObject( json, "documentSymbolProvider" );
	caps.documentHighlightProvider = toBoolOrObject( json, "documentHighlightProvider" );
	caps.documentFormattingProvider = toBoolOrObject( json, "documentFormattingProvider" );
	caps.documentRangeFormattingProvider =
		toBoolOrObject( json, "documentRangeFormattingProvider" );
	fromJson( caps.documentOnTypeFormattingProvider, json["documentOnTypeFormattingProvider"] );
	caps.renameProvider = toBoolOrObject( json, "renameProvider" );
	if ( json.contains( "codeActionProvider" ) &&
		 json["codeActionProvider"].contains( "resolveProvider" ) ) {
		auto codeActionProvider = json["codeActionProvider"];
		caps.codeActionProvider = json["codeActionProvider"]["resolveProvider"].get<bool>();
	}
	// fromJson( caps.semanticTokenProvider, json["semanticTokensProvider"] );
	if ( json.contains( "workspace" ) ) {
		auto workspace = json["workspace"];
		fromJson( caps.workspaceFolders, workspace["workspaceFolders"] );
	}
	caps.selectionRangeProvider = toBoolOrObject( json, "selectionRangeProvider" );
}

LSPClientServer::LSPClientServer( LSPClientServerManager* manager, const String::HashType& id,
								  const LSPDefinition& lsp, const std::string& rootPath ) :
	mManager( manager ), mId( id ), mLSP( lsp ), mRootPath( rootPath ) {}

LSPClientServer::~LSPClientServer() {
	{
		Lock l( mClientsMutex );
		for ( const auto& client : mClients )
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

	if ( mReady || msg[MEMBER_METHOD] == "initialize" ) {
		std::string method;
		if ( msg.contains( MEMBER_METHOD ) )
			method = msg[MEMBER_METHOD].get<std::string>();
		else if ( msg.contains( MEMBER_MESSAGE ) )
			method = msg[MEMBER_MESSAGE];
		Log::info( "LSPClientServer server %s calling %s", mLSP.name.c_str(), method.c_str() );
		Log::debug( "LSPClientServer server %s sending message:\n%s", mLSP.name.c_str(),
					sjson.c_str() );
		mProcess.write( sjson );
	} else {
		mQueuedMessages.push_back( { std::move( ob ), h, eh } );
	}

	return ret;
}

LSPClientServer::RequestHandle LSPClientServer::send( const json& msg, const GenericReplyHandler& h,
													  const GenericReplyHandler& eh ) {
	if ( mProcess.isAlive() ) {
		return write( msg, h, eh );
	} else {
		Log::error( "LSPClientServer server %s Send for non-running server: %s", mLSP.name.c_str(),
					mLSP.name.c_str() );
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

bool LSPClientServer::hasDocuments() const {
	return !mDocs.empty();
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

/*enum class LSPWorkDoneProgressKind { Begin, Report, End };

struct LSPWorkDoneProgressValue {
	LSPWorkDoneProgressKind kind;
	std::string title;
	std::string message;
	bool cancellable;
	unsigned percentage;
};
template <typename T> struct LSPProgressParams {
	// number or string
	json token;
	T value;
};

using LSPWorkDoneProgressParams = LSPProgressParams<LSPWorkDoneProgressValue>;

void fromJson( LSPWorkDoneProgressValue& value, const json& json ) {
	if ( json ) {
		auto ob = json;
		auto kind = ob["kind"].get<std::string>();
		if ( kind == "begin" ) {
			value.kind = LSPWorkDoneProgressKind::Begin;
		} else if ( kind == "report" ) {
			value.kind = LSPWorkDoneProgressKind::Report;
		} else if ( kind == "end" ) {
			value.kind = LSPWorkDoneProgressKind::End;
		}
		value.title = ob["title"].get<std::string>();
		value.message = ob["message"].get<std::string>();
		value.cancellable = ob["cancellable"].get<bool>();
		value.percentage = ob["percentage"].get<int>();
	}
}

template <typename T> static LSPProgressParams<T> parseProgress( const json& json ) {
	LSPProgressParams<T> ret;
	ret.token = json["token"];
	fromJson( ret.value, json["value"] );
	return ret;
}

static LSPWorkDoneProgressParams parseWorkDone( const json& json ) {
	return parseProgress<LSPWorkDoneProgressValue>( json );
}*/

static json newError( const LSPErrorCode& code, const std::string& msg ) {
	return json{
		{ MEMBER_ERROR, { { MEMBER_CODE, static_cast<int>( code ) }, { MEMBER_MESSAGE, msg } } } };
}

void LSPClientServer::processNotification( const json& msg ) {
	Log::warning( "LSPClientServer::processNotification server %s: %s", mLSP.name.c_str(),
				  msg.dump().c_str() );
	auto method = msg[MEMBER_METHOD].get<std::string>();
	if ( method == "textDocument/publishDiagnostics" ) {
		// publishDiagnostics( msg );
	} else if ( method == ( "window/showMessage" ) ) {
		// showMessage( msg );
	} else if ( method == ( "window/logMessage" ) ) {
		// logMessage( msg );
	} else if ( method == ( "$/progress" ) ) {
		// workDoneProgress( parseWorkDone( msg["params"] ) );
	} else {
	}
}

void LSPClientServer::processRequest( const json& msg ) {
	Log::debug( "LSPClientServer::processRequest server %s: %s", mLSP.name.c_str(),
				msg.dump().c_str() );
	auto method = msg[MEMBER_METHOD].get<std::string>();
	auto msgid = msg[MEMBER_ID].get<int>();
	//	auto params = msg[MEMBER_PARAMS];
	//	bool handled = false;
	write( newError( LSPErrorCode::MethodNotFound, method ), nullptr, nullptr, msgid );
}

void LSPClientServer::readStdOut( const char* bytes, size_t n ) {
	mReceive.append( bytes, n );

	std::string& buffer = mReceive;

	while ( true ) {
		auto index = buffer.find_first_of( CONTENT_LENGTH_HEADER );
		if ( index == std::string::npos ) {
			if ( buffer.size() > ( 1 << 20 ) )
				buffer.clear();
			break;
		}

		index += strlen( CONTENT_LENGTH_HEADER );
		auto endindex = buffer.find_first_of( "\r\n", index );
		auto msgstart = buffer.find_first_of( "\r\n\r\n", index );
		if ( endindex == std::string::npos || msgstart == std::string::npos )
			break;

		msgstart += 4;
		int length = 0;
		bool ok = String::fromString( length, buffer.substr( index, endindex - index ) );
		// FIXME perhaps detect if no reply for some time
		// then again possibly better left to user to restart in such case
		if ( !ok ) {
			Log::error( "LSPClientServer::readStdOut server %s invalid " CONTENT_LENGTH,
						mLSP.name.c_str() );
			// flush and try to carry on to some next header
			buffer.erase( 0, msgstart );
			continue;
		}
		// sanity check to avoid extensive buffering
		if ( length > ( 1 << 29 ) ) {
			Log::error( "LSPClientServer::readStdOut server %s excessive size", mLSP.name.c_str() );
			buffer.clear();
			continue;
		}
		if ( msgstart + length > buffer.length() ) {
			break;
		}

		// now onto payload
		auto payload = buffer.substr( msgstart, length );
		buffer.erase( 0, msgstart + length );

		if ( !payload.empty() ) {
			try {
				auto res = json::parse( payload );

				int msgid = -1;
				if ( res.contains( MEMBER_ID ) ) {
					msgid = res[MEMBER_ID].get<int>();
				} else {
					processNotification( res );
					continue;
				}

				if ( res.contains( MEMBER_METHOD ) ) {
					processRequest( res );
					continue;
				}

				Log::debug( "LSPClientServer::readStdOut server %s said: \n%s", mLSP.name.c_str(),
							res.dump().c_str() );

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
					Log::debug( "LSPClientServer::readStdOut server %s unexpected reply id: %d",
								mLSP.name.c_str(), msgid );
				}

				continue;
			} catch ( const json::exception& e ) {
				Log::debug(
					"LSPClientServer::readStdOut server %s said: Coudln't parse json err: %s",
					mLSP.name.c_str(), e.what() );
			}
		}
		Log::debug( "LSPClientServer::readStdOut server %s said: \n%s", mLSP.name.c_str(),
					payload.c_str() );
	}
}

void LSPClientServer::readStdErr( const char* bytes, size_t n ) {
	mReceiveErr += std::string( bytes, n );
	LSPShowMessageParams msg;
	const auto lastNewLineIndex = mReceiveErr.find_last_of( '\n' );
	if ( lastNewLineIndex != std::string::npos ) {
		msg.message = mReceiveErr.substr( 0, lastNewLineIndex );
		mReceiveErr.erase( 0, lastNewLineIndex + 1 );
	}
	if ( !msg.message.empty() ) {
		msg.type = LSPMessageType::Log;
		Log::warning( "LSPClientServer::readStdOut server %s: %s", mLSP.name.c_str(),
					  msg.message.c_str() );
	}
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
				 { "capabilities", capabilities },
				 { "initializationOptions", {} } };

	std::string rootPath = mRootPath;
	if ( rootPath.empty() ) {
		if ( !mManager->getLSPWorkspaceFolder().uri.empty() )
			rootPath = mManager->getLSPWorkspaceFolder().uri.getPath();
		else
			rootPath = FileSystem::getCurrentWorkingDirectory();
	}

	std::string uriRootPath = "file://" + rootPath;
	params["rootPath"] = rootPath;
	params["rootUri"] = uriRootPath;
	params["workspaceFolders"] =
		toJson( { LSPWorkspaceFolder{ uriRootPath, FileSystem::fileNameFromPath( rootPath ) } } );
	capabilities["workspace"] = json{ { "workspaceFolders", true }, { "configuration", false } };

	write(
		newRequest( "initialize", params ),
		[&]( const json& resp ) {
			try {
				fromJson( mCapabilities, resp["capabilities"] );
			} catch ( const json::exception& e ) {
				Log::warning(
					"LSPClientServer::initialize server %s error parsing capabilities: %s",
					mLSP.name.c_str(), e.what() );
			}

			mReady = true;
			write( newRequest( "initialized" ) );
			sendQueuedMessages();
		},
		[&]( const json& ) {} );
}

void LSPClientServer::sendQueuedMessages() {
	for ( const auto& msg : mQueuedMessages )
		write( msg.msg, msg.h, msg.eh );
	mQueuedMessages.clear();
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

LSPClientServer::RequestHandle
LSPClientServer::didChangeWorkspaceFolders( const std::vector<LSPWorkspaceFolder>& added,
											const std::vector<LSPWorkspaceFolder>& removed ) {
	auto params = changeWorkspaceFoldersParams( added, removed );
	return send( newRequest( "workspace/didChangeWorkspaceFolders", params ) );
}

} // namespace ecode
