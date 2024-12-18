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

	DebuggerClientDap( std::unique_ptr<Bus>&& bus );

	bool hasBreakpoint( const std::string& path, size_t line ) override;

	bool addBreakpoint( const std::string& path, size_t line ) override;

	bool removeBreakpoint( const std::string& path, size_t line ) override;

	bool start() override;

	bool attach() override;

	bool cont( int threadId, bool singleThread = false ) override;

	bool pause( int threadId ) override;

	bool next( int threadId, bool singleThread = false ) override;

	bool goTo( int threadId, int targetId ) override;

	bool stepInto( int threadId, bool singleThread = false ) override;

	bool stepOut( int threadId, bool singleThread = false ) override;

	bool terminate( bool restart ) override;

	bool disconnect( bool restart = false ) override;

	bool threads() override;

	bool stackTrace( int threadId, int startFrame, int levels ) override;

	bool isServerConnected() const override;

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
	std::string mLaunchCommand;

	void makeRequest( const std::string_view& command, const nlohmann::json& arguments,
					  ResponseHandler onFinish = nullptr );

	void asyncRead( const char* bytes, size_t n );

	void processProtocolMessage( const nlohmann::json& msg );

	void processResponse( const nlohmann::json& msg );

	void processEvent( const nlohmann::json& msg );

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
