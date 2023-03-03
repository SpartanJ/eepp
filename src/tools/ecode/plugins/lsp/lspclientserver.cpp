#include "lspclientserver.hpp"
#include "lspclientplugin.hpp"
#include "lspclientservermanager.hpp"
#include <algorithm>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/iostreamstring.hpp>
#include <eepp/system/lock.hpp>
#include <eepp/system/log.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/ui/doc/textdocument.hpp>
#include <eepp/window/engine.hpp>

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
static const char* MEMBER_POSITIONS = "positions";
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
static const char* MEMBER_PREVIOUS_RESULT_ID = "previousResultId";
static const char* MEMBER_QUERY = "query";
static const char* MEMBER_SUCCESS = "success";
static const char* MEMBER_LIMIT = "limit";

static json newRequest( const std::string& method, const json& params = json{} ) {
	json j;
	j[MEMBER_METHOD] = method;
	j[MEMBER_PARAMS] = params.empty() ? json( json::value_t::object ) : params;
	return j;
}

static json newID( const PluginIDType& id ) {
	json j;
	if ( id.isInteger() )
		j[MEMBER_ID] = id.asInt();
	else
		j[MEMBER_ID] = id.asString();
	return j;
}

static json newEmptyResult( const PluginIDType& id ) {
	json j;
	j[MEMBER_RESULT] = json( json::value_t::object );
	if ( id.isInteger() )
		j[MEMBER_ID] = id.asInt();
	else
		j[MEMBER_ID] = id.asString();
	return j;
}

static json executeCommandParams( const std::string& command, const json& args ) {
	return json{ { MEMBER_COMMAND, command }, { MEMBER_ARGUMENTS, args } };
}

static json newSuccessResult( const PluginIDType& id, bool success = true ) {
	json j;
	json res;
	res[MEMBER_SUCCESS] = success;
	j[MEMBER_RESULT] = res;
	if ( id.isInteger() )
		j[MEMBER_ID] = id.asInt();
	else
		j[MEMBER_ID] = id.asString();
	return j;
}

static std::string jsonString( const json& container, const std::string& member,
							   const std::string& def ) {
	return container.is_object() && container.contains( member ) && container[member].is_string()
			   ? container.at( member ).get<std::string>()
			   : def;
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
	json vrange = loc[MEMBER_TARGET_SELECTION_RANGE];
	if ( vrange.is_null() )
		vrange = loc[MEMBER_TARGET_RANGE];
	auto range = parseRange( vrange );
	return { uri, range };
}

static std::vector<LSPLocation> parseDocumentLocation( const json& result ) {
	std::vector<LSPLocation> ret;
	if ( result.is_array() ) {
		for ( const auto& def : result ) {
			ret.push_back( parseLocation( def ) );
			if ( ret.back().uri.empty() )
				ret.back() = parseLocationLink( def );
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

static json referenceParams( const URI& document, TextPosition pos, bool decl ) {
	auto params = textDocumentPositionParams( document, pos );
	json refCtx;
	refCtx["includeDeclaration"] = decl;
	params["context"] = refCtx;
	return params;
}

static json toJson( const std::vector<TextPosition>& positions ) {
	json result;
	for ( const auto& position : positions )
		result.push_back( toJson( position ) );
	return result;
}

static json textDocumentPositionsParams( const URI& document,
										 const std::vector<TextPosition>& positions ) {
	auto params = textDocumentParams( document );
	params[MEMBER_POSITIONS] = toJson( positions );
	return params;
}

static void fromJson( std::vector<char>& trigger, const json& json ) {
	if ( !json.empty() ) {
		const auto& triggersArray = json;
		for ( const auto& t : triggersArray ) {
			auto st = t.get<std::string>();
			if ( st.length() )
				trigger.push_back( st.at( 0 ) );
		}
	}
}

static void fromJson( LSPCompletionOptions& options, const json& json ) {
	if ( !json.empty() && json.is_object() ) {
		options.provider = true;
		if ( json.contains( "resolveProvider" ) && !json["resolveProvider"].is_null() )
			options.resolveProvider = json["resolveProvider"].get<bool>();
		if ( json.contains( "triggerCharacters" ) && !json["triggerCharacters"].is_null() )
			fromJson( options.triggerCharacters, json["triggerCharacters"] );
	}
}

static void fromJson( LSPSignatureHelpOptions& options, const json& json ) {
	if ( !json.empty() && json.is_object() ) {
		auto& ob = json;
		options.provider = true;
		fromJson( options.triggerCharacters, ob["triggerCharacters"] );
	}
}

static void fromJson( LSPDocumentOnTypeFormattingOptions& options, const json& json ) {
	if ( !json.empty() && json.is_object() ) {
		auto& ob = json;
		options.provider = true;
		if ( ob.contains( "moreTriggerCharacter" ) )
			fromJson( options.triggerCharacters, ob["moreTriggerCharacter"] );
		auto trigger = ob["firstTriggerCharacter"].get<std::string>();
		if ( trigger.size() )
			options.triggerCharacters.push_back( trigger.at( 0 ) );
	}
}

static void fromJson( LSPWorkspaceFoldersServerCapabilities& options, const json& json ) {
	if ( json.is_object() ) {
		auto& ob = json;
		options.supported = ob.value( "supported", false );
		if ( ob["changeNotifications"].is_boolean() ) {
			options.changeNotifications = ob["changeNotifications"].get<bool>();
		} else if ( ob["changeNotifications"].is_string() ) {
			options.changeNotifications = true;
		}
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

	auto& sync = json["textDocumentSync"];
	caps.textDocumentSync.change = static_cast<LSPDocumentSyncKind>(
		( sync.is_object() ? sync["change"].get<int>() : sync.get<int>() ) );
	if ( sync.is_object() && sync.contains( "save" ) ) {
		auto& save = sync["save"];
		if ( save.is_boolean() ) {
			caps.textDocumentSync.save.includeText = save.get<bool>();
		} else if ( save.is_object() && save.contains( "includeText" ) ) {
			caps.textDocumentSync.save.includeText = save["includeText"].get<bool>();
		}
	}

	caps.hoverProvider = toBoolOrObject( json, "hoverProvider" );
	if ( json.contains( "completionProvider" ) )
		fromJson( caps.completionProvider, json["completionProvider"] );
	if ( json.contains( "signatureHelpProvider" ) )
		fromJson( caps.signatureHelpProvider, json["signatureHelpProvider"] );
	caps.astProvider = toBoolOrObject( json, "astProvider" );
	caps.definitionProvider = toBoolOrObject( json, "definitionProvider" );
	caps.declarationProvider = toBoolOrObject( json, "declarationProvider" );
	caps.typeDefinitionProvider = toBoolOrObject( json, "typeDefinitionProvider" );
	caps.referencesProvider = toBoolOrObject( json, "referencesProvider" );
	caps.implementationProvider = toBoolOrObject( json, "implementationProvider" );
	caps.documentSymbolProvider = toBoolOrObject( json, "documentSymbolProvider" );
	caps.documentHighlightProvider = toBoolOrObject( json, "documentHighlightProvider" );
	caps.documentFormattingProvider = toBoolOrObject( json, "documentFormattingProvider" );
	caps.workspaceSymbolProvider = toBoolOrObject( json, "workspaceSymbolProvider" );
	if ( json.contains( "documentRangeFormattingProvider" ) )
		caps.documentRangeFormattingProvider =
			toBoolOrObject( json, "documentRangeFormattingProvider" );
	if ( json.contains( "documentOnTypeFormattingProvider" ) )
		fromJson( caps.documentOnTypeFormattingProvider, json["documentOnTypeFormattingProvider"] );
	caps.renameProvider = toBoolOrObject( json, "renameProvider" );
	caps.codeActionProvider = json.contains( "codeActionProvider" );
	caps.executeCommandProvider = json.contains( "executeCommandProvider" );
	// fromJson( caps.semanticTokenProvider, json["semanticTokensProvider"] );
	if ( json.contains( "workspace" ) ) {
		auto& workspace = json["workspace"];
		if ( workspace.contains( "workspaceFolders" ) )
			fromJson( caps.workspaceFolders, workspace["workspaceFolders"] );
	}
	caps.selectionRangeProvider = toBoolOrObject( json, "selectionRangeProvider" );
	caps.ready = true;
}

static bool isPositionValid( const TextPosition& pos ) {
	return pos.column() >= 0 && pos.line() >= 0;
}

struct LSPSymbolInformationTmp {
	LSPSymbolInformationTmp() = default;
	LSPSymbolInformationTmp( const std::string& _name, LSPSymbolKind _kind, TextRange _range,
							 const std::string& _detail ) :
		name( _name ), detail( _detail ), kind( _kind ), range( _range ) {}
	std::string name;
	std::string detail;
	LSPSymbolKind kind{ LSPSymbolKind::File };
	URI url;
	TextRange range;
	TextRange selectionRange;
	double score = 0.0;
	std::list<LSPSymbolInformationTmp> children;
	static LSPSymbolInformation fromTmp( const LSPSymbolInformationTmp& tmp ) {
		LSPSymbolInformation info;
		info.name = std::move( tmp.name );
		info.detail = std::move( tmp.detail );
		info.kind = std::move( tmp.kind );
		info.url = std::move( tmp.url );
		info.range = std::move( tmp.range );
		info.selectionRange = std::move( tmp.selectionRange );
		info.score = std::move( tmp.score );
		for ( const auto& child : tmp.children )
			info.children.push_back( fromTmp( child ) );
		return info;
	}
};

static LSPSymbolInformationList parseDocumentSymbols( const json& result ) {
	// TODO: Optimize this
	Clock clock;
	std::list<LSPSymbolInformationTmp> ret;
	std::map<std::string, LSPSymbolInformationTmp*> index;

	std::function<void( const json& symbol, LSPSymbolInformationTmp* parent )> parseSymbol =
		[&]( const json& symbol, LSPSymbolInformationTmp* parent ) {
			const auto& mrange = symbol.contains( MEMBER_RANGE )
									 ? symbol.at( MEMBER_RANGE )
									 : symbol[MEMBER_LOCATION].at( MEMBER_RANGE );
			auto range = parseRange( mrange );
			auto it = index.end();
			if ( !parent ) {
				auto container = symbol.value( "containerName", "" );
				it = index.find( container );
				if ( it != index.end() )
					parent = it->second;
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
				index.insert( std::make_pair( name, &list->back() ) );
				if ( symbol.contains( "children" ) ) {
					const auto& children = symbol.at( ( "children" ) );
					for ( const auto& child : children )
						parseSymbol( child, &list->back() );
				}
			}
		};

	const auto& symInfos = result;
	for ( const auto& info : symInfos )
		parseSymbol( info, nullptr );

	LSPSymbolInformationList rret;
	for ( const auto& r : ret )
		rret.push_back( LSPSymbolInformationTmp::fromTmp( r ) );

	Log::debug( "LSPClientServer - parseDocumentSymbols took: %.2fms",
				clock.getElapsed().asMilliseconds() );
	return rret;
}

static LSPSymbolInformationList parseWorkspaceSymbols( const json& res ) {
	LSPSymbolInformationList symbols;

	std::transform( res.cbegin(), res.cend(), std::back_inserter( symbols ),
					[]( const json& symbol ) {
						LSPSymbolInformation symInfo;

						const auto& location = symbol.at( MEMBER_LOCATION );
						const auto& mrange = location.at( MEMBER_RANGE );

						auto containerName = symbol.value( "containerName", "" );
						if ( !containerName.empty() )
							containerName.append( "::" );
						symInfo.name = containerName + symbol.value( "name", "" );
						symInfo.kind = (LSPSymbolKind)symbol.value( MEMBER_KIND, 1 );
						symInfo.range = parseRange( mrange );
						symInfo.url = URI( location.value( MEMBER_URI, "" ) );
						symInfo.score = symbol.value( "score", 0.0 );
						return symInfo;
					} );

	std::sort( symbols.begin(), symbols.end(),
			   []( const LSPSymbolInformation& l, const LSPSymbolInformation& r ) {
				   return l.score > r.score;
			   } );

	return symbols;
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

static LSPTextEdit parseTextEdit( const json& result ) {
	LSPTextEdit edit;
	edit.text = result.at( "newText" ).get<std::string>();
	edit.range = parseRange( result.at( MEMBER_RANGE ) );
	return edit;
}

static std::vector<LSPTextEdit> parseTextEditArray( const json& result ) {
	std::vector<LSPTextEdit> ret;
	for ( const auto& edit : result )
		ret.push_back( parseTextEdit( edit ) );
	return ret;
}

static void fromJson( LSPVersionedTextDocumentIdentifier& id, const json& json ) {
	if ( json.is_object() ) {
		auto& ob = json;
		id.uri = URI( ob.at( MEMBER_URI ).get<std::string>() );
		id.version = ob.contains( MEMBER_VERSION ) ? ob.at( MEMBER_VERSION ).get<int>() : -1;
	}
}

static LSPTextDocumentEdit parseTextDocumentEdit( const json& result ) {
	LSPTextDocumentEdit ret;
	auto& ob = result;
	fromJson( ret.textDocument, ob.at( "textDocument" ) );
	ret.edits = parseTextEditArray( ob.at( "edits" ) );
	return ret;
}

static LSPWorkspaceEdit parseWorkSpaceEdit( const json& result ) {
	LSPWorkspaceEdit ret;
	if ( result.contains( "changes" ) ) {
		auto& changes = result.at( "changes" );
		for ( auto it = changes.begin(); it != changes.end(); ++it ) {
			ret.changes.insert( std::pair<URI, std::vector<LSPTextEdit>>(
				URI( it.key() ), parseTextEditArray( it.value() ) ) );
		}
	}
	if ( result.contains( "documentChanges" ) ) {
		auto& documentChanges = result.at( "documentChanges" );
		// resourceOperations not supported for now
		for ( auto& edit : documentChanges ) {
			ret.documentChanges.push_back( parseTextDocumentEdit( edit ) );
		}
	}
	return ret;
}

static LSPCommand parseCommand( const json& result ) {
	auto title = result.at( MEMBER_TITLE ).get<std::string>();
	auto command = result.at( MEMBER_COMMAND ).get<std::string>();
	auto& args = result.at( MEMBER_ARGUMENTS );
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
		ret.push_back( { range, severity, code, source, message, relatedInfoList, {} } );
	}
	return ret;
}

static std::vector<LSPCodeAction> parseCodeAction( const json& result ) {
	std::vector<LSPCodeAction> ret;
	const auto& codeActions = result;
	for ( const auto& vaction : codeActions ) {
		auto& action = vaction;
		// entry could be Command or CodeAction
		if ( !action.contains( MEMBER_COMMAND ) || !action.at( MEMBER_COMMAND ).is_string() ) {
			// CodeAction
			auto title = action.at( MEMBER_TITLE ).get<std::string>();
			auto kind = action.value( MEMBER_KIND, "" );
			auto command = action.contains( MEMBER_COMMAND )
							   ? parseCommand( action.at( MEMBER_COMMAND ) )
							   : LSPCommand{};
			auto edit = action.contains( MEMBER_EDIT )
							? parseWorkSpaceEdit( action.at( MEMBER_EDIT ) )
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

static std::vector<LSPDiagnosticsCodeAction> parseDiagnosticsCodeAction( const json& result ) {
	std::vector<LSPDiagnosticsCodeAction> ret;
	const auto& codeActions = result;
	for ( const auto& action : codeActions ) {
		if ( !action.contains( MEMBER_COMMAND ) || !action.at( MEMBER_COMMAND ).is_string() ) {
			auto title = action.at( MEMBER_TITLE ).get<std::string>();
			auto kind = action.value( MEMBER_KIND, "" );
			auto isPreferred = action.value( "isPreferred", false );
			auto edit = action.contains( MEMBER_EDIT )
							? parseWorkSpaceEdit( action.at( MEMBER_EDIT ) )
							: LSPWorkspaceEdit{};
			LSPDiagnosticsCodeAction action = { title, kind, isPreferred, edit };
			ret.push_back( action );
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
	json diags = json::array();
	for ( const auto& diagnostic : diagnostics )
		diags.push_back( toJson( diagnostic ) );
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

static std::vector<LSPDiagnostic> parseDiagnosticsArr( const json& result ) {
	std::vector<LSPDiagnostic> ret;
	for ( const auto& diag : result ) {
		auto range = parseRange( diag[MEMBER_RANGE] );
		auto severity = static_cast<LSPDiagnosticSeverity>( diag["severity"].get<int>() );
		auto code = diag.contains( "code" ) ? ( diag["code"].is_number_integer()
													? String::toString( diag["code"].get<int>() )
													: diag.at( "code" ).get<std::string>() )
											: "";
		auto source = diag.value( "source", "" );
		auto message = diag.value( MEMBER_MESSAGE, "" );
		std::vector<LSPDiagnosticRelatedInformation> relatedInfoList;
		if ( diag.contains( "relatedInformation" ) ) {
			const auto& relatedInfo = diag.at( "relatedInformation" );
			for ( const auto& related : relatedInfo ) {
				auto relLocation = parseLocation( related.at( MEMBER_LOCATION ) );
				auto relMessage = related.at( MEMBER_MESSAGE ).get<std::string>();
				relatedInfoList.push_back( { relLocation, relMessage } );
			}
		}
		// clang providers codeActions from diagnostics
		std::vector<LSPDiagnosticsCodeAction> codeActions;
		if ( diag.contains( "codeActions" ) )
			codeActions = parseDiagnosticsCodeAction( diag["codeActions"] );
		ret.push_back( { range, severity, code, source, message, relatedInfoList, codeActions } );
	}
	return ret;
}

static LSPPublishDiagnosticsParams parsePublishDiagnostics( const json& result ) {
	LSPPublishDiagnosticsParams ret;
	ret.uri = URI( result.at( MEMBER_URI ).get<std::string>() );
	ret.diagnostics = parseDiagnosticsArr( result.at( MEMBER_DIAGNOSTICS ) );
	return ret;
}

static LSPCompletionList parseDocumentCompletion( const json& result ) {
	LSPCompletionList ret;
	if ( result.empty() )
		return {};
#ifndef EE_DEBUG
	try {
#endif
		ret.isIncomplete =
			result.contains( "isIncomplete" ) ? result["isIncomplete"].get<bool>() : false;
		const json& items =
			( result.is_object() && result.contains( "items" ) ) ? result["items"] : result;

		for ( const auto& item : items ) {
			auto label = jsonString( item, MEMBER_LABEL, "" );
			auto detail = jsonString( item, MEMBER_DETAIL, "" );
			LSPMarkupContent doc = item.contains( MEMBER_DOCUMENTATION )
									   ? parseMarkupContent( item.at( MEMBER_DOCUMENTATION ) )
									   : LSPMarkupContent{};
			auto filterText = jsonString( item, "filterText", label );
			auto insertText = jsonString( item, "insertText", label );
			auto sortText = jsonString( item, "sortText", label );
			LSPTextEdit textEdit;
			if ( item.contains( "textEdit" ) && !item["textEdit"].is_null() )
				textEdit = parseTextEdit( item["textEdit"] );
			auto kind = static_cast<LSPCompletionItemKind>( item.value( MEMBER_KIND, 1 ) );
			const std::vector<LSPTextEdit> additionalTextEdits =
				item.contains( "additionalTextEdits" )
					? parseTextEditArray( item.at( "additionalTextEdits" ) )
					: std::vector<LSPTextEdit>{};

			ret.items.push_back( { label, kind, detail, doc, sortText, insertText, filterText,
								   textEdit, additionalTextEdits } );
		}
#ifndef EE_DEBUG
	} catch ( const json::exception& err ) {
		Log::warning( "Error parsing parseDocumentCompletion: %s", err.what() );
	}
#endif
	return ret;
}

static LSPSignatureInformation parseSignatureInformation( const json& json ) {
	LSPSignatureInformation info;

	info.label = json.value( MEMBER_LABEL, "" );
	if ( json.contains( MEMBER_DOCUMENTATION ) )
		info.documentation = parseMarkupContent( json.at( MEMBER_DOCUMENTATION ) );
	const auto& params = json.at( "parameters" );
	for ( const auto& par : params ) {
		auto& label = par.at( MEMBER_LABEL );
		int begin = -1, end = -1;
		if ( label.is_array() ) {
			auto& range = label;
			if ( range.size() == 2 ) {
				begin = range[0].get<int>();
				end = range[1].get<int>();
				if ( begin > (int)info.label.length() )
					begin = -1;
				if ( end > (int)info.label.length() )
					end = -1;
			}
		} else {
			auto sub = label.get<std::string>();
			if ( sub.length() ) {
				begin = info.label.find( sub );
				if ( begin >= 0 )
					end = begin + sub.length();
			}
		}
		info.parameters.push_back( { begin, end } );
	}
	return info;
}

static LSPSignatureHelp parseSignatureHelp( const json& sig ) {
	LSPSignatureHelp ret;
#ifndef EE_DEBUG
	try {
#endif
		if ( !sig.contains( "signatures" ) )
			return ret;
		const auto& sigInfos = sig.at( "signatures" );
		for ( const auto& info : sigInfos )
			ret.signatures.push_back( parseSignatureInformation( info ) );
		ret.activeSignature = sig.value( "activeSignature", 0 );
		ret.activeParameter = sig.value( "activeParameter", 0 );
		ret.activeSignature = eemin( eemax( ret.activeSignature, 0 ), (int)ret.signatures.size() );
		ret.activeParameter = eemax( ret.activeParameter, 0 );
		if ( !ret.signatures.empty() ) {
			ret.activeParameter =
				eemin( ret.activeParameter,
					   (int)ret.signatures.at( ret.activeSignature ).parameters.size() );
		}
#ifndef EE_DEBUG
	} catch ( const json::exception& err ) {
		Log::warning( "Error parsing parseSignatureHelp: %s", err.what() );
	}
#endif
	return ret;
}

static std::shared_ptr<LSPSelectionRange> parseSelectionRange( const json& selectionRange ) {
	auto current = std::make_shared<LSPSelectionRange>( LSPSelectionRange{} );
	std::shared_ptr<LSPSelectionRange> ret = current;
	json selRange = std::move( selectionRange );

	while ( selRange.is_object() ) {
		current->range = parseRange( selRange[MEMBER_RANGE] );
		if ( !selRange["parent"].is_object() ) {
			current->parent = nullptr;
			break;
		}
		selRange = selRange["parent"];
		current->parent = std::make_shared<LSPSelectionRange>( LSPSelectionRange{} );
		current = current->parent;
	}
	return ret;
}

static std::vector<std::shared_ptr<LSPSelectionRange>>
parseSelectionRanges( const json& selectionRanges ) {
	std::vector<std::shared_ptr<LSPSelectionRange>> ret;
	for ( const auto& selectionRange : selectionRanges )
		ret.push_back( parseSelectionRange( selectionRange ) );
	return ret;
}

static LSPShowMessageParams parseMessageRequest( const json& json ) {
	LSPShowMessageParams msg;
	msg.type = (LSPMessageType)json["type"].get<int>();
	msg.message = json["message"];
	if ( json.contains( "actions" ) && json["actions"].is_array() ) {
		LSPMessageActionItem item;
		auto& actions = json["actions"];
		for ( const auto& ma : actions ) {
			if ( ma.contains( "title" ) )
				item.title = ma["title"];
		}
		if ( !item.title.empty() )
			msg.actions.emplace_back( item );
	}
	return msg;
}

static LSPShowDocumentParams parseShowDocument( const json& json ) {
	LSPShowDocumentParams params;
	params.uri = URI( json.value( "uri", "" ) );
	params.external = json.value<bool>( "external", false );
	params.takeFocus = json.value<bool>( "takeFocus", false );
	if ( json.contains( "selection" ) )
		params.selection = parseRange( json["selection"] );
	return params;
}

static LSPApplyWorkspaceEditParams parseApplyWorkspaceEditParams( const json& result ) {
	LSPApplyWorkspaceEditParams ret;
	ret.label = result.value( MEMBER_LABEL, "" );
	if ( result.contains( MEMBER_EDIT ) )
		ret.edit = parseWorkSpaceEdit( result.at( MEMBER_EDIT ) );
	return ret;
}

static json applyWorkspaceEditResponse( const PluginIDType& msgid,
										const LSPApplyWorkspaceEditResponse& response ) {
	json j = newID( msgid );
	j[MEMBER_RESULT] =
		json{ { "applied", response.applied }, { "failureReason", response.failureReason } };
	return j;
}

void LSPClientServer::registerCapabilities( const json& jcap ) {
	if ( !jcap.is_object() || !jcap.contains( "registrations" ) ||
		 !jcap["registrations"].is_array() )
		return;
	bool registered = false;
	const auto& registrations = jcap["registrations"];

	for ( const auto& reg : registrations ) {
		if ( reg.contains( "method" ) ) {
			if ( reg["method"] == "workspace/executeCommand" ) {
				mCapabilities.executeCommandProvider = true;
				registered = true;
			} else if ( reg["method"] == "textDocument/documentSymbol" ) {
				mCapabilities.documentSymbolProvider = true;
				registered = true;
			}
		}
	}

	if ( !registered )
		return;

	// Broadcast the new language capabilities to all the interested plugins
	mManager->getPluginManager()->sendBroadcast(
		mManager->getPlugin(), PluginMessageType::LanguageServerCapabilities,
		PluginMessageFormat::LanguageServerCapabilities, &mCapabilities );
}

void LSPClientServer::initialize() {
	json codeAction{
		{ "codeActionLiteralSupport",
		  json{ { "codeActionKind", json{ { "valueSet", json::array( { "quickfix", "refactor",
																	   "source" } ) } } } } } };

	json semanticTokens{
		{ "requests", json{ { "range", true }, { "full", json{ { "delta", true } } } } },
		{ "tokenTypes", supportedSemanticTokenTypes() },
		{ "tokenModifiers", json::array() },
		{ "formats", { "relative" } },
	};

	json completionItem{ { "snippetSupport", true } };

	json showMessage;
	showMessage["messageActionItem"] = json{ { "additionalPropertiesSupport", false } };

	json showDocument;
	showDocument["support"] = true;

	json workspace;
	workspace["applyEdit"] = true;
	workspace["executeCommand"] = json{ { "dynamicRegistration", true } };
	workspace["workspaceFolders"] = true;

	json capabilities{
		{ "textDocument",
		  json{
			  { "documentSymbol", json{ { "dynamicRegistration", true },
										{ "hierarchicalDocumentSymbolSupport", true } } },
			  { "publishDiagnostics",
				json{ { "relatedInformation", true }, { "codeActionsInline", true } } },
			  { "codeAction", codeAction },
			  { "semanticTokens", semanticTokens },
			  { "synchronization", json{ { "didSave", true } } },
			  { "selectionRange", json{ { "dynamicRegistration", false } } },
			  { "hover", json{ { "contentFormat", { "plaintext", "markdown" } } } },
			  { "completion", completionItem },
		  } },
		{ "window", json{ { "workDoneProgress", true },
						  { "showMessage", showMessage },
						  { "showDocument", showDocument } } },
		{ "workspace", workspace },
		{ "general", json{ { "positionEncodings", json::array( { "utf-32" } ) } } } };

	json params{ { "processId", Sys::getProcessID() },
				 { "capabilities", capabilities },
				 { "initializationOptions", mLSP.initializationOptions } };

	URI rootPath( String::startsWith( mRootPath, "file://" ) && mRootPath != "file://"
					  ? mRootPath
					  : ( !mRootPath.empty() ? "file://" + mRootPath : "" ) );
	if ( rootPath.empty() ) {
		if ( !mManager->getLSPWorkspaceFolder().uri.empty() ) {
			rootPath = mManager->getLSPWorkspaceFolder().uri;
			if ( rootPath.getScheme().empty() )
				rootPath = URI( "file://" + mManager->getLSPWorkspaceFolder().uri.toString() );
		} else {
			rootPath = URI( "file://" + FileSystem::getCurrentWorkingDirectory() );
		}
	}
	std::string rpath = rootPath.getFSPath();
#if EE_PLATFORM == EE_PLATFORM_WIN
	if ( rpath.size() > 2 && rpath[1] == ':' )
		rpath[0] = std::tolower( rpath[0] );
#endif
	std::string rootUri = rootPath.toString();
	params["rootPath"] = rpath;
	params["rootUri"] = rootUri;
	params["workspaceFolders"] =
		toJson( { LSPWorkspaceFolder{ rootUri, FileSystem::fileNameFromPath( rootUri ) } } );
	mWorkspaceFolder = rootUri;

	write(
		newRequest( "initialize", params ),
		[&]( const IdType&, const json& resp ) {
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
			mCapabilities.language = mLSP.language;
			mReady = true;
			write( newRequest( "initialized" ) );
			sendQueuedMessages();
			notifyServerInitialized();

			// Broadcast the language capabilities to all the interested plugins
			mManager->getPluginManager()->sendBroadcast(
				mManager->getPlugin(), PluginMessageType::LanguageServerCapabilities,
				PluginMessageFormat::LanguageServerCapabilities, &mCapabilities );
		},
		[&]( const IdType&, const json& ) {} );
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
	std::string cmd( mLSP.command );
	if ( !mLSP.commandParameters.empty() ) {
		if ( mLSP.commandParameters.front() != ' ' )
			mLSP.commandParameters = " " + mLSP.commandParameters;
		cmd += mLSP.commandParameters;
	}
	bool ret =
		mProcess.create( cmd, Process::getDefaultOptions() | Process::EnableAsync, {}, mRootPath );
	if ( ret && mProcess.isAlive() ) {
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

void LSPClientServer::notifyServerInitialized() {
	for ( const auto& client : mClients )
		client.second->onServerInitialized();
}

bool LSPClientServer::needsAsync() {
	return Engine::isEngineRunning() &&
		   Engine::instance()->getMainThreadId() == Thread::getCurrentThreadId();
}

bool LSPClientServer::isRunning() {
	return mProcess.isAlive() && !mProcess.isShootingDown();
}

void LSPClientServer::removeDoc( TextDocument* doc ) {
	Lock l( mClientsMutex );
	if ( mClients.erase( doc ) > 0 ) {
		auto it = std::find( mDocs.begin(), mDocs.end(), doc );
		if ( it != mDocs.end() )
			mDocs.erase( it );
	}
}

const LSPServerCapabilities& LSPClientServer::getCapabilities() const {
	return mCapabilities;
}

LSPClientServer::LSPRequestHandle LSPClientServer::cancel( const PluginIDType& reqid ) {
	size_t res = 0;
	{
		Lock l( mHandlersMutex );
		res = mHandlers.erase( reqid ) > 0;
	}
	if ( res > 0 ) {
		auto params = newID( reqid );
		if ( needsAsync() ) {
			sendAsync( newRequest( "$/cancelRequest", params ) );
			return {};
		}
		return send( newRequest( "$/cancelRequest", params ) );
	}
	return LSPRequestHandle();
}

LSPClientServer::LSPRequestHandle LSPClientServer::write( const json& msg,
														  const JsonReplyHandler& h,
														  const JsonReplyHandler& eh,
														  const int id ) {
	LSPRequestHandle ret;
	ret.server = this;

	if ( !mProcess.isAlive() )
		return ret;

	json ob = msg;
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

void LSPClientServer::sendAsync( const json& msg, const JsonReplyHandler& h,
								 const JsonReplyHandler& eh ) {
	getThreadPool()->run( [this, msg, h, eh] { send( msg, h, eh ); } );
}

LSPClientServer::LSPRequestHandle LSPClientServer::send( const json& msg, const JsonReplyHandler& h,
														 const JsonReplyHandler& eh ) {
#ifdef EE_DEBUG
	eeASSERT( !needsAsync() );
#endif
	if ( mProcess.isAlive() ) {
		return write( msg, h, eh );
	} else {
		Log::debug( "LSPClientServer server %s Send for non-running server: %s", mLSP.name.c_str(),
					mLSP.name.c_str() );
	}
	return LSPRequestHandle();
}

LSPClientServer::LSPRequestHandle LSPClientServer::didOpen( const URI& document,
															const std::string& text, int version ) {
	auto params = textDocumentParams( textDocumentItem( document, mLSP.language, text, version ) );
	return send( newRequest( "textDocument/didOpen", params ) );
}

LSPClientServer::LSPRequestHandle LSPClientServer::didOpen( TextDocument* doc, int version ) {
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

LSPClientServer::LSPRequestHandle LSPClientServer::didSave( const URI& document,
															const std::string& text ) {
	auto params = textDocumentParams( document );
	if ( !text.empty() )
		params["text"] = text;
	return send( newRequest( "textDocument/didSave", params ) );
}

LSPClientServer::LSPRequestHandle LSPClientServer::didSave( TextDocument* doc ) {
	return didSave( doc->getURI(), mCapabilities.textDocumentSync.save.includeText
									   ? doc->getText().toUtf8()
									   : "" );
}

LSPClientServer::LSPRequestHandle
LSPClientServer::didChange( const URI& document, int version, const std::string& text,
							const std::vector<DocumentContentChange>& change ) {
	auto params = textDocumentParams( document, version );
	params["contentChanges"] = !text.empty() ? json{ json{ MEMBER_TEXT, text } } : toJson( change );
	return send( newRequest( "textDocument/didChange", params ) );
}

LSPClientServer::LSPRequestHandle
LSPClientServer::didChange( TextDocument* doc, const std::vector<DocumentContentChange>& change ) {
	Lock l( mClientsMutex );
	auto it = mClients.find( doc );
	if ( it != mClients.end() )
		return didChange( doc->getURI(), it->second->getVersion(),
						  change.empty() ? doc->getText().toUtf8() : "", change );
	return LSPRequestHandle();
}

void LSPClientServer::queueDidChange( const URI& document, int version, const std::string&,
									  const std::vector<DocumentContentChange>& change ) {
	Lock l( mDidChangeMutex );
	mDidChangeQueue.push( { document, version, change } );
}

void LSPClientServer::processDidChangeQueue() {
	Lock l( mDidChangeMutex );
	while ( !mDidChangeQueue.empty() ) {
		auto& change = mDidChangeQueue.front();
		didChange( change.uri, change.version, "", change.change );
		mDidChangeQueue.pop();
	}
}

bool LSPClientServer::hasDocument( TextDocument* doc ) const {
	return std::find( mDocs.begin(), mDocs.end(), doc ) != mDocs.end();
}

bool LSPClientServer::hasDocument( const URI& uri ) const {
	for ( const auto& doc : mDocs ) {
		if ( doc->getURI() == uri )
			return true;
	}
	return false;
}

bool LSPClientServer::hasDocuments() const {
	return !mDocs.empty();
}

LSPClientServer::LSPRequestHandle LSPClientServer::didClose( const URI& document ) {
	auto params = textDocumentParams( document );
	return send( newRequest( "textDocument/didClose", params ) );
}

LSPClientServerManager* LSPClientServer::getManager() const {
	return mManager;
}

const std::shared_ptr<ThreadPool>& LSPClientServer::getThreadPool() const {
	return mManager->getThreadPool();
}

LSPClientServer::LSPRequestHandle LSPClientServer::documentSymbols( const URI& document,
																	const JsonReplyHandler& h,
																	const JsonReplyHandler& eh ) {
	auto params = textDocumentParams( document );
	return send( newRequest( "textDocument/documentSymbol", params ), h, eh );
}

LSPClientServer::LSPRequestHandle
LSPClientServer::documentSymbols( const URI& document,
								  const WReplyHandler<LSPSymbolInformationList>& h,
								  const ReplyHandler<LSPResponseError>& eh ) {
	return documentSymbols(
		document,
		[h]( const IdType& id, const json& json ) {
			if ( h )
				h( id, parseDocumentSymbols( json ) );
		},
		[eh]( const IdType& id, const json& json ) {
			if ( eh )
				eh( id, parseResponseError( json ) );
		} );
}

void LSPClientServer::workspaceSymbol( const std::string& querySymbol, const JsonReplyHandler& h,
									   const size_t& limit ) {
	auto params = json{ { MEMBER_QUERY, querySymbol }, { MEMBER_LIMIT, limit } };
	sendAsync( newRequest( "workspace/symbol", params ), h );
}

void LSPClientServer::workspaceSymbol( const std::string& querySymbol,
									   const SymbolInformationHandler& h, const size_t& limit ) {
	workspaceSymbol(
		querySymbol,
		[h]( const IdType& id, const json& json ) {
			if ( h )
				h( id, parseWorkspaceSymbols( json ) );
		},
		limit );
}

void fromJson( LSPWorkDoneProgressValue& value, const json& data ) {
	if ( !data.empty() ) {
		json ob = data;
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

PluginIDType LSPClientServer::getID( const json& json ) {
	const auto& memberID = json[MEMBER_ID];
	if ( memberID.is_string() ) {
		return memberID.get<std::string>();
	} else if ( memberID.is_number_integer() ) {
		return memberID.get<int>();
	} else if ( memberID.is_number_unsigned() ) {
		return memberID.get<unsigned int>();
	}
	eeASSERT( true );
	return {};
}

static json newError( const LSPErrorCode& code, const std::string& msg ) {
	return json{
		{ MEMBER_ERROR, { { MEMBER_CODE, static_cast<int>( code ) }, { MEMBER_MESSAGE, msg } } } };
}

void LSPClientServer::publishDiagnostics( const json& msg ) {
	LSPPublishDiagnosticsParams res = parsePublishDiagnostics( msg[MEMBER_PARAMS] );
	if ( mManager && mManager->getPluginManager() && mManager->getPlugin() ) {
		mManager->getPluginManager()->sendBroadcast( mManager->getPlugin(),
													 PluginMessageType::Diagnostics,
													 PluginMessageFormat::Diagnostics, &res );
	}
	Log::debug( "LSPClientServer::publishDiagnostics: %s - returned %zu items",
				res.uri.toString().c_str(), res.diagnostics.size() );
	Log::info( "LSPClientServer::publishDiagnostics: %s", msg.dump().c_str() );
}

void LSPClientServer::workDoneProgress( const LSPWorkDoneProgressParams& workDoneParams ) {
	// should emit event somewhere
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
	} else if ( method == ( "window/showMessage" ) && msg.contains( MEMBER_PARAMS ) ) {
		auto msgReq = parseMessageRequest( msg[MEMBER_PARAMS] );
		mManager->getPluginManager()->sendBroadcast( PluginMessageType::ShowMessage,
													 PluginMessageFormat::ShowMessage, &msgReq );
		return;
	} else if ( method == ( "window/logMessage" ) && msg.contains( MEMBER_PARAMS ) ) {
		auto lm = parseMessageRequest( msg[MEMBER_PARAMS] );
		if ( !lm.message.empty() ) {
			switch ( lm.type ) {
				case LSPMessageType::Log:
				case LSPMessageType::Info:
					Log::notice( lm.message );
					break;
				case LSPMessageType::Warning:
					Log::warning( lm.message );
					break;
				case LSPMessageType::Error:
					Log::error( lm.message );
					break;
				default:
					break;
			}
		}
		return;
	} else if ( method == ( "$/progress" ) && msg.contains( MEMBER_PARAMS ) ) {
		workDoneProgress( parseWorkDone( msg[MEMBER_PARAMS] ) );
		return;
	}
	Log::debug( "LSPClientServer::processNotification server %s: %s", mLSP.name.c_str(),
				msg.dump().c_str() );
}

void LSPClientServer::processRequest( const json& msg ) {
	Log::debug( "LSPClientServer::processRequest server %s:\n%s", mLSP.name.c_str(),
				msg.dump().c_str() );
	auto method = msg[MEMBER_METHOD].get<std::string>();
	auto msgid = getID( msg );
	if ( method == "workspace/applyEdit" ) {
		auto workspaceEdit = parseApplyWorkspaceEditParams( msg[MEMBER_PARAMS] );
		mManager->applyWorkspaceEdit( workspaceEdit.edit,
									  [&, msgid]( const LSPApplyWorkspaceEditResponse& res ) {
										  getThreadPool()->run( [this, msgid, res] {
											  write( applyWorkspaceEditResponse( msgid, res ) );
										  } );
									  } );
		return;
	} else if ( method == "window/workDoneProgress/create" ) {
		write( newEmptyResult( msgid ) );
		return;
	} else if ( method == "client/registerCapability" ) {
		registerCapabilities( msg[MEMBER_PARAMS] );
		write( newEmptyResult( msgid ) );
		return;
	} else if ( method == "window/showMessageRequest" ) {
		auto msgReq = parseMessageRequest( msg[MEMBER_PARAMS] );
		mManager->getPluginManager()->sendBroadcast( PluginMessageType::ShowMessage,
													 PluginMessageFormat::ShowMessage, &msgReq );
		write( newEmptyResult( msgid ) );
		return;
	} else if ( method == "window/showDocument" ) {
		auto showDoc = parseShowDocument( msg[MEMBER_PARAMS] );
		if ( !showDoc.external ) {
			LSPLocation loc;
			loc.uri = showDoc.uri;
			loc.range = showDoc.selection;
			mManager->goToLocation( loc );
		} else {
			mManager->getPluginManager()->sendBroadcast(
				PluginMessageType::ShowDocument, PluginMessageFormat::ShowDocument, &showDoc );
		}
		write( newSuccessResult( msgid ) );
		return;
	}
	write( newError( LSPErrorCode::MethodNotFound, method ), nullptr, nullptr, msgid );
}

void LSPClientServer::readStdOut( const char* bytes, size_t n ) {
	mReceive.append( bytes, n );

	std::string& buffer = mReceive;

	while ( !mProcess.isShootingDown() ) {
		auto index = buffer.find( CONTENT_LENGTH_HEADER );
		if ( index == std::string::npos ) {
			if ( buffer.size() > ( (Uint64)1 << 20 ) )
				buffer.clear();
			break;
		}

		index += strlen( CONTENT_LENGTH_HEADER );
		auto endindex = buffer.find( "\r\n", index );
		auto msgstart = buffer.find( "\r\n\r\n", index );
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

			PluginIDType msgid;
			if ( res.contains( MEMBER_ID ) ) {
				msgid = getID( res );
			} else {
				processNotification( res );
				continue;
			}

			if ( res.contains( MEMBER_METHOD ) ) {
				processRequest( res );
				continue;
			}

			Log::debug( "LSPClientServer::readStdOut server %s said:\n%s", mLSP.name.c_str(),
						res.dump().c_str() );

			HandlersMap::iterator it = mHandlers.end();
			HandlersMap::iterator itEnd = mHandlers.end();
			JsonReplyHandler handlerOK;
			JsonReplyHandler handlerErr;
			bool handlerFound = false;
			{
				Lock l( mHandlersMutex );
				it = mHandlers.find( msgid );
				itEnd = mHandlers.end();
				handlerFound = it != itEnd;
				if ( handlerFound ) {
					handlerOK = it->second.first;
					handlerErr = it->second.second;
					mHandlers.erase( it );
				}
			}

			if ( handlerFound ) {
				if ( res.contains( MEMBER_ERROR ) && handlerErr ) {
					handlerErr( msgid, res[MEMBER_ERROR] );
				} else {
					handlerOK( msgid, res[MEMBER_RESULT] );
				}
			} else {
				Log::debug( "LSPClientServer::readStdOut server %s unexpected reply id: %s",
							mLSP.name.c_str(), msgid.toString().c_str() );
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
		Log::debug( "LSPClientServer::readStdErr server %s:\n%s", mLSP.name.c_str(),
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

void LSPClientServer::getAndGoToLocation( const URI& document, const TextPosition& pos,
										  const std::string& search, const LocationHandler& h ) {
	auto params = textDocumentPositionParams( document, pos );
	sendAsync( newRequest( search, params ), [h]( const IdType& id, const json& json ) {
		if ( h )
			h( id, parseDocumentLocation( json ) );
	} );
}

void LSPClientServer::getAndGoToLocation( const URI& document, const TextPosition& pos,
										  const std::string& search ) {
	getAndGoToLocation( document, pos, search,
						[&]( const IdType&, const std::vector<LSPLocation>& locs ) {
							if ( !locs.empty() )
								mManager->goToLocation( locs.front() );
						} );
}

void LSPClientServer::documentDefinition( const URI& document, const TextPosition& pos ) {
	getAndGoToLocation( document, pos, "textDocument/definition" );
}

void LSPClientServer::documentDeclaration( const URI& document, const TextPosition& pos ) {
	getAndGoToLocation( document, pos, "textDocument/declaration" );
}

void LSPClientServer::documentTypeDefinition( const URI& document, const TextPosition& pos ) {
	getAndGoToLocation( document, pos, "textDocument/typeDefinition" );
}

void LSPClientServer::documentImplementation( const URI& document, const TextPosition& pos ) {
	getAndGoToLocation( document, pos, "textDocument/implementation" );
}

static std::vector<URI> switchHeaderSourceName( const URI& uri ) {
	std::string basePath( "file://" + FileSystem::fileRemoveExtension( uri.getFSPath() ) );
	if ( FileSystem::fileExtension( uri.getPath() ) == "cpp" ) {
		return { basePath + ".hpp", basePath + ".h" };
	} else if ( FileSystem::fileExtension( uri.getPath() ) == "hpp" ) {
		return { basePath + ".cpp" };
	} else if ( FileSystem::fileExtension( uri.getPath() ) == "c" ) {
		return { basePath + ".h" };
	} else if ( FileSystem::fileExtension( uri.getPath() ) == "h" ) {
		return { basePath + ".c" };
	}
	return {};
}

void LSPClientServer::switchSourceHeader( const URI& document ) {
	sendAsync(
		newRequest( "textDocument/switchSourceHeader", textDocumentURI( document ) ),
		[this, document]( const IdType&, json res ) {
			std::vector<URI> uris( switchHeaderSourceName( document ) );
			for ( const auto& uri : uris ) {
				if ( res.is_string() &&
					 ( uri.empty() || FileSystem::fileNameFromPath( res.get<std::string>() ) ==
										  FileSystem::fileNameFromPath( uri.getFSPath() ) ) ) {
					mManager->goToLocation( { res.get<std::string>(), TextRange() } );
					break;
				} else if ( !uri.empty() ) {
					mManager->findAndOpenClosestURI( uris );
					break;
				}
			}
		} );
}

LSPClientServer::LSPRequestHandle
LSPClientServer::didChangeWorkspaceFolders( const std::vector<LSPWorkspaceFolder>& added,
											const std::vector<LSPWorkspaceFolder>& removed,
											bool async ) {
	if ( mCapabilities.workspaceFolders.supported ) {
		// Don't send the didChangeWorkspaceFolder if already in the same directory
		if ( !added.empty() && mWorkspaceFolder == added[0].uri )
			return {};
		auto params = changeWorkspaceFoldersParams( added, removed );
		// Update CWD
		if ( !added.empty() )
			mWorkspaceFolder = added[0].uri;
		if ( async && needsAsync() ) {
			sendAsync( newRequest( "workspace/didChangeWorkspaceFolders", params ) );
			return {};
		}
		return send( newRequest( "workspace/didChangeWorkspaceFolders", params ) );
	}
	return {};
}

void LSPClientServer::documentCodeAction( const URI& document, const TextRange& range,
										  const std::vector<std::string>& kinds,
										  std::vector<LSPDiagnostic> diagnostics,
										  const JsonReplyHandler& h ) {
	auto params = codeActionParams( document, range, kinds, std::move( diagnostics ) );
	sendAsync( newRequest( "textDocument/codeAction", params ), h );
}

void LSPClientServer::documentCodeAction( const URI& document, const TextRange& range,
										  const std::vector<std::string>& kinds,
										  std::vector<LSPDiagnostic> diagnostics,
										  const CodeActionHandler& h ) {
	documentCodeAction( document, range, kinds, diagnostics,
						[h]( const IdType& id, const json& json ) {
							if ( h )
								h( id, parseCodeAction( json ) );
						} );
}

void LSPClientServer::documentHover( const URI& document, const TextPosition& pos,
									 const JsonReplyHandler& h ) {
	auto params = textDocumentPositionParams( document, pos );
	return sendAsync( newRequest( "textDocument/hover", params ), h );
}

void LSPClientServer::documentHover( const URI& document, const TextPosition& pos,
									 const HoverHandler& h ) {
	documentHover( document, pos, [h]( const IdType& id, const json& json ) {
		if ( h )
			h( id, parseHover( json ) );
	} );
}

LSPClientServer::LSPRequestHandle LSPClientServer::documentCompletion( const URI& document,
																	   const TextPosition& pos,
																	   const JsonReplyHandler& h ) {
	auto params = textDocumentPositionParams( document, pos );
	return send( newRequest( "textDocument/completion", params ), h );
}

LSPClientServer::LSPRequestHandle
LSPClientServer::documentCompletion( const URI& document, const TextPosition& pos,
									 const CompletionHandler& h ) {
	return documentCompletion( document, pos, [h]( const IdType& id, const json& json ) {
		if ( h )
			h( id, parseDocumentCompletion( json ) );
	} );
}

LSPClientServer::LSPRequestHandle LSPClientServer::signatureHelp( const URI& document,
																  const TextPosition& pos,
																  const JsonReplyHandler& h ) {
	auto params = textDocumentPositionParams( document, pos );
	return send( newRequest( "textDocument/signatureHelp", params ), h );
}

LSPClientServer::LSPRequestHandle LSPClientServer::signatureHelp( const URI& document,
																  const TextPosition& pos,
																  const SignatureHelpHandler& h ) {
	return signatureHelp( document, pos, [h]( const IdType& id, const json& json ) {
		if ( h )
			h( id, parseSignatureHelp( json ) );
	} );
}

LSPClientServer::LSPRequestHandle
LSPClientServer::selectionRange( const URI& document, const std::vector<TextPosition>& positions,
								 const JsonReplyHandler& h ) {
	auto params = textDocumentPositionsParams( document, positions );
	return send( newRequest( "textDocument/selectionRange", params ), h );
}

LSPClientServer::LSPRequestHandle
LSPClientServer::selectionRange( const URI& document, const std::vector<TextPosition>& positions,
								 const SelectionRangeHandler& h ) {
	return selectionRange( document, positions, [h]( const IdType& id, const json& json ) {
		if ( h )
			h( id, parseSelectionRanges( json ) );
	} );
}

void LSPClientServer::documentReferences( const URI& document, const TextPosition& pos, bool decl,
										  const JsonReplyHandler& h ) {
	auto params = referenceParams( document, pos, decl );
	sendAsync( newRequest( "textDocument/references", params ), h );
}

void LSPClientServer::documentReferences( const URI& document, const TextPosition& pos, bool decl,
										  const LocationHandler& h ) {
	documentReferences( document, pos, decl, [h]( const IdType& id, const json& json ) {
		if ( h )
			h( id, parseDocumentLocation( json ) );
	} );
}

static json textDocumentOptions( const URI& doc, json opts ) {
	auto params = textDocumentParams( doc );
	params["options"] = opts;
	return params;
}

LSPClientServer::LSPRequestHandle LSPClientServer::documentFormatting( const URI& document,
																	   const json& options,
																	   const JsonReplyHandler& h ) {
	auto params = textDocumentOptions( document, options );
	return send( newRequest( "textDocument/formatting", params ), h );
}

LSPClientServer::LSPRequestHandle
LSPClientServer::documentFormatting( const URI& document, const json& options,
									 const TextEditArrayHandler& h ) {
	return documentFormatting( document, options, [h]( const IdType& id, const json& json ) {
		if ( h )
			h( id, parseTextEditArray( json ) );
	} );
}

void LSPClientServer::memoryUsage( const JsonReplyHandler& h ) {
	return sendAsync( newRequest( "$/memoryUsage" ), h );
}

void LSPClientServer::memoryUsage() {
	memoryUsage( []( const IdType&, const json& json ) {
		Log::warning( "Received Memory Usage Information:\n%s", json.dump( 2 ).c_str() );
	} );
}

void LSPClientServer::executeCommand( const std::string& cmd, const json& params ) {
	sendAsync( newRequest( "workspace/executeCommand", executeCommandParams( cmd, params ) ),
			   []( const auto&, const auto& ) {} );
}

LSPClientServer::LSPRequestHandle
LSPClientServer::documentSemanticTokensFull( const URI& document, bool delta,
											 const std::string& requestId, const TextRange& range,
											 const JsonReplyHandler& h ) {
	auto params = textDocumentParams( document );
	// Delta
	if ( delta && !requestId.empty() ) {
		params[MEMBER_PREVIOUS_RESULT_ID] = requestId;
		return send( newRequest( "textDocument/semanticTokens/full/delta", params ), h );
	}
	// Range
	if ( range.isValid() ) {
		params[MEMBER_RANGE] = toJson( range );
		return send( newRequest( "textDocument/semanticTokens/range", params ), h );
	}

	return send( newRequest( "textDocument/semanticTokens/full", params ), h );
}

} // namespace ecode
