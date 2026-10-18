#ifndef EE_VFS_SFTP_SSHCONNECTION_HPP
#define EE_VFS_SFTP_SSHCONNECTION_HPP

#include "sshagent.hpp"
#include "sshchannelpump.hpp"
#include "sshiocontext.hpp"
#include "sshoperation.hpp"
#include "sshtransport.hpp"
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <deque>
#include <eepp/vfs/sftp/sftpprovider.hpp>
#include <eepp/vfs/ssh/sshconfig.hpp>
#include <libssh2_sftp.h>
#include <memory>
#include <mutex>
#include <vector>

namespace EE { namespace VFS { namespace sftp {

/** Callbacks used during connection setup. */
struct SSHConnectionHooks {
	std::shared_ptr<SSHHostKeyVerifier> hostKeyVerifier;
	std::shared_ptr<SSHAuthenticationHandler> authenticationHandler;
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
				   Time connectTimeout, Time operationTimeout );

	~SSHConnection();

	// Thread-safe API ----------------------------------------------------------------------

	/** Queues an operation. Its function runs on the I/O thread once the connection is ready and
	 * returns nullopt while it needs to wait (EAGAIN). */
	template <typename T>
	VFSTask<T> submit( CancellationToken token, URI uri,
					   typename SSHTaskOperation<T>::Function function ) {
		TaskResolver<VFSResult<T>> resolve;
		auto task = VFSTask<T>::create( resolve );
		enqueue( std::make_unique<SSHTaskOperation<T>>( std::move( resolve ), std::move( token ),
														std::move( uri ), std::move( function ) ) );
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

	/** True only for ready sessions with different visible TCP peers and different verified
	 * host keys of the same algorithm. Unknown, proxied or coincident peers remain ambiguous.
	 * Logical server separation cannot establish that their backing storage is not shared. */
	bool hasDistinctPeer( const SSHConnection& other ) const;

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

	/** Transport under the session (its send/recv callbacks reach it through the session). */
	SSHTransport* transport() const { return mTransport.get(); }

	State state() const { return mState.load( std::memory_order_acquire ); }

	/** Keyboard-interactive callback bridge (libssh2 C callback). */
	static void kbdInteractiveCallback( const char* name, int nameLength, const char* instruction,
										int instructionLength, int promptCount,
										const LIBSSH2_USERAUTH_KBDINT_PROMPT* prompts,
										LIBSSH2_USERAUTH_KBDINT_RESPONSE* responses,
										void** abstract );

  private:
	friend class SSHIOContext;

	/** Authentication steps (I/O thread). */
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

	/** Queues operation, or fails it right away when the connection is already terminal. */
	void enqueue( std::unique_ptr<SSHOperation> operation );

	bool hasQueuedOperations() const;

	void fail( VFSError error );

	Step establish();

	Step stepHandshake();

	Step stepHostKey();

	Step stepAuthentication();

	bool saveHostKey( const char* key, size_t keyLength, int keyBits, std::string& error );

	/** Asks the authentication handler (attempt and host filled in) and waits for its answer,
	 * then continues at resume. */
	void requestExternal( SSHAuthenticationRequest request, AuthPhase resume );

	/** The request for the passphrase of an identity file. */
	SSHAuthenticationRequest passphraseRequest( const std::string& user,
												const std::string& identityFile ) const;

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
	const std::optional<SSHSocket::Endpoint> mPeerEndpoint;
	int mHostKeyType{ 0 };
	/** Written during host-key verification, immutable after Ready publishes it. */
	std::array<Uint8, 32> mHostKeyHash{};
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
	std::vector<TaskResolver<VFSResult<void>>> mReadyWaiters;
	std::chrono::steady_clock::time_point mLastUse;
	std::chrono::steady_clock::time_point mDeadline;
	std::chrono::steady_clock::time_point mNextKeepalive;
	std::optional<std::chrono::steady_clock::time_point> mOperationDeadline;
	Time mOperationTimeout;
	Time mConnectTimeout;
	VFSError mFailure;

	// Authentication state (I/O thread).
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
	bool mHasHostKeyHash{ false };
	bool mSFTPRequired{ true };
	bool mWaitingExternal{ false };
};

}}} // namespace EE::VFS::sftp

#endif
