#ifndef ECODE_LSPCLIENTSERVER_HPP
#define ECODE_LSPCLIENTSERVER_HPP

#include "lspclientprotocol.hpp"
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

	class RequestHandle {
		friend class LSPClientServer;
		LSPClientServer* mServer;
		int mId = 0;

	  public:
		RequestHandle& cancel() {
			if ( mServer )
				mServer->cancel( mId );
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

	LSPClientServer::RequestHandle cancel( int id );

	LSPClientServer::RequestHandle send( const json& msg, const JsonReplyHandler& h = nullptr,
										 const JsonReplyHandler& eh = nullptr );

	const LSPDefinition& getDefinition() const { return mLSP; }

	LSPClientServer::RequestHandle documentSymbols( const URI& document, const JsonReplyHandler& h,
													const JsonReplyHandler& eh );

	LSPClientServer::RequestHandle didOpen( const URI& document, const std::string& text,
											int version );

	LSPClientServer::RequestHandle didOpen( TextDocument* doc, int version );

	LSPClientServer::RequestHandle didSave( const URI& document, const std::string& text );

	LSPClientServer::RequestHandle didSave( TextDocument* doc );

	LSPClientServer::RequestHandle didClose( const URI& document );

	LSPClientServer::RequestHandle didClose( TextDocument* document );

	LSPClientServer::RequestHandle
	didChange( const URI& document, int version, const std::string& text,
			   const std::vector<DocumentContentChange>& change = {} );

	LSPClientServer::RequestHandle
	didChange( TextDocument* doc, const std::vector<DocumentContentChange>& change = {} );

	LSPClientServer::RequestHandle documentDefinition( const URI& document,
													   const TextPosition& pos );

	LSPClientServer::RequestHandle documentDeclaration( const URI& document,
														const TextPosition& pos );

	LSPClientServer::RequestHandle documentTypeDefinition( const URI& document,
														   const TextPosition& pos );

	LSPClientServer::RequestHandle documentImplementation( const URI& document,
														   const TextPosition& pos );

	void updateDirty();

	bool hasDocument( TextDocument* doc ) const;

	bool hasDocuments() const;

	LSPClientServer::RequestHandle
	didChangeWorkspaceFolders( const std::vector<LSPWorkspaceFolder>& added,
							   const std::vector<LSPWorkspaceFolder>& removed );

	void publishDiagnostics( const json& msg );

	void workDoneProgress( const LSPWorkDoneProgressParams& workDoneParams );

	LSPClientServer::RequestHandle getAndGoToLocation( const URI& document, const TextPosition& pos,
													   const std::string& search );

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

	LSPClientServer::RequestHandle write( const json& msg, const JsonReplyHandler& h = nullptr,
										  const JsonReplyHandler& eh = nullptr, const int id = 0 );

	void initialize();

	void sendQueuedMessages();

	void processNotification( const json& msg );

	void processRequest( const json& msg );

	void goToLocation( const json& res );
};

} // namespace ecode

#endif // ECODE_LSPCLIENTSERVER_HPP
