#ifndef ECODE_LSPCLIENTPROTOCOL_HPP
#define ECODE_LSPCLIENTPROTOCOL_HPP

#include <eepp/ui/doc/textdocument.hpp>
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

} // namespace ecode

#endif // ECODE_LSPCLIENTPROTOCOL_HPP
