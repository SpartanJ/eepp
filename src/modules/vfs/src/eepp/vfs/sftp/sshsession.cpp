#include "sshsession.hpp"
#include <algorithm>
#include <cerrno>
#include <cstring>
#include <eepp/core/string.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/process.hpp>
#include <eepp/system/sys.hpp>
#include <mutex>

#if EE_PLATFORM != EE_PLATFORM_WIN
#include <sys/socket.h>
#endif

namespace EE { namespace VFS { namespace sftp {

using System::FileSystem;

namespace {

void libssh2GlobalInit() {
	static std::once_flag once;
	std::call_once( once, [] {
		socketsInit();
		libssh2_init( 0 );
	} );
}

/** Overwrites a secret before releasing it. */
void scrub( std::string& secret ) {
	std::fill( secret.begin(), secret.end(), '\0' );
	secret.clear();
}

void scrub( std::vector<std::string>& secrets ) {
	for ( auto& secret : secrets )
		scrub( secret );
	secrets.clear();
}

const char* hostKeyTypeName( int type ) {
	switch ( type ) {
		case LIBSSH2_HOSTKEY_TYPE_RSA:
			return "ssh-rsa";
		case LIBSSH2_HOSTKEY_TYPE_DSS:
			return "ssh-dss";
		case LIBSSH2_HOSTKEY_TYPE_ECDSA_256:
			return "ecdsa-sha2-nistp256";
		case LIBSSH2_HOSTKEY_TYPE_ECDSA_384:
			return "ecdsa-sha2-nistp384";
		case LIBSSH2_HOSTKEY_TYPE_ECDSA_521:
			return "ecdsa-sha2-nistp521";
#ifdef LIBSSH2_HOSTKEY_TYPE_ED25519
		case LIBSSH2_HOSTKEY_TYPE_ED25519:
			return "ssh-ed25519";
#endif
		default:
			return "unknown";
	}
}

int knownHostKeyBits( int type ) {
	switch ( type ) {
		case LIBSSH2_HOSTKEY_TYPE_RSA:
			return LIBSSH2_KNOWNHOST_KEY_SSHRSA;
		case LIBSSH2_HOSTKEY_TYPE_DSS:
			return LIBSSH2_KNOWNHOST_KEY_SSHDSS;
		case LIBSSH2_HOSTKEY_TYPE_ECDSA_256:
			return LIBSSH2_KNOWNHOST_KEY_ECDSA_256;
		case LIBSSH2_HOSTKEY_TYPE_ECDSA_384:
			return LIBSSH2_KNOWNHOST_KEY_ECDSA_384;
		case LIBSSH2_HOSTKEY_TYPE_ECDSA_521:
			return LIBSSH2_KNOWNHOST_KEY_ECDSA_521;
#ifdef LIBSSH2_HOSTKEY_TYPE_ED25519
		case LIBSSH2_HOSTKEY_TYPE_ED25519:
			return LIBSSH2_KNOWNHOST_KEY_ED25519;
#endif
		default:
			return LIBSSH2_KNOWNHOST_KEY_UNKNOWN;
	}
}

/** Loads known_hosts line by line: libssh2_knownhost_readfile() stops at the first entry it
 * cannot parse (certificate authorities, security-key types...), which would hide every later
 * host. */
void loadKnownHosts( LIBSSH2_KNOWNHOSTS* hosts, const std::string& path ) {
	std::string data;
	if ( !FileSystem::fileExists( path ) || !FileSystem::fileGet( path, data ) )
		return;
	size_t start = 0;
	while ( start < data.size() ) {
		size_t end = data.find( '\n', start );
		if ( end == std::string::npos )
			end = data.size();
		std::string_view line( data.data() + start, end - start );
		start = end + 1;
		while ( !line.empty() && ( line.back() == '\r' || line.back() == ' ' ) )
			line.remove_suffix( 1 );
		size_t first = line.find_first_not_of( " \t" );
		if ( first == std::string_view::npos || line[first] == '#' || line[first] == '@' )
			continue;
		// libssh2 inspects the byte after the given length (expecting an optional newline), so
		// hand it a NUL-terminated copy rather than a view into the file buffer.
		std::string entry( line );
		libssh2_knownhost_readline( hosts, entry.c_str(), entry.size(),
									LIBSSH2_KNOWNHOST_FILE_OPENSSH );
	}
}

/** Detects passphrase-protected private keys (PEM "ENCRYPTED" or OpenSSH format with a
 * cipher other than "none"). */
bool isKeyEncrypted( const std::string& path ) {
	std::string data;
	if ( !FileSystem::fileGet( path, data ) )
		return false;
	if ( data.find( "ENCRYPTED" ) != std::string::npos )
		return true;
	static constexpr std::string_view Begin = "-----BEGIN OPENSSH PRIVATE KEY-----";
	size_t begin = data.find( Begin );
	if ( begin == std::string::npos )
		return false;
	size_t end = data.find( "-----END", begin + Begin.size() );
	std::string base64;
	for ( size_t i = begin + Begin.size(); i < end && i < data.size(); ++i )
		if ( !std::isspace( static_cast<unsigned char>( data[i] ) ) )
			base64 += data[i];
	// Decode just the header: "openssh-key-v1\0" + uint32 length + cipher name.
	static constexpr char Alphabet[] =
		"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	std::string decoded;
	Uint32 buffer = 0;
	int bits = 0;
	for ( char c : base64 ) {
		if ( c == '=' || decoded.size() > 64 )
			break;
		const char* pos = std::strchr( Alphabet, c );
		if ( !pos )
			break;
		buffer = ( buffer << 6 ) | static_cast<Uint32>( pos - Alphabet );
		bits += 6;
		if ( bits >= 8 ) {
			bits -= 8;
			decoded += static_cast<char>( ( buffer >> bits ) & 0xFF );
		}
	}
	static constexpr size_t Magic = 15; // "openssh-key-v1" + NUL
	if ( decoded.size() < Magic + 4 )
		return false;
	Uint32 length = ( Uint8( decoded[Magic] ) << 24 ) | ( Uint8( decoded[Magic + 1] ) << 16 ) |
					( Uint8( decoded[Magic + 2] ) << 8 ) | Uint8( decoded[Magic + 3] );
	if ( decoded.size() < Magic + 4 + length )
		return false;
	return decoded.compare( Magic + 4, length, "none" ) != 0;
}

/** Reads the key blob of an OpenSSH ".pub" file (second field, base64). */
std::optional<std::string> readPublicKeyBlob( const std::string& path ) {
	std::string data;
	if ( !FileSystem::fileGet( path, data ) )
		return std::nullopt;
	auto fields = String::split( data, ' ' );
	if ( fields.size() < 2 )
		return std::nullopt;
	static constexpr char Alphabet[] =
		"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	std::string decoded;
	Uint32 buffer = 0;
	int bits = 0;
	for ( char c : fields[1] ) {
		if ( c == '=' || c == '\n' || c == '\r' )
			break;
		const char* pos = std::strchr( Alphabet, c );
		if ( !pos )
			return std::nullopt;
		buffer = ( buffer << 6 ) | static_cast<Uint32>( pos - Alphabet );
		bits += 6;
		if ( bits >= 8 ) {
			bits -= 8;
			decoded += static_cast<char>( ( buffer >> bits ) & 0xFF );
		}
	}
	return decoded;
}

/** Host key algorithms of a known_hosts key type, in libssh2 method names. */
std::vector<std::string_view> hostKeyAlgorithmsForKnownType( int keyBits ) {
	switch ( keyBits ) {
		case LIBSSH2_KNOWNHOST_KEY_ED25519:
			return { "ssh-ed25519" };
		case LIBSSH2_KNOWNHOST_KEY_ECDSA_256:
			return { "ecdsa-sha2-nistp256" };
		case LIBSSH2_KNOWNHOST_KEY_ECDSA_384:
			return { "ecdsa-sha2-nistp384" };
		case LIBSSH2_KNOWNHOST_KEY_ECDSA_521:
			return { "ecdsa-sha2-nistp521" };
		case LIBSSH2_KNOWNHOST_KEY_SSHRSA:
			return { "rsa-sha2-512", "rsa-sha2-256", "ssh-rsa" };
		case LIBSSH2_KNOWNHOST_KEY_SSHDSS:
			return { "ssh-dss" };
		default:
			return {};
	}
}

bool isSocketError( int rc ) {
	return rc == LIBSSH2_ERROR_SOCKET_SEND || rc == LIBSSH2_ERROR_SOCKET_RECV ||
		   rc == LIBSSH2_ERROR_SOCKET_DISCONNECT || rc == LIBSSH2_ERROR_SOCKET_TIMEOUT ||
		   rc == LIBSSH2_ERROR_TIMEOUT || rc == LIBSSH2_ERROR_SOCKET_NONE ||
		   rc == LIBSSH2_ERROR_BANNER_RECV || rc == LIBSSH2_ERROR_BANNER_SEND ||
		   rc == LIBSSH2_ERROR_KEX_FAILURE;
}

} // namespace

std::string base64Unpadded( const unsigned char* data, size_t length ) {
	static constexpr char Alphabet[] =
		"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	std::string out;
	size_t i = 0;
	for ( ; i + 2 < length; i += 3 ) {
		Uint32 v = ( data[i] << 16 ) | ( data[i + 1] << 8 ) | data[i + 2];
		out += Alphabet[( v >> 18 ) & 63];
		out += Alphabet[( v >> 12 ) & 63];
		out += Alphabet[( v >> 6 ) & 63];
		out += Alphabet[v & 63];
	}
	if ( i < length ) {
		Uint32 v = data[i] << 16;
		if ( i + 1 < length )
			v |= data[i + 1] << 8;
		out += Alphabet[( v >> 18 ) & 63];
		out += Alphabet[( v >> 12 ) & 63];
		if ( i + 1 < length )
			out += Alphabet[( v >> 6 ) & 63];
	}
	return out;
}

VFSErrorCode mapSFTPStatus( unsigned long status ) {
	switch ( status ) {
		case LIBSSH2_FX_OK:
			return VFSErrorCode::None;
		case LIBSSH2_FX_EOF:
			return VFSErrorCode::IO;
		case LIBSSH2_FX_NO_SUCH_FILE:
		case LIBSSH2_FX_NO_SUCH_PATH:
			return VFSErrorCode::NotFound;
		case LIBSSH2_FX_PERMISSION_DENIED:
			return VFSErrorCode::PermissionDenied;
		case LIBSSH2_FX_FILE_ALREADY_EXISTS:
			return VFSErrorCode::AlreadyExists;
		case LIBSSH2_FX_WRITE_PROTECT:
			return VFSErrorCode::ReadOnly;
		case LIBSSH2_FX_NO_SPACE_ON_FILESYSTEM:
		case LIBSSH2_FX_QUOTA_EXCEEDED:
			return VFSErrorCode::NoSpace;
		case LIBSSH2_FX_DIR_NOT_EMPTY:
			return VFSErrorCode::NotEmpty;
		case LIBSSH2_FX_NOT_A_DIRECTORY:
			return VFSErrorCode::NotDirectory;
		case LIBSSH2_FX_INVALID_FILENAME:
			return VFSErrorCode::InvalidURI;
		case LIBSSH2_FX_LINK_LOOP:
			return VFSErrorCode::TooManySymlinks;
		case LIBSSH2_FX_OP_UNSUPPORTED:
			return VFSErrorCode::Unsupported;
		case LIBSSH2_FX_NO_CONNECTION:
		case LIBSSH2_FX_CONNECTION_LOST:
			return VFSErrorCode::ConnectionLost;
		case LIBSSH2_FX_BAD_MESSAGE:
			return VFSErrorCode::ProtocolError;
		default:
			return VFSErrorCode::IO;
	}
}

// ---------------------------------------------------------------------------------------------
// I/O context
// ---------------------------------------------------------------------------------------------

SSHIOContext::SSHIOContext() {
	libssh2GlobalInit();
}

SSHIOContext::~SSHIOContext() {
	stop();
}

void SSHIOContext::start() {
	if ( mThread.joinable() )
		return;
	mThread = std::thread( [this] { loop(); } );
	mThreadId = mThread.get_id();
}

void SSHIOContext::stop() {
	if ( !mThread.joinable() )
		return;
	mStopping = true;
	mWake.signal();
	mThread.join();
	// The I/O thread is gone: fail everything that is still queued from here.
	for ( auto& connection : mConnections )
		connection->fail( VFSError( VFSErrorCode::ConnectionLost, URI(), "SSH layer shut down" ) );
	for ( auto& connection : mConnections )
		connection->drive();
	mConnections.clear();
	std::vector<Command> commands;
	{
		std::lock_guard<std::mutex> lock( mMutex );
		commands.swap( mCommands );
	}
}

void SSHIOContext::post( Command command ) {
	{
		std::lock_guard<std::mutex> lock( mMutex );
		mCommands.emplace_back( std::move( command ) );
	}
	mWake.signal();
}

void SSHIOContext::addConnection( std::shared_ptr<SSHConnection> connection ) {
	mConnections.push_back( std::move( connection ) );
}

void SSHIOContext::loop() {
	std::vector<Command> commands;
	std::vector<PollFd> fds;
	while ( !mStopping.load( std::memory_order_acquire ) ) {
		{
			std::lock_guard<std::mutex> lock( mMutex );
			commands.swap( mCommands );
		}
		for ( auto& command : commands )
			command();
		commands.clear();

		for ( size_t i = 0; i < mConnections.size(); ++i )
			mConnections[i]->drive();
		// Drop finished connections; their owners (pool, handles) may still reference them.
		mConnections.erase( std::remove_if( mConnections.begin(), mConnections.end(),
											[]( const std::shared_ptr<SSHConnection>& c ) {
												return !c->isUsable() && c->mQueue.empty();
											} ),
							mConnections.end() );

		fds.clear();
		PollFd wake{};
		wake.fd = mWake.pollHandle();
		wake.events = POLLIN;
		fds.push_back( wake );
		int timeout = -1;
		for ( const auto& connection : mConnections ) {
			connection->collectPoll( fds );
			int next = connection->nextTimeoutMs();
			if ( next >= 0 && ( timeout < 0 || next < timeout ) )
				timeout = next;
		}
		pollSockets( fds.data(), fds.size(), timeout );
		mWake.drain();
	}
}

// ---------------------------------------------------------------------------------------------
// Connection
// ---------------------------------------------------------------------------------------------

SSHConnection::SSHConnection( std::shared_ptr<SSHIOContext> context, SSHResolvedConfig config,
							  SSHConnectionHooks hooks, std::unique_ptr<SSHTransport> transport,
							  Time connectTimeout ) :
	mContext( std::move( context ) ),
	mConfig( std::move( config ) ),
	mHooks( std::move( hooks ) ),
	mTransport( std::move( transport ) ),
	mLastUse( std::chrono::steady_clock::now() ),
	mConnectTimeout( connectTimeout ) {
	mDeadline =
		std::chrono::steady_clock::now() +
		std::chrono::milliseconds( std::max<Int64>( 1000, connectTimeout.asMilliseconds() ) );
}

SSHConnection::~SSHConnection() {
	if ( mAgent ) {
		libssh2_agent_disconnect( mAgent );
		libssh2_agent_free( mAgent );
	}
	if ( mSession ) {
		// Teardown may need a few writes (channel close, disconnect): do them bounded and
		// blocking rather than leaking the session.
		libssh2_session_set_timeout( mSession, 2000 );
		libssh2_session_set_blocking( mSession, 1 );
		// libssh2 frees a handle on every completed close, even when sending fails.
		for ( auto* handle : mHandles )
			libssh2_sftp_close_handle( handle );
		mHandles.clear();
		if ( mSFTP )
			libssh2_sftp_shutdown( mSFTP );
		if ( isReady() || mState == State::Closed )
			libssh2_session_disconnect( mSession, "eepp-vfs closing" );
		libssh2_session_free( mSession );
	}
	scrub( mPassword );
	scrub( mPassphrase );
	scrub( mKbdAnswers );
}

VFSTask<void> SSHConnection::whenReady() {
	TaskSource<VFSResult<void>> source;
	auto task = source.task();
	std::weak_ptr<SSHConnection> weak = weak_from_this();
	mContext->post( [weak, source = std::move( source )]() mutable {
		auto self = weak.lock();
		if ( !self ) {
			source.resolve( Err( VFSError( VFSErrorCode::ConnectionLost ) ) );
			return;
		}
		State state = self->mState.load();
		if ( state == State::Ready )
			source.resolve( Ok() );
		else if ( state == State::Failed || state == State::Closed )
			source.resolve( Err( self->mFailure ) );
		else
			self->mReadyWaiters.push_back( std::move( source ) );
	} );
	return task;
}

void SSHConnection::close() {
	std::weak_ptr<SSHConnection> weak = weak_from_this();
	mContext->post( [weak] {
		if ( auto self = weak.lock() ) {
			if ( self->isUsable() ) {
				self->fail( VFSError( VFSErrorCode::ConnectionLost, URI(), "Connection closed" ) );
				self->mState = State::Closed;
			}
		}
	} );
}

std::chrono::steady_clock::time_point SSHConnection::lastUse() const {
	std::lock_guard<std::mutex> lock( mQueueMutex );
	return mLastUse;
}

void SSHConnection::markUsed() {
	std::lock_guard<std::mutex> lock( mQueueMutex );
	mLastUse = std::chrono::steady_clock::now();
}

void SSHConnection::enqueue( std::unique_ptr<SSHOperation> operation ) {
	{
		std::lock_guard<std::mutex> lock( mQueueMutex );
		mLastUse = std::chrono::steady_clock::now();
		mQueue.push_back( std::move( operation ) );
	}
	mContext->wake();
}

void SSHConnection::fail( VFSError error ) {
	State state = mState.load();
	if ( state == State::Failed || state == State::Closed )
		return;
	mFailure = std::move( error );
	mState = State::Failed;
	for ( auto& waiter : mReadyWaiters )
		waiter.resolve( Err( mFailure ) );
	mReadyWaiters.clear();
}

VFSError SSHConnection::lastError( const URI& uri, const char* what ) {
	char* message = nullptr;
	int rc = mSession ? libssh2_session_last_error( mSession, &message, nullptr, 0 ) : 0;
	std::string text( what );
	if ( message && *message ) {
		text += ": ";
		text += message;
	}
	if ( rc == LIBSSH2_ERROR_SFTP_PROTOCOL && mSFTP ) {
		unsigned long status = libssh2_sftp_last_error( mSFTP );
		return VFSError( mapSFTPStatus( status ), uri, std::move( text ),
						 static_cast<Int64>( status ) );
	}
	if ( isSocketError( rc ) || rc == LIBSSH2_ERROR_CHANNEL_CLOSED ||
		 rc == LIBSSH2_ERROR_CHANNEL_EOF_SENT ) {
		VFSError error( VFSErrorCode::ConnectionLost, uri, std::move( text ), rc );
		fail( error );
		return error;
	}
	if ( rc == LIBSSH2_ERROR_TIMEOUT )
		return VFSError( VFSErrorCode::TimedOut, uri, std::move( text ), rc );
	return VFSError( VFSErrorCode::ProtocolError, uri, std::move( text ), rc );
}

short SSHConnection::pollEvents() const {
	int directions = mSession ? libssh2_session_block_directions( mSession ) : 0;
	short events = 0;
	if ( directions & LIBSSH2_SESSION_BLOCK_INBOUND )
		events |= POLLIN;
	if ( directions & LIBSSH2_SESSION_BLOCK_OUTBOUND )
		events |= POLLOUT;
	return events;
}

void SSHConnection::collectPoll( std::vector<PollFd>& fds ) const {
	if ( !mTransport || mWaitingExternal || !isUsable() )
		return;
	// Pumps wait for unsolicited channel data, so a session with pumps is always polled.
	bool busy = mState.load() != State::Ready || !mPumps.empty();
	if ( !busy ) {
		std::lock_guard<std::mutex> lock( mQueueMutex );
		busy = !mQueue.empty();
	}
	if ( !busy )
		return;
	SocketHandle socket = mTransport->pollSocket();
	if ( socket == InvalidSocket )
		return;
	short events = mTransport->pollEvents( pollEvents() );
	if ( events == 0 )
		events = POLLIN;
	PollFd fd{};
	fd.fd = socket;
	fd.events = events;
	fds.push_back( fd );
}

int SSHConnection::nextTimeoutMs() const {
	auto now = std::chrono::steady_clock::now();
	int timeout = -1;
	auto consider = [&]( std::chrono::steady_clock::time_point when ) {
		int ms = static_cast<int>( std::max<Int64>(
			0, std::chrono::duration_cast<std::chrono::milliseconds>( when - now ).count() ) );
		if ( timeout < 0 || ms < timeout )
			timeout = ms;
	};
	State state = mState.load();
	if ( state != State::Ready && state != State::Failed && state != State::Closed &&
		 !mWaitingExternal )
		consider( mDeadline );
	if ( state == State::Ready && mConfig.serverAliveInterval )
		consider( mNextKeepalive );
	return timeout;
}

void SSHConnection::drive() {
	if ( mTransport && !mTransport->alive() && isUsable() )
		fail( VFSError( VFSErrorCode::ConnectionLost, URI(), "Transport closed" ) );

	if ( isUsable() && mState.load() != State::Ready ) {
		if ( !mWaitingExternal && std::chrono::steady_clock::now() > mDeadline ) {
			fail( VFSError( VFSErrorCode::TimedOut, URI(),
							"Timed out connecting to " + mConfig.hostName ) );
		} else if ( establish() == Step::Again ) {
			return;
		}
	}

	if ( !isUsable() ) {
		std::deque<std::unique_ptr<SSHOperation>> queue;
		{
			std::lock_guard<std::mutex> lock( mQueueMutex );
			queue.swap( mQueue );
		}
		for ( auto& operation : queue )
			operation->fail( mFailure );
		auto pumps = std::move( mPumps );
		mPumps.clear();
		for ( auto& pump : pumps )
			pump->connectionLost( mFailure );
		return;
	}

	while ( true ) {
		SSHOperation* operation;
		{
			std::lock_guard<std::mutex> lock( mQueueMutex );
			if ( mQueue.empty() )
				break;
			operation = mQueue.front().get();
		}
		if ( operation->step( *this ) == Step::Again )
			break;
		std::unique_ptr<SSHOperation> done;
		{
			std::lock_guard<std::mutex> lock( mQueueMutex );
			done = std::move( mQueue.front() );
			mQueue.pop_front();
			mLastUse = std::chrono::steady_clock::now();
		}
		if ( !isUsable() ) {
			// The operation detected a dead transport: fail the rest.
			drive();
			return;
		}
	}

	for ( size_t i = 0; i < mPumps.size(); ) {
		if ( mPumps[i]->pump( *this ) ) {
			++i;
		} else {
			mPumps.erase( mPumps.begin() + i );
		}
		if ( !isUsable() ) {
			drive();
			return;
		}
	}

	if ( mConfig.serverAliveInterval && std::chrono::steady_clock::now() >= mNextKeepalive ) {
		int seconds = 0;
		int rc = libssh2_keepalive_send( mSession, &seconds );
		if ( rc != 0 && rc != LIBSSH2_ERROR_EAGAIN && isSocketError( rc ) ) {
			fail( VFSError( VFSErrorCode::ConnectionLost, URI(), "Keepalive failed" ) );
			return;
		}
		mNextKeepalive =
			std::chrono::steady_clock::now() + std::chrono::seconds( std::max( 1, seconds ) );
	}
}

std::vector<std::string_view> SSHConnection::knownHostKeyAlgorithms() {
	std::vector<std::string_view> known;
	LIBSSH2_KNOWNHOSTS* hosts = libssh2_knownhost_init( mSession );
	if ( !hosts )
		return known;
	for ( const auto& file : mConfig.userKnownHostsFiles )
		loadKnownHosts( hosts, file );
	for ( const auto& file : mConfig.globalKnownHostsFiles )
		loadKnownHosts( hosts, file );
	std::string checkHost = mConfig.hostKeyAlias.value_or( mConfig.hostName );
	// A key that can never be stored: a "mismatch" means an entry of that type exists for the
	// host. This lets libssh2 handle hashed names and [host]:port entries.
	static constexpr char Probe[] = "\x01";
	for ( int keyBits : { LIBSSH2_KNOWNHOST_KEY_ED25519, LIBSSH2_KNOWNHOST_KEY_ECDSA_256,
						  LIBSSH2_KNOWNHOST_KEY_ECDSA_384, LIBSSH2_KNOWNHOST_KEY_ECDSA_521,
						  LIBSSH2_KNOWNHOST_KEY_SSHRSA, LIBSSH2_KNOWNHOST_KEY_SSHDSS } ) {
		int check = libssh2_knownhost_checkp(
			hosts, checkHost.c_str(), mConfig.port, Probe, 1,
			LIBSSH2_KNOWNHOST_TYPE_PLAIN | LIBSSH2_KNOWNHOST_KEYENC_RAW | keyBits, nullptr );
		if ( check == LIBSSH2_KNOWNHOST_CHECK_MISMATCH ) {
			for ( auto alg : hostKeyAlgorithmsForKnownType( keyBits ) )
				known.push_back( alg );
		}
	}
	libssh2_knownhost_free( hosts );
	return known;
}

void SSHConnection::applyAlgorithmPreferences() {
	// Like OpenSSH, unless HostKeyAlgorithms is configured, prefer the host key algorithms that
	// known_hosts already has keys for, so a known host is not reported as unknown just because
	// the server also offers another key type.
	if ( !mConfig.hostKeyAlgorithms ) {
		auto known = knownHostKeyAlgorithms();
		const char** algs = nullptr;
		int count = known.empty()
						? 0
						: libssh2_session_supported_algs( mSession, LIBSSH2_METHOD_HOSTKEY, &algs );
		if ( count > 0 ) {
			std::vector<std::string> supported( algs, algs + count );
			libssh2_free( mSession, algs );
			std::stable_partition(
				supported.begin(), supported.end(), [&known]( const std::string& alg ) {
					return std::find( known.begin(), known.end(), alg ) != known.end();
				} );
			std::string joined;
			for ( const auto& alg : supported ) {
				if ( !joined.empty() )
					joined += ',';
				joined += alg;
			}
			libssh2_session_method_pref( mSession, LIBSSH2_METHOD_HOSTKEY, joined.c_str() );
		}
	}

	struct Pref {
		const std::optional<std::string>* spec;
		int method;
	};
	const Pref prefs[] = {
		{ &mConfig.kexAlgorithms, LIBSSH2_METHOD_KEX },
		{ &mConfig.hostKeyAlgorithms, LIBSSH2_METHOD_HOSTKEY },
		{ &mConfig.ciphers, LIBSSH2_METHOD_CRYPT_CS },
		{ &mConfig.ciphers, LIBSSH2_METHOD_CRYPT_SC },
		{ &mConfig.macs, LIBSSH2_METHOD_MAC_CS },
		{ &mConfig.macs, LIBSSH2_METHOD_MAC_SC },
	};
	for ( const auto& pref : prefs ) {
		if ( !*pref.spec )
			continue;
		const char** algs = nullptr;
		int count = libssh2_session_supported_algs( mSession, pref.method, &algs );
		if ( count <= 0 )
			continue;
		std::vector<std::string> supported( algs, algs + count );
		libssh2_free( mSession, algs );
		auto list = SSHConfigEvaluator::applyAlgorithmSpec( supported, **pref.spec );
		// Keep only what this libssh2 build implements.
		std::string joined;
		for ( const auto& alg : list ) {
			if ( std::find( supported.begin(), supported.end(), alg ) == supported.end() )
				continue;
			if ( !joined.empty() )
				joined += ',';
			joined += alg;
		}
		if ( !joined.empty() )
			libssh2_session_method_pref( mSession, pref.method, joined.c_str() );
	}
}

Step SSHConnection::establish() {
	switch ( mState.load() ) {
		case State::Starting: {
			mSession = libssh2_session_init_ex( nullptr, nullptr, nullptr, this );
			if ( !mSession ) {
				fail( VFSError( VFSErrorCode::IO, URI(), "libssh2_session_init failed" ) );
				return Step::Done;
			}
			libssh2_session_set_blocking( mSession, 0 );
			if ( mConfig.compression )
				libssh2_session_flag( mSession, LIBSSH2_FLAG_COMPRESS, 1 );
			applyAlgorithmPreferences();
			mTransport->install( mSession );
			mState = State::Handshake;
			[[fallthrough]];
		}
		case State::Handshake:
			if ( stepHandshake() == Step::Again )
				return Step::Again;
			if ( !isUsable() )
				return Step::Done;
			[[fallthrough]];
		case State::HostKey:
		case State::WaitingHostKeyDecision:
			if ( stepHostKey() == Step::Again )
				return Step::Again;
			if ( !isUsable() )
				return Step::Done;
			[[fallthrough]];
		case State::Authenticating:
			if ( stepAuthentication() == Step::Again )
				return Step::Again;
			if ( !isUsable() )
				return Step::Done;
			mState = State::SFTPInit;
			[[fallthrough]];
		case State::SFTPInit:
			mSFTP = libssh2_sftp_init( mSession );
			if ( !mSFTP ) {
				if ( libssh2_session_last_errno( mSession ) == LIBSSH2_ERROR_EAGAIN )
					return Step::Again;
				fail( lastError( URI(), "Unable to start the SFTP subsystem" ) );
				return Step::Done;
			}
			if ( mConfig.serverAliveInterval ) {
				libssh2_keepalive_config(
					mSession, 1,
					static_cast<unsigned>( std::max<Int64>(
						1, static_cast<Int64>( mConfig.serverAliveInterval->asSeconds() ) ) ) );
				mNextKeepalive = std::chrono::steady_clock::now();
			}
			mState = State::Ready;
			markUsed();
			for ( auto& waiter : mReadyWaiters )
				waiter.resolve( Ok() );
			mReadyWaiters.clear();
			return Step::Done;
		default:
			return Step::Done;
	}
}

Step SSHConnection::stepHandshake() {
	int rc = libssh2_session_handshake( mSession, mTransport->handshakeSocket() );
	if ( rc == LIBSSH2_ERROR_EAGAIN )
		return Step::Again;
	if ( rc != 0 ) {
		VFSError error = lastError( URI(), "SSH handshake failed" );
		error.code = VFSErrorCode::ConnectionLost;
		fail( std::move( error ) );
		return Step::Done;
	}
	mState = State::HostKey;
	return Step::Done;
}

Step SSHConnection::stepHostKey() {
	if ( mState == State::WaitingHostKeyDecision ) {
		if ( !mHostKeyDecision )
			return Step::Again;
		SSHHostKeyDecision decision = *mHostKeyDecision;
		if ( decision == SSHHostKeyDecision::Reject ) {
			fail( VFSError( VFSErrorCode::HostKeyRejected, URI(),
							"Host key for " + mConfig.hostName + " was rejected" ) );
			return Step::Done;
		}
		if ( decision == SSHHostKeyDecision::AcceptAndSave ) {
			std::string error;
			if ( !saveHostKey( mPendingHostKey.data(), mPendingHostKey.size(), mPendingHostKeyBits,
							   error ) ) {
				fail( VFSError( VFSErrorCode::IO, URI(), "Could not save host key: " + error ) );
				return Step::Done;
			}
		}
		mState = State::Authenticating;
		return Step::Done;
	}

	size_t keyLength = 0;
	int keyType = 0;
	const char* key = libssh2_session_hostkey( mSession, &keyLength, &keyType );
	if ( !key ) {
		fail( VFSError( VFSErrorCode::HostKeyRejected, URI(), "Server sent no host key" ) );
		return Step::Done;
	}
	std::string checkHost = mConfig.hostKeyAlias.value_or( mConfig.hostName );
	int keyBits = knownHostKeyBits( keyType );
	LIBSSH2_KNOWNHOSTS* hosts = libssh2_knownhost_init( mSession );
	for ( const auto& file : mConfig.userKnownHostsFiles )
		loadKnownHosts( hosts, file );
	for ( const auto& file : mConfig.globalKnownHostsFiles )
		loadKnownHosts( hosts, file );
	struct libssh2_knownhost* entry = nullptr;
	int check = libssh2_knownhost_checkp(
		hosts, checkHost.c_str(), mConfig.port, key, keyLength,
		LIBSSH2_KNOWNHOST_TYPE_PLAIN | LIBSSH2_KNOWNHOST_KEYENC_RAW | keyBits, &entry );
	libssh2_knownhost_free( hosts );

	const char* hash = libssh2_hostkey_hash( mSession, LIBSSH2_HOSTKEY_HASH_SHA256 );
	std::string fingerprint =
		hash ? "SHA256:" + base64Unpadded( reinterpret_cast<const unsigned char*>( hash ), 32 )
			 : std::string( "unknown" );

	switch ( check ) {
		case LIBSSH2_KNOWNHOST_CHECK_MATCH:
			mState = State::Authenticating;
			return Step::Done;
		case LIBSSH2_KNOWNHOST_CHECK_MISMATCH:
			// Never accepted, whatever StrictHostKeyChecking says.
			fail( VFSError( VFSErrorCode::HostKeyRejected, URI(),
							"REMOTE HOST IDENTIFICATION HAS CHANGED for " + checkHost + " (" +
								hostKeyTypeName( keyType ) + " " + fingerprint + ")" ) );
			return Step::Done;
		case LIBSSH2_KNOWNHOST_CHECK_FAILURE:
			fail( VFSError( VFSErrorCode::HostKeyRejected, URI(),
							"Host key verification failed for " + checkHost ) );
			return Step::Done;
		default:
			break;
	}

	// Unknown host key.
	switch ( mConfig.strictHostKeyChecking ) {
		case SSHStrictHostKeyChecking::Yes:
			fail( VFSError( VFSErrorCode::HostKeyRejected, URI(),
							"No " + std::string( hostKeyTypeName( keyType ) ) +
								" host key is known for " + checkHost +
								" and StrictHostKeyChecking is enabled (" + fingerprint + ")" ) );
			return Step::Done;
		case SSHStrictHostKeyChecking::AcceptNew:
		case SSHStrictHostKeyChecking::No: {
			std::string error;
			if ( !saveHostKey( key, keyLength, keyBits, error ) ) {
				fail( VFSError( VFSErrorCode::IO, URI(), "Could not save host key: " + error ) );
				return Step::Done;
			}
			mState = State::Authenticating;
			return Step::Done;
		}
		case SSHStrictHostKeyChecking::Ask:
			break;
	}
	if ( !mHooks.hostKeyVerifier ) {
		fail( VFSError( VFSErrorCode::HostKeyRejected, URI(),
						"Unknown host key for " + checkHost + " (" + fingerprint +
							") and no host key verifier is installed" ) );
		return Step::Done;
	}
	SSHHostKeyInfo info;
	info.host = checkHost;
	info.port = mConfig.port;
	info.keyType = hostKeyTypeName( keyType );
	info.fingerprint = fingerprint;
	info.knownHostsFile =
		mConfig.userKnownHostsFiles.empty() ? std::string() : mConfig.userKnownHostsFiles.front();
	mPendingHostKey.assign( key, key + keyLength );
	mPendingHostKeyBits = keyBits;
	mState = State::WaitingHostKeyDecision;
	mWaitingExternal = true;
	std::weak_ptr<SSHConnection> weak = weak_from_this();
	auto context = mContext;
	mHooks.hostKeyVerifier->verify( info ).onComplete(
		[weak, context]( Task<SSHHostKeyDecision>&& result ) {
			SSHHostKeyDecision decision =
				result.isReady() ? std::move( result ).takeValue() : SSHHostKeyDecision::Reject;
			context->post( [weak, decision] {
				if ( auto self = weak.lock() ) {
					self->mHostKeyDecision = decision;
					self->mWaitingExternal = false;
					// User think time does not count against the connect timeout.
					self->mDeadline = std::chrono::steady_clock::now() +
									  std::chrono::milliseconds( std::max<Int64>(
										  1000, self->mConnectTimeout.asMilliseconds() ) );
				}
			} );
		} );
	return Step::Again;
}

bool SSHConnection::saveHostKey( const char* key, size_t keyLength, int keyBits,
								 std::string& error ) {
	if ( mConfig.userKnownHostsFiles.empty() ) {
		error = "No user known_hosts file configured";
		return false;
	}
	std::string host = mConfig.hostKeyAlias.value_or( mConfig.hostName );
	if ( mConfig.port != 22 )
		host = "[" + host + "]:" + String::toString( mConfig.port );
	LIBSSH2_KNOWNHOSTS* hosts = libssh2_knownhost_init( mSession );
	struct libssh2_knownhost* entry = nullptr;
	int rc = libssh2_knownhost_addc(
		hosts, host.c_str(), nullptr, key, keyLength, nullptr, 0,
		LIBSSH2_KNOWNHOST_TYPE_PLAIN | LIBSSH2_KNOWNHOST_KEYENC_RAW | keyBits, &entry );
	char line[16384];
	size_t lineLength = 0;
	if ( rc == 0 )
		rc = libssh2_knownhost_writeline( hosts, entry, line, sizeof( line ), &lineLength,
										  LIBSSH2_KNOWNHOST_FILE_OPENSSH );
	libssh2_knownhost_free( hosts );
	if ( rc != 0 ) {
		error = "Could not format the known_hosts entry";
		return false;
	}

	// Append atomically: existing content + new line into a sibling temp file, then rename. The
	// file is never rewritten through libssh2, so entries it cannot parse are preserved.
	const std::string& path = mConfig.userKnownHostsFiles.front();
	std::string dir = FileSystem::fileRemoveFileName( path );
	if ( !dir.empty() && !FileSystem::isDirectory( dir ) )
		FileSystem::makeDir( dir, true, 0700 );
	std::string content;
	if ( FileSystem::fileExists( path ) )
		FileSystem::fileGet( path, content );
	if ( !content.empty() && content.back() != '\n' )
		content += '\n';
	content.append( line, lineLength );
	if ( content.back() != '\n' )
		content += '\n';
	std::string temporary =
		path + ".eepp-" + String::toString( static_cast<Uint64>( System::Sys::getProcessID() ) );
	if ( !FileSystem::fileWrite( temporary, std::string_view( content ) ) ||
		 !FileSystem::fileMove( temporary, path ) ) {
		FileSystem::fileRemove( temporary );
		error = "Could not write " + path;
		return false;
	}
	return true;
}

void SSHConnection::requestExternal( const SSHAuthenticationRequest& request ) {
	mWaitingExternal = true;
	mExternalAnswer.reset();
	mAuthPhase = AuthPhase::WaitingInput;
	std::weak_ptr<SSHConnection> weak = weak_from_this();
	auto context = mContext;
	mHooks.authenticationHandler->request( request ).onComplete(
		[weak, context]( Task<std::optional<std::vector<std::string>>>&& result ) {
			auto answers = std::make_shared<std::optional<std::vector<std::string>>>();
			if ( result.isReady() )
				*answers = std::move( result ).takeValue();
			context->post( [weak, answers] {
				if ( auto self = weak.lock() ) {
					self->mExternalAnswer = std::move( *answers );
					self->mWaitingExternal = false;
					self->mDeadline = std::chrono::steady_clock::now() +
									  std::chrono::milliseconds( std::max<Int64>(
										  1000, self->mConnectTimeout.asMilliseconds() ) );
				}
				if ( answers->has_value() )
					scrub( **answers );
			} );
		} );
}

void SSHConnection::kbdInteractiveCallback( const char* name, int nameLength,
											const char* instruction, int instructionLength,
											int promptCount,
											const LIBSSH2_USERAUTH_KBDINT_PROMPT* prompts,
											LIBSSH2_USERAUTH_KBDINT_RESPONSE* responses,
											void** abstract ) {
	auto* self = static_cast<SSHConnection*>( *abstract );
	self->mKbdName.assign( name ? name : "", name ? nameLength : 0 );
	self->mKbdInstruction.assign( instruction ? instruction : "",
								  instruction ? instructionLength : 0 );
	self->mKbdPrompts.clear();
	for ( int i = 0; i < promptCount; ++i ) {
		self->mKbdPrompts.push_back( SSHPrompt{
			std::string( reinterpret_cast<const char*>( prompts[i].text ), prompts[i].length ),
			prompts[i].echo != 0 } );
	}
	// Answers are only usable when they were collected for this exact prompt count; otherwise
	// empty answers fail this round and the prompts are forwarded to the application.
	bool usable = static_cast<int>( self->mKbdAnswers.size() ) == promptCount;
	self->mKbdHadAnswers = usable;
	for ( int i = 0; i < promptCount; ++i ) {
		const std::string answer = usable ? self->mKbdAnswers[i] : std::string();
		responses[i].text = static_cast<char*>( std::malloc( answer.size() + 1 ) );
		std::memcpy( responses[i].text, answer.c_str(), answer.size() + 1 );
		responses[i].length = static_cast<unsigned int>( answer.size() );
	}
	scrub( self->mKbdAnswers );
}

Step SSHConnection::stepAuthentication() {
	const std::string& user = mConfig.user;
	const unsigned userLength = static_cast<unsigned>( user.size() );
	while ( true ) {
		switch ( mAuthPhase ) {
			case AuthPhase::List: {
				char* list = libssh2_userauth_list( mSession, user.c_str(), userLength );
				if ( !list ) {
					if ( libssh2_userauth_authenticated( mSession ) ) {
						mAuthPhase = AuthPhase::Done;
						continue;
					}
					if ( libssh2_session_last_errno( mSession ) == LIBSSH2_ERROR_EAGAIN )
						return Step::Again;
					fail( lastError( URI(), "Could not query authentication methods" ) );
					return Step::Done;
				}
				mServerMethods = list;
				auto server = String::split( mServerMethods, ',' );
				for ( const auto& method : mConfig.preferredAuthentications ) {
					bool enabled = ( method == "publickey" && mConfig.pubkeyAuthentication ) ||
								   ( method == "keyboard-interactive" &&
									 mConfig.kbdInteractiveAuthentication && !mConfig.batchMode ) ||
								   ( method == "password" && mConfig.passwordAuthentication &&
									 !mConfig.batchMode );
					if ( enabled &&
						 std::find( server.begin(), server.end(), method ) != server.end() )
						mMethods.push_back( method );
				}
				mMethodIndex = 0;
				mAuthPhase = AuthPhase::NextMethod;
				continue;
			}
			case AuthPhase::NextMethod: {
				if ( mMethodIndex >= mMethods.size() ) {
					fail( VFSError( VFSErrorCode::AuthenticationFailed, URI(),
									user + "@" + mConfig.hostName + ": Permission denied (" +
										mServerMethods + ")" ) );
					return Step::Done;
				}
				const std::string& method = mMethods[mMethodIndex++];
				mAttempts = 0;
				if ( method == "publickey" ) {
					mAuthPhase = AuthPhase::AgentConnect;
				} else if ( method == "keyboard-interactive" ) {
					if ( !mHooks.authenticationHandler )
						continue;
					mKbdPrompts.clear();
					mAuthPhase = AuthPhase::KbdAuth;
				} else {
					if ( !mHooks.authenticationHandler )
						continue;
					mAuthPhase = AuthPhase::PasswordAuth;
				}
				continue;
			}
			case AuthPhase::AgentConnect: {
				mKeyIndex = 0;
				if ( !mConfig.identityAgent ) {
					mAuthPhase = AuthPhase::KeyNext;
					continue;
				}
				mAgent = libssh2_agent_init( mSession );
				if ( mAgent ) {
					libssh2_agent_set_identity_path( mAgent, mConfig.identityAgent->c_str() );
					// Agent socket I/O is local and short; libssh2 performs it blocking.
					if ( libssh2_agent_connect( mAgent ) == 0 &&
						 libssh2_agent_list_identities( mAgent ) == 0 ) {
						mAgentIdentity = nullptr;
						mAllowedAgentBlobs.clear();
						if ( mConfig.identitiesOnly ) {
							for ( const auto& file : mConfig.identityFiles )
								if ( auto blob = readPublicKeyBlob( file + ".pub" ) )
									mAllowedAgentBlobs.push_back( std::move( *blob ) );
						}
						mAuthPhase = AuthPhase::AgentNext;
						continue;
					}
					libssh2_agent_free( mAgent );
					mAgent = nullptr;
				}
				mAuthPhase = AuthPhase::KeyNext;
				continue;
			}
			case AuthPhase::AgentNext: {
				struct libssh2_agent_publickey* next = nullptr;
				int rc = libssh2_agent_get_identity( mAgent, &next, mAgentIdentity );
				if ( rc != 0 ) {
					libssh2_agent_disconnect( mAgent );
					libssh2_agent_free( mAgent );
					mAgent = nullptr;
					mAuthPhase = AuthPhase::KeyNext;
					continue;
				}
				mAgentIdentity = next;
				if ( mConfig.identitiesOnly ) {
					std::string blob( reinterpret_cast<const char*>( next->blob ), next->blob_len );
					if ( std::find( mAllowedAgentBlobs.begin(), mAllowedAgentBlobs.end(), blob ) ==
						 mAllowedAgentBlobs.end() )
						continue;
				}
				mAuthPhase = AuthPhase::AgentAuth;
				continue;
			}
			case AuthPhase::AgentAuth: {
				int rc = libssh2_agent_userauth( mAgent, user.c_str(), mAgentIdentity );
				if ( rc == LIBSSH2_ERROR_EAGAIN )
					return Step::Again;
				if ( rc == 0 ) {
					mAuthPhase = AuthPhase::Done;
					continue;
				}
				if ( isSocketError( libssh2_session_last_errno( mSession ) ) ) {
					fail( lastError( URI(), "Authentication failed" ) );
					return Step::Done;
				}
				mAuthPhase = AuthPhase::AgentNext;
				continue;
			}
			case AuthPhase::KeyNext: {
				while ( mKeyIndex < mConfig.identityFiles.size() &&
						!FileSystem::fileExists( mConfig.identityFiles[mKeyIndex] ) )
					++mKeyIndex;
				if ( mKeyIndex >= mConfig.identityFiles.size() ) {
					mAuthPhase = AuthPhase::NextMethod;
					continue;
				}
				mAttempts = 0;
				scrub( mPassphrase );
				const std::string& file = mConfig.identityFiles[mKeyIndex];
				if ( isKeyEncrypted( file ) ) {
					if ( !mHooks.authenticationHandler || mConfig.batchMode ) {
						++mKeyIndex;
						continue;
					}
					SSHAuthenticationRequest request;
					request.kind = SSHAuthenticationRequest::Kind::Passphrase;
					request.user = user;
					request.host = mConfig.hostName;
					request.identityFile = file;
					request.prompts = {
						SSHPrompt{ "Enter passphrase for key '" + file + "': ", false } };
					request.attempt = ++mAttempts;
					mResumePhase = AuthPhase::KeyAuth;
					requestExternal( request );
					return Step::Again;
				}
				mAuthPhase = AuthPhase::KeyAuth;
				continue;
			}
			case AuthPhase::KeyAuth: {
				const std::string& file = mConfig.identityFiles[mKeyIndex];
				std::string pub = file + ".pub";
				bool hasPub = FileSystem::fileExists( pub );
				int rc = libssh2_userauth_publickey_fromfile_ex(
					mSession, user.c_str(), userLength, hasPub ? pub.c_str() : nullptr,
					file.c_str(), mPassphrase.empty() ? nullptr : mPassphrase.c_str() );
				if ( rc == LIBSSH2_ERROR_EAGAIN )
					return Step::Again;
				if ( rc == 0 ) {
					scrub( mPassphrase );
					mAuthPhase = AuthPhase::Done;
					continue;
				}
				if ( isSocketError( libssh2_session_last_errno( mSession ) ) ) {
					fail( lastError( URI(), "Authentication failed" ) );
					return Step::Done;
				}
				// A wrong passphrase surfaces either as a key file error, or (when a .pub file
				// let libssh2 query the server first) as a failed signing callback. A key the
				// server does not accept fails before signing with a different message.
				char* message = nullptr;
				libssh2_session_last_error( mSession, &message, nullptr, 0 );
				bool wrongPassphrase = rc == LIBSSH2_ERROR_FILE ||
									   ( rc == LIBSSH2_ERROR_PUBLICKEY_UNVERIFIED && message &&
										 std::strcmp( message, "Callback returned error" ) == 0 );
				if ( wrongPassphrase && !mPassphrase.empty() &&
					 mAttempts < mConfig.numberOfPasswordPrompts && mHooks.authenticationHandler ) {
					scrub( mPassphrase );
					SSHAuthenticationRequest request;
					request.kind = SSHAuthenticationRequest::Kind::Passphrase;
					request.user = user;
					request.host = mConfig.hostName;
					request.identityFile = file;
					request.prompts = {
						SSHPrompt{ "Enter passphrase for key '" + file + "': ", false } };
					request.attempt = ++mAttempts;
					mResumePhase = AuthPhase::KeyAuth;
					requestExternal( request );
					return Step::Again;
				}
				scrub( mPassphrase );
				++mKeyIndex;
				mAuthPhase = AuthPhase::KeyNext;
				continue;
			}
			case AuthPhase::KbdAuth: {
				int rc = libssh2_userauth_keyboard_interactive_ex(
					mSession, user.c_str(), userLength, &SSHConnection::kbdInteractiveCallback );
				if ( rc == LIBSSH2_ERROR_EAGAIN )
					return Step::Again;
				if ( rc == 0 ) {
					mAuthPhase = AuthPhase::Done;
					continue;
				}
				if ( isSocketError( libssh2_session_last_errno( mSession ) ) ) {
					fail( lastError( URI(), "Authentication failed" ) );
					return Step::Done;
				}
				if ( !mKbdPrompts.empty() && mAttempts < mConfig.numberOfPasswordPrompts ) {
					SSHAuthenticationRequest request;
					request.kind = SSHAuthenticationRequest::Kind::KeyboardInteractive;
					request.user = user;
					request.host = mConfig.hostName;
					request.name = mKbdName;
					request.instruction = mKbdInstruction;
					request.prompts = mKbdPrompts;
					request.attempt = ++mAttempts;
					mResumePhase = AuthPhase::KbdAuth;
					requestExternal( request );
					return Step::Again;
				}
				mAuthPhase = AuthPhase::NextMethod;
				continue;
			}
			case AuthPhase::PasswordAuth: {
				if ( mPassword.empty() ) {
					if ( mAttempts >= mConfig.numberOfPasswordPrompts ) {
						mAuthPhase = AuthPhase::NextMethod;
						continue;
					}
					SSHAuthenticationRequest request;
					request.kind = SSHAuthenticationRequest::Kind::Password;
					request.user = user;
					request.host = mConfig.hostName;
					request.prompts = {
						SSHPrompt{ user + "@" + mConfig.hostName + "'s password: ", false } };
					request.attempt = ++mAttempts;
					mResumePhase = AuthPhase::PasswordAuth;
					requestExternal( request );
					return Step::Again;
				}
				int rc = libssh2_userauth_password_ex(
					mSession, user.c_str(), userLength, mPassword.c_str(),
					static_cast<unsigned>( mPassword.size() ), nullptr );
				if ( rc == LIBSSH2_ERROR_EAGAIN )
					return Step::Again;
				scrub( mPassword );
				if ( rc == 0 ) {
					mAuthPhase = AuthPhase::Done;
					continue;
				}
				if ( isSocketError( libssh2_session_last_errno( mSession ) ) ) {
					fail( lastError( URI(), "Authentication failed" ) );
					return Step::Done;
				}
				continue; // Ask again (bounded by NumberOfPasswordPrompts).
			}
			case AuthPhase::WaitingInput: {
				if ( mWaitingExternal || !mExternalAnswer )
					return Step::Again;
				auto answers = std::move( *mExternalAnswer );
				mExternalAnswer.reset();
				if ( !answers ) {
					// The user declined: move on to the next key / method.
					if ( mResumePhase == AuthPhase::KeyAuth ) {
						++mKeyIndex;
						mAuthPhase = AuthPhase::KeyNext;
					} else {
						mAuthPhase = AuthPhase::NextMethod;
					}
					continue;
				}
				if ( mResumePhase == AuthPhase::KeyAuth )
					mPassphrase = answers->empty() ? std::string() : ( *answers )[0];
				else if ( mResumePhase == AuthPhase::PasswordAuth )
					mPassword = answers->empty() ? std::string() : ( *answers )[0];
				else
					mKbdAnswers = *answers;
				scrub( *answers );
				mAuthPhase = mResumePhase;
				continue;
			}
			case AuthPhase::Done:
				if ( mAgent ) {
					libssh2_agent_disconnect( mAgent );
					libssh2_agent_free( mAgent );
					mAgent = nullptr;
				}
				return Step::Done;
		}
	}
}

LIBSSH2_CHANNEL* SSHConnection::openDirectTcpip( const std::string& host, Uint16 port,
												 bool& again ) {
	again = false;
	LIBSSH2_CHANNEL* channel =
		libssh2_channel_direct_tcpip_ex( mSession, host.c_str(), port, "127.0.0.1", 22 );
	if ( !channel && libssh2_session_last_errno( mSession ) == LIBSSH2_ERROR_EAGAIN )
		again = true;
	return channel;
}

// ---------------------------------------------------------------------------------------------
// Transports
// ---------------------------------------------------------------------------------------------

short SocketTransport::pollEvents( short directions ) const {
	return directions;
}

ChannelTransport::ChannelTransport( std::shared_ptr<SSHConnection> parent, std::string host,
									Uint16 port ) :
	mParent( std::move( parent ) ), mHost( std::move( host ) ), mPort( port ) {}

ChannelTransport::~ChannelTransport() {
	// The channel belongs to the parent session; libssh2_session_free() of the parent releases
	// it. Freeing it here could race with the parent's I/O thread usage.
}

short ChannelTransport::pollEvents( short directions ) const {
	// Data for the child arrives through the parent's socket.
	return static_cast<short>( directions | mParent->pollEvents() | POLLIN );
}

libssh2_socket_t ChannelTransport::handshakeSocket() const {
	return static_cast<libssh2_socket_t>( mParent->pollSocket() );
}

Step ChannelTransport::open( VFSError& error ) {
	bool again = false;
	mChannel = mParent->openDirectTcpip( mHost, mPort, again );
	if ( mChannel )
		return Step::Done;
	if ( again )
		return Step::Again;
	error = mParent->lastError( URI(), ( "ProxyJump: could not open a channel to " + mHost + ":" +
										 String::toString( mPort ) )
										   .c_str() );
	return Step::Done;
}

void ChannelTransport::install( LIBSSH2_SESSION* session ) {
	libssh2_session_callback_set2( session, LIBSSH2_CALLBACK_SEND,
								   reinterpret_cast<libssh2_cb_generic*>( &sendCallback ) );
	libssh2_session_callback_set2( session, LIBSSH2_CALLBACK_RECV,
								   reinterpret_cast<libssh2_cb_generic*>( &recvCallback ) );
}

ssize_t ChannelTransport::sendCallback( libssh2_socket_t, const void* buffer, size_t length, int,
										void** abstract ) {
	auto* connection = static_cast<SSHConnection*>( *abstract );
	auto* transport = static_cast<ChannelTransport*>( connection->mTransport.get() );
	ssize_t rc =
		libssh2_channel_write( transport->mChannel, static_cast<const char*>( buffer ), length );
	if ( rc == LIBSSH2_ERROR_EAGAIN )
		return -EAGAIN;
	if ( rc < 0 )
		return -ECONNRESET;
	return rc;
}

ssize_t ChannelTransport::recvCallback( libssh2_socket_t, void* buffer, size_t length, int,
										void** abstract ) {
	auto* connection = static_cast<SSHConnection*>( *abstract );
	auto* transport = static_cast<ChannelTransport*>( connection->mTransport.get() );
	ssize_t rc = libssh2_channel_read( transport->mChannel, static_cast<char*>( buffer ), length );
	if ( rc == LIBSSH2_ERROR_EAGAIN )
		return -EAGAIN;
	if ( rc < 0 )
		return -ECONNRESET;
	if ( rc == 0 && !libssh2_channel_eof( transport->mChannel ) )
		return -EAGAIN;
	return rc;
}

ProcessTransport::ProcessTransport( std::shared_ptr<SSHIOContext> context,
									const std::string& command ) :
	mContext( std::move( context ) ), mProcess( std::make_unique<System::Process>() ) {
	mPlaceholder = ::socket( AF_INET, SOCK_STREAM, 0 );
#if EE_PLATFORM == EE_PLATFORM_WIN
	std::string shell = "cmd";
	std::vector<std::string> args{ "/c", command };
#else
	const char* envShell = std::getenv( "SHELL" );
	std::string shell = envShell && *envShell ? envShell : "/bin/sh";
	std::vector<std::string> args{ "-c", command };
#endif
	if ( !mProcess->create( shell, args,
							System::Process::getDefaultOptions() |
								System::Process::Options::EnableAsync ) ) {
		mProcess.reset();
		return;
	}
	mProcess->startAsyncRead( [this]( const char* bytes, size_t n ) {
		{
			std::lock_guard<std::mutex> lock( mMutex );
			mBuffer.append( bytes, n );
		}
		mContext->wake();
	} );
}

ProcessTransport::~ProcessTransport() {
	if ( mProcess ) {
		mProcess->kill();
		mProcess.reset();
	}
	closeSocket( mPlaceholder );
}

bool ProcessTransport::started() const {
	return nullptr != mProcess;
}

bool ProcessTransport::alive() const {
	if ( !mProcess )
		return false;
	std::lock_guard<std::mutex> lock( mMutex );
	return !mBuffer.empty() || mProcess->isAlive();
}

libssh2_socket_t ProcessTransport::handshakeSocket() const {
	// libssh2 only hands this value to the callbacks below.
	return static_cast<libssh2_socket_t>( mPlaceholder );
}

void ProcessTransport::install( LIBSSH2_SESSION* session ) {
	libssh2_session_callback_set2( session, LIBSSH2_CALLBACK_SEND,
								   reinterpret_cast<libssh2_cb_generic*>( &sendCallback ) );
	libssh2_session_callback_set2( session, LIBSSH2_CALLBACK_RECV,
								   reinterpret_cast<libssh2_cb_generic*>( &recvCallback ) );
}

ssize_t ProcessTransport::sendCallback( libssh2_socket_t, const void* buffer, size_t length, int,
										void** abstract ) {
	auto* connection = static_cast<SSHConnection*>( *abstract );
	auto* transport = static_cast<ProcessTransport*>( connection->mTransport.get() );
	if ( !transport->mProcess || !transport->mProcess->isAlive() )
		return -ECONNRESET;
	size_t written = transport->mProcess->write( static_cast<const char*>( buffer ), length );
	return written == 0 ? -EAGAIN : static_cast<ssize_t>( written );
}

ssize_t ProcessTransport::recvCallback( libssh2_socket_t, void* buffer, size_t length, int,
										void** abstract ) {
	auto* connection = static_cast<SSHConnection*>( *abstract );
	auto* transport = static_cast<ProcessTransport*>( connection->mTransport.get() );
	std::lock_guard<std::mutex> lock( transport->mMutex );
	if ( transport->mBuffer.empty() ) {
		if ( !transport->mProcess || !transport->mProcess->isAlive() )
			return 0;
		return -EAGAIN;
	}
	size_t count = std::min( length, transport->mBuffer.size() );
	std::memcpy( buffer, transport->mBuffer.data(), count );
	transport->mBuffer.erase( 0, count );
	return static_cast<ssize_t>( count );
}

}}} // namespace EE::VFS::sftp
