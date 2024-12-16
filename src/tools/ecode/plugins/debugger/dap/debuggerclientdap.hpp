#pragma once

#include "../bus.hpp"
#include "../debuggerclient.hpp"
#include <atomic>
#include <eepp/config.hpp>

using namespace EE;

namespace ecode::dap {

class DebuggerClientDap : public DebuggerClient {
  public:
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
	UnorderedMap<Uint64, std::function<void()>> mCommandQueue;
	std::string mBuffer;

	void makeRequest( const std::string& command, const nlohmann::json& arguments,
					  std::function<void()> onFinish = nullptr );

	void asyncRead( const char* bytes, size_t n );

	void processProtocolMessage( const nlohmann::json& msg );

	void processResponse( const nlohmann::json& msg );

	void processEvent( const nlohmann::json& msg );

	struct HeaderInfo {
		Uint64 payloadStart;
		Uint64 payloadLength;
	};
	std::optional<HeaderInfo> readHeader();
};

} // namespace ecode::dap
