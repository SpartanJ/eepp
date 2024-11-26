#ifndef ECODE_LSPCLIENTSERVER_HPP
#define ECODE_LSPCLIENTSERVER_HPP

#include "../pluginmanager.hpp"
#include "lspdefinition.hpp"
#include "lspdocumentclient.hpp"
#include "lspprotocol.hpp"
#include <atomic>
#include <eepp/network/tcpsocket.hpp>
#include <eepp/system/process.hpp>
#include <eepp/ui/doc/textdocument.hpp>
#include <eepp/ui/uicodeeditor.hpp>
#include <eepp/ui/uipopupmenu.hpp>
#include <memory>
#include <nlohmann/json.hpp>
#include <queue>

using json = nlohmann::json;

using namespace EE;
using namespace EE::System;
using namespace EE::UI;
using namespace EE::UI::Doc;

namespace ecode {

class LSPClientServerManager;

class LSPClientServer {
  public:
	static PluginIDType getID( const json& json );

	using IdType = PluginIDType;
	template <typename T> using ReplyHandler = std::function<void( const IdType& id, const T& )>;

	template <typename T> using WReplyHandler = std::function<void( const IdType& id, T&& )>;

	using JsonReplyHandler = ReplyHandler<json>;
	using CodeActionHandler = ReplyHandler<std::vector<LSPCodeAction>>;
	using CodeLensHandler = ReplyHandler<std::vector<LSPCodeLens>>;
	using HoverHandler = ReplyHandler<LSPHover>;
	using CompletionHandler = ReplyHandler<LSPCompletionList>;
	using SymbolInformationHandler = WReplyHandler<LSPSymbolInformationList>;
	using SelectionRangeHandler = ReplyHandler<std::vector<std::shared_ptr<LSPSelectionRange>>>;
	using SignatureHelpHandler = ReplyHandler<LSPSignatureHelp>;
	using LocationHandler = ReplyHandler<std::vector<LSPLocation>>;
	using TextEditArrayHandler = ReplyHandler<std::vector<LSPTextEdit>>;
	using WorkspaceEditHandler = ReplyHandler<LSPWorkspaceEdit>;
	using SemanticTokensDeltaHandler = WReplyHandler<LSPSemanticTokensDelta>;
	using WorkspaceDiagnosticHandler = WReplyHandler<LSPWorkspaceDiagnosticReport>;

	class LSPRequestHandle : public PluginRequestHandle {
	  public:
		void cancel() {
			if ( server && mId.isValid() )
				server->cancel( mId );
		}

	  private:
		friend class LSPClientServer;
		LSPClientServer* server;
	};

	LSPClientServer( LSPClientServerManager* manager, const String::HashType& id,
					 const LSPDefinition& lsp, const std::string& rootPath,
					 const std::vector<std::string>& languagesSupported );

	~LSPClientServer();

	bool start();

	bool registerDoc( const std::shared_ptr<TextDocument>& doc );

	bool isRunning();

	bool isReady() const;

	const LSPServerCapabilities& getCapabilities() const;

	LSPClientServerManager* getManager() const;

	const std::shared_ptr<ThreadPool>& getThreadPool() const;

	LSPRequestHandle cancel( const PluginIDType& id );

	LSPRequestHandle send( json&& msg, const JsonReplyHandler& h = nullptr,
						   const JsonReplyHandler& eh = nullptr );

	void sendAsync( json&& msg, const JsonReplyHandler& h = nullptr,
					const JsonReplyHandler& eh = nullptr );

	const LSPDefinition& getDefinition() const { return mLSP; }

	LSPRequestHandle documentSymbols( const URI& document, const JsonReplyHandler& h,
									  const JsonReplyHandler& eh );

	LSPRequestHandle documentSymbols( const URI& document,
									  const WReplyHandler<LSPSymbolInformationList>& h,
									  const ReplyHandler<LSPResponseError>& eh = {} );

	LSPClientServer::LSPRequestHandle documentFoldingRange( const URI& document,
															const JsonReplyHandler& h,
															const JsonReplyHandler& eh );

	LSPRequestHandle documentFoldingRange( const URI& document,
										   const ReplyHandler<std::vector<LSPFoldingRange>>& h,
										   const ReplyHandler<LSPResponseError>& eh = {} );

	LSPRequestHandle documentSymbolsBroadcast( const URI& document );

	LSPRequestHandle didOpen( const URI& document, const std::string& text, int version );

	LSPRequestHandle didOpen( TextDocument* doc, int version );

	LSPRequestHandle didSave( const URI& document, const std::string& text );

	LSPRequestHandle didSave( TextDocument* doc );

	LSPRequestHandle didClose( const URI& document );

	LSPRequestHandle didChange( const URI& document, int version, const std::string& text,
								const std::vector<DocumentContentChange>& change = {} );

	LSPRequestHandle didChange( TextDocument* doc,
								const std::vector<DocumentContentChange>& change = {} );

	void queueDidChange( const URI& document, int version, const std::string& text,
						 const std::vector<DocumentContentChange>& change = {} );

	void processDidChangeQueue();

	void documentDefinition( const URI& document, const TextPosition& pos );

	void documentDeclaration( const URI& document, const TextPosition& pos );

	void documentTypeDefinition( const URI& document, const TextPosition& pos );

	void documentImplementation( const URI& document, const TextPosition& pos );

	bool hasDocument( TextDocument* doc ) const;

	bool hasDocument( const URI& uri ) const;

	bool hasDocuments() const;

	LSPRequestHandle didChangeWorkspaceFolders( const std::vector<LSPWorkspaceFolder>& added,
												const std::vector<LSPWorkspaceFolder>& removed,
												bool async );

	void publishDiagnostics( const json& msg );

	void workDoneProgress( const LSPWorkDoneProgressParams& workDoneParams );

	void getAndGoToLocation( const URI& document, const TextPosition& pos,
							 const std::string& search, const LocationHandler& h );

	void getAndGoToLocation( const URI& document, const TextPosition& pos,
							 const std::string& search );

	void switchSourceHeader( const URI& document );

	void documentCodeAction( const URI& document, const TextRange& range,
							 const std::vector<std::string>& kinds,
							 const nlohmann::json& diagnostics, const JsonReplyHandler& h );

	void documentCodeAction( const URI& document, const TextRange& range,
							 const std::vector<std::string>& kinds,
							 const nlohmann::json& diagnostics, const CodeActionHandler& h );

	void documentCodeLens( const URI& document, const JsonReplyHandler& h );

	void documentCodeLens( const URI& document, const CodeLensHandler& h );

	void documentHover( const URI& document, const TextPosition& pos, const JsonReplyHandler& h );

	void documentHover( const URI& document, const TextPosition& pos, const HoverHandler& h );

	void documentReferences( const URI& document, const TextPosition& pos, bool decl,
							 const JsonReplyHandler& h );

	void documentReferences( const URI& document, const TextPosition& pos, bool decl,
							 const LocationHandler& h );

	LSPRequestHandle documentCompletion( const URI& document, const TextPosition& pos,
										 const JsonReplyHandler& h );

	LSPRequestHandle documentCompletion( const URI& document, const TextPosition& pos,
										 const CompletionHandler& h );

	void workspaceSymbolAsync( const std::string& querySymbol, const JsonReplyHandler& h,
							   const size_t& limit = 100 );

	void workspaceSymbolAsync( const std::string& querySymbol, const SymbolInformationHandler& h,
							   const size_t& limit = 100 );

	LSPRequestHandle workspaceSymbol( const std::string& querySymbol, const JsonReplyHandler& h,
									  const size_t& limit = 100 );

	LSPRequestHandle workspaceSymbol( const std::string& querySymbol,
									  const SymbolInformationHandler& h,
									  const size_t& limit = 100 );

	void workspaceDiagnosticAsync( const JsonReplyHandler& h );

	void workspaceDiagnosticAsync( const WorkspaceDiagnosticHandler& h );

	LSPRequestHandle selectionRange( const URI& document,
									 const std::vector<TextPosition>& positions,
									 const JsonReplyHandler& h );

	LSPRequestHandle selectionRange( const URI& document,
									 const std::vector<TextPosition>& positions,
									 const SelectionRangeHandler& h );

	void documentSemanticTokensFull( const URI& document, bool delta, const std::string& requestId,
									 const TextRange& range, const JsonReplyHandler& h );

	void documentSemanticTokensFull( const URI& document, bool delta, const std::string& requestId,
									 const TextRange& range, const SemanticTokensDeltaHandler& h );

	LSPRequestHandle signatureHelp( const URI& document, const TextPosition& pos,
									const JsonReplyHandler& h );

	LSPRequestHandle signatureHelp( const URI& document, const TextPosition& pos,
									const SignatureHelpHandler& h );

	LSPRequestHandle documentFormatting( const URI& document, const json& options,
										 const JsonReplyHandler& h );

	LSPRequestHandle documentFormatting( const URI& document, const json& options,
										 const TextEditArrayHandler& h );

	LSPRequestHandle documentRangeFormatting( const URI& document, const TextRange& range,
											  const json& options, const JsonReplyHandler& h );

	LSPRequestHandle documentRangeFormatting( const URI& document, const TextRange& range,
											  const json& options, const TextEditArrayHandler& h );

	void documentRename( const URI& document, const TextPosition& pos, const std::string& newName,
						 const JsonReplyHandler& h );

	void documentRename( const URI& document, const TextPosition& pos, const std::string& newName,
						 const WorkspaceEditHandler& h );

	void memoryUsage( const JsonReplyHandler& h );

	void memoryUsage();

	void executeCommand( const std::string& cmd, const json& params );

	void registerCapabilities( const json& jcap );

	void removeDoc( TextDocument* doc );

	void shutdown();

	bool supportsLanguage( const std::string& lang ) const;

	LSPDocumentClient* getLSPDocumentClient( TextDocument* doc );

	bool isSilent() const;

  protected:
	LSPClientServerManager* mManager{ nullptr };
	String::HashType mId;
	LSPDefinition mLSP;
	std::string mRootPath;
	Process mProcess;
	TcpSocket* mSocket{ nullptr };
	std::vector<TextDocument*> mDocs;
	std::unordered_map<TextDocument*, std::unique_ptr<LSPDocumentClient>> mClients;
	using HandlersMap = std::map<PluginIDType, std::pair<JsonReplyHandler, JsonReplyHandler>>;
	HandlersMap mHandlers;
	Mutex mClientsMutex;
	Mutex mHandlersMutex;
	bool mReady{ false };
	bool mEnded{ false };
	bool mUsingProcess{ false };
	bool mUsingSocket{ false };
	bool mNotifiedServerError{ false };
	struct QueueMessage {
		json msg;
		JsonReplyHandler h;
		JsonReplyHandler eh;
	};
	std::vector<QueueMessage> mQueuedMessages;
	std::string mReceive;
	std::string mReceiveErr;
	LSPServerCapabilities mCapabilities;
	URI mWorkspaceFolder;
	std::vector<std::string> mLanguagesSupported;

	struct DidChangeQueue {
		URI uri;
		IdType version;
		std::vector<DocumentContentChange> change;
	};
	std::queue<DidChangeQueue> mDidChangeQueue;
	Mutex mDidChangeMutex;
	std::mutex mShutdownMutex;
	std::condition_variable mShutdownCond;

	std::atomic<int> mLastMsgId{ 0 };

	void readStdOut( const char* bytes, size_t n );

	void readStdErr( const char* bytes, size_t n );

	LSPRequestHandle write( json&& msg, const JsonReplyHandler& h = nullptr,
							const JsonReplyHandler& eh = nullptr, const int id = 0 );

	void initialize();

	void sendQueuedMessages();

	void processNotification( const json& msg );

	void processRequest( const json& msg );

	void goToLocation( const json& res );

	void notifyServerInitialized();

	bool needsAsync();

	bool socketConnect();

	void socketInitialize();

	LSPClientServer::LSPRequestHandle sendSync( json&& msg, const JsonReplyHandler& h = nullptr,
												const JsonReplyHandler& eh = nullptr );

	void refreshSmenaticHighlighting();

	void refreshCodeLens();

	bool trimLogs() const;

	void notifyServerError();
};

} // namespace ecode

#endif // ECODE_LSPCLIENTSERVER_HPP
