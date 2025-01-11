#pragma once

#include "../bus.hpp"
#include "../config.hpp"
#include "../debuggerclient.hpp"
#include "protocol.hpp"
#include <atomic>
#include <eepp/config.hpp>
#include <eepp/core.hpp>

using namespace EE;

namespace ecode::dap {

class DebuggerClientDap : public DebuggerClient {
  public:
	typedef std::function<void( const Response&, const nlohmann::json& )> ResponseHandler;

	std::function<void( bool isIntegrated, std::string cmd,
						const std::vector<std::string>& args, const std::string& cwd,
						const std::unordered_map<std::string, std::string>& env,
						std::function<void( int )> doneFn )>
		runInTerminalCb;

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

	bool disconnect( bool restart = false ) override;

	bool threads() override;

	bool stackTrace( int threadId, int startFrame = 0, int levels = 0 ) override;

	bool scopes( int frameId ) override;

	bool variables( int variablesReference, Variable::Type filter = Variable::Type::Both,
					int start = 0, int count = 0 ) override;

	bool modules( int start, int count ) override;

	bool evaluate(
		const std::string& expression, const std::string& context, std::optional<int> frameId,
		std::function<void( const std::string& expression, const std::optional<EvaluateInfo>& )>
			cb = {} ) override;

	bool isServerConnected() const override;

	bool supportsTerminate() const override;

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

	bool configurationDone() override;

	bool started() const override;

  protected:
	std::unique_ptr<Bus> mBus;
	UnorderedMap<std::string, UnorderedSet<int>> mBreakpoints;
	std::atomic<Uint64> mIdx{ 1 };
	Uint64 mThreadId{ 1 };
	bool mDebug{ true };
	bool mStarted{ false };
	struct Request {
		std::string command;
		nlohmann::json arguments;
		ResponseHandler handler;
	};
	std::unordered_map<Uint64, Request> mRequests;
	std::string mBuffer;
	ProtocolSettings mProtocol;
	Capabilities mAdapterCapabilities;

	void makeRequest( const std::string_view& command, const nlohmann::json& arguments,
					  ResponseHandler onFinish = nullptr );

	void makeResponse( int reqSeq, bool success, const std::string& command,
					   const nlohmann::json& body );

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

	void errorResponse( const std::string& summary, const std::optional<Message>& message );

	void processEventInitialized();

	void processEventTerminated();

	void processEventExited( const nlohmann::json& body );

	void processEventOutput( const nlohmann::json& body );

	void processEventProcess( const nlohmann::json& body );

	void processEventThread( const nlohmann::json& body );

	void processEventStopped( const nlohmann::json& body );

	void processEventModule( const nlohmann::json& body );

	void processEventContinued( const nlohmann::json& body );

	void processEventBreakpoint( const nlohmann::json& body );

	void requestInitialize();

	void requestLaunchCommand();

	void processResponseInitialize( const Response& response, const nlohmann::json& );

	void processResponseNext( const Response& response, const nlohmann::json& request );
};

} // namespace ecode::dap
