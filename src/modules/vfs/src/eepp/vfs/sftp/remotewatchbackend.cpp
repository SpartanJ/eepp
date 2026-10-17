#include "sftpproviderimpl.hpp"
#include "sshconnection.hpp"
#include <condition_variable>
#include <eepp/vfs/sftp/remotewatchbackend.hpp>
#include <eepp/vfs/virtualfilesystem.hpp>
#include <eepp/vfs/watchers/pollingwatchbackend.hpp>
#include <efsw/agent_protocol.hpp>
#include <map>
#include <mutex>
#include <thread>

namespace EE { namespace VFS {
namespace sftp {

namespace agent = efsw::agent;

class AgentClient;

/** Logical watch: survives agent restarts and connection loss. */
struct RemoteWatch {
	enum class Mode : Uint8 { Establishing, Agent, Polling, Reconnecting, Closed };

	Uint32 id{ 0 };
	URI root;
	WatchOptions options;
	std::weak_ptr<WatchSink> sink;
	std::shared_ptr<AgentClient> agent;
	std::unique_ptr<WatchRegistration> polling;
	std::optional<TaskResolver<VFSResult<void>>> establishment;
	Uint32 retries{ 0 };
	Mode mode{ Mode::Establishing };
	bool established{ false };
};

class RemoteWatchBackendImpl;

/** One efsw-agent process on a remote host, reached through an exec channel. Lives on the SSH
 * I/O thread as a channel pump. */
class AgentClient : public SSHChannelPump, public std::enable_shared_from_this<AgentClient> {
  public:
	enum class State : Uint8 { Starting, Ready, Dead };

	AgentClient( std::weak_ptr<RemoteWatchBackendImpl> backend,
				 std::shared_ptr<SSHConnection> connection, std::string key ) :
		mBackend( std::move( backend ) ),
		mConnection( std::move( connection ) ),
		mKey( std::move( key ) ) {}

	const std::string& key() const { return mKey; }

	State state() const { return mState.load(); }

	const std::shared_ptr<SSHConnection>& connection() const { return mConnection; }

	/** Opens the exec channel (serialized on the connection queue) and starts the handshake. */
	void start( const std::string& command );

	/** Thread-safe: queue a frame for the agent. */
	void send( std::vector<uint8_t> frame ) {
		auto self = shared_from_this();
		mConnection->ioContext()->post( [self, frame = std::move( frame )]() mutable {
			if ( self->mState.load() == State::Dead )
				return;
			self->mOut.insert( self->mOut.end(), frame.begin(), frame.end() );
		} );
	}

	bool pump( SSHConnection& connection ) override;

	void connectionLost( const VFSError& error ) override {
		die( error.message );
		// A failed session frees its remaining channels during teardown.
		mChannel = nullptr;
	}

	/** Thread-safe: gives up on the agent (as if its channel closed). */
	void stop( std::string reason ) {
		auto self = shared_from_this();
		mConnection->ioContext()->post(
			[self, reason = std::move( reason )] { self->die( reason ); } );
	}

  private:
	void die( const std::string& reason );

	void handle( const agent::Frame& frame );

	std::weak_ptr<RemoteWatchBackendImpl> mBackend;
	std::shared_ptr<SSHConnection> mConnection;
	std::string mKey;
	LIBSSH2_CHANNEL* mChannel{ nullptr };
	agent::FrameParser mParser;
	std::vector<uint8_t> mOut;
	std::string mStderr;
	std::atomic<State> mState{ State::Starting };
	bool mHello{ false };
	bool mPumping{ false };
};

/** Delayed callbacks for retries and backoff. */
class DelayedRunner {
  public:
	DelayedRunner() : mThread( [this] { loop(); } ) {}

	~DelayedRunner() {
		{
			std::lock_guard<std::mutex> lock( mMutex );
			mStop = true;
		}
		mWake.notify_all();
		mThread.join();
	}

	void runAfter( std::chrono::milliseconds delay, std::function<void()> fn ) {
		{
			std::lock_guard<std::mutex> lock( mMutex );
			mQueue.emplace( std::chrono::steady_clock::now() + delay, std::move( fn ) );
		}
		mWake.notify_all();
	}

  private:
	void loop() {
		std::unique_lock<std::mutex> lock( mMutex );
		while ( !mStop ) {
			if ( mQueue.empty() ) {
				mWake.wait( lock );
				continue;
			}
			auto next = mQueue.begin();
			if ( next->first > std::chrono::steady_clock::now() ) {
				mWake.wait_until( lock, next->first );
				continue;
			}
			auto fn = std::move( next->second );
			mQueue.erase( next );
			lock.unlock();
			fn();
			lock.lock();
		}
	}

	std::mutex mMutex;
	std::condition_variable mWake;
	std::multimap<std::chrono::steady_clock::time_point, std::function<void()>> mQueue;
	bool mStop{ false };
	std::thread mThread;
};

class RemoteWatchBackendImpl : public std::enable_shared_from_this<RemoteWatchBackendImpl> {
  public:
	/** An agent that failed before its handshake is not retried for this long (probably not
	 * installed); watches use polling meanwhile. */
	static constexpr auto UnavailableFor = std::chrono::minutes( 5 );

	static constexpr Uint32 MaxReconnectDelaySeconds = 30;

	/** How long an agent may take to confirm a watch (its handshake included) before the watch
	 * falls back to polling: the directory listing waits for the watch. */
	static constexpr auto EstablishTimeout = std::chrono::seconds( 5 );

	RemoteWatchBackendImpl( std::shared_ptr<SFTPProviderImpl> provider, VirtualFileSystem& vfs ) :
		mProvider( provider ),
		mPolling( std::make_shared<PollingWatchBackend>( vfs ) ),
		mCommand( provider->config().watchAgentCommand ) {}

	~RemoteWatchBackendImpl() { shutdown(); }

	void shutdown() {
		std::map<Uint32, std::shared_ptr<RemoteWatch>> watches;
		std::map<std::string, AgentEntry> agents;
		{
			std::lock_guard<std::mutex> lock( mMutex );
			watches.swap( mWatches );
			agents.swap( mAgents );
			for ( auto& watch : watches )
				watch.second->mode = RemoteWatch::Mode::Closed;
		}
		for ( auto& entry : agents ) {
			if ( entry.second.agent )
				entry.second.agent->stop( "Watch backend closed" );
		}
		for ( auto& watch : watches )
			watch.second->polling.reset();
	}

	VFSTask<Uint32> add( const URI& root, const WatchOptions& options,
						 std::shared_ptr<WatchSink> sink ) {
		auto watch = std::make_shared<RemoteWatch>();
		watch->root = root;
		watch->options = options;
		watch->sink = sink;
		TaskResolver<VFSResult<void>> resolveEstablished;
		auto established = VFSTask<void>::create( resolveEstablished );
		watch->establishment = std::move( resolveEstablished );
		{
			std::lock_guard<std::mutex> lock( mMutex );
			watch->id = ++mLastId;
			mWatches[watch->id] = watch;
		}
		establish( watch );
		Uint32 id = watch->id;
		std::weak_ptr<RemoteWatchBackendImpl> weak = weak_from_this();
		return std::move( established )
			.then( [id, weak]( VFSResult<void>&& result ) -> VFSResult<Uint32> {
				if ( !result ) {
					if ( auto self = weak.lock() )
						self->remove( id );
					return Err( std::move( result ).error() );
				}
				return Ok( id );
			} );
	}

	void remove( Uint32 id ) {
		std::shared_ptr<RemoteWatch> watch;
		{
			std::lock_guard<std::mutex> lock( mMutex );
			auto it = mWatches.find( id );
			if ( it == mWatches.end() )
				return;
			watch = std::move( it->second );
			mWatches.erase( it );
			watch->mode = RemoteWatch::Mode::Closed;
		}
		if ( watch->agent && watch->agent->state() != AgentClient::State::Dead ) {
			agent::Writer w;
			w.u32( id );
			watch->agent->send( w.frame( agent::MessageUnwatch ) );
		}
		stopUnusedAgent( watch->agent );
		watch->polling.reset();
	}

	// Agent notifications (SSH I/O thread) --------------------------------------------------

	void agentReady( const std::shared_ptr<AgentClient>& client ) {
		std::vector<std::shared_ptr<RemoteWatch>> pending;
		{
			std::lock_guard<std::mutex> lock( mMutex );
			auto& entry = mAgents[client->key()];
			entry.failures = 0;
			for ( auto& watch : mWatches ) {
				if ( watch.second->agent == client &&
					 watch.second->mode != RemoteWatch::Mode::Agent )
					pending.push_back( watch.second );
			}
		}
		for ( auto& watch : pending )
			sendWatch( client, *watch );
	}

	void agentDead( const std::shared_ptr<AgentClient>& client, bool wasReady,
					const std::string& reason ) {
		std::vector<std::shared_ptr<RemoteWatch>> affected;
		{
			std::lock_guard<std::mutex> lock( mMutex );
			auto it = mAgents.find( client->key() );
			if ( it != mAgents.end() && it->second.agent == client ) {
				it->second.agent.reset();
				it->second.lastError = reason;
				if ( !wasReady && client->connection()->isUsable() ) {
					// The SSH session is fine but the agent never answered: not installed or
					// not runnable on that host.
					it->second.unavailableUntil = std::chrono::steady_clock::now() + UnavailableFor;
				}
			}
			for ( auto& watch : mWatches ) {
				if ( watch.second->agent == client ) {
					watch.second->agent.reset();
					affected.push_back( watch.second );
				}
			}
		}
		bool agentMissing = !wasReady && client->connection()->isUsable();
		for ( auto& watch : affected ) {
			if ( agentMissing ) {
				usePolling( watch );
			} else {
				{
					std::lock_guard<std::mutex> lock( mMutex );
					if ( watch->mode == RemoteWatch::Mode::Closed )
						continue;
					watch->mode = RemoteWatch::Mode::Reconnecting;
				}
				if ( auto sink = watch->sink.lock() )
					sink->setStatus( WatchStatus::Reconnecting );
				scheduleRetry( watch );
			}
		}
	}

	void watchOk( const std::shared_ptr<AgentClient>& client, Uint32 id, bool generic ) {
		std::shared_ptr<RemoteWatch> watch;
		bool wasEstablished;
		{
			std::lock_guard<std::mutex> lock( mMutex );
			auto it = mWatches.find( id );
			if ( it == mWatches.end() || it->second->agent != client )
				return;
			watch = it->second;
			wasEstablished = watch->established;
			watch->mode = RemoteWatch::Mode::Agent;
			watch->established = true;
			watch->retries = 0;
		}
		if ( auto sink = watch->sink.lock() ) {
			sink->setStatus( generic ? WatchStatus::Polling : WatchStatus::Active );
			// Continuity was lost while re-establishing.
			if ( wasEstablished )
				sink->rescanRequired();
		}
		completeEstablishment( *watch, Ok() );
	}

	void watchError( const std::shared_ptr<AgentClient>& client, Uint32 id, Int32 code,
					 const std::string& message ) {
		std::shared_ptr<RemoteWatch> watch;
		bool established;
		{
			std::lock_guard<std::mutex> lock( mMutex );
			auto it = mWatches.find( id );
			if ( it == mWatches.end() || it->second->agent != client )
				return;
			watch = it->second;
			established = watch->established;
			if ( established )
				watch->mode = RemoteWatch::Mode::Closed;
		}
		VFSErrorCode errorCode = VFSErrorCode::IO;
		if ( code == -1 ) // efsw::Errors::FileNotFound
			errorCode = VFSErrorCode::NotFound;
		else if ( code == -4 ) // efsw::Errors::FileNotReadable
			errorCode = VFSErrorCode::PermissionDenied;
		VFSError error( errorCode, watch->root, "efsw-agent: " + message, code );
		if ( !established ) {
			completeEstablishment( *watch, Err( std::move( error ) ) );
			return;
		}
		// The directory vanished while re-registering: tell consumers to re-list.
		if ( auto sink = watch->sink.lock() ) {
			sink->setStatus( WatchStatus::Failed );
			sink->rescanRequired();
		}
	}

	void events( Uint32 id, std::vector<WatchEvent>&& events ) {
		std::shared_ptr<WatchSink> sink;
		{
			std::lock_guard<std::mutex> lock( mMutex );
			auto it = mWatches.find( id );
			if ( it == mWatches.end() || it->second->mode != RemoteWatch::Mode::Agent )
				return;
			sink = it->second->sink.lock();
		}
		if ( sink )
			sink->push( std::move( events ) );
	}

	void missed( Uint32 id ) {
		std::shared_ptr<WatchSink> sink;
		{
			std::lock_guard<std::mutex> lock( mMutex );
			auto it = mWatches.find( id );
			if ( it == mWatches.end() )
				return;
			sink = it->second->sink.lock();
		}
		if ( sink )
			sink->rescanRequired();
	}

  private:
	struct AgentEntry {
		std::shared_ptr<AgentClient> agent;
		std::chrono::steady_clock::time_point unavailableUntil;
		std::string lastError;
		Uint32 failures{ 0 };
	};

	void stopUnusedAgent( const std::shared_ptr<AgentClient>& client ) {
		if ( !client )
			return;
		{
			std::lock_guard<std::mutex> lock( mMutex );
			for ( const auto& watch : mWatches ) {
				if ( watch.second->agent == client )
					return;
			}
			auto it = mAgents.find( client->key() );
			if ( it != mAgents.end() && it->second.agent == client )
				it->second.agent.reset();
		}
		// An unused pump otherwise counts as an active user forever, preventing idle cleanup.
		client->stop( "No watches remain" );
	}

	void completeEstablishment( RemoteWatch& watch, VFSResult<void>&& result ) {
		std::optional<TaskResolver<VFSResult<void>>> resolve;
		{
			std::lock_guard<std::mutex> lock( mMutex );
			resolve = std::move( watch.establishment );
			watch.establishment.reset();
		}
		if ( resolve )
			( *resolve )( std::move( result ) );
	}

	void sendWatch( const std::shared_ptr<AgentClient>& client, const RemoteWatch& watch ) {
		Uint8 flags = 0;
		if ( watch.options.recursive )
			flags |= agent::WatchRecursive;
		if ( watch.options.followSymlinks )
			flags |= agent::WatchFollowSymlinks;
		if ( watch.options.allowOutOfScopeLinks )
			flags |= agent::WatchAllowOutOfScopeLinks;
		if ( watch.options.reportCrossDirectoryMoves )
			flags |= agent::WatchReportCrossDirectoryMoves;
		agent::Writer w;
		std::string path = watch.root.getPath().empty() ? std::string( "/" ) : watch.root.getPath();
		w.u32( watch.id ).str( path ).u8( flags );
		w.u32( static_cast<Uint32>( watch.options.pollingInterval.asMilliseconds() ) );
		client->send( w.frame( agent::MessageWatch ) );
	}

	void usePolling( const std::shared_ptr<RemoteWatch>& watch ) {
		auto sink = watch->sink.lock();
		if ( !sink || !watch->options.allowPolling ) {
			if ( !watch->established ) {
				completeEstablishment(
					*watch,
					Err( VFSError( VFSErrorCode::Unsupported, watch->root,
								   "efsw-agent is not available and polling is disabled" ) ) );
			}
			return;
		}
		std::shared_ptr<AgentClient> previousAgent;
		{
			std::lock_guard<std::mutex> lock( mMutex );
			if ( watch->mode == RemoteWatch::Mode::Closed )
				return;
			watch->mode = RemoteWatch::Mode::Polling;
			previousAgent = std::move( watch->agent );
		}
		stopUnusedAgent( previousAgent );
		std::weak_ptr<RemoteWatchBackendImpl> weak = weak_from_this();
		std::weak_ptr<RemoteWatch> weakWatch = watch;
		mPolling->watch( watch->root, watch->options, sink )
			.then( [weak, weakWatch]( VFSResult<std::unique_ptr<WatchRegistration>>&& result ) {
				auto self = weak.lock();
				auto watch = weakWatch.lock();
				if ( !self || !watch )
					return;
				bool wasEstablished;
				{
					std::lock_guard<std::mutex> lock( self->mMutex );
					wasEstablished = watch->established;
					if ( result && watch->mode == RemoteWatch::Mode::Polling ) {
						watch->polling = std::move( result ).value();
						watch->established = true;
					}
				}
				if ( !result ) {
					if ( !wasEstablished )
						self->completeEstablishment( *watch, Err( std::move( result ).error() ) );
					return;
				}
				if ( wasEstablished ) {
					if ( auto sink = watch->sink.lock() )
						sink->rescanRequired();
				}
				self->completeEstablishment( *watch, Ok() );
			} );
	}

	void scheduleRetry( const std::shared_ptr<RemoteWatch>& watch ) {
		Uint32 retries;
		{
			std::lock_guard<std::mutex> lock( mMutex );
			retries = watch->retries++;
		}
		Uint32 seconds =
			std::min<Uint32>( MaxReconnectDelaySeconds, 1u << std::min<Uint32>( retries, 5 ) );
		std::weak_ptr<RemoteWatchBackendImpl> weak = weak_from_this();
		std::weak_ptr<RemoteWatch> weakWatch = watch;
		mRunner.runAfter( std::chrono::seconds( seconds ), [weak, weakWatch] {
			auto self = weak.lock();
			auto watch = weakWatch.lock();
			if ( !self || !watch )
				return;
			{
				std::lock_guard<std::mutex> lock( self->mMutex );
				if ( watch->mode != RemoteWatch::Mode::Reconnecting )
					return;
			}
			self->establish( watch );
		} );
	}

	/** Gets (or re-creates) the pooled connection and its agent, then registers the watch. */
	void establish( const std::shared_ptr<RemoteWatch>& watch ) {
		auto provider = mProvider.lock();
		if ( !provider ) {
			completeEstablishment( *watch,
								   Err( VFSError( VFSErrorCode::ConnectionLost, watch->root ) ) );
			return;
		}
		if ( mCommand.empty() ) {
			usePolling( watch );
			return;
		}
		std::weak_ptr<RemoteWatchBackendImpl> weak = weak_from_this();
		std::weak_ptr<RemoteWatch> weakWatch = watch;
		provider->connection( watch->root )
			.then( [weak, weakWatch]( VFSResult<std::shared_ptr<SSHConnection>>&& connection ) {
				auto self = weak.lock();
				auto watch = weakWatch.lock();
				if ( !self || !watch )
					return;
				bool established;
				{
					std::lock_guard<std::mutex> lock( self->mMutex );
					if ( watch->mode == RemoteWatch::Mode::Closed )
						return;
					established = watch->established;
				}
				if ( !connection ) {
					if ( !established ) {
						self->completeEstablishment( *watch,
													 Err( std::move( connection ).error() ) );
					} else {
						if ( auto sink = watch->sink.lock() )
							sink->setStatus( WatchStatus::Reconnecting );
						self->scheduleRetry( watch );
					}
					return;
				}
				self->attach( watch, std::move( connection ).value() );
			} );
	}

	void attach( const std::shared_ptr<RemoteWatch>& watch,
				 const std::shared_ptr<SSHConnection>& connection ) {
		std::string key = connection->config().connectionKey();
		std::shared_ptr<AgentClient> client;
		bool start = false;
		bool ready = false;
		bool unavailable = false;
		{
			std::lock_guard<std::mutex> lock( mMutex );
			if ( watch->mode == RemoteWatch::Mode::Closed )
				return;
			auto& entry = mAgents[key];
			if ( entry.unavailableUntil > std::chrono::steady_clock::now() ) {
				unavailable = true;
			} else {
				if ( !entry.agent || entry.agent->state() == AgentClient::State::Dead ||
					 entry.agent->connection() != connection ) {
					entry.agent =
						std::make_shared<AgentClient>( weak_from_this(), connection, key );
					start = true;
				}
				client = entry.agent;
				ready = client->state() == AgentClient::State::Ready;
				watch->agent = client;
			}
		}
		if ( unavailable ) {
			usePolling( watch );
			return;
		}
		if ( start )
			client->start( mCommand );
		else if ( ready )
			sendWatch( client, *watch );
		// Otherwise agentReady() sends it once the handshake completes.
		scheduleEstablishTimeout( watch, client );
	}

	/** An agent that never answers (stalled, or a command that is not the agent) must not keep
	 * the watch, and the listing waiting on it, pending forever. */
	void scheduleEstablishTimeout( const std::shared_ptr<RemoteWatch>& watch,
								   const std::shared_ptr<AgentClient>& client ) {
		std::weak_ptr<RemoteWatchBackendImpl> weak = weak_from_this();
		std::weak_ptr<RemoteWatch> weakWatch = watch;
		std::weak_ptr<AgentClient> weakClient = client;
		mRunner.runAfter( EstablishTimeout, [weak, weakWatch, weakClient] {
			auto self = weak.lock();
			auto watch = weakWatch.lock();
			auto client = weakClient.lock();
			if ( !self || !watch || !client )
				return;
			{
				std::lock_guard<std::mutex> lock( self->mMutex );
				if ( watch->mode == RemoteWatch::Mode::Closed ||
					 watch->mode == RemoteWatch::Mode::Agent || watch->agent != client )
					return;
			}
			if ( client->state() != AgentClient::State::Ready ) {
				// No handshake: the agent is marked unavailable and its watches poll.
				client->stop( "efsw-agent did not answer" );
				return;
			}
			self->usePolling( watch );
		} );
	}

	std::weak_ptr<SFTPProviderImpl> mProvider;
	std::shared_ptr<PollingWatchBackend> mPolling;
	std::string mCommand;
	std::mutex mMutex;
	std::map<Uint32, std::shared_ptr<RemoteWatch>> mWatches;
	std::map<std::string, AgentEntry> mAgents;
	Uint32 mLastId{ 0 };
	DelayedRunner mRunner;
};

// ---------------------------------------------------------------------------------------------
// Agent client
// ---------------------------------------------------------------------------------------------

void AgentClient::start( const std::string& command ) {
	auto self = shared_from_this();
	mConnection
		->submit<void>(
			{}, URI(),
			[self, command]( SSHConnection& c ) -> std::optional<VFSResult<void>> {
				if ( !self->mChannel ) {
					self->mChannel = libssh2_channel_open_session( c.session() );
					if ( !self->mChannel ) {
						if ( libssh2_session_last_errno( c.session() ) == LIBSSH2_ERROR_EAGAIN )
							return std::nullopt;
						return VFSResult<void>(
							Err( c.lastError( URI(), "efsw-agent: could not open a channel" ) ) );
					}
				}
				int rc = libssh2_channel_exec( self->mChannel, command.c_str() );
				if ( rc == LIBSSH2_ERROR_EAGAIN )
					return std::nullopt;
				if ( rc != 0 ) {
					return VFSResult<void>(
						Err( c.lastError( URI(), "efsw-agent: could not run the agent" ) ) );
				}
				// A stop may have arrived while open/exec was waiting on EAGAIN. Complete that
				// operation before releasing its channel, but do not start a stopped agent.
				if ( self->mState.load() != State::Dead ) {
					agent::Writer hello;
					hello.str( "eepp-vfs" );
					auto frame = hello.frame( agent::MessageHello );
					self->mOut.insert( self->mOut.end(), frame.begin(), frame.end() );
				}
				self->mPumping = true;
				c.addPump( self );
				return VFSResult<void>( Ok() );
			} )
		.then( [self]( VFSResult<void>&& result ) {
			if ( !result )
				self->die( result.error().message );
			// Exec failure can leave an open channel before the ordinary pump was installed.
			if ( self->mChannel && !self->mPumping && self->mConnection->isReady() ) {
				self->mPumping = true;
				self->mConnection->addPump( self );
			}
		} );
}

void AgentClient::die( const std::string& reason ) {
	State previous = mState.exchange( State::Dead );
	if ( previous == State::Dead )
		return;
	// Keep the channel until the next pump pass. Releasing it can return EAGAIN, and must
	// never run inside an agent callback or abandon resources on a still-healthy SFTP session.
	mConnection->ioContext()->wake();
	if ( auto backend = mBackend.lock() ) {
		std::string message = reason;
		if ( !mStderr.empty() )
			message += " (" + mStderr + ")";
		backend->agentDead( shared_from_this(), previous == State::Ready, message );
	}
}

bool AgentClient::pump( SSHConnection& c ) {
	if ( !mChannel )
		return false;
	if ( mState.load() == State::Dead ) {
		// Channel-local nonblocking cleanup: do not hold the SFTP operation queue while a
		// peer acknowledges close. Other channels and sessions continue on the same I/O loop.
		if ( libssh2_channel_free( mChannel ) == LIBSSH2_ERROR_EAGAIN )
			return true;
		mChannel = nullptr;
		return false;
	}
	while ( !mOut.empty() ) {
		ssize_t rc = libssh2_channel_write( mChannel, reinterpret_cast<const char*>( mOut.data() ),
											mOut.size() );
		if ( rc == LIBSSH2_ERROR_EAGAIN )
			break;
		if ( rc < 0 ) {
			die( c.lastError( URI(), "efsw-agent write failed" ).message );
			return true;
		}
		mOut.erase( mOut.begin(), mOut.begin() + rc );
	}
	char buffer[16384];
	while ( true ) {
		ssize_t rc = libssh2_channel_read( mChannel, buffer, sizeof( buffer ) );
		if ( rc == LIBSSH2_ERROR_EAGAIN )
			break;
		if ( rc < 0 ) {
			die( c.lastError( URI(), "efsw-agent read failed" ).message );
			return true;
		}
		if ( rc == 0 )
			break;
		mParser.feed( buffer, static_cast<size_t>( rc ) );
		agent::Frame frame;
		std::string error;
		int result;
		while ( ( result = mParser.next( frame, error ) ) == 1 ) {
			handle( frame );
			if ( mState.load() == State::Dead )
				return true;
		}
		if ( result < 0 ) {
			die( "efsw-agent protocol error: " + error );
			return true;
		}
	}
	// Drain stderr so the channel window never fills; keep a short tail for diagnostics.
	while ( true ) {
		ssize_t rc = libssh2_channel_read_stderr( mChannel, buffer, sizeof( buffer ) );
		if ( rc <= 0 )
			break;
		mStderr.append( buffer, static_cast<size_t>( rc ) );
		if ( mStderr.size() > 512 )
			mStderr.erase( 0, mStderr.size() - 512 );
	}
	if ( libssh2_channel_eof( mChannel ) ) {
		while ( !mStderr.empty() && ( mStderr.back() == '\n' || mStderr.back() == '\r' ) )
			mStderr.pop_back();
		die( "efsw-agent exited" );
		return true;
	}
	return true;
}

void AgentClient::handle( const agent::Frame& frame ) {
	auto backend = mBackend.lock();
	if ( !backend )
		return;
	if ( frame.version != agent::ProtocolVersion ) {
		die( "efsw-agent protocol version mismatch" );
		return;
	}
	agent::Reader r( frame.payload );
	switch ( frame.type ) {
		case agent::MessageHelloReply: {
			Uint16 version = r.u16();
			if ( !r.ok() || version != agent::ProtocolVersion ) {
				die( "efsw-agent protocol version mismatch" );
				return;
			}
			mHello = true;
			mState = State::Ready;
			backend->agentReady( shared_from_this() );
			break;
		}
		case agent::MessageWatchOk: {
			Uint32 id = r.u32();
			Uint8 kind = r.u8();
			if ( r.ok() )
				backend->watchOk( shared_from_this(), id, kind == agent::BackendGeneric );
			break;
		}
		case agent::MessageWatchError: {
			Uint32 id = r.u32();
			Int32 code = r.i32();
			std::string message = r.str();
			if ( r.ok() )
				backend->watchError( shared_from_this(), id, code, message );
			break;
		}
		case agent::MessageEventBatch: {
			Uint32 id = r.u32();
			Uint32 count = r.u32();
			std::vector<WatchEvent> events;
			events.reserve( std::min<Uint32>( count, 4096 ) );
			for ( Uint32 i = 0; i < count && r.ok(); ++i ) {
				Uint8 action = r.u8();
				WatchEvent event;
				event.relativePath = r.str();
				event.oldRelativePath = r.str();
				switch ( action ) {
					case agent::ActionAdd:
						event.type = WatchEventType::Added;
						break;
					case agent::ActionDelete:
						event.type = WatchEventType::Removed;
						break;
					case agent::ActionMoved:
						event.type = WatchEventType::Moved;
						break;
					default:
						event.type = WatchEventType::Modified;
						break;
				}
				if ( event.relativePath.empty() && event.type != WatchEventType::Modified )
					event = WatchEvent{ {}, {}, WatchEventType::RescanRequired };
				events.push_back( std::move( event ) );
			}
			if ( !r.ok() ) {
				die( "efsw-agent sent a malformed event batch" );
				return;
			}
			backend->events( id, std::move( events ) );
			break;
		}
		case agent::MessageMissedEvents: {
			Uint32 id = r.u32();
			if ( r.ok() )
				backend->missed( id );
			break;
		}
		case agent::MessageUnwatchOk:
		case agent::MessagePong:
			break;
		case agent::MessageError:
			die( "efsw-agent: " + r.str() );
			break;
		default:
			die( "efsw-agent sent an unknown message" );
			break;
	}
}

class RemoteWatchRegistration : public WatchRegistration {
  public:
	RemoteWatchRegistration( std::weak_ptr<RemoteWatchBackendImpl> impl, Uint32 id ) :
		mImpl( std::move( impl ) ), mId( id ) {}

	~RemoteWatchRegistration() override {
		if ( auto impl = mImpl.lock() )
			impl->remove( mId );
	}

  private:
	std::weak_ptr<RemoteWatchBackendImpl> mImpl;
	Uint32 mId;
};

} // namespace sftp

RemoteWatchBackend::RemoteWatchBackend( std::shared_ptr<sftp::SFTPProviderImpl> provider,
										VirtualFileSystem& vfs ) :
	mImpl( std::make_shared<sftp::RemoteWatchBackendImpl>( std::move( provider ), vfs ) ) {}

RemoteWatchBackend::~RemoteWatchBackend() {
	mImpl->shutdown();
}

VFSTask<std::unique_ptr<WatchRegistration>>
RemoteWatchBackend::watch( const URI& root, const WatchOptions& options,
						   std::shared_ptr<WatchSink> sink ) {
	std::weak_ptr<sftp::RemoteWatchBackendImpl> weak = mImpl;
	return mImpl->add( root, options, std::move( sink ) )
		.then( [weak]( VFSResult<Uint32>&& id ) -> VFSResult<std::unique_ptr<WatchRegistration>> {
			if ( !id )
				return Err( std::move( id ).error() );
			return Ok( std::unique_ptr<WatchRegistration>(
				std::make_unique<sftp::RemoteWatchRegistration>( weak, id.value() ) ) );
		} );
}

}} // namespace EE::VFS
