#include "utest.hpp"
#include "vfs_test_utils.hpp"
#include <condition_variable>
#include <deque>
#include <eepp/system/process.hpp>
#include <efsw/agent_protocol.hpp>
#include <map>
#include <set>

using namespace vfstest;
using EE::System::Process;
namespace agent = efsw::agent;

namespace {

std::string agentPath() {
	std::string path = Sys::getProcessPath();
	FileSystem::dirAddSlashAtEnd( path );
#if EE_PLATFORM == EE_PLATFORM_WIN
	return path + "../efsw-agent.exe";
#else
	return path + "../efsw-agent";
#endif
}

/** Runs efsw-agent and exchanges frames with it through its stdin/stdout. */
class AgentProcess {
  public:
	bool start() {
		if ( !FileSystem::fileExists( agentPath() ) )
			return false;
		if ( !mProcess.create( agentPath(), std::vector<std::string>{ "--stdio" },
							   Process::getDefaultOptions() | Process::EnableAsync ) )
			return false;
		mProcess.startAsyncRead( [this]( const char* bytes, size_t n ) {
			std::lock_guard<std::mutex> lock( mMutex );
			mParser.feed( bytes, n );
			agent::Frame frame;
			std::string error;
			while ( mParser.next( frame, error ) == 1 )
				mFrames.push_back( frame );
			mReady.notify_all();
		} );
		return true;
	}

	void send( const std::vector<uint8_t>& frame ) {
		mProcess.write( reinterpret_cast<const char*>( frame.data() ), frame.size() );
	}

	void send( Uint16 type, const agent::Writer& payload ) { send( payload.frame( type ) ); }

	/** Waits for the next frame of a type (other frames are kept). */
	std::optional<agent::Frame> waitFor( Uint16 type, int timeoutMs = 5000 ) {
		std::unique_lock<std::mutex> lock( mMutex );
		std::optional<agent::Frame> found;
		mReady.wait_for( lock, std::chrono::milliseconds( timeoutMs ), [&] {
			for ( auto it = mFrames.begin(); it != mFrames.end(); ++it ) {
				if ( it->type == type ) {
					found = std::move( *it );
					mFrames.erase( it );
					return true;
				}
			}
			return false;
		} );
		return found;
	}

	/** Collects event paths per watch id until pred is satisfied or the timeout expires. */
	template <typename Pred> bool collectEvents( Pred pred, int timeoutMs = 5000 ) {
		auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds( timeoutMs );
		while ( std::chrono::steady_clock::now() < deadline ) {
			auto frame = waitFor( agent::MessageEventBatch, 100 );
			if ( frame ) {
				agent::Reader r( frame->payload );
				Uint32 id = r.u32();
				Uint32 count = r.u32();
				for ( Uint32 i = 0; i < count; ++i ) {
					Uint8 action = r.u8();
					std::string path = r.str();
					r.str();
					if ( action == agent::ActionAdd )
						events[id].insert( path );
				}
			}
			if ( pred() )
				return true;
		}
		return pred();
	}

	/** Closes the agent's stdin (EOF) and waits for it to exit. */
	int join() {
		int code = -1;
		mProcess.join( &code );
		return code;
	}

	std::map<Uint32, std::set<std::string>> events;

  private:
	Process mProcess;
	std::mutex mMutex;
	std::condition_variable mReady;
	agent::FrameParser mParser;
	std::deque<agent::Frame> mFrames;
};

#define REQUIRE_AGENT( agentProcess )                           \
	if ( !( agentProcess ).start() ) {                          \
		printf( "  efsw-agent not available, test skipped\n" ); \
		return;                                                 \
	}

agent::Writer watchMessage( Uint32 id, const std::string& path, bool recursive ) {
	agent::Writer w;
	w.u32( id ).str( path ).u8( recursive ? agent::WatchRecursive : 0 ).u32( 200 );
	return w;
}

} // namespace

UTEST( EfswAgent, multipleWatchesBurstsAndUnwatch ) {
	AgentProcess process;
	REQUIRE_AGENT( process );
	TempDir a, b;
	process.send( agent::MessageHello, agent::Writer().str( "test" ) );
	auto hello = process.waitFor( agent::MessageHelloReply );
	ASSERT_TRUE( hello.has_value() );
	agent::Reader reply( hello->payload );
	EXPECT_EQ( agent::ProtocolVersion, reply.u16() );

	process.send( agent::MessageWatch, watchMessage( 1, a.path, true ) );
	process.send( agent::MessageWatch, watchMessage( 2, b.path, false ) );
	EXPECT_TRUE( process.waitFor( agent::MessageWatchOk ).has_value() );
	EXPECT_TRUE( process.waitFor( agent::MessageWatchOk ).has_value() );

	// A burst on one watch, a single file on the other: events carry the right watch id.
	for ( int i = 0; i < 300; ++i )
		FileSystem::fileWrite( a.path + "f" + String::toString( i ), std::string_view( "x" ) );
	FileSystem::fileWrite( b.path + "only.txt", std::string_view( "x" ) );
	EXPECT_TRUE( process.collectEvents( [&] {
		return process.events[1].size() >= 300 && process.events[2].count( "only.txt" );
	} ) );
	EXPECT_EQ( 0u, process.events[2].count( "f0" ) );

	process.send( agent::MessageUnwatch, agent::Writer().u32( 2 ) );
	EXPECT_TRUE( process.waitFor( agent::MessageUnwatchOk ).has_value() );
	FileSystem::fileWrite( b.path + "after.txt", std::string_view( "x" ) );
	process.collectEvents( [] { return false; }, 500 );
	EXPECT_EQ( 0u, process.events[2].count( "after.txt" ) );

	// Errors are reported per watch, the agent keeps running.
	process.send( agent::MessageWatch, watchMessage( 3, a.path + "missing-dir", false ) );
	auto error = process.waitFor( agent::MessageWatchError );
	ASSERT_TRUE( error.has_value() );
	EXPECT_EQ( 3u, agent::Reader( error->payload ).u32() );
	process.send( agent::MessagePing, agent::Writer().u32( 42 ) );
	auto pong = process.waitFor( agent::MessagePong );
	ASSERT_TRUE( pong.has_value() );
	EXPECT_EQ( 42u, agent::Reader( pong->payload ).u32() );

	// Abrupt EOF on stdin: the agent exits cleanly.
	EXPECT_EQ( 0, process.join() );
}

UTEST( EfswAgent, rejectsProtocolMismatchAndMalformedInput ) {
	{
		AgentProcess process;
		REQUIRE_AGENT( process );
		std::vector<uint8_t> frame = agent::Writer().str( "x" ).frame( agent::MessageHello );
		frame[5] = 99; // protocol version
		process.send( frame );
		EXPECT_TRUE( process.waitFor( agent::MessageError ).has_value() );
		EXPECT_EQ( 2, process.join() );
	}
	{
		AgentProcess process;
		REQUIRE_AGENT( process );
		std::vector<uint8_t> oversized = { 0xFF, 0xFF, 0xFF, 0xFF, 0, 1, 0, 1 };
		process.send( oversized );
		EXPECT_TRUE( process.waitFor( agent::MessageError ).has_value() );
		EXPECT_EQ( 2, process.join() );
	}
	{
		AgentProcess process;
		REQUIRE_AGENT( process );
		process.send( 0x7777, agent::Writer() );
		EXPECT_TRUE( process.waitFor( agent::MessageError ).has_value() );
		EXPECT_EQ( 2, process.join() );
	}
}
