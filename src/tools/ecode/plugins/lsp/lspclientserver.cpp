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
static const char* MEMBER_LOCATION = "location";
static const char* MEMBER_RANGE = "range";
static const char* MEMBER_LINE = "line";
static const char* MEMBER_CHARACTER = "character";
static const char* MEMBER_KIND = "kind";
static const char* MEMBER_LABEL = "label";
static const char* MEMBER_DOCUMENTATION = "documentation";
static const char* MEMBER_DETAIL = "detail";
static const char* MEMBER_COMMAND = "command";
static const char* MEMBER_EDIT = "edit";
static const char* MEMBER_TITLE = "title";
static const char* MEMBER_ARGUMENTS = "arguments";
static const char* MEMBER_DIAGNOSTICS = "diagnostics";
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

static json textDocumentURI( const URI& document ) {
	return json{ { MEMBER_URI, document.toString() } };
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

static json toJson( const TextRange& pos ) {
	return json{ { MEMBER_START, toJson( pos.start() ) }, { MEMBER_END, toJson( pos.end() ) } };
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

static json toJson( const std::vector<DocumentContentChange>& changes ) {
	json result;
	for ( const auto& change : changes ) {
		result.push_back(
			{ { MEMBER_RANGE, toJson( change.range ) }, { MEMBER_TEXT, change.text } } );
	}
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
		return value.contains( valueName ) &&
			   ( value[valueName].is_boolean() || value[valueName].is_object() );
	};

	auto sync = json["textDocumentSync"];
	caps.textDocumentSync.change = static_cast<LSPDocumentSyncKind>(
		( sync.is_object() ? sync["change"].get<int>() : sync.get<int>() ) );
	if ( sync.is_object() && sync.contains( "save" ) ) {
		auto save = sync["save"];
		if ( save.is_boolean() ) {
			caps.textDocumentSync.save.includeText = save.get<bool>();
		} else if ( save.is_object() && save.contains( "includeText" ) ) {
			caps.textDocumentSync.save.includeText = save["includeText"].get<bool>();
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
	if ( json.contains( "documentRangeFormattingProvider" ) )
		caps.documentRangeFormattingProvider =
			toBoolOrObject( json, "documentRangeFormattingProvider" );
	if ( json.contains( "documentOnTypeFormattingProvider" ) )
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

static std::vector<LSPDiagnostic> parseDiagnosticsArr( const json& result ) {
	std::vector<LSPDiagnostic> ret;
	for ( const auto& vdiag : result ) {
		auto diag = vdiag;
		auto range = parseRange( diag[MEMBER_RANGE] );
		auto severity = static_cast<LSPDiagnosticSeverity>( diag["severity"].get<int>() );
		auto code = diag.contains( "code" ) ? ( diag["code"].is_number_integer()
													? String::toString( diag["code"].get<int>() )
													: diag.at( "code" ).get<std::string>() )
											: "";
		auto source = diag.at( "source" ).get<std::string>();
		auto message = diag.at( MEMBER_MESSAGE ).get<std::string>();
		std::vector<LSPDiagnosticRelatedInformation> relatedInfoList;
		if ( diag.contains( "relatedInformation" ) ) {
			const auto relatedInfo = diag.at( "relatedInformation" );
			for ( const auto& vrelated : relatedInfo ) {
				auto related = vrelated;
				auto relLocation = parseLocation( related.at( MEMBER_LOCATION ) );
				auto relMessage = related.at( MEMBER_MESSAGE ).get<std::string>();
				relatedInfoList.push_back( { relLocation, relMessage } );
			}
		}
		ret.push_back( { range, severity, code, source, message, relatedInfoList } );
	}
	return ret;
}

static LSPPublishDiagnosticsParams parsePublishDiagnostics( const json& result ) {
	LSPPublishDiagnosticsParams ret;
	ret.uri = URI( result.at( MEMBER_URI ).get<std::string>() );
	ret.diagnostics = parseDiagnosticsArr( result.at( MEMBER_DIAGNOSTICS ) );
	return ret;
}

static bool isPositionValid( const TextPosition& pos ) {
	return pos.column() >= 0 && pos.line() >= 0;
}

static std::vector<LSPSymbolInformation> parseDocumentSymbols( const json& result ) {
	// the reply could be old SymbolInformation[] or new (hierarchical) DocumentSymbol[]
	// try to parse it adaptively in any case
	// if new style, hierarchy is specified clearly in reply
	// if old style, it is assumed the values enter linearly, that is;
	// * a parent/container is listed before its children
	// * if a name is defined/declared several times and then used as a parent,
	//   then we try to find such a parent whose range contains current range
	//   (otherwise fall back to using the last instance as a parent)
	std::vector<LSPSymbolInformation> ret;
	std::map<std::string, LSPSymbolInformation*> index;

	std::function<void( const json& symbol, LSPSymbolInformation* parent )> parseSymbol =
		[&]( const json& symbol, LSPSymbolInformation* parent ) {
			const auto& mrange = symbol.contains( MEMBER_RANGE )
									 ? symbol.at( MEMBER_RANGE )
									 : symbol[MEMBER_LOCATION].at( MEMBER_RANGE );
			auto range = parseRange( mrange );
			std::map<std::string, LSPSymbolInformation*>::iterator it = index.end();
			// if flat list, try to find parent by name
			if ( !parent ) {
				auto container = symbol.value( "containerName", "" );
				it = index.find( container );
				// default to last inserted
				if ( it != index.end() ) {
					parent = it->second;
				}
				// but prefer a containing range
				while ( it != index.end() && it->first == container ) {
					if ( it->second->range.contains( range ) ) {
						parent = it->second;
						break;
					}
					++it;
				}
			}
			auto list = parent ? &parent->children : &ret;
			if ( isPositionValid( range.start() ) && isPositionValid( range.end() ) ) {
				auto name = symbol.at( ( "name" ) ).get<std::string>();
				auto kind = static_cast<LSPSymbolKind>( symbol.at( MEMBER_KIND ).get<int>() );
				auto detail = symbol.value( MEMBER_DETAIL, "" );
				list->push_back( { name, kind, range, detail } );
				index.insert( std::pair( name, &list->back() ) );
				// proceed recursively
				if ( symbol.contains( "children" ) ) {
					const auto& children = symbol.at( ( "children" ) );
					for ( const auto& child : children )
						parseSymbol( child, &list->back() );
				}
			}
		};

	const auto symInfos = result;
	for ( const auto& info : symInfos )
		parseSymbol( info, nullptr );
	return ret;
}

static LSPResponseError parseResponseError( const json& v ) {
	LSPResponseError ret;
	if ( v.is_object() ) {
		const auto& vm = v;
		ret.code = LSPErrorCode( vm.at( MEMBER_CODE ).get<int>() );
		ret.message = vm.at( MEMBER_MESSAGE ).get<std::string>();
		ret.data = vm.value( "data", json() );
	}
	return ret;
}

static std::vector<LSPTextEdit> parseTextEdit( const json& result ) {
	std::vector<LSPTextEdit> ret;
	const auto textEdits = result;
	for ( const auto& redit : textEdits ) {
		auto edit = redit;
		auto text = edit.at( "newText" ).get<std::string>();
		auto range = parseRange( edit.at( MEMBER_RANGE ) );
		ret.push_back( { range, text } );
	}
	return ret;
}

static void fromJson( LSPVersionedTextDocumentIdentifier& id, const json& json ) {
	if ( json.is_object() ) {
		auto ob = json;
		id.uri = URI( ob.at( MEMBER_URI ).get<std::string>() );
		id.version = ob.contains( MEMBER_VERSION ) ? ob.at( MEMBER_VERSION ).get<int>() : -1;
	}
}

static LSPTextDocumentEdit parseTextDocumentEdit( const json& result ) {
	LSPTextDocumentEdit ret;
	auto ob = result;
	fromJson( ret.textDocument, ob.at( "textDocument" ) );
	ret.edits = parseTextEdit( ob.at( "edits" ) );
	return ret;
}

static LSPWorkspaceEdit parseWorkSpaceEdit( const json& result ) {
	LSPWorkspaceEdit ret;
	auto changes = result.at( "changes" );
	for ( auto it = changes.begin(); it != changes.end(); ++it ) {
		ret.changes.insert( std::pair<URI, std::vector<LSPTextEdit>>(
			URI( it.key() ), parseTextEdit( it.value() ) ) );
	}
	auto documentChanges = result.at( "documentChanges" );
	// resourceOperations not supported for now
	for ( auto edit : documentChanges ) {
		ret.documentChanges.push_back( parseTextDocumentEdit( edit ) );
	}
	return ret;
}

static LSPCommand parseCommand( const json& result ) {
	auto title = result.at( MEMBER_TITLE ).get<std::string>();
	auto command = result.at( MEMBER_COMMAND ).get<std::string>();
	auto args = result.at( MEMBER_ARGUMENTS );
	return { title, command, args };
}

static std::vector<LSPDiagnostic> parseDiagnostics( const json& result ) {
	std::vector<LSPDiagnostic> ret;
	for ( const auto& vdiag : result ) {
		const auto& diag = vdiag;
		auto range = parseRange( diag.at( MEMBER_RANGE ) );
		auto severity = static_cast<LSPDiagnosticSeverity>( diag.value<int>( "severity", 0 ) );
		std::string code;
		if ( diag.contains( "code" ) ) {
			if ( diag["code"].is_number_integer() )
				code = String::toString( diag["code"].get<int>() );
			else
				code = diag.value( "code", "" );
		}

		auto source = diag.value( "source", "" );
		auto message = diag.value( MEMBER_MESSAGE, "" );
		std::vector<LSPDiagnosticRelatedInformation> relatedInfoList;
		if ( diag.contains( "relatedInformation" ) ) {
			const auto& relatedInfo = diag.at( "relatedInformation" );
			for ( const auto& related : relatedInfo ) {
				auto relLocation = parseLocation( related.at( MEMBER_LOCATION ) );
				auto relMessage = related.value( MEMBER_MESSAGE, "" );
				relatedInfoList.push_back( { relLocation, relMessage } );
			}
		}
		ret.push_back( { range, severity, code, source, message, relatedInfoList } );
	}
	return ret;
}

static std::vector<LSPCodeAction> parseCodeAction( const json& result ) {
	std::vector<LSPCodeAction> ret;
	const auto& codeActions = result;
	for ( const auto& vaction : codeActions ) {
		auto& action = vaction;
		// entry could be Command or CodeAction
		if ( !action.at( MEMBER_COMMAND ).is_string() ) {
			// CodeAction
			auto title = action.at( MEMBER_TITLE ).get<std::string>();
			auto kind = action.value( MEMBER_KIND, "" );
			auto command = action.contains( MEMBER_COMMAND )
							   ? parseCommand( action.at( MEMBER_COMMAND ) )
							   : LSPCommand{};
			auto edit = action.at( MEMBER_EDIT ) ? parseWorkSpaceEdit( action.at( MEMBER_EDIT ) )
												 : LSPWorkspaceEdit{};
			auto diagnostics = action.contains( MEMBER_DIAGNOSTICS )
								   ? parseDiagnostics( action.at( MEMBER_DIAGNOSTICS ) )
								   : std::vector<LSPDiagnostic>{};
			LSPCodeAction action = { title, kind, diagnostics, edit, command };
			ret.push_back( action );
		} else {
			// Command
			auto command = parseCommand( action );
			ret.push_back( { command.title, std::string(), {}, {}, command } );
		}
	}
	return ret;
}

static json toJson( const LSPLocation& location ) {
	if ( !location.uri.empty() ) {
		return json{ { MEMBER_URI, location.uri.toString() },
					 { MEMBER_RANGE, toJson( location.range ) } };
	}
	return json();
}

static json toJson( const LSPDiagnosticRelatedInformation& related ) {
	auto loc = toJson( related.location );
	if ( loc.is_object() ) {
		return json{ { MEMBER_LOCATION, toJson( related.location ) },
					 { MEMBER_MESSAGE, related.message } };
	}
	return json();
}

static json toJson( const LSPDiagnostic& diagnostic ) {
	// required
	auto result = json();
	result[MEMBER_RANGE] = toJson( diagnostic.range );
	result[MEMBER_MESSAGE] = diagnostic.message;
	// optional
	if ( !diagnostic.code.empty() )
		result[( "code" )] = diagnostic.code;
	if ( diagnostic.severity != LSPDiagnosticSeverity::Unknown )
		result[( "severity" )] = static_cast<int>( diagnostic.severity );
	if ( !diagnostic.source.empty() )
		result[( "source" )] = diagnostic.source;
	json relatedInfo;
	for ( const auto& vrelated : diagnostic.relatedInformation ) {
		auto related = toJson( vrelated );
		if ( related.is_object() ) {
			relatedInfo.push_back( related );
		}
	}
	result[( "relatedInformation" )] = relatedInfo;
	return result;
}

static json codeActionParams( const URI& document, const TextRange& range,
							  const std::vector<std::string>& kinds,
							  const std::vector<LSPDiagnostic>& diagnostics ) {
	auto params = textDocumentParams( document );
	params[MEMBER_RANGE] = toJson( range );
	json context;
	json diags;
	for ( const auto& diagnostic : diagnostics ) {
		diags.push_back( toJson( diagnostic ) );
	}
	context[MEMBER_DIAGNOSTICS] = diags;
	if ( !kinds.empty() )
		context["only"] = json( kinds );
	params["context"] = context;
	return params;
}

static LSPMarkupContent parseMarkupContent( const json& v ) {
	LSPMarkupContent ret;
	if ( v.is_object() ) {
		ret.value = v.at( "value" );
		auto kind = v.value( MEMBER_KIND, "plaintext" );
		if ( kind == "plaintext" ) {
			ret.kind = LSPMarkupKind::PlainText;
		} else if ( kind == "markdown" ) {
			ret.kind = LSPMarkupKind::MarkDown;
		}
	} else if ( v.is_string() ) {
		ret.kind = LSPMarkupKind::PlainText;
		ret.value = v.get<std::string>();
	}
	return ret;
}

static LSPHover parseHover( const json& result ) {
	LSPHover ret;
	if ( result.is_null() )
		return ret;

	if ( result.contains( MEMBER_RANGE ) )
		ret.range = parseRange( result.at( MEMBER_RANGE ) );
	const auto& contents = result.at( "contents" );

	if ( contents.is_array() ) {
		for ( const auto& c : contents )
			ret.contents.push_back( parseMarkupContent( c ) );
	} else {
		ret.contents.push_back( parseMarkupContent( contents ) );
	}

	return ret;
}

static std::vector<std::string> supportedSemanticTokenTypes() {
	return { "namespace",	  "type",	   "class",	   "enum",	   "interface",	 "struct",
			 "typeParameter", "parameter", "variable", "property", "enumMember", "event",
			 "function",	  "method",	   "macro",	   "keyword",  "modifier",	 "comment",
			 "string",		  "number",	   "regexp",   "operator" };
}

static std::vector<LSPCompletionItem> parseDocumentCompletion( const json& result ) {
	std::vector<LSPCompletionItem> ret;
	if ( result.empty() )
		return {};
	const json& items =
		( result.is_object() && result.contains( "items" ) ) ? result["items"] : result;

	for ( const auto& item : items ) {
		auto label = item.value( MEMBER_LABEL, "" );
		auto detail = item.value( MEMBER_DETAIL, "" );
		LSPMarkupContent doc = item.contains( MEMBER_DOCUMENTATION )
								   ? parseMarkupContent( item.at( MEMBER_DOCUMENTATION ) )
								   : LSPMarkupContent{};
		auto sortText = item.value( "sortText", "" );
		if ( sortText.empty() )
			sortText = label;
		auto insertText = item.value( "insertText", "" );
		if ( insertText.empty() )
			insertText = label;
		if ( item.contains( "textEdit" ) ) {
			const auto& textEdit = item["textEdit"];
			if ( !textEdit.empty() ) {
				auto newText = textEdit.value( "newText", "" );
				insertText = newText;
			}
		}
		auto kind = static_cast<LSPCompletionItemKind>( item.value( MEMBER_KIND, 1 ) );

		const std::vector<LSPTextEdit> additionalTextEdits =
			item.contains( "additionalTextEdits" )
				? parseTextEdit( item.at( "additionalTextEdits" ) )
				: std::vector<LSPTextEdit>{};

		ret.push_back( { label, kind, detail, doc, sortText, insertText,
						 additionalTextEdits /*, textEdit*/ } );
	}
	return ret;
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
				  { "hover", json{ { "contentFormat", { "plaintext" } } } } },
		},
		{ "window", json{ { "workDoneProgress", true } } },
		{ "general", json{ { "positionEncodings", json::array( { "utf-32" } ) } } } };

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
#ifndef EE_DEBUG
			try {
#endif
				fromJson( mCapabilities, resp["capabilities"] );
#ifndef EE_DEBUG
			} catch ( const json::exception& e ) {
				Log::warning(
					"LSPClientServer::initialize server %s error parsing capabilities: %s",
					mLSP.name.c_str(), e.what() );
			}
#endif

			mReady = true;
			write( newRequest( "initialized" ) );
			sendQueuedMessages();
		},
		[&]( const json& ) {} );
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

const LSPServerCapabilities& LSPClientServer::getCapabilities() const {
	return mCapabilities;
}

LSPClientServer::RequestHandle LSPClientServer::cancel( int reqid ) {
	Lock l( mHandlersMutex );
	if ( mHandlers.erase( reqid ) > 0 ) {
		auto params = json{ MEMBER_ID, reqid };
		return write( newRequest( "$/cancelRequest", params ) );
	}
	return RequestHandle();
}

LSPClientServer::RequestHandle LSPClientServer::write( const json& msg, const JsonReplyHandler& h,
													   const JsonReplyHandler& eh, const int id ) {
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
		Lock l( mHandlersMutex );
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

LSPClientServer::RequestHandle LSPClientServer::send( const json& msg, const JsonReplyHandler& h,
													  const JsonReplyHandler& eh ) {
	if ( mProcess.isAlive() ) {
		return write( msg, h, eh );
	} else {
		Log::debug( "LSPClientServer server %s Send for non-running server: %s", mLSP.name.c_str(),
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
	return didSave( doc->getURI(), mCapabilities.textDocumentSync.save.includeText
									   ? doc->getText().toUtf8()
									   : "" );
}

LSPClientServer::RequestHandle
LSPClientServer::didChange( const URI& document, int version, const std::string& text,
							const std::vector<DocumentContentChange>& change ) {
	auto params = textDocumentParams( document, version );
	params["contentChanges"] = !text.empty() ? json{ json{ MEMBER_TEXT, text } } : toJson( change );
	return send( newRequest( "textDocument/didChange", params ) );
}

LSPClientServer::RequestHandle
LSPClientServer::didChange( TextDocument* doc, const std::vector<DocumentContentChange>& change ) {
	Lock l( mClientsMutex );
	auto it = mClients.find( doc );
	if ( it != mClients.end() )
		return didChange( doc->getURI(), it->second->getVersion(),
						  change.empty() ? doc->getText().toUtf8() : "", change );
	return RequestHandle();
}

void LSPClientServer::updateDirty() {
	Lock l( mClientsMutex );
	for ( auto& client : mClients ) {
		if ( client.second->isDirty() ) {
			TextDocument* doc = client.first;
			LSPDocumentClient* docClient = client.second.get();
			mManager->getThreadPool()->run( [this, doc, docClient]() {
				didChange( doc, docClient->getDocChange() );
				docClient->clearDocChange();
			} );
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
																 const JsonReplyHandler& h,
																 const JsonReplyHandler& eh ) {
	auto params = textDocumentParams( document );
	return send( newRequest( "textDocument/documentSymbol", params ), h, eh );
}

LSPClientServer::RequestHandle
LSPClientServer::documentSymbols( const URI& document,
								  const ReplyHandler<std::vector<LSPSymbolInformation>>& h,
								  const ReplyHandler<LSPResponseError>& eh ) {
	return documentSymbols(
		document,
		[h]( const json& json ) {
			if ( h )
				h( parseDocumentSymbols( json ) );
		},
		[eh]( const json& json ) {
			if ( eh )
				eh( parseResponseError( json ) );
		} );
}

void fromJson( LSPWorkDoneProgressValue& value, const json& json ) {
	if ( !json.empty() ) {
		auto ob = json;
		auto kind = ob["kind"].get<std::string>();
		if ( kind == "begin" ) {
			value.kind = LSPWorkDoneProgressKind::Begin;
		} else if ( kind == "report" ) {
			value.kind = LSPWorkDoneProgressKind::Report;
		} else if ( kind == "end" ) {
			value.kind = LSPWorkDoneProgressKind::End;
		}
		value.title = ob.contains( "title" ) ? ob["title"].get<std::string>() : "";
		value.message = ob.contains( "message" ) ? ob["message"].get<std::string>() : "";
		value.cancellable = ob.contains( "cancellable" ) ? ob["cancellable"].get<bool>() : false;
		value.percentage = ob.contains( "percentage" ) ? ob["percentage"].get<int>() : 0;
	}
}

template <typename T> static LSPProgressParams<T> parseProgress( const json& json ) {
	LSPProgressParams<T> ret;
	ret.token = json.at( "token" );
	fromJson( ret.value, json.at( "value" ) );
	return ret;
}

static LSPWorkDoneProgressParams parseWorkDone( const json& json ) {
	return parseProgress<LSPWorkDoneProgressValue>( json );
}

static json newError( const LSPErrorCode& code, const std::string& msg ) {
	return json{
		{ MEMBER_ERROR, { { MEMBER_CODE, static_cast<int>( code ) }, { MEMBER_MESSAGE, msg } } } };
}

void LSPClientServer::publishDiagnostics( const json& msg ) {
	// should emmit event somewhere
	auto res = parsePublishDiagnostics( msg[MEMBER_PARAMS] );
	Log::debug( "LSPClientServer::publishDiagnostics: %s - returned %zu items",
				res.uri.toString().c_str(), res.diagnostics.size() );
}

void LSPClientServer::workDoneProgress( const LSPWorkDoneProgressParams& workDoneParams ) {
	// should emmit event somewhere
	Log::debug( "LSPClientServer::workDoneProgress: %s",
				workDoneParams.token.is_string()
					? workDoneParams.token.get<std::string>().c_str()
					: String::toString( workDoneParams.token.get<int>() ).c_str() );
}

void LSPClientServer::processNotification( const json& msg ) {
	auto method = msg[MEMBER_METHOD].get<std::string>();
	if ( method == "textDocument/publishDiagnostics" ) {
		publishDiagnostics( msg );
		return;
	} else if ( method == ( "window/showMessage" ) ) {
		// showMessage( msg );
	} else if ( method == ( "window/logMessage" ) ) {
		// logMessage( msg );
	} else if ( method == ( "$/progress" ) ) {
		workDoneProgress( parseWorkDone( msg[MEMBER_PARAMS] ) );
		return;
	} else {
	}
	Log::debug( "LSPClientServer::processNotification server %s: %s", mLSP.name.c_str(),
				msg.dump().c_str() );
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
			Log::debug( "LSPClientServer::readStdOut server %s invalid " CONTENT_LENGTH,
						mLSP.name.c_str() );
			// flush and try to carry on to some next header
			buffer.erase( 0, msgstart );
			continue;
		}
		// sanity check to avoid extensive buffering
		if ( length > ( 1 << 29 ) ) {
			Log::debug( "LSPClientServer::readStdOut server %s excessive size", mLSP.name.c_str() );
			buffer.clear();
			continue;
		}
		if ( msgstart + length > buffer.length() ) {
			break;
		}

		// now onto payload
		auto payload = buffer.substr( msgstart, length );
		buffer.erase( 0, msgstart + length );

		if ( payload.empty() ) {
			Log::debug( "LSPClientServer::readStdOut server %s empty payload", mLSP.name.c_str() );
			continue;
		}

#ifndef EE_DEBUG
		try {
#endif
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

			Lock l( mHandlersMutex );
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
#ifndef EE_DEBUG
		} catch ( const json::exception& e ) {
			Log::debug( "LSPClientServer::readStdOut server %s said: Coudln't parse json err: %s",
						mLSP.name.c_str(), e.what() );
		}
#endif
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
		Log::debug( "LSPClientServer::readStdErr server %s: %s", mLSP.name.c_str(),
					msg.message.c_str() );
	}
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

LSPClientServer::RequestHandle LSPClientServer::switchSourceHeader( const URI& document ) {
	return send( newRequest( "textDocument/switchSourceHeader", textDocumentURI( document ) ),
				 [this]( json res ) {
					 if ( res.is_string() ) {
						 mManager->goToLocation(
							 { res.get<std::string>(), TextRange{ { 0, 0 }, { 0, 0 } } } );
					 }
				 } );
}

LSPClientServer::RequestHandle
LSPClientServer::didChangeWorkspaceFolders( const std::vector<LSPWorkspaceFolder>& added,
											const std::vector<LSPWorkspaceFolder>& removed ) {
	auto params = changeWorkspaceFoldersParams( added, removed );
	return send( newRequest( "workspace/didChangeWorkspaceFolders", params ) );
}

LSPClientServer::RequestHandle LSPClientServer::documentCodeAction(
	const URI& document, const TextRange& range, const std::vector<std::string>& kinds,
	std::vector<LSPDiagnostic> diagnostics, const JsonReplyHandler& h ) {
	auto params = codeActionParams( document, range, kinds, std::move( diagnostics ) );
	return send( newRequest( "textDocument/codeAction", params ), h );
}

LSPClientServer::RequestHandle LSPClientServer::documentCodeAction(
	const URI& document, const TextRange& range, const std::vector<std::string>& kinds,
	std::vector<LSPDiagnostic> diagnostics, const CodeActionHandler& h ) {
	return documentCodeAction( document, range, kinds, diagnostics, [h]( const json& json ) {
		if ( h )
			h( parseCodeAction( json ) );
	} );
}

LSPClientServer::RequestHandle LSPClientServer::documentHover( const URI& document,
															   const TextPosition& pos,
															   const JsonReplyHandler& h ) {
	auto params = textDocumentPositionParams( document, pos );
	return send( newRequest( "textDocument/hover", params ), h );
}

LSPClientServer::RequestHandle LSPClientServer::documentHover( const URI& document,
															   const TextPosition& pos,
															   const HoverHandler& h ) {
	return documentHover( document, pos, [h]( const json& json ) {
		if ( h )
			h( parseHover( json ) );
	} );
}

LSPClientServer::RequestHandle LSPClientServer::documentHover( TextDocument* doc,
															   const HoverHandler& h ) {
	return documentHover( doc->getURI(), doc->getSelection().start(), h );
}

LSPClientServer::RequestHandle LSPClientServer::documentCompletion( const URI& document,
																	const TextPosition& pos,
																	const JsonReplyHandler& h ) {
	auto params = textDocumentPositionParams( document, pos );
	return send( newRequest( "textDocument/completion", params ), h );
}

LSPClientServer::RequestHandle LSPClientServer::documentCompletion( const URI& document,
																	const TextPosition& pos,
																	const CompletionHandler& h ) {
	return documentCompletion( document, pos, [h]( const json& json ) {
		if ( h )
			h( parseDocumentCompletion( json ) );
	} );
}

} // namespace ecode
