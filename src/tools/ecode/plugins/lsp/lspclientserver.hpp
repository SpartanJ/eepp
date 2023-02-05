#ifndef ECODE_LSPCLIENTSERVER_HPP
#define ECODE_LSPCLIENTSERVER_HPP

#include "../pluginmanager.hpp"
#include "lspdefinition.hpp"
#include "lspdocumentclient.hpp"
#include "lspprotocol.hpp"
#include <eepp/system/process.hpp>
#include <eepp/ui/doc/textdocument.hpp>
#include <eepp/ui/doc/undostack.hpp>
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

	using JsonReplyHandler = ReplyHandler<json>;
	using CodeActionHandler = ReplyHandler<std::vector<LSPCodeAction>>;
	using HoverHandler = ReplyHandler<LSPHover>;
	using CompletionHandler = ReplyHandler<LSPCompletionList>;
	using SymbolInformationHandler = ReplyHandler<std::vector<LSPSymbolInformation>>;
	using SelectionRangeHandler = ReplyHandler<std::vector<std::shared_ptr<LSPSelectionRange>>>;
	using SignatureHelpHandler = ReplyHandler<LSPSignatureHelp>;
	using LocationHandler = ReplyHandler<std::vector<LSPLocation>>;
	using TextEditArrayHandler = ReplyHandler<std::vector<LSPTextEdit>>;

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
					 const LSPDefinition& lsp, const std::string& rootPath );

	~LSPClientServer();

	bool start();

	bool registerDoc( const std::shared_ptr<TextDocument>& doc );

	const LSPServerCapabilities& getCapabilities() const;

	LSPClientServerManager* getManager() const;

	const std::shared_ptr<ThreadPool>& getThreadPool() const;

	LSPRequestHandle cancel( const PluginIDType& id );

	LSPRequestHandle send( const json& msg, const JsonReplyHandler& h = nullptr,
						   const JsonReplyHandler& eh = nullptr );

	const LSPDefinition& getDefinition() const { return mLSP; }

	LSPRequestHandle documentSymbols( const URI& document, const JsonReplyHandler& h,
									  const JsonReplyHandler& eh );

	LSPRequestHandle documentSymbols( const URI& document,
									  const ReplyHandler<std::vector<LSPSymbolInformation>>& h,
									  const ReplyHandler<LSPResponseError>& eh );

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

	LSPRequestHandle documentDefinition( const URI& document, const TextPosition& pos );

	LSPRequestHandle documentDeclaration( const URI& document, const TextPosition& pos );

	LSPRequestHandle documentTypeDefinition( const URI& document, const TextPosition& pos );

	LSPRequestHandle documentImplementation( const URI& document, const TextPosition& pos );

	bool hasDocument( TextDocument* doc ) const;

	bool hasDocument( const URI& uri ) const;

	bool hasDocuments() const;

	LSPRequestHandle didChangeWorkspaceFolders( const std::vector<LSPWorkspaceFolder>& added,
												const std::vector<LSPWorkspaceFolder>& removed );

	void publishDiagnostics( const json& msg );

	void workDoneProgress( const LSPWorkDoneProgressParams& workDoneParams );

	LSPRequestHandle getAndGoToLocation( const URI& document, const TextPosition& pos,
										 const std::string& search );

	LSPRequestHandle switchSourceHeader( const URI& document );

	LSPRequestHandle documentCodeAction( const URI& document, const TextRange& range,
										 const std::vector<std::string>& kinds,
										 std::vector<LSPDiagnostic> diagnostics,
										 const JsonReplyHandler& h );

	LSPRequestHandle documentCodeAction( const URI& document, const TextRange& range,
										 const std::vector<std::string>& kinds,
										 std::vector<LSPDiagnostic> diagnostics,
										 const CodeActionHandler& h );

	LSPRequestHandle documentHover( const URI& document, const TextPosition& pos,
									const JsonReplyHandler& h );

	LSPRequestHandle documentHover( const URI& document, const TextPosition& pos,
									const HoverHandler& h );

	LSPRequestHandle documentReferences( const URI& document, const TextPosition& pos, bool decl,
										 const JsonReplyHandler& h );

	LSPRequestHandle documentCompletion( const URI& document, const TextPosition& pos,
										 const JsonReplyHandler& h );

	LSPRequestHandle documentCompletion( const URI& document, const TextPosition& pos,
										 const CompletionHandler& h );

	LSPRequestHandle workspaceSymbol( const std::string& querySymbol, const JsonReplyHandler& h );

	LSPRequestHandle workspaceSymbol( const std::string& querySymbol,
									  const SymbolInformationHandler& h );

	LSPRequestHandle selectionRange( const URI& document,
									 const std::vector<TextPosition>& positions,
									 const JsonReplyHandler& h );

	LSPRequestHandle selectionRange( const URI& document,
									 const std::vector<TextPosition>& positions,
									 const SelectionRangeHandler& h );

	LSPRequestHandle documentSemanticTokensFull( const URI& document, bool delta,
												 const std::string& requestId,
												 const TextRange& range,
												 const JsonReplyHandler& h );

	LSPRequestHandle signatureHelp( const URI& document, const TextPosition& pos,
									const JsonReplyHandler& h );

	LSPRequestHandle signatureHelp( const URI& document, const TextPosition& pos,
									const SignatureHelpHandler& h );

	LSPRequestHandle documentFormatting( const URI& document, const json& options,
										 const JsonReplyHandler& h );

	LSPRequestHandle documentFormatting( const URI& document, const json& options,
										 const TextEditArrayHandler& h );

	void removeDoc( TextDocument* doc );

	LSPClientServer::LSPRequestHandle documentReferences( const URI& document,
														  const TextPosition& pos, bool decl,
														  const LocationHandler& h );

  protected:
	LSPClientServerManager* mManager{ nullptr };
	String::HashType mId;
	LSPDefinition mLSP;
	std::string mRootPath;
	Process mProcess;
	std::vector<TextDocument*> mDocs;
	std::map<TextDocument*, std::unique_ptr<LSPDocumentClient>> mClients;
	using HandlersMap = std::map<PluginIDType, std::pair<JsonReplyHandler, JsonReplyHandler>>;
	HandlersMap mHandlers;
	Mutex mClientsMutex;
	Mutex mHandlersMutex;
	bool mReady{ false };
	struct QueueMessage {
		json msg;
		JsonReplyHandler h;
		JsonReplyHandler eh;
	};
	std::vector<QueueMessage> mQueuedMessages;
	std::string mReceive;
	std::string mReceiveErr;
	LSPServerCapabilities mCapabilities;

	struct DidChangeQueue {
		URI uri;
		IdType version;
		std::vector<DocumentContentChange> change;
	};
	std::queue<DidChangeQueue> mDidChangeQueue;
	Mutex mDidChangeMutex;

	int mLastMsgId{ 0 };

	void readStdOut( const char* bytes, size_t n );

	void readStdErr( const char* bytes, size_t n );

	LSPRequestHandle write( const json& msg, const JsonReplyHandler& h = nullptr,
							const JsonReplyHandler& eh = nullptr, const int id = 0 );

	void initialize();

	void sendQueuedMessages();

	void processNotification( const json& msg );

	void processRequest( const json& msg );

	void goToLocation( const json& res );
};

} // namespace ecode

#endif // ECODE_LSPCLIENTSERVER_HPP
