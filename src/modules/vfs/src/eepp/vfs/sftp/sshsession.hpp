#ifndef EE_VFS_SFTP_SSHSESSION_HPP
#define EE_VFS_SFTP_SSHSESSION_HPP

#include "sshagent.hpp"
#include "sshsocket.hpp"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <deque>
#include <eepp/core/cancellationtoken.hpp>
#include <eepp/core/small_unique_function.hpp>
#include <eepp/vfs/error.hpp>
#include <eepp/vfs/sftp/sftpprovider.hpp>
#include <eepp/vfs/ssh/sshconfig.hpp>
#include <functional>
#include <libssh2.h>
#include <libssh2_sftp.h>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

namespace EE { namespace System {
class Process;
}} // namespace EE::System

namespace EE { namespace VFS { namespace sftp {

class SSHConnection;

enum class Step : Uint8 {
	Done,
	/** Waiting for socket readiness (libssh2 returned EAGAIN) or an external event. */
	Again,
};

/** Unit of work queued on a connection and executed on the I/O thread.
 *
 * Operations of one connection run strictly one at a time, in submission order: libssh2 keeps
 * per-session and per-SFTP-session request state, so interleaving non-blocking calls of
 * different operations is not safe. Cancellation is honored before an operation starts and at
 * the safe points an operation checks itself; a libssh2 call is never abandoned midway.
 */
class SSHOperation {
  public:
	virtual ~SSHOperation() = default;

	virtual Step step( SSHConnection& connection ) = 0;

	virtual void fail( const VFSError& error ) = 0;
};

template <typename T> class SSHTaskOperation : public SSHOperation {
  public:
	using Function = std::function<std::optional<VFSResult<T>>( SSHConnection& )>;

	SSHTaskOperation( CancellationToken token, URI uri, Function function ) :
		mToken( std::move( token ) ),
		mUri( std::move( uri ) ),
		mFunction( std::move( function ) ) {}

	VFSTask<T> task() { return mSource.task(); }

	Step step( SSHConnection& connection ) override {
		if ( !mStarted && mToken.isCancellationRequested() ) {
			mSource.resolve( Err( cancelledError( mUri ) ) );
			return Step::Done;
		}
		mStarted = true;
		auto result = mFunction( connection );
		if ( !result )
			return Step::Again;
		mSource.resolve( std::move( *result ) );
		return Step::Done;
	}

	void fail( const VFSError& error ) override {
		VFSError copy( error );
		if ( copy.uri.empty() )
			copy.uri = mUri;
		mSource.resolve( Err( std::move( copy ) ) );
	}

  private:
	TaskSource<VFSResult<T>> mSource;
	CancellationToken mToken;
	URI mUri;
	Function mFunction;
	bool mStarted{ false };
};

/** Long-lived channel consumer (for example the remote watch agent) serviced on every pass of
 * the I/O loop, independently of the serialized operation queue. Pumps must only touch their own
 * channel, so they never interleave with the session-level state of queued operations. */
class SSHChannelPump {
  public:
	virtual ~SSHChannelPump() = default;

	/** Non-blocking read/write of the pump's channel. @return False to unregister. */
	virtual bool pump( SSHConnection& connection ) = 0;

	/** The connection failed or closed; the pump is unregistered afterwards. */
	virtual void connectionLost( const VFSError& error ) = 0;
};

/** @brief Single I/O thread that owns and drives SSH sessions.
 *
 * Every libssh2 call of a registered connection happens on this thread. Other threads interact
 * only through post(), which queues a command and wakes the poll loop.
 */
class SSHIOContext : public std::enable_shared_from_this<SSHIOContext> {
  public:
	using Command = SmallUniqueFunction<void(), 64>;

	SSHIOContext();

	~SSHIOContext();

	void start();

	void stop();

	void post( Command command );

	void wake() { mWake.signal(); }

	bool isIOThread() const { return std::this_thread::get_id() == mThreadId; }

	/** Registers a connection (I/O thread only). */
	void addConnection( std::shared_ptr<SSHConnection> connection );

	/** Fails registered sessions, including sessions still awaiting authentication. */
	void closeConnections();

  private:
	void loop();

	WakeSignal mWake;
	std::thread mThread;
	std::thread::id mThreadId;
	std::mutex mMutex;
	std::vector<Command> mCommands;
	std::vector<std::shared_ptr<SSHConnection>> mConnections;
	std::atomic<bool> mStopping{ false };
};

/** Callbacks used during connection setup. */
struct SSHConnectionHooks {
	std::shared_ptr<SSHHostKeyVerifier> hostKeyVerifier;
	std::shared_ptr<SSHAuthenticationHandler> authenticationHandler;
};

/** Byte stream under an SSH session: a TCP socket, a direct-tcpip channel of a parent session
 * (ProxyJump) or a ProxyCommand process. */
class SSHTransport {
  public:
	virtual ~SSHTransport() = default;

	/** Socket to poll, or InvalidSocket when readiness is signaled through the wake pipe. */
	virtual SocketHandle pollSocket() const = 0;

	virtual short pollEvents( short sessionDirections ) const = 0;

	/** libssh2 socket argument for the handshake. */
	virtual libssh2_socket_t handshakeSocket() const = 0;

	/** Installs send/recv callbacks for non-socket transports. */
	virtual void install( LIBSSH2_SESSION* session ) { (void)session; }

	/** Called on the I/O thread before stepping; @return False when the transport died. */
	virtual bool alive() const { return true; }
};

/** @brief One SSH session with its SFTP subsystem.
 *
 * Owns the libssh2 session, its transport, the SFTP channel and the per-session queue of
 * operations. All members except the thread-safe submission API belong to the I/O thread.
 */
class SSHConnection : public std::enable_shared_from_this<SSHConnection> {
  public:
	enum class State : Uint8 {
		Starting,
		Handshake,
		HostKey,
		WaitingHostKeyDecision,
		Authenticating,
		SFTPInit,
		Ready,
		Failed,
		Closed,
	};

	SSHConnection( std::shared_ptr<SSHIOContext> context, SSHResolvedConfig config,
				   SSHConnectionHooks hooks, std::unique_ptr<SSHTransport> transport,
				   Time connectTimeout );

	~SSHConnection();

	// Thread-safe API ----------------------------------------------------------------------

	/** Queues an operation. Its function runs on the I/O thread once the connection is ready and
	 * returns nullopt while it needs to wait (EAGAIN). */
	template <typename T>
	VFSTask<T> submit( CancellationToken token, URI uri,
					   typename SSHTaskOperation<T>::Function function ) {
		auto operation = std::make_unique<SSHTaskOperation<T>>(
			std::move( token ), std::move( uri ), std::move( function ) );
		auto task = operation->task();
		enqueue( std::move( operation ) );
		return task;
	}

	/** @return A task resolved when the connection is ready or failed. */
	VFSTask<void> whenReady();

	/** Closes the connection; queued and future operations fail with ConnectionLost. */
	void close();

	bool isUsable() const {
		State state = mState.load( std::memory_order_acquire );
		return state != State::Failed && state != State::Closed;
	}

	bool isReady() const { return mState.load( std::memory_order_acquire ) == State::Ready; }

	const SSHResolvedConfig& config() const { return mConfig; }

	const std::shared_ptr<SSHIOContext>& ioContext() const { return mContext; }

	/** Last activity time (any thread), for idle pooling. */
	std::chrono::steady_clock::time_point lastUse() const;

	// I/O thread API -------------------------------------------------------------------------

	void drive();

	/** Adds pollfd entries for this connection. */
	void collectPoll( std::vector<PollFd>& fds ) const;

	/** @return Milliseconds until a timer of this connection fires, -1 if none. */
	int nextTimeoutMs() const;

	std::chrono::milliseconds idleTimeout() const;

	/** @return True when the connection stayed idle longer than its idle timeout. */
	bool idleExpired() const;

	LIBSSH2_SESSION* session() const { return mSession; }

	LIBSSH2_SFTP* sftp() const { return mSFTP; }

	/** Maps the last libssh2 / SFTP error to a VFSError, marking the connection failed when the
	 * transport is gone. */
	VFSError lastError( const URI& uri, const char* what );

	/** @return True if the last error code means "would block". */
	bool wouldBlock( long rc ) const { return rc == LIBSSH2_ERROR_EAGAIN; }

	/** Cached SFTP extension support: -1 unknown, 0 no, 1 yes. */
	int posixRenameSupport{ -1 };
	int fsyncSupport{ -1 };
	int statvfsSupport{ -1 };

	/** Tracks an open SFTP handle (I/O thread only). Handles still open when the connection is
	 * destroyed (for example because their owners died after the connection failed) are closed
	 * then, so libssh2 releases them. */
	void trackHandle( LIBSSH2_SFTP_HANDLE* handle ) {
		mHandles.push_back( handle );
		addUser();
	}

	void untrackHandle( LIBSSH2_SFTP_HANDLE* handle ) {
		auto it = std::find( mHandles.begin(), mHandles.end(), handle );
		if ( it != mHandles.end() ) {
			mHandles.erase( it );
			removeUser();
		}
	}

	/** Registers a channel pump (I/O thread only, connection must be ready). */
	void addPump( std::shared_ptr<SSHChannelPump> pump ) {
		mPumps.push_back( std::move( pump ) );
		addUser();
	}

	/** Something keeps using the connection without queued operations: an open handle, a pump,
	 * a jump child's channel. A connection in use is never closed as idle. Any thread. */
	void addUser() { mUsers.fetch_add( 1, std::memory_order_relaxed ); }

	void removeUser() { mUsers.fetch_sub( 1, std::memory_order_relaxed ); }

	/** @return True when nothing uses the connection (no users, no queued operations). */
	bool isIdle() const {
		return mUsers.load( std::memory_order_relaxed ) == 0 && !hasQueuedOperations();
	}

	/** Closes the connection once it stayed idle this long (zero: never). Set it before the
	 * connection is added to the I/O context. */
	void setIdleTimeout( Time timeout ) { mIdleTimeout = timeout; }

	/** Whether Ready includes a started SFTP subsystem (true by default). Jump hosts only open
	 * channels for the next hop and need no SFTP. Set it before the connection is added to the
	 * I/O context. */
	void setSFTPRequired( bool required ) { mSFTPRequired = required; }

	/** Opens a direct-tcpip channel for a child session (ProxyJump). I/O thread only. */
	LIBSSH2_CHANNEL* openDirectTcpip( const std::string& host, Uint16 port, bool& again );

	/** Root transport socket of this connection (used by jump children for polling). */
	SocketHandle pollSocket() const {
		return mTransport ? mTransport->pollSocket() : InvalidSocket;
	}

	short pollEvents() const;

	State state() const { return mState.load( std::memory_order_acquire ); }

	/** Keyboard-interactive callback bridge (libssh2 C callback). */
	static void kbdInteractiveCallback( const char* name, int nameLength, const char* instruction,
										int instructionLength, int promptCount,
										const LIBSSH2_USERAUTH_KBDINT_PROMPT* prompts,
										LIBSSH2_USERAUTH_KBDINT_RESPONSE* responses,
										void** abstract );

  private:
	friend class SSHIOContext;
	friend class ChannelTransport;
	friend class ProcessTransport;

	/** Queues operation, or fails it right away when the connection is already terminal. */
	void enqueue( std::unique_ptr<SSHOperation> operation );

	bool hasQueuedOperations() const;

	void fail( VFSError error );

	Step establish();

	Step stepHandshake();

	Step stepHostKey();

	Step stepAuthentication();

	bool saveHostKey( const char* key, size_t keyLength, int keyBits, std::string& error );

	void requestExternal( const SSHAuthenticationRequest& request );

	/** Lists the agent's keys on a worker thread (AgentListing until it answers). */
	void startAgentListing();

	/** Asks the agent for the signature of payload on a worker thread. */
	void startAgentSignature( std::string_view payload );

	/** libssh2 sign callback of agent keys: EAGAIN until the agent thread answered. */
	static int agentSign( LIBSSH2_SESSION* session, unsigned char** signature,
						  size_t* signatureLength, const unsigned char* data, size_t dataLength,
						  void** abstract );

	/** Applies the configured algorithm restrictions; fails when one permits nothing available. */
	VFSResult<void> applyAlgorithmPreferences();

	/** Host key algorithms for which known_hosts holds a key of this host. */
	std::vector<std::string_view> knownHostKeyAlgorithms();

	void markUsed();

	std::shared_ptr<SSHIOContext> mContext;
	SSHResolvedConfig mConfig;
	SSHConnectionHooks mHooks;
	std::unique_ptr<SSHTransport> mTransport;
	LIBSSH2_SESSION* mSession{ nullptr };
	LIBSSH2_SFTP* mSFTP{ nullptr };
	LIBSSH2_AGENT* mAgent{ nullptr };

	mutable std::mutex mQueueMutex;
	std::deque<std::unique_ptr<SSHOperation>> mQueue;
	/** Set (under mQueueMutex) once the connection failed or closed: new operations fail with it
	 * instead of being queued. */
	std::optional<VFSError> mQueueFailure;
	std::vector<std::shared_ptr<SSHChannelPump>> mPumps;
	std::vector<LIBSSH2_SFTP_HANDLE*> mHandles;
	std::vector<TaskSource<VFSResult<void>>> mReadyWaiters;
	std::chrono::steady_clock::time_point mLastUse;
	std::chrono::steady_clock::time_point mDeadline;
	std::chrono::steady_clock::time_point mNextKeepalive;
	Time mConnectTimeout;
	VFSError mFailure;

	// Authentication state (I/O thread).
	enum class AuthPhase : Uint8 {
		List,
		NextMethod,
		AgentConnect,
		/** libssh2's agent (where SSHAgent is not supported). */
		AgentNext,
		AgentAuth,
		/** SSHAgent on a worker thread: listing, then each key. */
		AgentListing,
		AgentKeyNext,
		AgentKeyAuth,
		KeyNext,
		KeyAuth,
		KbdAuth,
		PasswordAuth,
		WaitingInput,
		Done,
	};
	AuthPhase mAuthPhase{ AuthPhase::List };
	AuthPhase mResumePhase{ AuthPhase::NextMethod };
	std::vector<std::string> mMethods;
	size_t mMethodIndex{ 0 };
	std::string mServerMethods;
	struct libssh2_agent_publickey* mAgentIdentity{ nullptr };
	std::vector<std::string> mAllowedAgentBlobs;
	/** Agent calls block (signing may wait for a hardware key touch): they run on a worker
	 * thread while the I/O thread keeps serving the other sessions. */
	std::shared_ptr<SSHAgent> mAgentClient;
	std::vector<SSHAgent::Identity> mAgentKeys;
	size_t mAgentKeyIndex{ 0 };
	/** The signature libssh2 asked for (its payload), and the agent's answer once it arrived. */
	std::string mAgentSignPayload;
	std::optional<VFSResult<std::string>> mAgentSignature;
	bool mAgentSigning{ false };
	size_t mKeyIndex{ 0 };
	std::string mPassphrase;
	Uint32 mAttempts{ 0 };
	std::vector<std::string> mKbdAnswers;
	std::vector<SSHPrompt> mKbdPrompts;
	std::string mKbdName;
	std::string mKbdInstruction;
	bool mKbdHadAnswers{ false };
	std::string mPassword;
	std::mutex mExternalMutex;
	std::optional<std::optional<std::vector<std::string>>> mExternalAnswer;
	std::optional<SSHHostKeyDecision> mHostKeyDecision;
	std::vector<char> mPendingHostKey;
	int mPendingHostKeyBits{ 0 };

	std::atomic<State> mState{ State::Starting };
	std::atomic<Uint32> mUsers{ 0 };
	Time mIdleTimeout;
	bool mSFTPRequired{ true };
	bool mWaitingExternal{ false };
};

/** TCP transport. */
class SocketTransport : public SSHTransport {
  public:
	explicit SocketTransport( SocketHandle socket ) : mSocket( socket ) {}

	~SocketTransport() override { closeSocket( mSocket ); }

	SocketHandle pollSocket() const override { return mSocket; }

	short pollEvents( short directions ) const override;

	libssh2_socket_t handshakeSocket() const override {
		return static_cast<libssh2_socket_t>( mSocket );
	}

  private:
	SocketHandle mSocket;
};

/** Transport tunneled through a direct-tcpip channel of a parent connection (ProxyJump). */
class ChannelTransport : public SSHTransport {
  public:
	ChannelTransport( std::shared_ptr<SSHConnection> parent, std::string host, Uint16 port );

	~ChannelTransport() override;

	SocketHandle pollSocket() const override { return mParent->pollSocket(); }

	short pollEvents( short directions ) const override;

	libssh2_socket_t handshakeSocket() const override;

	void install( LIBSSH2_SESSION* session ) override;

	bool alive() const override { return mParent->isUsable(); }

	/** Opens the channel; @return Done once open (check channel()), Again to retry. */
	Step open( VFSError& error );

	LIBSSH2_CHANNEL* channel() const { return mChannel; }

	static ssize_t sendCallback( libssh2_socket_t, const void* buffer, size_t length, int,
								 void** abstract );

	static ssize_t recvCallback( libssh2_socket_t, void* buffer, size_t length, int,
								 void** abstract );

  private:
	std::shared_ptr<SSHConnection> mParent;
	std::string mHost;
	Uint16 mPort;
	LIBSSH2_CHANNEL* mChannel{ nullptr };
};

/** Transport through a ProxyCommand child process (stdin/stdout). */
class ProcessTransport : public SSHTransport {
  public:
	ProcessTransport( std::shared_ptr<SSHIOContext> context, const std::string& command );

	~ProcessTransport() override;

	bool started() const;

	SocketHandle pollSocket() const override { return InvalidSocket; }

	short pollEvents( short ) const override { return 0; }

	libssh2_socket_t handshakeSocket() const override;

	void install( LIBSSH2_SESSION* session ) override;

	bool alive() const override;

	static ssize_t sendCallback( libssh2_socket_t, const void* buffer, size_t length, int,
								 void** abstract );

	static ssize_t recvCallback( libssh2_socket_t, void* buffer, size_t length, int,
								 void** abstract );

  private:
	std::shared_ptr<SSHIOContext> mContext;
	std::unique_ptr<System::Process> mProcess;
	/** libssh2 requires a valid socket value for the handshake even with custom send/recv
	 * callbacks; this unconnected socket only satisfies that check. */
	SocketHandle mPlaceholder{ InvalidSocket };
	mutable std::mutex mMutex;
	std::string mBuffer;
	bool mEOF{ false };
};

}}} // namespace EE::VFS::sftp

#endif
