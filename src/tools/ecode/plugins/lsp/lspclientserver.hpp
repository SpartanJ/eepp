#ifndef ECODE_LSPCLIENTSERVER_HPP
#define ECODE_LSPCLIENTSERVER_HPP

#include "lspprotocol.hpp"
#include "lspdefinition.hpp"
#include "lspdocumentclient.hpp"
#include <eepp/system/process.hpp>
#include <eepp/ui/doc/textdocument.hpp>
#include <eepp/ui/doc/undostack.hpp>
#include <eepp/ui/uicodeeditor.hpp>
#include <eepp/ui/uipopupmenu.hpp>
#include <memory>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

using namespace EE;
using namespace EE::System;
using namespace EE::UI;
using namespace EE::UI::Doc;

namespace ecode {

class LSPClientServerManager;

class LSPClientServer {
  public:
	template <typename T> using ReplyHandler = std::function<void( const T& )>;

	using JsonReplyHandler = ReplyHandler<json>;
	using CodeActionHandler = ReplyHandler<std::vector<LSPCodeAction>>;
	using HoverHandler = ReplyHandler<LSPHover>;
	using CompletionHandler = ReplyHandler<std::vector<LSPCompletionItem>>;
	using SymbolInformationHandler = ReplyHandler<std::vector<LSPSymbolInformation>>;
	using SelectionRangeHandler = ReplyHandler<std::vector<std::shared_ptr<LSPSelectionRange>>>;

	class RequestHandle {
	  private:
		friend class LSPClientServer;
		LSPClientServer* server;
		int id = 0;

	  public:
		RequestHandle& cancel() {
			if ( server )
				server->cancel( id );
			return *this;
		}
	};

	LSPClientServer( LSPClientServerManager* manager, const String::HashType& id,
					 const LSPDefinition& lsp, const std::string& rootPath );

	~LSPClientServer();

	bool start();

	bool registerDoc( const std::shared_ptr<TextDocument>& doc );

	const LSPServerCapabilities& getCapabilities() const;

	LSPClientServerManager* getManager() const;

	const std::shared_ptr<ThreadPool>& getThreadPool() const;

	RequestHandle cancel( int id );

	RequestHandle send( const json& msg, const JsonReplyHandler& h = nullptr,
						const JsonReplyHandler& eh = nullptr );

	const LSPDefinition& getDefinition() const { return mLSP; }

	RequestHandle documentSymbols( const URI& document, const JsonReplyHandler& h,
								   const JsonReplyHandler& eh );

	RequestHandle documentSymbols( const URI& document,
								   const ReplyHandler<std::vector<LSPSymbolInformation>>& h,
								   const ReplyHandler<LSPResponseError>& eh );

	RequestHandle didOpen( const URI& document, const std::string& text, int version );

	RequestHandle didOpen( TextDocument* doc, int version );

	RequestHandle didSave( const URI& document, const std::string& text );

	RequestHandle didSave( TextDocument* doc );

	RequestHandle didClose( const URI& document );

	RequestHandle didClose( TextDocument* document );

	RequestHandle didChange( const URI& document, int version, const std::string& text,
							 const std::vector<DocumentContentChange>& change = {} );

	RequestHandle didChange( TextDocument* doc,
							 const std::vector<DocumentContentChange>& change = {} );

	RequestHandle documentDefinition( const URI& document, const TextPosition& pos );

	RequestHandle documentDeclaration( const URI& document, const TextPosition& pos );

	RequestHandle documentTypeDefinition( const URI& document, const TextPosition& pos );

	RequestHandle documentImplementation( const URI& document, const TextPosition& pos );

	void updateDirty();

	bool hasDocument( TextDocument* doc ) const;

	bool hasDocuments() const;

	RequestHandle didChangeWorkspaceFolders( const std::vector<LSPWorkspaceFolder>& added,
											 const std::vector<LSPWorkspaceFolder>& removed );

	void publishDiagnostics( const json& msg );

	void workDoneProgress( const LSPWorkDoneProgressParams& workDoneParams );

	RequestHandle getAndGoToLocation( const URI& document, const TextPosition& pos,
									  const std::string& search );

	RequestHandle switchSourceHeader( const URI& document );

	RequestHandle documentCodeAction( const URI& document, const TextRange& range,
									  const std::vector<std::string>& kinds,
									  std::vector<LSPDiagnostic> diagnostics,
									  const JsonReplyHandler& h );

	RequestHandle documentCodeAction( const URI& document, const TextRange& range,
									  const std::vector<std::string>& kinds,
									  std::vector<LSPDiagnostic> diagnostics,
									  const CodeActionHandler& h );

	RequestHandle documentHover( const URI& document, const TextPosition& pos,
								 const JsonReplyHandler& h );

	RequestHandle documentHover( const URI& document, const TextPosition& pos,
								 const HoverHandler& h );

	RequestHandle documentCompletion( const URI& document, const TextPosition& pos,
									  const JsonReplyHandler& h );

	RequestHandle documentCompletion( const URI& document, const TextPosition& pos,
									  const CompletionHandler& h );

	RequestHandle workspaceSymbol( const std::string& querySymbol, const JsonReplyHandler& h );

	RequestHandle workspaceSymbol( const std::string& querySymbol,
								   const SymbolInformationHandler& h );

	RequestHandle selectionRange( const URI& document, const std::vector<TextPosition>& positions,
								  const JsonReplyHandler& h );

	RequestHandle selectionRange( const URI& document, const std::vector<TextPosition>& positions,
								  const SelectionRangeHandler& h );

	RequestHandle documentSemanticTokensFull( const URI& document, bool delta,
											  const std::string& requestId, const TextRange& range,
											  const JsonReplyHandler& h );

  protected:
	LSPClientServerManager* mManager{ nullptr };
	String::HashType mId;
	LSPDefinition mLSP;
	std::string mRootPath;
	Process mProcess;
	std::vector<TextDocument*> mDocs;
	std::map<TextDocument*, std::unique_ptr<LSPDocumentClient>> mClients;
	std::map<int, std::pair<JsonReplyHandler, JsonReplyHandler>> mHandlers;
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

	int mLastMsgId{ -1 };

	void readStdOut( const char* bytes, size_t n );

	void readStdErr( const char* bytes, size_t n );

	RequestHandle write( const json& msg, const JsonReplyHandler& h = nullptr,
						 const JsonReplyHandler& eh = nullptr, const int id = 0 );

	void initialize();

	void sendQueuedMessages();

	void processNotification( const json& msg );

	void processRequest( const json& msg );

	void goToLocation( const json& res );
};

} // namespace ecode

#endif // ECODE_LSPCLIENTSERVER_HPP
