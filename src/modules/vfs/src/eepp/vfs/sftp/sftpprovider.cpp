#include "sftpproviderimpl.hpp"
#include "sshsession.hpp"
#include <eepp/core/string.hpp>
#include <eepp/system/threadpool.hpp>
#include <eepp/vfs/detail/await.hpp>
#include <eepp/vfs/sftp/remotewatchbackend.hpp>
#include <eepp/vfs/sftp/sftpprovider.hpp>
#include <eepp/vfs/virtualfilesystem.hpp>
#include <eepp/vfs/watchers/pollingwatchbackend.hpp>
#include <thread>

namespace EE { namespace VFS {

using namespace sftp;
using detail::await;

namespace sftp {

namespace {

/** Set while a helper pool job runs, so the provider knows when it is destroyed on one. */
thread_local bool tHelperThread = false;

struct HelperThreadScope {
	HelperThreadScope() { tHelperThread = true; }

	~HelperThreadScope() { tHelperThread = false; }
};

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

std::string sftpPath( const URI& uri ) {
	const std::string& path = uri.getPath();
	return path.empty() ? std::string( "/" ) : path;
}

FileType typeFromPermissions( unsigned long permissions ) {
	switch ( permissions & LIBSSH2_SFTP_S_IFMT ) {
		case LIBSSH2_SFTP_S_IFREG:
			return FileType::Regular;
		case LIBSSH2_SFTP_S_IFDIR:
			return FileType::Directory;
		case LIBSSH2_SFTP_S_IFLNK:
			return FileType::Symlink;
		case LIBSSH2_SFTP_S_IFBLK:
			return FileType::BlockDevice;
		case LIBSSH2_SFTP_S_IFCHR:
			return FileType::CharacterDevice;
		case LIBSSH2_SFTP_S_IFIFO:
			return FileType::FIFO;
		case LIBSSH2_SFTP_S_IFSOCK:
			return FileType::Socket;
		default:
			return FileType::Unknown;
	}
}

void fillFromAttributes( FileInfo& info, const LIBSSH2_SFTP_ATTRIBUTES& attrs ) {
	if ( attrs.flags & LIBSSH2_SFTP_ATTR_PERMISSIONS ) {
		info.type = typeFromPermissions( attrs.permissions );
		info.setPermissions( static_cast<Uint32>( attrs.permissions & 07777 ) );
	}
	if ( ( attrs.flags & LIBSSH2_SFTP_ATTR_SIZE ) && info.type != FileType::Directory )
		info.setSize( attrs.filesize );
	if ( attrs.flags & LIBSSH2_SFTP_ATTR_UIDGID ) {
		info.setUid( static_cast<Uint32>( attrs.uid ) );
		info.setGid( static_cast<Uint32>( attrs.gid ) );
	}
	if ( attrs.flags & LIBSSH2_SFTP_ATTR_ACMODTIME ) {
		info.setModifiedTime( static_cast<Int64>( attrs.mtime ) );
		info.setAccessedTime( static_cast<Int64>( attrs.atime ) );
	}
	// SFTP v3 only offers second-resolution mtime and size: a best-effort revision.
	info.revision = FileRevision::fromValues(
		{ static_cast<Uint64>( attrs.mtime ), static_cast<Uint64>( attrs.filesize ),
		  static_cast<Uint64>( attrs.permissions & LIBSSH2_SFTP_S_IFMT ) } );
}

/** Extracts owner and group from an `ls -l` style SFTP long name. */
void ownerFromLongName( FileInfo& info, const char* longEntry ) {
	std::vector<std::string> fields;
	std::string current;
	for ( const char* c = longEntry; *c && fields.size() < 4; ++c ) {
		if ( *c == ' ' || *c == '\t' ) {
			if ( !current.empty() ) {
				fields.push_back( std::move( current ) );
				current.clear();
			}
		} else {
			current += *c;
		}
	}
	if ( fields.size() >= 4 ) {
		info.owner = std::move( fields[2] );
		info.group = std::move( fields[3] );
	}
}

/** Refines an SFTP v3 generic FAILURE by inspecting the target (OpenSSH reports EEXIST,
 * ENOTEMPTY... as SSH_FX_FAILURE). */
bool isGenericFailure( const VFSError& error ) {
	return error.code == VFSErrorCode::IO && error.nativeCode == LIBSSH2_FX_FAILURE;
}

/** Non-blocking lstat helper. @return nullopt while it must be retried. */
std::optional<VFSResult<LIBSSH2_SFTP_ATTRIBUTES>> sftpStat( SSHConnection& c, const URI& uri,
															const std::string& path, bool follow ) {
	LIBSSH2_SFTP_ATTRIBUTES attrs{};
	int rc = libssh2_sftp_stat_ex( c.sftp(), path.c_str(), static_cast<unsigned>( path.size() ),
								   follow ? LIBSSH2_SFTP_STAT : LIBSSH2_SFTP_LSTAT, &attrs );
	if ( rc == LIBSSH2_ERROR_EAGAIN )
		return std::nullopt;
	if ( rc != 0 )
		return VFSResult<LIBSSH2_SFTP_ATTRIBUTES>( Err( c.lastError( uri, "stat failed" ) ) );
	return VFSResult<LIBSSH2_SFTP_ATTRIBUTES>( Ok( attrs ) );
}

// ---------------------------------------------------------------------------------------------
// Handles
// ---------------------------------------------------------------------------------------------

/** Closes an SFTP handle on the I/O thread. */
VFSTask<void> closeHandle( const std::shared_ptr<SSHConnection>& connection, URI uri,
						   LIBSSH2_SFTP_HANDLE* handle ) {
	return connection->submit<void>(
		{}, std::move( uri ), [handle]( SSHConnection& c ) -> std::optional<VFSResult<void>> {
			int rc = libssh2_sftp_close_handle( handle );
			if ( rc == LIBSSH2_ERROR_EAGAIN )
				return std::nullopt;
			c.untrackHandle( handle );
			if ( rc != 0 )
				return VFSResult<void>( Err( c.lastError( URI(), "close failed" ) ) );
			return VFSResult<void>( Ok() );
		} );
}

class SFTPDirectoryImpl : public DirectoryHandleImpl {
  public:
	SFTPDirectoryImpl( std::shared_ptr<SSHConnection> connection, URI uri,
					   LIBSSH2_SFTP_HANDLE* handle, DirectoryOpenOptions options ) :
		mConnection( std::move( connection ) ),
		mUri( std::move( uri ) ),
		mHandle( handle ),
		mOptions( options ) {}

	~SFTPDirectoryImpl() override {
		if ( mHandle && mConnection->isUsable() )
			closeHandle( mConnection, mUri, mHandle );
	}

	VFSTask<DirectoryBatch> read( size_t maxEntries, CancellationToken token ) override {
		if ( !mHandle || *mEOF ) {
			DirectoryBatch batch;
			batch.endOfDirectory = true;
			return readyValue<DirectoryBatch>( std::move( batch ) );
		}
		struct State {
			DirectoryBatch batch;
			std::vector<size_t> links;
			size_t linkIndex{ 0 };
			bool listed{ false };
		};
		auto state = std::make_shared<State>();
		LIBSSH2_SFTP_HANDLE* handle = mHandle;
		URI base = mUri;
		bool resolveLinks = mOptions.resolveSymlinkTargets;
		// Shared so a read still queued when the handle object dies stays valid.
		std::shared_ptr<bool> eof = mEOF;
		return mConnection->submit<DirectoryBatch>(
			token, mUri,
			[state, handle, base, resolveLinks, maxEntries, token,
			 eof]( SSHConnection& c ) mutable -> std::optional<VFSResult<DirectoryBatch>> {
				char name[1024];
				char longEntry[2048];
				while ( !state->listed ) {
					if ( state->batch.entries.size() >= maxEntries ) {
						state->listed = true;
						break;
					}
					LIBSSH2_SFTP_ATTRIBUTES attrs{};
					int rc = libssh2_sftp_readdir_ex( handle, name, sizeof( name ), longEntry,
													  sizeof( longEntry ), &attrs );
					if ( rc == LIBSSH2_ERROR_EAGAIN )
						return std::nullopt;
					if ( rc < 0 )
						return VFSResult<DirectoryBatch>(
							Err( c.lastError( base, "readdir failed" ) ) );
					if ( rc == 0 ) {
						state->batch.endOfDirectory = true;
						*eof = true;
						state->listed = true;
						break;
					}
					std::string entryName( name, static_cast<size_t>( rc ) );
					if ( entryName == "." || entryName == ".." )
						continue;
					FileInfo info;
					info.name = std::move( entryName );
					info.hidden = info.name[0] == '.';
					fillFromAttributes( info, attrs );
					ownerFromLongName( info, longEntry );
					if ( info.type == FileType::Symlink && resolveLinks )
						state->links.push_back( state->batch.entries.size() );
					state->batch.entries.push_back( std::move( info ) );
				}
				// Only symlinks cost an extra round trip, and only when targets are requested.
				while ( state->linkIndex < state->links.size() ) {
					if ( token.isCancellationRequested() )
						return VFSResult<DirectoryBatch>( Err( cancelledError( base ) ) );
					FileInfo& link = state->batch.entries[state->links[state->linkIndex]];
					URI linkURI = base.withAppendedSegment( link.name );
					std::string path = sftpPath( linkURI );
					auto target = sftpStat( c, linkURI, path, true );
					if ( !target )
						return std::nullopt;
					if ( *target )
						link.targetType = typeFromPermissions( target->value().permissions );
					else if ( !c.isUsable() )
						return VFSResult<DirectoryBatch>( Err( target->error() ) );
					++state->linkIndex;
				}
				return VFSResult<DirectoryBatch>( Ok( std::move( state->batch ) ) );
			} );
	}

	VFSTask<void> close() override {
		LIBSSH2_SFTP_HANDLE* handle = mHandle;
		mHandle = nullptr;
		if ( !handle )
			return readyValue<void>();
		return closeHandle( mConnection, mUri, handle );
	}

  private:
	std::shared_ptr<SSHConnection> mConnection;
	URI mUri;
	LIBSSH2_SFTP_HANDLE* mHandle;
	DirectoryOpenOptions mOptions;
	std::shared_ptr<bool> mEOF{ std::make_shared<bool>( false ) };
};

class SFTPFileImpl : public FileHandleImpl {
  public:
	SFTPFileImpl( std::shared_ptr<SSHConnection> connection, URI uri, LIBSSH2_SFTP_HANDLE* handle,
				  ProviderCapabilities caps ) :
		mConnection( std::move( connection ) ),
		mUri( std::move( uri ) ),
		mHandle( handle ),
		mCaps( caps ) {}

	~SFTPFileImpl() override {
		if ( mHandle && mConnection->isUsable() )
			closeHandle( mConnection, mUri, mHandle );
	}

	VFSTask<size_t> read( Uint64 offset, std::span<Uint8> data, CancellationToken token ) override {
		if ( !mHandle )
			return readyError<size_t>( VFSErrorCode::InvalidOperation, mUri, "File closed" );
		auto done = std::make_shared<size_t>( 0 );
		auto seeked = std::make_shared<bool>( false );
		LIBSSH2_SFTP_HANDLE* handle = mHandle;
		URI uri = mUri;
		return mConnection->submit<size_t>(
			token, mUri,
			[handle, data, offset, done, seeked,
			 uri]( SSHConnection& c ) -> std::optional<VFSResult<size_t>> {
				while ( *done < data.size() ) {
					if ( !*seeked ) {
						// Seeking resets libssh2's read-ahead: only once per contiguous run.
						libssh2_sftp_seek64( handle, offset + *done );
						*seeked = true;
					}
					ssize_t rc =
						libssh2_sftp_read( handle, reinterpret_cast<char*>( data.data() + *done ),
										   data.size() - *done );
					if ( rc == LIBSSH2_ERROR_EAGAIN )
						return std::nullopt;
					if ( rc < 0 )
						return VFSResult<size_t>( Err( c.lastError( uri, "read failed" ) ) );
					if ( rc == 0 )
						break;
					*done += static_cast<size_t>( rc );
				}
				return VFSResult<size_t>( Ok( *done ) );
			} );
	}

	VFSTask<size_t> write( Uint64 offset, std::span<const Uint8> data,
						   CancellationToken token ) override {
		if ( !mHandle )
			return readyError<size_t>( VFSErrorCode::InvalidOperation, mUri, "File closed" );
		auto done = std::make_shared<size_t>( 0 );
		auto seeked = std::make_shared<bool>( false );
		LIBSSH2_SFTP_HANDLE* handle = mHandle;
		URI uri = mUri;
		return mConnection->submit<size_t>(
			token, mUri,
			[handle, data, offset, done, seeked,
			 uri]( SSHConnection& c ) -> std::optional<VFSResult<size_t>> {
				if ( !*seeked ) {
					libssh2_sftp_seek64( handle, offset );
					*seeked = true;
				}
				while ( *done < data.size() ) {
					// libssh2 pipelines writes and must be re-called with the same remaining
					// buffer after EAGAIN.
					ssize_t rc = libssh2_sftp_write(
						handle, reinterpret_cast<const char*>( data.data() + *done ),
						data.size() - *done );
					if ( rc == LIBSSH2_ERROR_EAGAIN )
						return std::nullopt;
					if ( rc < 0 )
						return VFSResult<size_t>( Err( c.lastError( uri, "write failed" ) ) );
					*done += static_cast<size_t>( rc );
				}
				return VFSResult<size_t>( Ok( *done ) );
			} );
	}

	VFSTask<void> truncate( Uint64 size ) override {
		if ( !mHandle )
			return readyError<void>( VFSErrorCode::InvalidOperation, mUri, "File closed" );
		LIBSSH2_SFTP_HANDLE* handle = mHandle;
		URI uri = mUri;
		return mConnection->submit<void>(
			{}, mUri, [handle, size, uri]( SSHConnection& c ) -> std::optional<VFSResult<void>> {
				LIBSSH2_SFTP_ATTRIBUTES attrs{};
				attrs.flags = LIBSSH2_SFTP_ATTR_SIZE;
				attrs.filesize = size;
				int rc = libssh2_sftp_fstat_ex( handle, &attrs, 1 );
				if ( rc == LIBSSH2_ERROR_EAGAIN )
					return std::nullopt;
				if ( rc != 0 )
					return VFSResult<void>( Err( c.lastError( uri, "truncate failed" ) ) );
				return VFSResult<void>( Ok() );
			} );
	}

	VFSTask<void> flush() override {
		if ( !mHandle )
			return readyError<void>( VFSErrorCode::InvalidOperation, mUri, "File closed" );
		LIBSSH2_SFTP_HANDLE* handle = mHandle;
		URI uri = mUri;
		return mConnection->submit<void>(
			{}, mUri, [handle, uri]( SSHConnection& c ) -> std::optional<VFSResult<void>> {
				if ( c.fsyncSupport == 0 )
					return VFSResult<void>( Ok() );
				int rc = libssh2_sftp_fsync( handle );
				if ( rc == LIBSSH2_ERROR_EAGAIN )
					return std::nullopt;
				if ( rc == 0 ) {
					c.fsyncSupport = 1;
					return VFSResult<void>( Ok() );
				}
				VFSError error = c.lastError( uri, "fsync failed" );
				if ( error.code == VFSErrorCode::Unsupported ) {
					// Data was written; the server just cannot sync it explicitly.
					c.fsyncSupport = 0;
					return VFSResult<void>( Ok() );
				}
				return VFSResult<void>( Err( std::move( error ) ) );
			} );
	}

	VFSTask<FileInfo> stat() override {
		if ( !mHandle )
			return readyError<FileInfo>( VFSErrorCode::InvalidOperation, mUri, "File closed" );
		LIBSSH2_SFTP_HANDLE* handle = mHandle;
		URI uri = mUri;
		return mConnection->submit<FileInfo>(
			{}, mUri, [handle, uri]( SSHConnection& c ) -> std::optional<VFSResult<FileInfo>> {
				LIBSSH2_SFTP_ATTRIBUTES attrs{};
				int rc = libssh2_sftp_fstat_ex( handle, &attrs, 0 );
				if ( rc == LIBSSH2_ERROR_EAGAIN )
					return std::nullopt;
				if ( rc != 0 )
					return VFSResult<FileInfo>( Err( c.lastError( uri, "fstat failed" ) ) );
				FileInfo info;
				info.name = uri.getLastPathSegment();
				info.hidden = !info.name.empty() && info.name[0] == '.';
				fillFromAttributes( info, attrs );
				return VFSResult<FileInfo>( Ok( std::move( info ) ) );
			} );
	}

	VFSTask<void> close() override {
		LIBSSH2_SFTP_HANDLE* handle = mHandle;
		mHandle = nullptr;
		if ( !handle )
			return readyValue<void>();
		return closeHandle( mConnection, mUri, handle );
	}

	ProviderCapabilities capabilities() const override { return mCaps; }

  private:
	std::shared_ptr<SSHConnection> mConnection;
	URI mUri;
	LIBSSH2_SFTP_HANDLE* mHandle;
	ProviderCapabilities mCaps;
};

} // namespace

// ---------------------------------------------------------------------------------------------
// Provider implementation: configuration, connection pooling
// ---------------------------------------------------------------------------------------------

SFTPProviderImpl::SFTPProviderImpl( SFTPProviderConfig config ) :
	mConfig( std::move( config ) ),
	mContext( std::make_shared<SSHIOContext>() ),
	mHelperPool( System::ThreadPool::createShared( 2 ) ) {
	if ( !mConfig.configContextFactory )
		mConfig.configContextFactory = []( const std::string& host ) {
			return SSHConfigContext::system( host );
		};
	mContext->start();
}

SFTPProviderImpl::~SFTPProviderImpl() {
	if ( tHelperThread || mContext->isIOThread() ) {
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
	if ( std::holds_alternative<std::string>( result ) )
		return Err( VFSError( VFSErrorCode::InvalidOperation, URI(),
							  "ssh_config: " + std::get<std::string>( result ) ) );
	return Ok( std::move( std::get<SSHResolvedConfig>( result ) ) );
}

VFSResult<SSHResolvedConfig> SFTPProviderImpl::resolveForURI( const URI& uri ) const {
	if ( uri.getHost().empty() )
		return Err( VFSError( VFSErrorCode::InvalidURI, uri, "sftp:// URI without host" ) );
	if ( uri.getUserInfo().find( ':' ) != std::string::npos )
		return Err( VFSError( VFSErrorCode::InvalidURI, uri,
							  "Passwords are not accepted in sftp:// URIs" ) );
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

/** Builds (blocking, on a helper thread) a ready connection for a resolved configuration,
 * including its ProxyJump chain or ProxyCommand transport. */
VFSResult<std::shared_ptr<SSHConnection>>
SFTPProviderImpl::buildConnection( const SSHResolvedConfig& config ) {
	std::unique_ptr<SSHTransport> transport;
	if ( !config.proxyJump.empty() ) {
		// Each jump host uses its own ssh_config (like `ssh -J`); the chain is the target's.
		std::shared_ptr<SSHConnection> parent;
		for ( size_t i = 0; i <= config.proxyJump.size(); ++i ) {
			bool last = i == config.proxyJump.size();
			SSHResolvedConfig hopConfig;
			if ( last ) {
				hopConfig = config;
			} else {
				const auto& jump = config.proxyJump[i];
				auto resolved = resolveSync(
					jump.host, jump.user.empty() ? std::nullopt : std::optional( jump.user ),
					jump.port ? std::optional<Uint16>( jump.port ) : std::nullopt );
				if ( !resolved )
					return Err( std::move( resolved ).error() );
				hopConfig = std::move( resolved ).value();
				hopConfig.proxyJump.clear();
				hopConfig.proxyCommand.reset();
			}
			std::unique_ptr<SSHTransport> hopTransport;
			if ( !parent ) {
				std::string error;
				SocketHandle socket = connectTcp(
					hopConfig.hostName, hopConfig.port,
					static_cast<int>( connectTimeout( hopConfig ).asMilliseconds() ), error );
				if ( socket == InvalidSocket )
					return Err( VFSError( VFSErrorCode::ConnectionLost, URI(), error ) );
				hopTransport = std::make_unique<SocketTransport>( socket );
			} else {
				auto channelTransport = std::make_unique<ChannelTransport>(
					parent, hopConfig.hostName, hopConfig.port );
				ChannelTransport* raw = channelTransport.get();
				// Opening the channel is an operation on the parent: it is serialized with the
				// parent's own SFTP traffic.
				auto opened = await( parent->submit<void>(
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
				hopTransport = std::move( channelTransport );
			}
			auto connection = std::make_shared<SSHConnection>( mContext, hopConfig, hooks(),
															   std::move( hopTransport ),
															   connectTimeout( hopConfig ) );
			mContext->post(
				[context = mContext, connection] { context->addConnection( connection ); } );
			auto ready = await( connection->whenReady() );
			if ( !ready ) {
				VFSError error = std::move( ready ).error();
				if ( !last )
					error.message = "ProxyJump " + hopConfig.hostName + ": " + error.message;
				return Err( std::move( error ) );
			}
			parent = std::move( connection );
		}
		return Ok( std::move( parent ) );
	}

	if ( config.proxyCommand ) {
		auto process = std::make_unique<ProcessTransport>( mContext, *config.proxyCommand );
		if ( !process->started() )
			return Err( VFSError( VFSErrorCode::ConnectionLost, URI(),
								  "Could not start ProxyCommand: " + *config.proxyCommand ) );
		transport = std::move( process );
	} else {
		std::string error;
		SocketHandle socket =
			connectTcp( config.hostName, config.port,
						static_cast<int>( connectTimeout( config ).asMilliseconds() ), error );
		if ( socket == InvalidSocket )
			return Err( VFSError( VFSErrorCode::ConnectionLost, URI(), error ) );
		transport = std::make_unique<SocketTransport>( socket );
	}
	auto connection = std::make_shared<SSHConnection>(
		mContext, config, hooks(), std::move( transport ), connectTimeout( config ) );
	mContext->post( [context = mContext, connection] { context->addConnection( connection ); } );
	auto ready = await( connection->whenReady() );
	if ( !ready )
		return Err( std::move( ready ).error() );
	return Ok( std::move( connection ) );
}

/** Probes SFTP extensions once per connection: posix-rename (atomic replace), statvfs. fsync is
 * detected on first use. */
void SFTPProviderImpl::probeExtensions( const std::shared_ptr<SSHConnection>& connection ) {
	static const std::string probe = "/.eepp-vfs-extension-probe-does-not-exist";
	await( connection->submit<void>(
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
		} ) );
}

void SFTPProviderImpl::pruneIdle() {
	auto now = std::chrono::steady_clock::now();
	auto idle =
		std::chrono::milliseconds( static_cast<Int64>( mConfig.idleTimeout.asMilliseconds() ) );
	for ( auto it = mPool.begin(); it != mPool.end(); ) {
		auto& entry = it->second;
		if ( entry.connection &&
			 ( !entry.connection->isUsable() || ( entry.connection.use_count() == 1 &&
												  now - entry.connection->lastUse() > idle ) ) ) {
			entry.connection->close();
			it = mPool.erase( it );
		} else {
			++it;
		}
	}
}

VFSTask<std::shared_ptr<SSHConnection>> SFTPProviderImpl::connection( const URI& uri ) {
	using Result = VFSResult<std::shared_ptr<SSHConnection>>;
	std::string key = authorityKey( uri );
	TaskSource<Result> source;
	auto task = source.task();
	{
		std::lock_guard<std::mutex> lock( mPoolMutex );
		pruneIdle();
		auto it = mPool.find( key );
		if ( it != mPool.end() ) {
			if ( it->second.connection && it->second.connection->isUsable() ) {
				source.resolve( Ok( it->second.connection ) );
				return task;
			}
			if ( !it->second.connection ) {
				it->second.waiters.push_back( std::move( source ) );
				return task;
			}
			mPool.erase( it );
		}
		mPool[key].waiters.push_back( std::move( source ) );
	}
	auto pool = mHelperPool;
	if ( !pool )
		return readyError<std::shared_ptr<SSHConnection>>( VFSErrorCode::ConnectionLost, uri,
														   "SFTP provider shutting down" );
	std::weak_ptr<SFTPProviderImpl> weak = weak_from_this();
	pool->run( [weak, uri, key]() {
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
				for ( auto& entry : self->mPool ) {
					if ( entry.second.connection && entry.second.connection->isUsable() &&
						 entry.second.connection->config().connectionKey() == connectionKey ) {
						existing = entry.second.connection;
						break;
					}
				}
			}
			if ( existing ) {
				result = Ok( existing );
			} else {
				result = self->buildConnection( config.value() );
				if ( result )
					self->probeExtensions( result.value() );
			}
			if ( !result && result.error().uri.empty() )
				result.error().uri = uri;
		}
		std::vector<TaskSource<Result>> waiters;
		{
			std::lock_guard<std::mutex> lock( self->mPoolMutex );
			auto it = self->mPool.find( key );
			if ( it != self->mPool.end() ) {
				waiters = std::move( it->second.waiters );
				if ( result )
					it->second.connection = result.value();
				else
					self->mPool.erase( it );
			}
		}
		for ( auto& waiter : waiters )
			waiter.resolve( Result( result ) );
	} );
	return task;
}

ProviderCapabilities SFTPProviderImpl::capabilities( const URI& uri ) const {
	ProviderCapabilities caps( BaseCapabilities );
	std::lock_guard<std::mutex> lock( mPoolMutex );
	auto it = mPool.find( authorityKey( uri ) );
	if ( it != mPool.end() && it->second.connection ) {
		const auto& c = *it->second.connection;
		caps.set( VFSCapability::AtomicRename, c.posixRenameSupport == 1 );
		caps.set( VFSCapability::AtomicReplace, c.posixRenameSupport == 1 );
		caps.set( VFSCapability::Fsync, c.fsyncSupport == 1 );
		caps.set( VFSCapability::FreeSpace, c.statvfsSupport == 1 );
	}
	return caps;
}

void SFTPProviderImpl::disconnectAll() {
	std::lock_guard<std::mutex> lock( mPoolMutex );
	for ( auto& entry : mPool )
		if ( entry.second.connection )
			entry.second.connection->close();
	mPool.clear();
}

size_t SFTPProviderImpl::connectionCount() const {
	std::lock_guard<std::mutex> lock( mPoolMutex );
	size_t count = 0;
	for ( const auto& entry : mPool )
		if ( entry.second.connection && entry.second.connection->isUsable() )
			++count;
	return count;
}

} // namespace sftp

// ---------------------------------------------------------------------------------------------
// Public provider
// ---------------------------------------------------------------------------------------------

namespace {

/** Runs fn( connection ) -> VFSTask<T> once the pooled connection for uri is available. */
template <typename T, typename F>
VFSTask<T> withConnection( const std::shared_ptr<SFTPProviderImpl>& impl, const URI& uri,
						   const CancellationToken& token, F&& fn ) {
	if ( token.isCancellationRequested() )
		return readyError<T>( cancelledError( uri ) );
	return impl->connection( uri ).then(
		[fn = std::forward<F>( fn ),
		 uri]( VFSResult<std::shared_ptr<SSHConnection>>&& c ) mutable -> VFSTask<T> {
			if ( !c )
				return readyError<T>( std::move( c ).error() );
			return fn( std::move( c ).value() );
		} );
}

} // namespace

SFTPProvider::SFTPProvider( SFTPProviderConfig config ) :
	mImpl( std::make_shared<SFTPProviderImpl>( std::move( config ) ) ) {}

SFTPProvider::~SFTPProvider() = default;

ProviderCapabilities SFTPProvider::capabilities( const URI& uri ) const {
	return mImpl->capabilities( uri );
}

void SFTPProvider::setHostKeyVerifier( std::shared_ptr<SSHHostKeyVerifier> verifier ) {
	mImpl->setHostKeyVerifier( std::move( verifier ) );
}

void SFTPProvider::setAuthenticationHandler( std::shared_ptr<SSHAuthenticationHandler> handler ) {
	mImpl->setAuthenticationHandler( std::move( handler ) );
}

void SFTPProvider::disconnectAll() {
	mImpl->disconnectAll();
}

size_t SFTPProvider::connectionCount() const {
	return mImpl->connectionCount();
}

VFSTask<SSHResolvedConfig> SFTPProvider::resolveConfig( const URI& uri ) {
	auto impl = mImpl;
	return mImpl->helperPool()->submit( [impl, uri]() mutable -> VFSResult<SSHResolvedConfig> {
		HelperThreadScope scope;
		auto result = impl->resolveForURI( uri );
		impl.reset();
		return result;
	} );
}

VFSTask<void> SFTPProvider::connect( const URI& uri ) {
	return mImpl->connection( uri ).then(
		[]( VFSResult<std::shared_ptr<SSHConnection>>&& c ) -> VFSResult<void> {
			if ( !c )
				return Err( std::move( c ).error() );
			return Ok();
		} );
}

VFSTask<FileInfo> SFTPProvider::stat( const URI& uri, const StatOptions& options,
									  CancellationToken token ) {
	bool follow = options.followSymlinks;
	return withConnection<FileInfo>(
		mImpl, uri, token, [uri, follow, token]( std::shared_ptr<SSHConnection> connection ) {
			struct State {
				std::optional<LIBSSH2_SFTP_ATTRIBUTES> attrs;
				std::string link;
				bool linkRead{ false };
				bool targetDone{ false };
				FileType targetType{ FileType::Unknown };
			};
			auto state = std::make_shared<State>();
			std::string path = sftpPath( uri );
			return connection->submit<FileInfo>(
				token, uri,
				[state, path, uri,
				 follow]( SSHConnection& c ) -> std::optional<VFSResult<FileInfo>> {
					if ( !state->attrs ) {
						auto attrs = sftpStat( c, uri, path, false );
						if ( !attrs )
							return std::nullopt;
						if ( !*attrs )
							return VFSResult<FileInfo>( Err( attrs->error() ) );
						state->attrs = attrs->value();
					}
					bool isLink =
						typeFromPermissions( state->attrs->permissions ) == FileType::Symlink;
					if ( isLink && !state->linkRead ) {
						char target[4096];
						int rc = libssh2_sftp_symlink_ex(
							c.sftp(), path.c_str(), static_cast<unsigned>( path.size() ), target,
							sizeof( target ), LIBSSH2_SFTP_READLINK );
						if ( rc == LIBSSH2_ERROR_EAGAIN )
							return std::nullopt;
						if ( rc > 0 )
							state->link.assign( target, static_cast<size_t>( rc ) );
						state->linkRead = true;
					}
					std::optional<LIBSSH2_SFTP_ATTRIBUTES> targetAttrs;
					if ( isLink && !state->targetDone ) {
						auto attrs = sftpStat( c, uri, path, true );
						if ( !attrs )
							return std::nullopt;
						state->targetDone = true;
						if ( *attrs ) {
							targetAttrs = attrs->value();
							state->targetType = typeFromPermissions( attrs->value().permissions );
						} else if ( follow ) {
							return VFSResult<FileInfo>( Err( attrs->error() ) );
						}
					}
					FileInfo info;
					info.name = uri.getLastPathSegment();
					if ( info.name.empty() )
						info.name = "/";
					info.hidden = !info.name.empty() && info.name[0] == '.';
					fillFromAttributes( info,
										follow && targetAttrs ? *targetAttrs : *state->attrs );
					info.symlinkTarget = state->link;
					if ( isLink && !follow )
						info.targetType = state->targetType;
					return VFSResult<FileInfo>( Ok( std::move( info ) ) );
				} );
		} );
}

VFSTask<DirectoryHandle> SFTPProvider::openDirectory( const URI& uri,
													  const DirectoryOpenOptions& options,
													  CancellationToken token ) {
	return withConnection<DirectoryHandle>(
		mImpl, uri, token, [uri, options, token]( std::shared_ptr<SSHConnection> connection ) {
			std::string path = sftpPath( uri );
			std::weak_ptr<SSHConnection> weak = connection;
			auto refine = std::make_shared<std::optional<VFSError>>();
			return connection->submit<DirectoryHandle>(
				token, uri,
				[path, uri, options, weak,
				 refine]( SSHConnection& c ) -> std::optional<VFSResult<DirectoryHandle>> {
					if ( !*refine ) {
						LIBSSH2_SFTP_HANDLE* handle = libssh2_sftp_open_ex(
							c.sftp(), path.c_str(), static_cast<unsigned>( path.size() ), 0, 0,
							LIBSSH2_SFTP_OPENDIR );
						if ( handle ) {
							c.trackHandle( handle );
							return VFSResult<DirectoryHandle>( Ok(
								DirectoryHandle( uri, std::make_shared<SFTPDirectoryImpl>(
														  weak.lock(), uri, handle, options ) ) ) );
						}
						if ( libssh2_session_last_errno( c.session() ) == LIBSSH2_ERROR_EAGAIN )
							return std::nullopt;
						VFSError error = c.lastError( uri, "opendir failed" );
						if ( !isGenericFailure( error ) && error.code != VFSErrorCode::NotFound )
							return VFSResult<DirectoryHandle>( Err( std::move( error ) ) );
						*refine = std::move( error );
					}
					// OpenSSH maps ENOTDIR to SSH_FX_NO_SUCH_FILE (and some servers to a
					// generic failure): report an existing non-directory as such.
					auto attrs = sftpStat( c, uri, path, true );
					if ( !attrs )
						return std::nullopt;
					VFSError error = std::move( **refine );
					if ( *attrs &&
						 typeFromPermissions( attrs->value().permissions ) != FileType::Directory )
						error.code = VFSErrorCode::NotDirectory;
					return VFSResult<DirectoryHandle>( Err( std::move( error ) ) );
				} );
		} );
}

VFSTask<FileHandle> SFTPProvider::open( const URI& uri, const OpenOptions& options,
										CancellationToken token ) {
	auto impl = mImpl;
	return withConnection<FileHandle>(
		mImpl, uri, token,
		[uri, options, token, impl]( std::shared_ptr<SSHConnection> connection ) {
			unsigned long flags = 0;
			if ( hasFlag( options.flags, OpenFlags::Read ) )
				flags |= LIBSSH2_FXF_READ;
			if ( hasFlag( options.flags, OpenFlags::Write ) )
				flags |= LIBSSH2_FXF_WRITE;
			if ( hasFlag( options.flags, OpenFlags::Create ) )
				flags |= LIBSSH2_FXF_CREAT;
			if ( hasFlag( options.flags, OpenFlags::Exclusive ) )
				flags |= LIBSSH2_FXF_EXCL;
			if ( hasFlag( options.flags, OpenFlags::Truncate ) )
				flags |= LIBSSH2_FXF_TRUNC;
			long mode = static_cast<long>( options.createPermissions.value_or( 0644 ) );
			std::string path = sftpPath( uri );
			std::weak_ptr<SSHConnection> weak = connection;
			struct State {
				LIBSSH2_SFTP_HANDLE* handle{ nullptr };
			};
			auto state = std::make_shared<State>();
			bool exclusive = hasFlag( options.flags, OpenFlags::Exclusive );
			ProviderCapabilities caps = impl->capabilities( uri );
			return connection->submit<FileHandle>(
				token, uri,
				[path, uri, flags, mode, weak, state, exclusive,
				 caps]( SSHConnection& c ) -> std::optional<VFSResult<FileHandle>> {
					if ( !state->handle ) {
						state->handle = libssh2_sftp_open_ex( c.sftp(), path.c_str(),
															  static_cast<unsigned>( path.size() ),
															  flags, mode, LIBSSH2_SFTP_OPENFILE );
						if ( !state->handle ) {
							if ( libssh2_session_last_errno( c.session() ) == LIBSSH2_ERROR_EAGAIN )
								return std::nullopt;
							VFSError error = c.lastError( uri, "open failed" );
							if ( exclusive && isGenericFailure( error ) )
								error.code = VFSErrorCode::AlreadyExists;
							return VFSResult<FileHandle>( Err( std::move( error ) ) );
						}
						c.trackHandle( state->handle );
					}
					// Servers happily open directories for reading; report them like local files.
					LIBSSH2_SFTP_ATTRIBUTES attrs{};
					int rc = libssh2_sftp_fstat_ex( state->handle, &attrs, 0 );
					if ( rc == LIBSSH2_ERROR_EAGAIN )
						return std::nullopt;
					if ( rc == 0 && ( attrs.flags & LIBSSH2_SFTP_ATTR_PERMISSIONS ) &&
						 typeFromPermissions( attrs.permissions ) == FileType::Directory ) {
						// Closed asynchronously by the handle object below.
						SFTPFileImpl closer( weak.lock(), uri, state->handle, caps );
						state->handle = nullptr;
						return VFSResult<FileHandle>(
							Err( VFSError( VFSErrorCode::IsDirectory, uri ) ) );
					}
					LIBSSH2_SFTP_HANDLE* handle = state->handle;
					state->handle = nullptr;
					return VFSResult<FileHandle>( Ok( FileHandle(
						uri, std::make_shared<SFTPFileImpl>( weak.lock(), uri, handle, caps ) ) ) );
				} );
		} );
}

VFSTask<void> SFTPProvider::mkdir( const URI& uri, const MkDirOptions& options,
								   CancellationToken token ) {
	long mode = static_cast<long>( options.permissions.value_or( 0777 ) );
	return withConnection<void>(
		mImpl, uri, token, [uri, mode, token]( std::shared_ptr<SSHConnection> connection ) {
			std::string path = sftpPath( uri );
			auto refine = std::make_shared<std::optional<VFSError>>();
			return connection->submit<void>(
				token, uri,
				[path, uri, mode, refine]( SSHConnection& c ) -> std::optional<VFSResult<void>> {
					if ( !*refine ) {
						int rc = libssh2_sftp_mkdir_ex(
							c.sftp(), path.c_str(), static_cast<unsigned>( path.size() ), mode );
						if ( rc == LIBSSH2_ERROR_EAGAIN )
							return std::nullopt;
						if ( rc == 0 )
							return VFSResult<void>( Ok() );
						VFSError error = c.lastError( uri, "mkdir failed" );
						if ( !isGenericFailure( error ) )
							return VFSResult<void>( Err( std::move( error ) ) );
						*refine = std::move( error );
					}
					// OpenSSH reports EEXIST as a generic failure: tell it apart.
					auto exists = sftpStat( c, uri, path, false );
					if ( !exists )
						return std::nullopt;
					VFSError error = std::move( **refine );
					if ( *exists )
						error.code = VFSErrorCode::AlreadyExists;
					return VFSResult<void>( Err( std::move( error ) ) );
				} );
		} );
}

VFSTask<void> SFTPProvider::remove( const URI& uri, const RemoveOptions&,
									CancellationToken token ) {
	return withConnection<void>(
		mImpl, uri, token, [uri, token]( std::shared_ptr<SSHConnection> connection ) {
			std::string path = sftpPath( uri );
			struct State {
				std::optional<bool> directory;
			};
			auto state = std::make_shared<State>();
			return connection->submit<void>(
				token, uri,
				[path, uri, state]( SSHConnection& c ) -> std::optional<VFSResult<void>> {
					if ( !state->directory ) {
						auto attrs = sftpStat( c, uri, path, false );
						if ( !attrs )
							return std::nullopt;
						if ( !*attrs )
							return VFSResult<void>( Err( attrs->error() ) );
						state->directory = typeFromPermissions( attrs->value().permissions ) ==
										   FileType::Directory;
					}
					int rc = *state->directory
								 ? libssh2_sftp_rmdir_ex( c.sftp(), path.c_str(),
														  static_cast<unsigned>( path.size() ) )
								 : libssh2_sftp_unlink_ex( c.sftp(), path.c_str(),
														   static_cast<unsigned>( path.size() ) );
					if ( rc == LIBSSH2_ERROR_EAGAIN )
						return std::nullopt;
					if ( rc == 0 )
						return VFSResult<void>( Ok() );
					VFSError error = c.lastError( uri, "remove failed" );
					// rmdir of a non-empty directory is a generic failure on OpenSSH.
					if ( *state->directory && isGenericFailure( error ) )
						error.code = VFSErrorCode::NotEmpty;
					return VFSResult<void>( Err( std::move( error ) ) );
				} );
		} );
}

VFSTask<void> SFTPProvider::rename( const URI& from, const URI& to, const RenameOptions& options,
									CancellationToken token ) {
	if ( SFTPProviderImpl::authorityKey( from ) != SFTPProviderImpl::authorityKey( to ) )
		return readyError<void>( VFSErrorCode::Unsupported, from,
								 "Rename across SFTP servers is not possible" );
	bool overwrite = options.overwrite;
	return withConnection<void>(
		mImpl, from, token,
		[from, to, overwrite, token]( std::shared_ptr<SSHConnection> connection ) {
			std::string src = sftpPath( from );
			std::string dst = sftpPath( to );
			struct State {
				int phase{ 0 };
				std::optional<VFSError> error;
			};
			auto state = std::make_shared<State>();
			return connection->submit<void>(
				token, from,
				[src, dst, from, to, overwrite,
				 state]( SSHConnection& c ) -> std::optional<VFSResult<void>> {
					auto finish = [&]( int rc,
									   const char* what ) -> std::optional<VFSResult<void>> {
						if ( rc == 0 )
							return VFSResult<void>( Ok() );
						return VFSResult<void>( Err( c.lastError( from, what ) ) );
					};
					if ( overwrite && c.posixRenameSupport == 1 ) {
						// Atomic replace (rename(2) semantics on the server).
						int rc = libssh2_sftp_posix_rename_ex(
							c.sftp(), src.c_str(), static_cast<unsigned>( src.size() ), dst.c_str(),
							static_cast<unsigned>( dst.size() ) );
						if ( rc == LIBSSH2_ERROR_EAGAIN )
							return std::nullopt;
						return finish( rc, "rename failed" );
					}
					if ( state->phase == 0 ) {
						long flags = overwrite ? LIBSSH2_SFTP_RENAME_OVERWRITE |
													 LIBSSH2_SFTP_RENAME_ATOMIC |
													 LIBSSH2_SFTP_RENAME_NATIVE
											   : 0;
						int rc = libssh2_sftp_rename_ex(
							c.sftp(), src.c_str(), static_cast<unsigned>( src.size() ), dst.c_str(),
							static_cast<unsigned>( dst.size() ), flags );
						if ( rc == LIBSSH2_ERROR_EAGAIN )
							return std::nullopt;
						if ( rc == 0 )
							return VFSResult<void>( Ok() );
						VFSError error = c.lastError( from, "rename failed" );
						if ( !isGenericFailure( error ) )
							return VFSResult<void>( Err( std::move( error ) ) );
						state->error = std::move( error );
						state->phase = 1;
					}
					if ( state->phase == 1 ) {
						// SFTP v3 rename refuses existing targets with a generic failure.
						auto exists = sftpStat( c, to, dst, false );
						if ( !exists )
							return std::nullopt;
						if ( !*exists )
							return VFSResult<void>( Err( std::move( *state->error ) ) );
						if ( !overwrite ) {
							state->error->code = VFSErrorCode::AlreadyExists;
							return VFSResult<void>( Err( std::move( *state->error ) ) );
						}
						if ( typeFromPermissions( exists->value().permissions ) ==
							 FileType::Directory )
							return VFSResult<void>(
								Err( VFSError( VFSErrorCode::IsDirectory, to ) ) );
						state->phase = 2;
					}
					if ( state->phase == 2 ) {
						// Without posix-rename the replace cannot be atomic (the provider does
						// not report AtomicReplace in that case).
						int rc = libssh2_sftp_unlink_ex( c.sftp(), dst.c_str(),
														 static_cast<unsigned>( dst.size() ) );
						if ( rc == LIBSSH2_ERROR_EAGAIN )
							return std::nullopt;
						if ( rc != 0 )
							return finish( rc, "rename (replace) failed" );
						state->phase = 3;
					}
					int rc = libssh2_sftp_rename_ex(
						c.sftp(), src.c_str(), static_cast<unsigned>( src.size() ), dst.c_str(),
						static_cast<unsigned>( dst.size() ), 0 );
					if ( rc == LIBSSH2_ERROR_EAGAIN )
						return std::nullopt;
					return finish( rc, "rename failed" );
				} );
		} );
}

VFSTask<URI> SFTPProvider::canonicalize( const URI& uri, CancellationToken token ) {
	return withConnection<URI>(
		mImpl, uri, token, [uri, token]( std::shared_ptr<SSHConnection> connection ) {
			std::string path = sftpPath( uri );
			return connection->submit<URI>(
				token, uri, [path, uri]( SSHConnection& c ) -> std::optional<VFSResult<URI>> {
					char buffer[4096];
					int rc = libssh2_sftp_symlink_ex( c.sftp(), path.c_str(),
													  static_cast<unsigned>( path.size() ), buffer,
													  sizeof( buffer ), LIBSSH2_SFTP_REALPATH );
					if ( rc == LIBSSH2_ERROR_EAGAIN )
						return std::nullopt;
					if ( rc < 0 )
						return VFSResult<URI>( Err( c.lastError( uri, "realpath failed" ) ) );
					URI result( uri );
					result.setQuery( "" );
					result.setFragment( "" );
					result.setRawPath( std::string( buffer, static_cast<size_t>( rc ) ) );
					return VFSResult<URI>( Ok( std::move( result ) ) );
				} );
		} );
}

VFSTask<void> SFTPProvider::setPermissions( const URI& uri, Uint32 permissions,
											CancellationToken token ) {
	return withConnection<void>(
		mImpl, uri, token, [uri, permissions, token]( std::shared_ptr<SSHConnection> connection ) {
			std::string path = sftpPath( uri );
			return connection->submit<void>(
				token, uri,
				[path, uri, permissions]( SSHConnection& c ) -> std::optional<VFSResult<void>> {
					LIBSSH2_SFTP_ATTRIBUTES attrs{};
					attrs.flags = LIBSSH2_SFTP_ATTR_PERMISSIONS;
					attrs.permissions = permissions & 07777;
					int rc = libssh2_sftp_stat_ex( c.sftp(), path.c_str(),
												   static_cast<unsigned>( path.size() ),
												   LIBSSH2_SFTP_SETSTAT, &attrs );
					if ( rc == LIBSSH2_ERROR_EAGAIN )
						return std::nullopt;
					if ( rc != 0 )
						return VFSResult<void>( Err( c.lastError( uri, "chmod failed" ) ) );
					return VFSResult<void>( Ok() );
				} );
		} );
}

VFSTask<SpaceInfo> SFTPProvider::spaceInfo( const URI& uri, CancellationToken token ) {
	return withConnection<SpaceInfo>(
		mImpl, uri, token, [uri, token]( std::shared_ptr<SSHConnection> connection ) {
			std::string path = sftpPath( uri );
			return connection->submit<SpaceInfo>(
				token, uri, [path, uri]( SSHConnection& c ) -> std::optional<VFSResult<SpaceInfo>> {
					if ( c.statvfsSupport == 0 )
						return VFSResult<SpaceInfo>(
							Err( VFSError( VFSErrorCode::Unsupported, uri,
										   "Server lacks statvfs@openssh.com" ) ) );
					LIBSSH2_SFTP_STATVFS st{};
					int rc = libssh2_sftp_statvfs( c.sftp(), path.c_str(), path.size(), &st );
					if ( rc == LIBSSH2_ERROR_EAGAIN )
						return std::nullopt;
					if ( rc != 0 )
						return VFSResult<SpaceInfo>( Err( c.lastError( uri, "statvfs failed" ) ) );
					Uint64 unit = st.f_frsize ? st.f_frsize : st.f_bsize;
					return VFSResult<SpaceInfo>( Ok(
						SpaceInfo{ unit * st.f_blocks, unit * st.f_bfree, unit * st.f_bavail } ) );
				} );
		} );
}

std::shared_ptr<WatchBackend> SFTPProvider::createWatchBackend( VirtualFileSystem& vfs ) {
	return std::make_shared<RemoteWatchBackend>( mImpl, vfs );
}

}} // namespace EE::VFS
