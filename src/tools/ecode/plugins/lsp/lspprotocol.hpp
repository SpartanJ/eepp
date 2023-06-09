#ifndef ECODE_LSPCLIENTPROTOCOL_HPP
#define ECODE_LSPCLIENTPROTOCOL_HPP

#include <eepp/ui/doc/textdocument.hpp>
#include <nlohmann/json.hpp>
#include <string>

using namespace EE;
using namespace EE::UI::Doc;
using namespace EE::Network;

// The LSP protocol will be consumed by any plugin, and it's not exclusive to the LSP plugin
// The protocol should be taken as a reference on how to implement the comunication between plugins

namespace ecode {

enum class LSPErrorCode {
	// Defined by JSON RPC
	ParseError = -32700,
	InvalidRequest = -32600,
	MethodNotFound = -32601,
	InvalidParams = -32602,
	InternalError = -32603,
	serverErrorStart = -32099,
	serverErrorEnd = -32000,
	ServerNotInitialized = -32002,
	UnknownErrorCode = -32001,

	// Defined by the protocol.
	RequestCancelled = -32800,
	ContentModified = -32801
};

struct LSPPosition {
	URI uri;
	TextPosition pos;
};

struct LSPLocation {
	URI uri;
	TextRange range;
};

struct LSPWorkspaceFolder {
	URI uri;
	std::string name;

	bool isEmpty() { return uri.empty(); }
};

enum class LSPDocumentSyncKind { None = 0, Full = 1, Incremental = 2 };

struct LSPSaveOptions {
	bool includeText = false;
};

struct LSPTextDocumentSyncOptions {
	LSPDocumentSyncKind change = LSPDocumentSyncKind::None;
	LSPSaveOptions save;
};

struct LSPCompletionOptions {
	bool provider = false;
	bool resolveProvider = false;
	std::vector<char> triggerCharacters;
};

struct LSPSignatureHelpOptions {
	bool provider = false;
	std::vector<char> triggerCharacters;
};

struct LSPDocumentOnTypeFormattingOptions : public LSPSignatureHelpOptions {};

struct SemanticTokenTypes {
	enum : Uint32 {
		Unknown = String::hash( "unknown" ),
		Namespace = String::hash( "namespace" ),
		Type = String::hash( "type" ),
		Class = String::hash( "class" ),
		Enum = String::hash( "enum" ),
		Interface = String::hash( "interface" ),
		Struct = String::hash( "struct" ),
		TypeParameter = String::hash( "typeParameter" ),
		Parameter = String::hash( "parameter" ),
		Variable = String::hash( "variable" ),
		Property = String::hash( "property" ),
		EnumMember = String::hash( "enumMember" ),
		Event = String::hash( "event" ),
		Function = String::hash( "function" ),
		Method = String::hash( "method" ),
		Macro = String::hash( "macro" ),
		Keyword = String::hash( "keyword" ),
		Modifier = String::hash( "modifier" ),
		Comment = String::hash( "comment" ),
		Str = String::hash( "string" ),
		Number = String::hash( "number" ),
		Regexp = String::hash( "regexp" ),
		Operator = String::hash( "operator" ),
		Decorator = String::hash( "decorator" ),
		Member = String::hash( "member" ),
	};
};

// Ref:
// https://microsoft.github.io/language-server-protocol/specification#textDocument_semanticTokens
struct SemanticTokensLegend {
	/**
	 * The token types a server uses.
	 */
	std::vector<std::string> tokenTypes;

	/**
	 * The token modifiers a server uses.
	 */
	std::vector<std::string> tokenModifiers;
};

struct LSPSemanticTokensOptions {
	bool full = false;
	bool fullDelta = false;
	bool range = false;
	SemanticTokensLegend legend;
};

struct LSPWorkspaceFoldersServerCapabilities {
	bool supported = false;
	bool changeNotifications = false;
};

struct LSPServerCapabilities {
	bool ready = false;
	std::vector<std::string> languages;
	LSPTextDocumentSyncOptions textDocumentSync;
	bool hoverProvider = false;
	LSPCompletionOptions completionProvider;
	LSPSignatureHelpOptions signatureHelpProvider;
	bool astProvider = false;
	bool definitionProvider = false;
	// official extension as of 3.14.0
	bool declarationProvider = false;
	bool typeDefinitionProvider = false;
	bool referencesProvider = false;
	bool implementationProvider = false;
	bool documentSymbolProvider = false;
	bool documentHighlightProvider = false;
	bool documentFormattingProvider = false;
	bool documentRangeFormattingProvider = false;
	bool workspaceSymbolProvider = false;
	LSPDocumentOnTypeFormattingOptions documentOnTypeFormattingProvider;
	bool renameProvider = false;
	// CodeActionOptions not useful/considered at present
	bool codeActionProvider = false;
	LSPSemanticTokensOptions semanticTokenProvider;
	// workspace caps flattened
	// (other parts not useful/considered at present)
	LSPWorkspaceFoldersServerCapabilities workspaceFolders;
	bool selectionRangeProvider = false;
	bool executeCommandProvider = false;
};

enum class LSPMessageType { Error = 1, Warning = 2, Info = 3, Log = 4 };

struct LSPMessageActionItem {
	std::string title;
};

struct LSPShowMessageParams {
	LSPMessageType type{ LSPMessageType::Log };
	std::string message;
	std::vector<LSPMessageActionItem> actions;
};

struct LSPTextDocumentContentChangeEvent {
	TextRange range;
	std::string text;
};

enum class LSPDiagnosticSeverity {
	Unknown = 0,
	Error = 1,
	Warning = 2,
	Information = 3,
	Hint = 4,
};

using LSPTextEdit = LSPTextDocumentContentChangeEvent;

struct LSPVersionedTextDocumentIdentifier {
	URI uri;
	int version = -1;
};

struct LSPTextDocumentEdit {
	LSPVersionedTextDocumentIdentifier textDocument;
	std::vector<LSPTextEdit> edits;
};

struct LSPWorkspaceEdit {
	// supported part for now
	std::map<URI, std::vector<LSPTextEdit>> changes;
	std::vector<LSPTextDocumentEdit> documentChanges;
};

struct LSPDiagnosticRelatedInformation {
	// empty url / invalid range when absent
	LSPLocation location;
	std::string message;
};

struct LSPDiagnosticsCodeAction {
	std::string title;
	std::string kind;
	bool isPreferred{ false };
	LSPWorkspaceEdit edit;
};

struct LSPDiagnostic {
	TextRange range;
	LSPDiagnosticSeverity severity;
	std::string code;
	std::string source;
	std::string message;
	std::vector<LSPDiagnosticRelatedInformation> relatedInformation;
	std::vector<LSPDiagnosticsCodeAction> codeActions;
};

struct LSPPublishDiagnosticsParams {
	URI uri;
	std::vector<LSPDiagnostic> diagnostics;
};

struct LSPCommand {
	std::string title;
	std::string command;
	// pretty opaque
	nlohmann::json arguments;
};

struct LSPCodeAction {
	std::string title;
	std::string kind;
	std::vector<LSPDiagnostic> diagnostics;
	LSPWorkspaceEdit edit;
	LSPCommand command;
	bool isPreferred{ false };
};

enum class LSPWorkDoneProgressKind { Begin, Report, End };

struct LSPWorkDoneProgressValue {
	LSPWorkDoneProgressKind kind;
	std::string title;
	std::string message;
	bool cancellable;
	unsigned percentage;
};
template <typename T> struct LSPProgressParams {
	// number or string
	nlohmann::json token;
	T value;
};

enum class LSPSymbolKind {
	File = 1,
	Module = 2,
	Namespace = 3,
	Package = 4,
	Class = 5,
	Method = 6,
	Property = 7,
	Field = 8,
	Constructor = 9,
	Enum = 10,
	Interface = 11,
	Function = 12,
	Variable = 13,
	Constant = 14,
	String = 15,
	Number = 16,
	Boolean = 17,
	Array = 18,
	Object = 19,
	Key = 20,
	Null = 21,
	EnumMember = 22,
	Struct = 23,
	Event = 24,
	Operator = 25,
	TypeParameter = 26,
};

class LSPSymbolKindHelper {
  public:
	// TODO: Complete this list
	static std::string toIconString( const LSPSymbolKind& kind ) {
		switch ( kind ) {
			case LSPSymbolKind::Method:
				return "symbol-method";
			case LSPSymbolKind::Function:
				return "symbol-function";
			case LSPSymbolKind::Constructor:
				return "symbol-constructor";
			case LSPSymbolKind::Field:
				return "symbol-field";
			case LSPSymbolKind::Variable:
				return "symbol-variable";
			case LSPSymbolKind::Class:
				return "symbol-class";
			case LSPSymbolKind::Interface:
				return "symbol-interface";
			case LSPSymbolKind::Module:
				return "symbol-module";
			case LSPSymbolKind::Property:
				return "symbol-property";
			case LSPSymbolKind::Enum:
				return "symbol-enum";
			case LSPSymbolKind::File:
				return "symbol-file";
			case LSPSymbolKind::EnumMember:
				return "symbol-enum-member";
			case LSPSymbolKind::Constant:
				return "symbol-constant";
			case LSPSymbolKind::Struct:
				return "symbol-struct";
			case LSPSymbolKind::Event:
				return "symbol-event";
			case LSPSymbolKind::Operator:
				return "symbol-operator";
			case LSPSymbolKind::TypeParameter:
				return "symbol-type-parameter";
			case LSPSymbolKind::Namespace:
				return "symbol-namespace";
			case LSPSymbolKind::Package:
				return "symbol-package";
			case LSPSymbolKind::String:
				return "symbol-string";
			case LSPSymbolKind::Number:
				return "symbol-number";
			case LSPSymbolKind::Boolean:
				return "symbol-boolean";
			case LSPSymbolKind::Array:
				return "symbol-array";
			case LSPSymbolKind::Object:
				return "symbol-object";
			case LSPSymbolKind::Key:
				return "symbol-key";
			case LSPSymbolKind::Null:
				return "symbol-null";
		}
		return "symbol-text";
	}
};

struct LSPSymbolInformation {
	LSPSymbolInformation() = default;
	LSPSymbolInformation( const std::string& _name, LSPSymbolKind _kind, TextRange _range,
						  const std::string& _detail ) :
		name( _name ), detail( _detail ), kind( _kind ), range( _range ) {}
	std::string name;
	std::string detail;
	LSPSymbolKind kind{ LSPSymbolKind::File };
	URI url;
	TextRange range;
	TextRange selectionRange;
	double score = 0.0;
	std::vector<LSPSymbolInformation> children;
};

using LSPSymbolInformationList = std::vector<LSPSymbolInformation>;

struct LSPSymbolInformationListHelper {
	static bool isFlat( const LSPSymbolInformationList& list ) {
		for ( const auto& l : list )
			if ( !l.children.empty() )
				return false;
		return true;
	}

	static LSPSymbolInformationList flatten( const LSPSymbolInformationList& list ) {
		LSPSymbolInformationList newList;
		for ( const auto& l : list ) {
			if ( l.children.empty() ) {
				newList.push_back( l );
			} else {
				auto nl = l;
				nl.children.clear();
				newList.push_back( nl );
				auto clist = flatten( l.children );
				for ( const auto& cl : clist )
					newList.push_back( cl );
			}
		}
		return newList;
	}
};

using LSPWorkDoneProgressParams = LSPProgressParams<LSPWorkDoneProgressValue>;

struct LSPResponseError {
	LSPErrorCode code{};
	std::string message;
	nlohmann::json data;
};

enum class LSPMarkupKind { None = 0, PlainText = 1, MarkDown = 2 };

struct LSPMarkupContent {
	LSPMarkupKind kind = LSPMarkupKind::None;
	std::string value;
};

struct LSPHover {
	std::vector<LSPMarkupContent> contents;
	TextRange range;
};

enum class LSPCompletionItemKind {
	Text = 1,
	Method = 2,
	Function = 3,
	Constructor = 4,
	Field = 5,
	Variable = 6,
	Class = 7,
	Interface = 8,
	Module = 9,
	Property = 10,
	Unit = 11,
	Value = 12,
	Enum = 13,
	Keyword = 14,
	Snippet = 15,
	Color = 16,
	File = 17,
	Reference = 18,
	Folder = 19,
	EnumMember = 20,
	Constant = 21,
	Struct = 22,
	Event = 23,
	Operator = 24,
	TypeParameter = 25,
};

class LSPCompletionItemHelper {
  public:
	static std::string toIconString( const LSPCompletionItemKind& kind ) {
		switch ( kind ) {
			case LSPCompletionItemKind::Text:
				return "symbol-text";
			case LSPCompletionItemKind::Method:
				return "symbol-method";
			case LSPCompletionItemKind::Function:
				return "symbol-function";
			case LSPCompletionItemKind::Constructor:
				return "symbol-constructor";
			case LSPCompletionItemKind::Field:
				return "symbol-field";
			case LSPCompletionItemKind::Variable:
				return "symbol-variable";
			case LSPCompletionItemKind::Class:
				return "symbol-class";
			case LSPCompletionItemKind::Interface:
				return "symbol-interface";
			case LSPCompletionItemKind::Module:
				return "symbol-module";
			case LSPCompletionItemKind::Property:
				return "symbol-property";
			case LSPCompletionItemKind::Unit:
				return "symbol-unit";
			case LSPCompletionItemKind::Value:
				return "symbol-value";
			case LSPCompletionItemKind::Enum:
				return "symbol-enum";
			case LSPCompletionItemKind::Keyword:
				return "symbol-keyword";
			case LSPCompletionItemKind::Snippet:
				return "symbol-snippet";
			case LSPCompletionItemKind::Color:
				return "symbol-color";
			case LSPCompletionItemKind::File:
				return "symbol-file";
			case LSPCompletionItemKind::Reference:
				return "symbol-reference";
			case LSPCompletionItemKind::Folder:
				return "symbol-folder";
			case LSPCompletionItemKind::EnumMember:
				return "symbol-enum-member";
			case LSPCompletionItemKind::Constant:
				return "symbol-constant";
			case LSPCompletionItemKind::Struct:
				return "symbol-struct";
			case LSPCompletionItemKind::Event:
				return "symbol-event";
			case LSPCompletionItemKind::Operator:
				return "symbol-operator";
			case LSPCompletionItemKind::TypeParameter:
				return "symbol-type-parameter";
		}
		return "symbol-text";
	}
};

struct LSPCompletionItem {
	std::string label;
	LSPCompletionItemKind kind;
	std::string detail;
	LSPMarkupContent documentation;
	std::string sortText;
	std::string insertText;
	std::string filterText;
	LSPTextEdit textEdit;
	std::vector<LSPTextEdit> additionalTextEdits;
};

struct LSPCompletionList {
	bool isIncomplete{ false };
	std::vector<LSPCompletionItem> items;
};

struct LSPSelectionRange {
	TextRange range;
	std::shared_ptr<LSPSelectionRange> parent;
};

struct LSPParameterInformation {
	// offsets into overall signature label
	// (-1 if invalid)
	int start{ -1 };
	int end{ -1 };
};

struct LSPSignatureInformation {
	std::string label;
	LSPMarkupContent documentation;
	std::vector<LSPParameterInformation> parameters;
};

struct LSPSignatureHelp {
	std::vector<LSPSignatureInformation> signatures;
	int activeSignature{ 0 };
	int activeParameter{ 0 };
};

struct LSPConverter {
	static TextPosition fromJSON( const nlohmann::json& data ) {
		if ( data.contains( "line" ) && data.contains( "character" ) )
			return { data["line"].get<Int64>(), data["character"].get<Int64>() };
		return {};
	}
};

struct LSPShowDocumentParams {
	URI uri;
	bool external{ true };
	bool takeFocus{ false };
	TextRange selection;
};

struct LSPApplyWorkspaceEditParams {
	std::string label;
	LSPWorkspaceEdit edit;
};

struct LSPApplyWorkspaceEditResponse {
	bool applied;
	std::string failureReason;
};

struct LSPSemanticTokensEdit {
	Uint32 start = 0;
	Uint32 deleteCount = 0;
	std::vector<Int32> data;
};

struct LSPSemanticTokensDelta {
	std::string resultId;
	std::vector<LSPSemanticTokensEdit> edits;
	std::vector<Int32> data;
};

} // namespace ecode

#endif // ECODE_LSPCLIENTPROTOCOL_HPP
