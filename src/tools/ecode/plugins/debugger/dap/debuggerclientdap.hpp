#pragma once

#include "../bus.hpp"
#include "../config.hpp"
#include "../debuggerclient.hpp"
#include "protocol.hpp"
#include <atomic>
#include <eepp/config.hpp>
#include <eepp/core.hpp>
#include <eepp/system/mutex.hpp>

using namespace EE;
using namespace EE::System;

namespace ecode::dap {

class DebuggerClientDap : public DebuggerClient {
  public:
	typedef std::function<void( const Response&, const nlohmann::json& )> ResponseHandler;

	std::function<void( bool isIntegrated, std::string cmd, const std::vector<std::string>& args,
						const std::string& cwd,
						const std::unordered_map<std::string, std::string>& env,
						std::function<void( int )> doneFn )>
		runInTerminalCb;

	std::function<void()> runTargetCb;

	DebuggerClientDap( const ProtocolSettings& protocolSettings, std::unique_ptr<Bus>&& bus );

	virtual ~DebuggerClientDap();

	bool start() override;

	bool resume( int threadId, bool singleThread = false ) override;

	bool pause( int threadId ) override;

	bool stepOver( int threadId, bool singleThread = false ) override;

	bool goTo( int threadId, int targetId ) override;

	bool stepInto( int threadId, bool singleThread = false ) override;

	bool stepOut( int threadId, bool singleThread = false ) override;

	bool terminate( bool restart ) override;

	bool disconnect( bool terminateDebuggee, bool restart = false ) override;
	bool threads() override;

	bool stackTrace( int threadId, int startFrame = 0, int levels = 0 ) override;
	bool scopes( int frameId ) override;

	bool variables( int variablesReference, Variable::Type filter = Variable::Type::Both,
					DebuggerClient::VariablesResponseCb responseCb = nullptr, int start = 0,
					int count = 0 ) override;

	bool modules( int start, int count ) override;

	bool evaluate( const std::string& expression, const std::string& context,
				   std::optional<int> frameId,
				   std::function<void( const std::string&, const std::optional<EvaluateInfo>& )>
					   cb = {} ) override;

	bool isServerConnected() const override;

	bool supportsTerminateRequest() const override;

	bool supportsTerminateDebuggee() const override;

	bool setBreakpoints( const std::string& path,
						 const std::vector<dap::SourceBreakpoint>& breakpoints,
						 bool sourceModified = false ) override;

	bool setBreakpoints( const dap::Source& source,
						 const std::vector<dap::SourceBreakpoint>& breakpoints,
						 bool sourceModified = false ) override;

	bool gotoTargets( const std::string& path, const int line,

					  const std::optional<int> column = std::nullopt ) override;
	bool gotoTargets( const dap::Source& source, const int line,

					  const std::optional<int> column = std::nullopt ) override;

	bool watch( const std::string& expression, std::optional<int> frameId ) override;

	bool configurationDone( const SessionId& sessionId ) override;

	bool started() const override;

	void setSilent( bool silent ) override { mDebug = !silent; }

	size_t sessionsActive() override { return mSessions.size(); }

  protected:
	// Session structure to hold per-session data
	struct Session {
		State state = State::None;
		std::string launchRequestType;
		nlohmann::json launchArgs;
		bool launched = false;
		bool configured = false;
		bool waitingToAttach = false;
		std::unique_ptr<Bus> bus;
		std::atomic<Uint64> idx{ 1 };
	};

	struct Request {
		std::string command;
		nlohmann::json arguments;
		ResponseHandler handler;
		std::string sessionId; // Associate request with a session
	};

	UnorderedMap<std::string, UnorderedSet<int>> mBreakpoints;
	bool mDebug{ true };
	bool mStarted{ false };
	bool mDestroying{ false };
	std::unordered_map<Uint64, Request> mRequests;
	std::string mBuffer;
	ProtocolSettings mProtocol;
	Capabilities mAdapterCapabilities;
	std::vector<std::unique_ptr<Bus>> mBusesToClose;

	// Session management
	mutable Mutex mSessionsMutex;
	std::unordered_map<std::string, Session> mSessions;	   // Session ID to Session
	std::string mCurrentSessionId;						   // Latest active session
	std::unordered_map<int, std::string> mThreadToSession; // Thread ID to Session ID

	void setState( const State& state, const SessionId& sessionId ) override;

	void checkRunning( const SessionId& sessionId ) override;

	void makeRequest( const std::string_view& command, const nlohmann::json& arguments,
					  ResponseHandler onFinish, const SessionId& sessionId );
	void makeResponse( int reqSeq, bool success, const std::string& command,
					   const nlohmann::json& body, const std::string& sessionId );
	void asyncRead( const char* bytes, size_t n );
	void processProtocolMessage( const nlohmann::json& msg );
	void processResponse( const nlohmann::json& msg );
	void processEvent( const nlohmann::json& msg );
	void processRequest( const nlohmann::json& msg );
	struct HeaderInfo {
		Uint64 payloadStart;
		Uint64 payloadLength;
	};
	std::optional<HeaderInfo> readHeader();
	void errorResponse( const std::string& command, const std::string& summary,
						const std::optional<Message>& message, const SessionId& sessionId = "" );

	// Event handlers
	void processEventInitialized( const SessionId& sessionId );
	void processEventTerminated( const SessionId& sessionId );
	void processEventExited( const nlohmann::json& body, const SessionId& sessionId );
	void processEventOutput( const nlohmann::json& body, const SessionId& sessionId );
	void processEventProcess( const nlohmann::json& body, const SessionId& sessionId );
	void processEventThread( const nlohmann::json& body, const SessionId& sessionId );
	void processEventStopped( const nlohmann::json& body, const SessionId& sessionId );
	void processEventModule( const nlohmann::json& body, const SessionId& sessionId );
	void processEventContinued( const nlohmann::json& body, const SessionId& sessionId );
	void processEventBreakpoint( const nlohmann::json& body, const SessionId& sessionId );

	// Session-specific methods
	void requestInitialize( const SessionId& sessionId, std::function<void()> onLaunch = {} );
	void requestLaunchCommand( const SessionId& sessionId, std::function<void()> onLaunch = {} );
	void processResponseInitialize( const Response& response, const nlohmann::json&,
									const SessionId& sessionId,
									std::function<void()> onLaunch = {} );
	void processResponseNext( const Response& response, const nlohmann::json& request,
							  const SessionId& sessionId );

	// Helper to find session by thread ID
	std::string findSessionByThread( int threadId ) const;
};

} // namespace ecode::dap
