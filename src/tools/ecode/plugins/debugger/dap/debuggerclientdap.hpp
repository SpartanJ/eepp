#pragma once

#include "../bus.hpp"
#include "../debuggerclient.hpp"
#include "protocol.hpp"
#include <atomic>
#include <eepp/config.hpp>

using namespace EE;

namespace ecode::dap {

class DebuggerClientDap : public DebuggerClient {
  public:
	typedef std::function<void( const Response&, const nlohmann::json& )> ResponseHandler;

	DebuggerClientDap( std::unique_ptr<Bus>&& bus );

	bool hasBreakpoint( const std::string& path, size_t line );

	bool addBreakpoint( const std::string& path, size_t line );

	bool removeBreakpoint( const std::string& path, size_t line );

	bool start();

	bool attach();

	bool started() const;

	bool cont( int threadId );

	bool pause( int threadId ) = 0;

	bool next( int threadId ) = 0;

	bool goTo( int threadId, int targetId ) = 0;

	bool stepInto( int threadId );

	bool stepOver( int threadId );

	bool stepOut( int threadId );

	bool halt();

	bool terminate();

	bool stopped();

	bool completed();

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

	void processResponseLaunch( const Response& response, const nlohmann::json& );
};

} // namespace ecode::dap
