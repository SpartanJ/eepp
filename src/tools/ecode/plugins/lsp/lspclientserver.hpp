#ifndef ECODE_LSPCLIENTSERVER_HPP
#define ECODE_LSPCLIENTSERVER_HPP

#include "lspdefinition.hpp"
#include "lspdocumentclient.hpp"
#include <eepp/system/process.hpp>
#include <eepp/ui/doc/textdocument.hpp>
#include <eepp/ui/doc/undostack.hpp>
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

	using GenericReplyHandler = ReplyHandler<json>;

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

	LSPClientServerManager* getManager() const;

	const std::shared_ptr<ThreadPool>& getThreadPool() const;

	LSPClientServer::RequestHandle cancel( int id );

	RequestHandle send( const json& msg, const GenericReplyHandler& h = nullptr,
						const GenericReplyHandler& eh = nullptr );

	const LSPDefinition& getDefinition() const { return mLSP; }

	LSPClientServer::RequestHandle documentSymbols( const URI& document,
													const GenericReplyHandler& h,
													const GenericReplyHandler& eh );

	LSPClientServer::RequestHandle didOpen( const URI& document, const std::string& text,
											int version );

	LSPClientServer::RequestHandle didOpen( TextDocument* doc, int version );

	LSPClientServer::RequestHandle didSave( const URI& document, const std::string& text );

	LSPClientServer::RequestHandle didSave( TextDocument* doc );

	LSPClientServer::RequestHandle didClose( const URI& document );

	LSPClientServer::RequestHandle didClose( TextDocument* document );

	LSPClientServer::RequestHandle didChange( const URI& document, int version,
											  const std::string& text );

	LSPClientServer::RequestHandle didChange( TextDocument* doc );

	void updateDirty();

  protected:
	LSPClientServerManager* mManager{ nullptr };
	String::HashType mId;
	LSPDefinition mLSP;
	std::string mRootPath;
	Process mProcess;
	std::vector<TextDocument*> mDocs;
	std::map<TextDocument*, std::unique_ptr<LSPDocumentClient>> mClients;
	std::map<int, std::pair<GenericReplyHandler, GenericReplyHandler>> mHandlers;
	Mutex mClientsMutex;

	int mLastMsgId{ 0 };

	void readStdOut( const char* bytes, size_t n );

	void readStdErr( const char* bytes, size_t n );

	LSPClientServer::RequestHandle write( const json& msg, const GenericReplyHandler& h = nullptr,
										  const GenericReplyHandler& eh = nullptr,
										  const int id = 0 );
	void initialize();
};

} // namespace ecode

#endif // ECODE_LSPCLIENTSERVER_HPP
