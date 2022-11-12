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

// Ref:
// https://microsoft.github.io/language-server-protocol/specification#textDocument_semanticTokens
struct LSPSemanticTokensOptions {
	bool full = false;
	bool fullDelta = false;
	bool range = false;
	// SemanticTokensLegend legend;
};

struct LSPWorkspaceFoldersServerCapabilities {
	bool supported = false;
	bool changeNotifications = false;
};

struct LSPServerCapabilities {
	bool ready = false;
	std::string language;
	LSPTextDocumentSyncOptions textDocumentSync;
	bool hoverProvider = false;
	LSPCompletionOptions completionProvider;
	LSPSignatureHelpOptions signatureHelpProvider;
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
	LSPDocumentOnTypeFormattingOptions documentOnTypeFormattingProvider;
	bool renameProvider = false;
	// CodeActionOptions not useful/considered at present
	bool codeActionProvider = false;
	LSPSemanticTokensOptions semanticTokenProvider;
	// workspace caps flattened
	// (other parts not useful/considered at present)
	LSPWorkspaceFoldersServerCapabilities workspaceFolders;
	bool selectionRangeProvider = false;
};

enum class LSPMessageType { Error = 1, Warning = 2, Info = 3, Log = 4 };

struct LSPShowMessageParams {
	LSPMessageType type;
	std::string message;
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

struct LSPDiagnosticRelatedInformation {
	// empty url / invalid range when absent
	LSPLocation location;
	std::string message;
};

struct LSPDiagnostic {
	TextRange range;
	LSPDiagnosticSeverity severity;
	std::string code;
	std::string source;
	std::string message;
	std::vector<LSPDiagnosticRelatedInformation> relatedInformation;
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

struct LSPVersionedTextDocumentIdentifier {
	URI uri;
	int version = -1;
};

using LSPTextEdit = LSPTextDocumentContentChangeEvent;

struct LSPTextDocumentEdit {
	LSPVersionedTextDocumentIdentifier textDocument;
	std::vector<LSPTextEdit> edits;
};

struct LSPWorkspaceEdit {
	// supported part for now
	std::map<URI, std::vector<LSPTextEdit>> changes;
	std::vector<LSPTextDocumentEdit> documentChanges;
};

struct LSPCodeAction {
	std::string title;
	std::string kind;
	std::vector<LSPDiagnostic> diagnostics;
	LSPWorkspaceEdit edit;
	LSPCommand command;
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
	int start;
	int end;
};

struct LSPSignatureInformation {
	std::string label;
	LSPMarkupContent documentation;
	std::vector<LSPParameterInformation> parameters;
};

struct LSPSignatureHelp {
	std::vector<LSPSignatureInformation> signatures;
	int activeSignature;
	int activeParameter;
};

struct LSPConverter {
	static TextPosition fromJSON( const nlohmann::json& data ) {
		if ( data.contains( "line" ) && data.contains( "character" ) )
			return { data["line"].get<Int64>(), data["character"].get<Int64>() };
		return {};
	}
};

} // namespace ecode

#endif // ECODE_LSPCLIENTPROTOCOL_HPP
