#include "sftpproviderimpl.hpp"
#include "sshconnection.hpp"
#include <eepp/core/string.hpp>
#include <eepp/system/threadpool.hpp>
#include <eepp/vfs/detail/await.hpp>
#include <eepp/vfs/path.hpp>
#include <eepp/vfs/sftp/remotewatchbackend.hpp>
#include <eepp/vfs/sftp/sftpprovider.hpp>
#include <eepp/vfs/virtualfilesystem.hpp>
#include <eepp/vfs/watchers/pollingwatchbackend.hpp>

namespace EE { namespace VFS {

using namespace sftp;
using detail::await;

namespace sftp { namespace {

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

void fillFromAttributes( VFSFileInfo& info, const LIBSSH2_SFTP_ATTRIBUTES& attrs ) {
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
void ownerFromLongName( VFSFileInfo& info, const char* longEntry ) {
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
					if ( rc < 0 ) {
						return VFSResult<DirectoryBatch>(
							Err( c.lastError( base, "readdir failed" ) ) );
					}
					if ( rc == 0 ) {
						state->batch.endOfDirectory = true;
						*eof = true;
						state->listed = true;
						break;
					}
					std::string entryName( name, static_cast<size_t>( rc ) );
					if ( entryName == "." || entryName == ".." )
						continue;
					if ( !VFSPath::validName( entryName, base ) ) {
						return VFSResult<DirectoryBatch>(
							Err( VFSError( VFSErrorCode::ProtocolError, base,
										   "Invalid SFTP directory entry name" ) ) );
					}
					VFSFileInfo info;
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
					VFSFileInfo& link = state->batch.entries[state->links[state->linkIndex]];
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

	VFSTask<VFSFileInfo> stat() override {
		if ( !mHandle )
			return readyError<VFSFileInfo>( VFSErrorCode::InvalidOperation, mUri, "File closed" );
		LIBSSH2_SFTP_HANDLE* handle = mHandle;
		URI uri = mUri;
		return mConnection->submit<VFSFileInfo>(
			{}, mUri, [handle, uri]( SSHConnection& c ) -> std::optional<VFSResult<VFSFileInfo>> {
				LIBSSH2_SFTP_ATTRIBUTES attrs{};
				int rc = libssh2_sftp_fstat_ex( handle, &attrs, 0 );
				if ( rc == LIBSSH2_ERROR_EAGAIN )
					return std::nullopt;
				if ( rc != 0 )
					return VFSResult<VFSFileInfo>( Err( c.lastError( uri, "fstat failed" ) ) );
				VFSFileInfo info;
				info.name = uri.getLastPathSegment();
				info.hidden = !info.name.empty() && info.name[0] == '.';
				fillFromAttributes( info, attrs );
				return VFSResult<VFSFileInfo>( Ok( std::move( info ) ) );
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

}} // namespace sftp

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
		SFTPProviderImpl::HelperThreadScope scope;
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

VFSTask<VFSFileInfo> SFTPProvider::stat( const URI& uri, const StatOptions& options,
										 CancellationToken token ) {
	bool follow = options.followSymlinks;
	return withConnection<VFSFileInfo>(
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
			return connection->submit<VFSFileInfo>(
				token, uri,
				[state, path, uri,
				 follow]( SSHConnection& c ) -> std::optional<VFSResult<VFSFileInfo>> {
					if ( !state->attrs ) {
						auto attrs = sftpStat( c, uri, path, false );
						if ( !attrs )
							return std::nullopt;
						if ( !*attrs )
							return VFSResult<VFSFileInfo>( Err( attrs->error() ) );
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
							return VFSResult<VFSFileInfo>( Err( attrs->error() ) );
						}
					}
					VFSFileInfo info;
					info.name = uri.getLastPathSegment();
					if ( info.name.empty() )
						info.name = "/";
					info.hidden = !info.name.empty() && info.name[0] == '.';
					fillFromAttributes( info,
										follow && targetAttrs ? *targetAttrs : *state->attrs );
					info.symlinkTarget = state->link;
					if ( isLink && !follow )
						info.targetType = state->targetType;
					return VFSResult<VFSFileInfo>( Ok( std::move( info ) ) );
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
	if ( !VFSPath::validURI( from ) || !VFSPath::validURI( to ) )
		return readyError<void>( VFSErrorCode::InvalidURI, to, "NUL in SFTP URI" );
	if ( SFTPProviderImpl::authorityKey( from ) != SFTPProviderImpl::authorityKey( to ) ) {
		return readyError<void>( VFSErrorCode::Unsupported, from,
								 "Rename across SFTP servers is not possible" );
	}
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
							 FileType::Directory ) {
							return VFSResult<void>(
								Err( VFSError( VFSErrorCode::IsDirectory, to ) ) );
						}
					}
					// Never unlink a good destination to emulate replacement: a later rename
					// can fail and leave the user with neither file at the destination.
					return VFSResult<void>(
						Err( VFSError( VFSErrorCode::Unsupported, to,
									   "Server cannot safely replace the destination" ) ) );
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
					if ( c.statvfsSupport == 0 ) {
						return VFSResult<SpaceInfo>(
							Err( VFSError( VFSErrorCode::Unsupported, uri,
										   "Server lacks statvfs@openssh.com" ) ) );
					}
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
