#ifndef ECODE_LSPCLIENTPROTOCOL_HPP
#define ECODE_LSPCLIENTPROTOCOL_HPP

#include <eepp/ui/doc/textdocument.hpp>
#include <nlohmann/json.hpp>
#include <string>

using namespace EE::UI::Doc;
using namespace EE::Network;

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

using LSPWorkDoneProgressParams = LSPProgressParams<LSPWorkDoneProgressValue>;

} // namespace ecode

#endif // ECODE_LSPCLIENTPROTOCOL_HPP
