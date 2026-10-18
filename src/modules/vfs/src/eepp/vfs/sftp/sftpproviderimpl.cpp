#include "sftpproviderimpl.hpp"
#include "sshchanneltransport.hpp"
#include "sshconnection.hpp"
#include "sshprocesstransport.hpp"
#include "sshsockettransport.hpp"
#include <eepp/core/string.hpp>
#include <eepp/vfs/detail/await.hpp>
#include <eepp/vfs/path.hpp>
#include <thread>

namespace EE { namespace VFS { namespace sftp {

using detail::await;

namespace {

thread_local bool tHelperThread = false;

constexpr Uint32 BaseCapabilities =
	static_cast<Uint32>( VFSCapability::Read ) | static_cast<Uint32>( VFSCapability::Write ) |
	static_cast<Uint32>( VFSCapability::List ) | static_cast<Uint32>( VFSCapability::Create ) |
	static_cast<Uint32>( VFSCapability::Remove ) | static_cast<Uint32>( VFSCapability::Rename ) |
	static_cast<Uint32>( VFSCapability::Seek ) |
	static_cast<Uint32>( VFSCapability::RandomAccess ) |
	static_cast<Uint32>( VFSCapability::Truncate ) |
	static_cast<Uint32>( VFSCapability::Symlinks ) |
	static_cast<Uint32>( VFSCapability::Permissions ) |
	static_cast<Uint32>( VFSCapability::Ownership ) |
	static_cast<Uint32>( VFSCapability::CaseSensitive );

} // namespace

SFTPProviderImpl::HelperThreadScope::HelperThreadScope() {
	tHelperThread = true;
}

SFTPProviderImpl::HelperThreadScope::~HelperThreadScope() {
	tHelperThread = false;
}

bool SFTPProviderImpl::HelperThreadScope::active() {
	return tHelperThread;
}

SFTPProviderImpl::SFTPProviderImpl( SFTPProviderConfig config ) :
	mConfig( std::move( config ) ),
	mContext( std::make_shared<SSHIOContext>() ),
	mHelperPool( System::ThreadPool::createShared( 2 ) ) {
	if ( !mConfig.configContextFactory ) {
		mConfig.configContextFactory = []( const std::string& host ) {
			return SSHConfigContext::system( host );
		};
	}
	mContext->start();
}

SFTPProviderImpl::~SFTPProviderImpl() {
	if ( HelperThreadScope::active() || mContext->isIOThread() ) {
		// The last reference was released by a job running on one of our own threads, which
		// cannot join itself: finish the teardown on a separate thread.
		std::thread( [pool = std::move( mHelperPool ), context = std::move( mContext )]() mutable {
			pool.reset();
			context->stop();
		} ).detach();
		return;
	}
	// Stop resolving new connections first: helper jobs wait on the I/O thread.
	mHelperPool.reset();
	mContext->stop();
}

SSHConnectionHooks SFTPProviderImpl::hooks() const {
	std::lock_guard<std::mutex> lock( mMutex );
	return SSHConnectionHooks{ mConfig.hostKeyVerifier, mConfig.authenticationHandler };
}

void SFTPProviderImpl::setHostKeyVerifier( std::shared_ptr<SSHHostKeyVerifier> verifier ) {
	std::lock_guard<std::mutex> lock( mMutex );
	mConfig.hostKeyVerifier = std::move( verifier );
}

void SFTPProviderImpl::setAuthenticationHandler(
	std::shared_ptr<SSHAuthenticationHandler> handler ) {
	std::lock_guard<std::mutex> lock( mMutex );
	mConfig.authenticationHandler = std::move( handler );
}

std::string SFTPProviderImpl::authorityKey( const URI& uri ) {
	std::string key( uri.getUserInfo() );
	key += '@';
	key += uri.getHost();
	key += ':';
	key += String::toString( uri.getSpecifiedPort() );
	return key;
}

VFSResult<SSHResolvedConfig> SFTPProviderImpl::resolveSync( const std::string& host,
															std::optional<std::string> user,
															std::optional<Uint16> port ) const {
	SSHConfigContext context;
	{
		std::lock_guard<std::mutex> lock( mMutex );
		context = mConfig.configContextFactory( host );
	}
	context.userOverride = std::move( user );
	context.portOverride = port;
	auto result = SSHConfigEvaluator( std::move( context ) ).evaluate();
	if ( std::holds_alternative<std::string>( result ) ) {
		return Err( VFSError( VFSErrorCode::InvalidOperation, URI(),
							  "ssh_config: " + std::get<std::string>( result ) ) );
	}
	return Ok( std::move( std::get<SSHResolvedConfig>( result ) ) );
}

VFSResult<SSHResolvedConfig> SFTPProviderImpl::resolveForURI( const URI& uri ) const {
	if ( !VFSPath::validURI( uri ) )
		return Err( VFSError( VFSErrorCode::InvalidURI, uri, "NUL in SFTP URI" ) );
	if ( uri.getHost().empty() )
		return Err( VFSError( VFSErrorCode::InvalidURI, uri, "sftp:// URI without host" ) );
	if ( uri.getUserInfo().find( ':' ) != std::string::npos ) {
		return Err( VFSError( VFSErrorCode::InvalidURI, uri,
							  "Passwords are not accepted in sftp:// URIs" ) );
	}
	std::optional<std::string> user;
	if ( !uri.getUserInfo().empty() )
		user = uri.getUserInfo();
	std::optional<Uint16> port;
	if ( uri.getSpecifiedPort() != 0 )
		port = uri.getSpecifiedPort();
	auto resolved = resolveSync( uri.getHost(), user, port );
	if ( !resolved )
		resolved.error().uri = uri;
	return resolved;
}

Time SFTPProviderImpl::connectTimeout( const SSHResolvedConfig& config ) const {
	return config.connectTimeout ? *config.connectTimeout : mConfig.defaultConnectTimeout;
}

/** A connection being built, one hop of its ProxyJump chain at a time. */
struct SFTPProviderImpl::Build {
	SSHResolvedConfig config;
	/** The previous hop, which carries the next one's channel. */
	std::shared_ptr<SSHConnection> parent;
	size_t hop{ 0 };
	TaskResolver<VFSResult<std::shared_ptr<SSHConnection>>> resolve;
};

VFSTask<std::shared_ptr<SSHConnection>>
SFTPProviderImpl::buildConnection( SSHResolvedConfig config ) {
	auto build = std::make_shared<Build>();
	build->config = std::move( config );
	auto task = VFSTask<std::shared_ptr<SSHConnection>>::create( build->resolve );
	buildHop( std::move( build ) );
	return task;
}

void SFTPProviderImpl::buildHop( std::shared_ptr<Build> build ) {
	// Transports are set up on a helper thread (TCP connect blocks, bounded by ConnectTimeout).
	// Waiting for the session is not: the handshake and authentication may wait on the user
	// (host key, password), so it continues from the connection's ready task instead.
	auto pool = mHelperPool;
	if ( !pool ) {
		build->resolve(
			Err( VFSError( VFSErrorCode::ConnectionLost, URI(), "SFTP provider shutting down" ) ) );
		return;
	}
	std::weak_ptr<SFTPProviderImpl> weak = weak_from_this();
	pool->run( [weak, build = std::move( build )]() mutable {
		HelperThreadScope scope;
		auto self = weak.lock();
		if ( !self )
			return; // The task is abandoned with the build.
		auto connection = self->startHop( *build );
		if ( !connection ) {
			build->resolve( Err( std::move( connection ).error() ) );
			return;
		}
		bool last = build->hop == build->config.proxyJump.size();
		std::string hopName = connection.value()->config().hostName;
		connection.value()->whenReady().then( [weak, build, connection = connection.value(), last,
											   hopName]( VFSResult<void>&& ready ) mutable {
			auto self = weak.lock();
			if ( !self )
				return;
			if ( !ready ) {
				VFSError error = std::move( ready ).error();
				if ( !last )
					error.message = String::format( "ProxyJump %s: %s", hopName, error.message );
				build->resolve( Err( std::move( error ) ) );
				return;
			}
			if ( !last ) {
				build->parent = std::move( connection );
				++build->hop;
				self->buildHop( std::move( build ) );
				return;
			}
			self->probeExtensions( connection )
				.then( [build, connection]( VFSResult<void>&& ) mutable {
					build->resolve( Ok( std::move( connection ) ) );
				} );
		} );
	} );
}

VFSResult<std::shared_ptr<SSHConnection>> SFTPProviderImpl::startHop( Build& build ) {
	const SSHResolvedConfig& config = build.config;
	bool last = build.hop == config.proxyJump.size();
	SSHResolvedConfig hopConfig;
	if ( last ) {
		hopConfig = config;
	} else {
		// Each jump host uses its own ssh_config (like `ssh -J`); the chain is the target's.
		const auto& jump = config.proxyJump[build.hop];
		auto resolved =
			resolveSync( jump.host, jump.user.empty() ? std::nullopt : std::optional( jump.user ),
						 jump.port ? std::optional<Uint16>( jump.port ) : std::nullopt );
		if ( !resolved )
			return Err( std::move( resolved ).error() );
		hopConfig = std::move( resolved ).value();
		hopConfig.proxyJump.clear();
		hopConfig.proxyCommand.reset();
	}

	std::unique_ptr<SSHTransport> transport;
	if ( build.parent ) {
		auto channelTransport = std::make_unique<SSHChannelTransport>(
			build.parent, hopConfig.hostName, hopConfig.port );
		SSHChannelTransport* raw = channelTransport.get();
		// Opening the channel is an operation on the parent: it is serialized with the parent's
		// own traffic. Bounded (no user interaction), so waiting here is fine.
		auto opened = await( build.parent->submit<void>(
			{}, URI(), [raw]( SSHConnection& ) -> std::optional<VFSResult<void>> {
				VFSError error;
				if ( raw->open( error ) == Step::Again )
					return std::nullopt;
				if ( !raw->channel() )
					return VFSResult<void>( Err( std::move( error ) ) );
				return VFSResult<void>( Ok() );
			} ) );
		if ( !opened )
			return Err( std::move( opened ).error() );
		transport = std::move( channelTransport );
	} else if ( last && config.proxyCommand ) {
		auto process = std::make_unique<SSHProcessTransport>( mContext, *config.proxyCommand );
		if ( !process->started() ) {
			return Err( VFSError( VFSErrorCode::ConnectionLost, URI(),
								  "Could not start ProxyCommand: " + *config.proxyCommand ) );
		}
		transport = std::move( process );
	} else {
		std::string error;
		SocketHandle socket = SSHSocket::connect(
			hopConfig.hostName, hopConfig.port,
			static_cast<int>( connectTimeout( hopConfig ).asMilliseconds() ), error );
		if ( socket == InvalidSocket )
			return Err( VFSError( VFSErrorCode::ConnectionLost, URI(), error ) );
		transport = std::make_unique<SSHSocketTransport>( socket );
	}

	Time timeout = connectTimeout( hopConfig );
	auto connection = std::make_shared<SSHConnection>( mContext, std::move( hopConfig ), hooks(),
													   std::move( transport ), timeout,
													   mConfig.operationTimeout );
	connection->setIdleTimeout( mConfig.idleTimeout );
	connection->setSFTPRequired( last );
	mContext->post( [context = mContext, connection] { context->addConnection( connection ); } );
	return Ok( std::move( connection ) );
}

/** Probes SFTP extensions once per connection: posix-rename (atomic replace), statvfs. fsync is
 * detected on first use. */
VFSTask<void>
SFTPProviderImpl::probeExtensions( const std::shared_ptr<SSHConnection>& connection ) {
	static const std::string probe = "/.eepp-vfs-extension-probe-does-not-exist";
	return connection->submit<void>(
		{}, URI(), []( SSHConnection& c ) -> std::optional<VFSResult<void>> {
			if ( c.posixRenameSupport < 0 ) {
				int rc = libssh2_sftp_posix_rename_ex(
					c.sftp(), probe.c_str(), static_cast<unsigned>( probe.size() ), probe.c_str(),
					static_cast<unsigned>( probe.size() ) );
				if ( rc == LIBSSH2_ERROR_EAGAIN )
					return std::nullopt;
				// libssh2 answers locally with SSH_FX_OP_UNSUPPORTED when the server did not
				// advertise the extension; any server reply means it exists.
				c.posixRenameSupport = rc == LIBSSH2_FX_OP_UNSUPPORTED ? 0 : 1;
				if ( rc == LIBSSH2_ERROR_SFTP_PROTOCOL &&
					 libssh2_sftp_last_error( c.sftp() ) == LIBSSH2_FX_OP_UNSUPPORTED )
					c.posixRenameSupport = 0;
			}
			if ( c.statvfsSupport < 0 ) {
				LIBSSH2_SFTP_STATVFS st{};
				int rc = libssh2_sftp_statvfs( c.sftp(), "/", 1, &st );
				if ( rc == LIBSSH2_ERROR_EAGAIN )
					return std::nullopt;
				c.statvfsSupport = rc == 0 ? 1 : 0;
			}
			if ( c.posixRenameSupport == 1 && c.fsyncSupport < 0 )
				c.fsyncSupport = 1; // Both arrived in OpenSSH 6.5; confirmed on first use.
			return VFSResult<void>( Ok() );
		} );
}

void SFTPProviderImpl::pruneIdle() {
	// Idle connections close themselves on the I/O thread (SSHConnection::setIdleTimeout());
	// here the pool forgets the closed ones, and closes those whose timer has not fired yet.
	for ( auto it = mPool.begin(); it != mPool.end(); ) {
		auto& entry = it->second;
		auto connection = entry.connection.lock();
		if ( connection && ( !connection->isUsable() || connection->idleExpired() ) ) {
			connection->close();
			it = mPool.erase( it );
		} else if ( !connection && entry.waiters.empty() ) {
			it = mPool.erase( it );
		} else {
			++it;
		}
	}
}

VFSTask<std::shared_ptr<SSHConnection>> SFTPProviderImpl::connection( const URI& uri ) {
	if ( !VFSPath::validURI( uri ) )
		return readyError<std::shared_ptr<SSHConnection>>( VFSErrorCode::InvalidURI, uri,
														   "NUL in SFTP URI" );
	using Result = VFSResult<std::shared_ptr<SSHConnection>>;
	std::string key = authorityKey( uri );
	Uint64 generation;
	TaskResolver<Result> resolve;
	auto task = Task<Result>::create( resolve );
	{
		std::lock_guard<std::mutex> lock( mPoolMutex );
		generation = mPoolGeneration;
		pruneIdle();
		auto it = mPool.find( key );
		if ( it != mPool.end() ) {
			auto connection = it->second.connection.lock();
			if ( connection && connection->isUsable() ) {
				resolve( Ok( std::move( connection ) ) );
				return task;
			}
			if ( !connection && !it->second.waiters.empty() ) {
				it->second.waiters.push_back( std::move( resolve ) );
				return task;
			}
			mPool.erase( it );
		}
		mPool[key].waiters.push_back( std::move( resolve ) );
	}
	auto pool = mHelperPool;
	if ( !pool ) {
		return readyError<std::shared_ptr<SSHConnection>>( VFSErrorCode::ConnectionLost, uri,
														   "SFTP provider shutting down" );
	}
	std::weak_ptr<SFTPProviderImpl> weak = weak_from_this();
	pool->run( [weak, uri, key, generation]() {
		HelperThreadScope scope;
		auto self = weak.lock();
		if ( !self )
			return;
		Result result = Err( VFSError( VFSErrorCode::Unknown ) );
		auto config = self->resolveForURI( uri );
		if ( !config ) {
			result = Err( std::move( config ).error() );
		} else {
			// Share sessions between URIs that resolve to the same effective configuration
			// (aliases, explicit vs. implicit user...).
			std::string connectionKey = config.value().connectionKey();
			std::shared_ptr<SSHConnection> existing;
			{
				std::lock_guard<std::mutex> lock( self->mPoolMutex );
				if ( self->mPoolGeneration != generation )
					return;
				for ( auto& entry : self->mPool ) {
					auto connection = entry.second.connection.lock();
					if ( connection && connection->isUsable() &&
						 connection->config().connectionKey() == connectionKey ) {
						existing = std::move( connection );
						break;
					}
				}
			}
			if ( existing ) {
				result = Ok( existing );
			} else {
				self->buildConnection( std::move( config ).value() )
					.then( [weak, uri, key, generation]( Result&& built ) {
						if ( auto self = weak.lock() )
							self->finishConnection( key, uri, generation, std::move( built ) );
					} );
				return;
			}
		}
		self->finishConnection( key, uri, generation, std::move( result ) );
	} );
	return task;
}

void SFTPProviderImpl::finishConnection( const std::string& key, const URI& uri, Uint64 generation,
										 VFSResult<std::shared_ptr<SSHConnection>>&& result ) {
	if ( !result && result.error().uri.empty() )
		result.error().uri = uri;
	std::vector<TaskResolver<VFSResult<std::shared_ptr<SSHConnection>>>> waiters;
	{
		std::lock_guard<std::mutex> lock( mPoolMutex );
		if ( generation != mPoolGeneration ) {
			if ( result )
				result.value()->close();
			return;
		}
		auto it = mPool.find( key );
		if ( it != mPool.end() ) {
			waiters = std::move( it->second.waiters );
			if ( result )
				it->second.connection = result.value();
			else
				mPool.erase( it );
		}
	}
	for ( auto& resolve : waiters )
		resolve( VFSResult<std::shared_ptr<SSHConnection>>( result ) );
}

ProviderCapabilities SFTPProviderImpl::capabilities( const URI& uri ) const {
	ProviderCapabilities caps( BaseCapabilities );
	std::lock_guard<std::mutex> lock( mPoolMutex );
	auto it = mPool.find( authorityKey( uri ) );
	auto connection = it != mPool.end() ? it->second.connection.lock() : nullptr;
	if ( connection ) {
		const auto& c = *connection;
		caps.set( VFSCapability::AtomicRename, c.posixRenameSupport == 1 );
		caps.set( VFSCapability::AtomicReplace, c.posixRenameSupport == 1 );
		caps.set( VFSCapability::Fsync, c.fsyncSupport == 1 );
		caps.set( VFSCapability::FreeSpace, c.statvfsSupport == 1 );
	}
	return caps;
}

void SFTPProviderImpl::disconnectAll() {
	std::map<std::string, PoolEntry> removed;
	{
		std::lock_guard<std::mutex> lock( mPoolMutex );
		++mPoolGeneration;
		removed.swap( mPool );
	}
	// Completing waiters can run arbitrary continuations: never under the pool lock. Include
	// pending connections, whose authentication may be waiting on UI that is shutting down.
	mContext->closeConnections();
	for ( auto& entry : removed ) {
		if ( auto connection = entry.second.connection.lock() )
			connection->close();
		for ( auto& resolve : entry.second.waiters )
			resolve( Err( VFSError( VFSErrorCode::ConnectionLost, URI(), "Connection closed" ) ) );
	}
}

size_t SFTPProviderImpl::connectionCount() const {
	std::lock_guard<std::mutex> lock( mPoolMutex );
	size_t count = 0;
	for ( const auto& entry : mPool ) {
		if ( auto connection = entry.second.connection.lock();
			 connection && connection->isUsable() )
			++count;
	}
	return count;
}

}}} // namespace EE::VFS::sftp
