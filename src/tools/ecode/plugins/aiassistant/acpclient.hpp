#pragma once

#include "acpprotocol.hpp"
#include <atomic>
#include <eepp/system/clock.hpp>
#include <eepp/system/mutex.hpp>
#include <eepp/system/process.hpp>
#include <eepp/system/threadpool.hpp>
#include <functional>
#include <map>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

using json = nlohmann::json;

using namespace EE;
using namespace EE::System;

namespace ecode { namespace acp {

class ACPClient {
  public:
	using IdType = int;
	using JsonReplyHandler = std::function<void( const IdType& id, const json& )>;

	struct Config {
		std::string command;
		std::vector<std::string> args;
		std::string workingDirectory;
		std::unordered_map<std::string, std::string> environment;
	};

	ACPClient( std::shared_ptr<ThreadPool> threadPool, const Config& config );
	~ACPClient();

	bool start();
	void stop();

	bool isRunning();
	bool isReady();

	const Config& getConfig() const { return mConfig; }

	void initialize( const InitializeRequest& req,
					 const std::function<void( const InitializeResponse& )>& cb );
	void newSession( const NewSessionRequest& req,
					 const std::function<void( const NewSessionResponse& )>& cb );
	void prompt( const PromptRequest& req, const std::function<void( const PromptResponse& )>& cb );

	// Notifications to agent
	void cancel( const std::string& sessionId );

	// Callbacks from agent
	std::function<void( const json& )> onSessionUpdate;
	std::function<void( const ReadTextFileRequest&,
						std::function<void( const ReadTextFileResponse& )> )>
		onReadTextFile;
	std::function<void( const WriteTextFileRequest&,
						std::function<void( const WriteTextFileResponse& )> )>
		onWriteTextFile;
	std::function<void( const RequestPermissionRequest&,
						std::function<void( const RequestPermissionResponse& )> )>
		onRequestPermission;
	std::function<void( const CreateTerminalRequest&,
						std::function<void( const CreateTerminalResponse& )> )>
		onCreateTerminal;
	std::function<void( const TerminalOutputRequest&,
						std::function<void( const TerminalOutputResponse& )> )>
		onTerminalOutput;
	std::function<void( const KillTerminalRequest&,
						std::function<void( const KillTerminalResponse& )> )>
		onKillTerminal;
	std::function<void( const ReleaseTerminalRequest&,
						std::function<void( const ReleaseTerminalResponse& )> )>
		onReleaseTerminal;
	std::function<void( const WaitForTerminalExitRequest&,
						std::function<void( const WaitForTerminalExitResponse& )> )>
		onWaitForTerminalExit;

  protected:
	std::shared_ptr<ThreadPool> mThreadPool;
	Config mConfig;
	Process mProcess;
	bool mReady{ false };
	bool mShuttingDown{ false };
	std::atomic<int> mLastMsgId{ 0 };

	Mutex mHandlersMutex;
	std::map<IdType, JsonReplyHandler> mHandlers;

	std::string mReceiveBuffer;

	void readStdOut( const char* bytes, size_t n );
	void readStdErr( const char* bytes, size_t n );

	void processMessage( const json& msg );
	void processRequest( const json& msg );
	void processNotification( const json& msg );
	void processResponse( const json& msg );

	int write( json&& msg, const JsonReplyHandler& h = nullptr );
	void sendResponse( const json& id, json&& result );
	void sendError( const json& id, int code, const std::string& message );
};

}} // namespace ecode::acp
