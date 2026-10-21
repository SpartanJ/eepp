#include <eepp/config.hpp>

#if EE_PLATFORM != EE_PLATFORM_WIN

#include <eepp/vfs/path.hpp>
#include <eepp/vfs/providers/localprovider.hpp>

#include <atomic>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <eepp/core/string.hpp>
#include <eepp/system/filesystem.hpp>
#include <fcntl.h>
#include <grp.h>
#include <mutex>
#include <pwd.h>
#include <shared_mutex>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/types.h>
#include <unistd.h>
#if defined( __linux__ )
#include <sys/syscall.h>
#ifndef RENAME_NOREPLACE
#define RENAME_NOREPLACE ( 1 << 0 )
#endif
#endif
#if defined( __APPLE__ ) || defined( __FreeBSD__ )
#include <stdio.h>
#endif

using namespace EE::System;

namespace EE { namespace VFS {

using Route = LocalIOScheduler::Route;

ProviderCapabilities LocalProvider::capabilities( const URI& ) const {
	ProviderCapabilities caps =
		VFSCapability::Read | VFSCapability::Write | VFSCapability::List | VFSCapability::Create |
		VFSCapability::Remove | VFSCapability::RecursiveRemove | VFSCapability::Rename |
		VFSCapability::Seek | VFSCapability::RandomAccess | VFSCapability::Truncate |
		VFSCapability::Symlinks | VFSCapability::Permissions | VFSCapability::AtomicRename |
		VFSCapability::AtomicReplace | VFSCapability::Fsync | VFSCapability::FreeSpace |
		VFSCapability::HardLinks | VFSCapability::Ownership;
#if EE_PLATFORM != EE_PLATFORM_MACOS
	// macOS file systems are case-insensitive by default.
	caps.set( VFSCapability::CaseSensitive );
#endif
	return caps;
}

VFSResult<std::string> LocalProvider::toNativePath( const URI& uri ) {
	if ( !VFSPath::validURI( uri ) )
		return Err( VFSError( VFSErrorCode::InvalidURI, uri, "NUL in file URI" ) );
	if ( uri.getScheme() != "file" )
		return Err( VFSError( VFSErrorCode::InvalidURI, uri, "Not a file:// URI" ) );
	if ( !uri.getHost().empty() && uri.getHost() != "localhost" )
		return Err( VFSError( VFSErrorCode::Unsupported, uri, "Remote file:// hosts" ) );
	if ( uri.getPath().empty() )
		return Err( VFSError( VFSErrorCode::InvalidURI, uri, "Empty path" ) );
	return Ok( uri.getFSPath() );
}

namespace {

VFSErrorCode mapErrno( int error ) {
	switch ( error ) {
		case ENOENT:
			return VFSErrorCode::NotFound;
		case EEXIST:
			return VFSErrorCode::AlreadyExists;
		case ENOTDIR:
			return VFSErrorCode::NotDirectory;
		case EISDIR:
			return VFSErrorCode::IsDirectory;
		case EACCES:
		case EPERM:
			return VFSErrorCode::PermissionDenied;
		case EROFS:
			return VFSErrorCode::ReadOnly;
		case ENOTEMPTY:
			return VFSErrorCode::NotEmpty;
		case ELOOP:
			return VFSErrorCode::TooManySymlinks;
		case ENOSPC:
#ifdef EDQUOT
		case EDQUOT:
#endif
			return VFSErrorCode::NoSpace;
		case EXDEV:
		case ENOTSUP:
#if defined( EOPNOTSUPP ) && EOPNOTSUPP != ENOTSUP
		case EOPNOTSUPP:
#endif
			return VFSErrorCode::Unsupported;
		case ENAMETOOLONG:
			return VFSErrorCode::InvalidURI;
		case EINVAL:
			return VFSErrorCode::InvalidOperation;
		case ETIMEDOUT:
			return VFSErrorCode::TimedOut;
		default:
			return VFSErrorCode::IO;
	}
}

VFSError errnoError( const std::string& path, const char* what, int error = errno ) {
	std::string message = String::format( "%s: %s", what, std::strerror( error ) );
	return VFSError( mapErrno( error ), URI::fromFSPath( path ), std::move( message ), error );
}

FileType typeFromMode( mode_t mode ) {
	if ( S_ISREG( mode ) )
		return FileType::Regular;
	if ( S_ISDIR( mode ) )
		return FileType::Directory;
	if ( S_ISLNK( mode ) )
		return FileType::Symlink;
	if ( S_ISBLK( mode ) )
		return FileType::BlockDevice;
	if ( S_ISCHR( mode ) )
		return FileType::CharacterDevice;
	if ( S_ISFIFO( mode ) )
		return FileType::FIFO;
	if ( S_ISSOCK( mode ) )
		return FileType::Socket;
	return FileType::Unknown;
}

Uint64 modifiedNanoseconds( const struct stat& st ) {
#if defined( __APPLE__ )
	return static_cast<Uint64>( st.st_mtimespec.tv_sec ) * 1000000000ull + st.st_mtimespec.tv_nsec;
#elif defined( __linux__ ) || defined( __FreeBSD__ ) || defined( __NetBSD__ ) || \
	defined( __OpenBSD__ )
	return static_cast<Uint64>( st.st_mtim.tv_sec ) * 1000000000ull + st.st_mtim.tv_nsec;
#else
	return static_cast<Uint64>( st.st_mtime ) * 1000000000ull;
#endif
}

void fillFromStat( VFSFileInfo& info, const struct stat& st ) {
	info.type = typeFromMode( st.st_mode );
	if ( info.type != FileType::Directory )
		info.setSize( static_cast<Uint64>( st.st_size ) );
	if ( st.st_blocks >= 0 )
		info.setAllocatedSize( static_cast<Uint64>( st.st_blocks ) * 512 );
	info.setIdentity( { static_cast<Uint64>( st.st_dev ), static_cast<Uint64>( st.st_ino ) },
					  st.st_nlink > 1 );
	info.setModifiedTime( static_cast<Int64>( st.st_mtime ) );
	info.setAccessedTime( static_cast<Int64>( st.st_atime ) );
#if defined( __APPLE__ ) || defined( __FreeBSD__ ) || defined( __NetBSD__ )
	info.setCreatedTime( static_cast<Int64>( st.st_birthtime ) );
#endif
	info.setPermissions( static_cast<Uint32>( st.st_mode & 07777 ) );
	info.setUid( static_cast<Uint32>( st.st_uid ) );
	info.setGid( static_cast<Uint32>( st.st_gid ) );
	info.revision = FileRevision::fromValues(
		{ static_cast<Uint64>( st.st_dev ), static_cast<Uint64>( st.st_ino ),
		  modifiedNanoseconds( st ), static_cast<Uint64>( st.st_size ) } );
}

std::string readLink( int dirFd, const char* name ) {
	char buffer[4096];
	ssize_t len = dirFd >= 0 ? readlinkat( dirFd, name, buffer, sizeof( buffer ) )
							 : readlink( name, buffer, sizeof( buffer ) );
	if ( len <= 0 )
		return {};
	return std::string( buffer, static_cast<size_t>( len ) );
}

std::mutex sNamesMutex;
std::unordered_map<Uint32, std::string> sUserNames;
std::unordered_map<Uint32, std::string> sGroupNames;

void fillOwnerNames( VFSFileInfo& info ) {
	if ( auto uid = info.uid() )
		info.owner = LocalProvider::userName( *uid );
	if ( auto gid = info.gid() )
		info.group = LocalProvider::groupName( *gid );
}

class LocalDirectoryImpl : public DirectoryHandleImpl,
						   public std::enable_shared_from_this<LocalDirectoryImpl> {
  public:
	/** @param physicalPath path with its links followed (LocalIOScheduler::resolvePath()). */
	LocalDirectoryImpl( std::shared_ptr<LocalIOScheduler> scheduler, URI uri, std::string path,
						const std::string& physicalPath, DIR* dir, DirectoryOpenOptions options ) :
		mScheduler( std::move( scheduler ) ),
		mRoute( mScheduler->routeFor( physicalPath ) ),
		mUri( std::move( uri ) ),
		mPath( std::move( path ) ),
		mNetworkMounts( LocalIOScheduler::networkMountsIn( physicalPath ) ),
		mDir( dir ),
		mOptions( options ) {}

	~LocalDirectoryImpl() override { closeNow(); }

	VFSTask<DirectoryBatch> read( size_t maxEntries, CancellationToken token ) override {
		return mScheduler->submit<DirectoryBatch>(
			mRoute, [self = shared_from_this(), maxEntries, token]() -> VFSResult<DirectoryBatch> {
				auto batch = self->readSync( maxEntries, token );
				// After releasing the mutex: a close() that could not take it set the flag first.
				if ( self->mClosePending )
					self->tryClose();
				return batch;
			} );
	}

	/** Never waits: a read in progress holds the mutex for as long as its file system takes (an
	 * unresponsive server, until the server answers), and the caller may be the UI thread. That
	 * read closes the directory when it ends. */
	VFSTask<void> close() override {
		mClosePending = true;
		tryClose();
		return readyValue<void>();
	}

  private:
	void tryClose() {
		std::unique_lock<std::mutex> lock( mMutex, std::try_to_lock );
		if ( lock.owns_lock() )
			closeNow();
	}

	VFSResult<DirectoryBatch> readSync( size_t maxEntries, const CancellationToken& token ) {
		std::lock_guard<std::mutex> lock( mMutex );
		DirectoryBatch batch;
		if ( token.isCancellationRequested() )
			return Err( cancelledError( mUri ) );
		if ( !mDir ) {
			batch.endOfDirectory = true;
			return Ok( std::move( batch ) );
		}
		batch.entries.reserve( std::min<size_t>( maxEntries, 1024 ) );
		const int dirFd = dirfd( mDir );
		const bool full = mOptions.metadata == DirectoryMetadata::Full;
		while ( batch.entries.size() < maxEntries ) {
			errno = 0;
			struct dirent* entry = readdir( mDir );
			if ( !entry ) {
				if ( errno != 0 )
					return Err( errnoError( mPath, "readdir failed" ) );
				closeNow();
				batch.endOfDirectory = true;
				break;
			}
			const char* name = entry->d_name;
			if ( name[0] == '.' && ( name[1] == 0 || ( name[1] == '.' && name[2] == 0 ) ) )
				continue;
			// Built in place: a VFSFileInfo is large, moving it into the batch is measurable.
			VFSFileInfo& info = batch.entries.emplace_back();
			info.name = name;
			info.hidden = name[0] == '.';
			// A network mount's root: reading its metadata would ask its server, which may not
			// answer. It is listed as the folder it is, without metadata.
			if ( !mNetworkMounts.empty() && std::find( mNetworkMounts.begin(), mNetworkMounts.end(),
													   info.name ) != mNetworkMounts.end() ) {
				info.type = FileType::Directory;
				continue;
			}
#ifdef DT_UNKNOWN
			switch ( entry->d_type ) {
				case DT_REG:
					info.type = FileType::Regular;
					break;
				case DT_DIR:
					info.type = FileType::Directory;
					break;
				case DT_LNK:
					info.type = FileType::Symlink;
					break;
				case DT_BLK:
					info.type = FileType::BlockDevice;
					break;
				case DT_CHR:
					info.type = FileType::CharacterDevice;
					break;
				case DT_FIFO:
					info.type = FileType::FIFO;
					break;
				case DT_SOCK:
					info.type = FileType::Socket;
					break;
				default:
					break;
			}
#endif
			if ( full || mOptions.metadata == DirectoryMetadata::Storage ||
				 info.type == FileType::Unknown ) {
				struct stat st;
				if ( fstatat( dirFd, name, &st, AT_SYMLINK_NOFOLLOW ) == 0 ) {
					fillFromStat( info, st );
					if ( full )
						fillOwnerNames( info );
				}
			}
			if ( info.type == FileType::Symlink ) {
				if ( full || mOptions.resolveSymlinkTargets )
					info.symlinkTarget = readLink( dirFd, name );
				// Not into a network mount: following the link would ask its server.
				if ( mOptions.resolveSymlinkTargets &&
					 !pointsIntoNetworkMount( info.symlinkTarget ) ) {
					struct stat target;
					if ( fstatat( dirFd, name, &target, 0 ) == 0 )
						info.targetType = typeFromMode( target.st_mode );
				}
				if ( !full )
					info.symlinkTarget.clear();
			}
		}
		return Ok( std::move( batch ) );
	}

	void closeNow() {
		if ( mDir ) {
			closedir( mDir );
			mDir = nullptr;
		}
	}

	bool pointsIntoNetworkMount( const std::string& target ) const {
		if ( target.empty() )
			return false;
		return LocalIOScheduler::findNetworkMount(
				   LocalIOScheduler::resolvePath( target[0] == '/' ? target : mPath + "/" + target,
												  true ) )
			.has_value();
	}

	std::shared_ptr<LocalIOScheduler> mScheduler;
	Route mRoute;
	URI mUri;
	std::string mPath;
	/** The network mounts in this directory, never stat'ed. */
	std::vector<std::string> mNetworkMounts;
	std::mutex mMutex;
	std::atomic<bool> mClosePending{ false };
	DIR* mDir;
	DirectoryOpenOptions mOptions;
};

class LocalFileImpl : public FileHandleImpl, public std::enable_shared_from_this<LocalFileImpl> {
  public:
	LocalFileImpl( std::shared_ptr<LocalIOScheduler> scheduler, Route route, URI uri,
				   std::string path, int fd, ProviderCapabilities caps ) :
		mScheduler( std::move( scheduler ) ),
		mRoute( std::move( route ) ),
		mUri( std::move( uri ) ),
		mPath( std::move( path ) ),
		mFd( fd ),
		mCaps( caps ) {}

	~LocalFileImpl() override {
		if ( mFd >= 0 )
			::close( mFd );
	}

	VFSTask<size_t> read( Uint64 offset, std::span<Uint8> data, CancellationToken token ) override {
		if ( !mRoute.isNetworkMount() ) {
			return submit<size_t>( token, [offset, data]( LocalFileImpl& self ) {
				return self.readInto( offset, data );
			} );
		}
		// The call can outlive the caller's wait (TimedOut), and with it data: it reads into a
		// buffer of its own, copied into data only while the caller still waits.
		return submitCommitted<std::vector<Uint8>, size_t>(
			token,
			[offset, size = data.size()]( LocalFileImpl& self ) -> VFSResult<std::vector<Uint8>> {
				std::vector<Uint8> buffer( size );
				auto read = self.readInto( offset, buffer );
				if ( !read )
					return Err( std::move( read ).error() );
				buffer.resize( read.value() );
				return Ok( std::move( buffer ) );
			},
			[data]( VFSResult<std::vector<Uint8>>&& read ) -> VFSResult<size_t> {
				if ( !read )
					return Err( std::move( read ).error() );
				std::copy( read.value().begin(), read.value().end(), data.begin() );
				return Ok( read.value().size() );
			} );
	}

	VFSTask<size_t> write( Uint64 offset, std::span<const Uint8> data,
						   CancellationToken token ) override {
		if ( !mRoute.isNetworkMount() ) {
			return submit<size_t>( token, [offset, data]( LocalFileImpl& self ) {
				return self.writeFrom( offset, data );
			} );
		}
		// As read(): the call may outlive data, so it writes from a copy.
		return submit<size_t>(
			token, [offset, copy = std::vector<Uint8>( data.begin(), data.end() )](
					   LocalFileImpl& self ) { return self.writeFrom( offset, copy ); } );
	}

	VFSTask<void> truncate( Uint64 size ) override {
		return submit<void>( {}, [size]( LocalFileImpl& self ) -> VFSResult<void> {
			if ( ::ftruncate( self.mFd, static_cast<off_t>( size ) ) != 0 )
				return Err( errnoError( self.mPath, "truncate failed" ) );
			return Ok();
		} );
	}

	VFSTask<void> flush() override {
		return submit<void>( {}, []( LocalFileImpl& self ) -> VFSResult<void> {
#if defined( __APPLE__ )
			// fsync() on macOS does not flush the drive cache; F_FULLFSYNC does.
			if ( ::fcntl( self.mFd, F_FULLFSYNC ) == 0 )
				return Ok();
#endif
			if ( ::fsync( self.mFd ) != 0 )
				return Err( errnoError( self.mPath, "fsync failed" ) );
			return Ok();
		} );
	}

	VFSTask<VFSFileInfo> stat() override {
		return submit<VFSFileInfo>( {}, []( LocalFileImpl& self ) -> VFSResult<VFSFileInfo> {
			struct stat st;
			if ( ::fstat( self.mFd, &st ) != 0 )
				return Err( errnoError( self.mPath, "fstat failed" ) );
			VFSFileInfo info;
			info.name = FileSystem::fileNameFromPath( self.mPath );
			info.hidden = !info.name.empty() && info.name[0] == '.';
			fillFromStat( info, st );
			return Ok( std::move( info ) );
		} );
	}

	VFSTask<void> close() override {
		return mScheduler->submit<void>( mRoute, [self = shared_from_this()]() -> VFSResult<void> {
			// Waits for in-flight operations on this descriptor.
			std::unique_lock<std::shared_mutex> lock( self->mMutex );
			if ( self->mFd >= 0 ) {
				int result = ::close( self->mFd );
				self->mFd = -1;
				if ( result != 0 && errno != EINTR )
					return Err( errnoError( self->mPath, "close failed" ) );
			}
			return Ok();
		} );
	}

	ProviderCapabilities capabilities() const override { return mCaps; }

  private:
	VFSResult<size_t> readInto( Uint64 offset, std::span<Uint8> data ) {
		size_t done = 0;
		while ( done < data.size() ) {
			ssize_t got = ::pread( mFd, data.data() + done, data.size() - done,
								   static_cast<off_t>( offset + done ) );
			if ( got < 0 ) {
				if ( errno == EINTR )
					continue;
				return Err( errnoError( mPath, "read failed" ) );
			}
			if ( got == 0 )
				break;
			done += static_cast<size_t>( got );
		}
		return Ok( done );
	}

	VFSResult<size_t> writeFrom( Uint64 offset, std::span<const Uint8> data ) {
		size_t done = 0;
		while ( done < data.size() ) {
			ssize_t put = ::pwrite( mFd, data.data() + done, data.size() - done,
									static_cast<off_t>( offset + done ) );
			if ( put < 0 ) {
				if ( errno == EINTR )
					continue;
				return Err( errnoError( mPath, "write failed" ) );
			}
			done += static_cast<size_t>( put );
		}
		return Ok( done );
	}

	/** Runs fn( *this ) -> VFSResult<T> where the file's calls run, unless cancelled or closed. */
	template <typename T, typename F> auto guarded( CancellationToken token, F&& fn ) {
		return [self = shared_from_this(), fn = std::forward<F>( fn ),
				token = std::move( token )]() mutable -> VFSResult<T> {
			if ( token.isCancellationRequested() )
				return Err( cancelledError( self->mUri ) );
			std::shared_lock<std::shared_mutex> lock( self->mMutex );
			if ( self->mFd < 0 )
				return Err( VFSError( VFSErrorCode::InvalidOperation, self->mUri, "File closed" ) );
			return fn( *self );
		};
	}

	template <typename T, typename F> VFSTask<T> submit( CancellationToken token, F&& fn ) {
		return mScheduler->submit<T>( mRoute,
									  guarded<T>( std::move( token ), std::forward<F>( fn ) ) );
	}

	/** submit() of fn -> VFSResult<T>, then commit( VFSResult<T>&& ) -> VFSResult<U> (see
	 * LocalIOScheduler::submitCommitted()). */
	template <typename T, typename U, typename F, typename C>
	VFSTask<U> submitCommitted( CancellationToken token, F&& fn, C&& commit ) {
		return mScheduler->submitCommitted<U>(
			mRoute, guarded<T>( std::move( token ), std::forward<F>( fn ) ),
			std::forward<C>( commit ) );
	}

	std::shared_ptr<LocalIOScheduler> mScheduler;
	Route mRoute;
	URI mUri;
	std::string mPath;
	std::shared_mutex mMutex;
	int mFd;
	ProviderCapabilities mCaps;
};

} // namespace

std::string LocalProvider::userName( Uint32 uid ) {
	// Directory entries almost always share an owner: a per-thread last-hit cache avoids the
	// shared lock on the listing hot path.
	thread_local std::pair<Uint32, std::string> lastHit{ ~0u, {} };
	if ( lastHit.first == uid )
		return lastHit.second;
	std::string name = userNameUncached( uid );
	lastHit = { uid, name };
	return name;
}

std::string LocalProvider::userNameUncached( Uint32 uid ) {
	{
		std::lock_guard<std::mutex> lock( sNamesMutex );
		auto it = sUserNames.find( uid );
		if ( it != sUserNames.end() )
			return it->second;
	}
	std::string name;
	struct passwd pwd;
	struct passwd* result = nullptr;
	char buffer[4096];
	if ( getpwuid_r( static_cast<uid_t>( uid ), &pwd, buffer, sizeof( buffer ), &result ) == 0 &&
		 result )
		name = result->pw_name;
	std::lock_guard<std::mutex> lock( sNamesMutex );
	sUserNames[uid] = name;
	return name;
}

std::string LocalProvider::groupName( Uint32 gid ) {
	thread_local std::pair<Uint32, std::string> lastHit{ ~0u, {} };
	if ( lastHit.first == gid )
		return lastHit.second;
	std::string name = groupNameUncached( gid );
	lastHit = { gid, name };
	return name;
}

std::string LocalProvider::groupNameUncached( Uint32 gid ) {
	{
		std::lock_guard<std::mutex> lock( sNamesMutex );
		auto it = sGroupNames.find( gid );
		if ( it != sGroupNames.end() )
			return it->second;
	}
	std::string name;
	struct group grp;
	struct group* result = nullptr;
	char buffer[8192];
	if ( getgrgid_r( static_cast<gid_t>( gid ), &grp, buffer, sizeof( buffer ), &result ) == 0 &&
		 result )
		name = result->gr_name;
	std::lock_guard<std::mutex> lock( sNamesMutex );
	sGroupNames[gid] = name;
	return name;
}

VFSResult<VFSFileInfo> LocalProvider::statSync( const URI& uri, const std::string& path,
												bool followSymlinks ) {
	struct stat st;
	if ( ::lstat( path.c_str(), &st ) != 0 )
		return Err( errnoError( path, "stat failed" ) );
	VFSFileInfo info;
	// The root keeps its name ("/"): it has no last component.
	info.name = path == "/" ? path : FileSystem::fileNameFromPath( path );
	info.hidden = !info.name.empty() && info.name[0] == '.';
	if ( S_ISLNK( st.st_mode ) ) {
		info.symlinkTarget = readLink( -1, path.c_str() );
		// Without following, the target's type is a detail: not worth asking a network mount's
		// server for it. Following is what the caller asked for.
		const std::string& link = info.symlinkTarget;
		const bool intoNetworkMount =
			!followSymlinks && !link.empty() &&
			LocalIOScheduler::findNetworkMount(
				LocalIOScheduler::resolvePath(
					link[0] == '/' ? link : FileSystem::fileRemoveFileName( path ) + link, true ) )
				.has_value();
		struct stat target;
		bool resolved = !intoNetworkMount && ::stat( path.c_str(), &target ) == 0;
		if ( followSymlinks ) {
			if ( !resolved )
				return Err( errnoError( path, "stat failed" ) );
			fillFromStat( info, target );
		} else {
			fillFromStat( info, st );
			if ( resolved )
				info.targetType = typeFromMode( target.st_mode );
		}
	} else {
		fillFromStat( info, st );
	}
	fillOwnerNames( info );
	return Ok( std::move( info ) );
}

VFSTask<VFSFileInfo> LocalProvider::stat( const URI& uri, const StatOptions& options,
										  CancellationToken token ) {
	const bool follow = options.followSymlinks;
	return run<VFSFileInfo>(
		uri, follow ? LastLink::Follow : LastLink::Keep, std::move( token ),
		[uri, follow]( const std::string& path ) { return statSync( uri, path, follow ); } );
}

VFSTask<DirectoryHandle> LocalProvider::openDirectory( const URI& uri,
													   const DirectoryOpenOptions& options,
													   CancellationToken token ) {
	return run<DirectoryHandle>(
		uri, LastLink::Follow, std::move( token ),
		[scheduler = mScheduler, uri,
		 options]( const std::string& path ) -> VFSResult<DirectoryHandle> {
			DIR* dir = ::opendir( path.c_str() );
			if ( !dir )
				return Err( errnoError( path, "opendir failed" ) );
			return Ok( DirectoryHandle( uri, std::make_shared<LocalDirectoryImpl>(
												 scheduler, uri, path,
												 LocalIOScheduler::resolvePath( path, true ), dir,
												 options ) ) );
		} );
}

VFSTask<FileHandle> LocalProvider::open( const URI& uri, const OpenOptions& options,
										 CancellationToken token ) {
	return run<FileHandle>(
		uri, LastLink::Follow, std::move( token ),
		[scheduler = mScheduler, uri, options,
		 caps = capabilities( uri )]( const std::string& path ) -> VFSResult<FileHandle> {
			OpenFlags flags = options.flags;
			int oflags = O_CLOEXEC;
			bool canRead = hasFlag( flags, OpenFlags::Read );
			bool canWrite = hasFlag( flags, OpenFlags::Write );
			oflags |= canRead && canWrite ? O_RDWR : ( canWrite ? O_WRONLY : O_RDONLY );
			if ( hasFlag( flags, OpenFlags::Create ) )
				oflags |= O_CREAT;
			if ( hasFlag( flags, OpenFlags::Exclusive ) )
				oflags |= O_EXCL;
			if ( hasFlag( flags, OpenFlags::Truncate ) )
				oflags |= O_TRUNC;
			mode_t mode = static_cast<mode_t>( options.createPermissions.value_or( 0666 ) );
			int fd;
			do {
				fd = ::open( path.c_str(), oflags, mode );
			} while ( fd < 0 && errno == EINTR );
			if ( fd < 0 )
				return Err( errnoError( path, "open failed" ) );
			struct stat st;
			if ( ::fstat( fd, &st ) == 0 && S_ISDIR( st.st_mode ) ) {
				::close( fd );
				return Err( VFSError( VFSErrorCode::IsDirectory, uri ) );
			}
			return Ok( FileHandle( uri, std::make_shared<LocalFileImpl>(
											scheduler, scheduler->resolvedRouteFor( path, true ),
											uri, path, fd, caps ) ) );
		} );
}

VFSTask<void> LocalProvider::mkdir( const URI& uri, const MkDirOptions& options,
									CancellationToken token ) {
	return run<void>( uri, LastLink::Keep, std::move( token ),
					  [mode = options.permissions.value_or( 0777 )](
						  const std::string& path ) -> VFSResult<void> {
						  if ( ::mkdir( path.c_str(), static_cast<mode_t>( mode ) ) != 0 )
							  return Err( errnoError( path, "mkdir failed" ) );
						  return Ok();
					  } );
}

namespace {

/** Keep every ancestor open: renaming an ancestor or replacing it with a link must not
 * redirect subsequent child operations. Diagnostic paths are never used for child syscalls. */
class LocalTreeRemover {
  public:
	static VFSResult<void> remove( std::string path, const CancellationToken& token,
								   bool networkMount ) {
		while ( path.size() > 1 && path.back() == '/' )
			path.pop_back();
		const size_t slash = path.find_last_of( '/' );
		const std::string name = path.substr( slash == std::string::npos ? 0 : slash + 1 );
		if ( name.empty() || name == "." || name == ".." ) {
			return Err( VFSError( VFSErrorCode::InvalidOperation, URI::fromFSPath( path ),
								  "Recursive removal requires a named entry" ) );
		}
		// Resolve only the supplied parent for mount routing/diagnostics. Descendant operations
		// below are descriptor-relative and never resolve their mutable pathname again.
		if ( !networkMount )
			path = LocalIOScheduler::resolvePath( path, false );
		const size_t parentSlash = path.find_last_of( '/' );
		const std::string parent = parentSlash == std::string::npos
									   ? "."
									   : ( parentSlash == 0 ? "/" : path.substr( 0, parentSlash ) );
		// Removing a known file needs search/write permission on its parent, not read permission.
		int flags = O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC;
#if defined( O_SEARCH )
		flags |= O_SEARCH;
#elif defined( O_PATH )
		flags |= O_PATH;
#else
		flags |= O_RDONLY;
#endif
		int fd;
		do {
			fd = ::open( parent.c_str(), flags );
		} while ( fd < 0 && errno == EINTR );
		if ( fd < 0 ) {
			const int error = errno;
			struct stat metadata;
			if ( ( error == ELOOP || error == ENOTDIR ) &&
				 ::lstat( parent.c_str(), &metadata ) == 0 && S_ISLNK( metadata.st_mode ) ) {
				return Err( VFSError( VFSErrorCode::Unsupported, URI::fromFSPath( path ),
									  "Recursive removal requires a resolved parent; use the real "
									  "directory path instead of a network-mount symlink",
									  error ) );
			}
			return Err( errnoError( path, "Cannot open removal parent", error ) );
		}
		struct Parent {
			int fd;

			~Parent() { ::close( fd ); }
		} directory{ fd };
		return removeAt( directory.fd, name.c_str(), path, token, 0 );
	}

  private:
	struct CloseDirectory {
		void operator()( DIR* directory ) const { ::closedir( directory ); }
	};

	using Directory = std::unique_ptr<DIR, CloseDirectory>;

	static VFSResult<void> changed( const std::string& path ) {
		return Err( VFSError( VFSErrorCode::ChangedExternally, URI::fromFSPath( path ),
							  "Directory changed during recursive removal" ) );
	}

	static VFSResult<void> removeAt( int parent, const char* name, std::string& path,
									 const CancellationToken& token, unsigned depth ) {
		if ( token.isCancellationRequested() )
			return Err( cancelledError( URI::fromFSPath( path ) ) );
		// Bound both open descriptors and recursive stack use. Worker threads on macOS can
		// have a 512 KiB stack; retaining 512 frames would exceed that before the depth check.
		if ( depth > 256 ) {
			return Err( VFSError( VFSErrorCode::TooManySymlinks, URI::fromFSPath( path ),
								  "Directory tree too deep" ) );
		}
		struct stat before;
		if ( ::fstatat( parent, name, &before, AT_SYMLINK_NOFOLLOW ) != 0 )
			return Err( errnoError( path, "Cannot stat removal entry" ) );
		if ( !S_ISDIR( before.st_mode ) ) {
			// unlinkat without AT_REMOVEDIR cannot follow a link or remove a directory that
			// replaced this entry after the stat.
			if ( ::unlinkat( parent, name, 0 ) != 0 )
				return Err( errnoError( path, "Cannot remove entry" ) );
			return Ok();
		}
		int fd;
		do {
			fd = ::openat( parent, name, O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC );
		} while ( fd < 0 && errno == EINTR );
		if ( fd < 0 )
			return Err(
				errnoError( path, "Cannot open removal directory without following links" ) );
		DIR* stream = ::fdopendir( fd );
		if ( !stream ) {
			const int error = errno;
			::close( fd );
			return Err( errnoError( path, "Cannot read removal directory", error ) );
		}
		Directory directory( stream );
		struct stat opened;
		if ( ::fstat( fd, &opened ) != 0 )
			return Err( errnoError( path, "Cannot stat opened removal directory" ) );
		if ( opened.st_dev != before.st_dev || opened.st_ino != before.st_ino )
			return changed( path );
		// Do not enter a nested network mount from this directory's I/O lane. Its server may
		// be unreachable; even stat'ing that entry would block this lane on a different mount.
		const auto networkMounts = LocalIOScheduler::networkMountsIn( path );
		const size_t pathLength = path.size();
		while ( true ) {
			if ( token.isCancellationRequested() )
				return Err( cancelledError( URI::fromFSPath( path ) ) );
			errno = 0;
			dirent* entry = ::readdir( stream );
			if ( !entry ) {
				if ( errno != 0 )
					return Err( errnoError( path, "Cannot read removal directory" ) );
				break;
			}
			if ( std::strcmp( entry->d_name, "." ) == 0 || std::strcmp( entry->d_name, ".." ) == 0 )
				continue;
			if ( std::find( networkMounts.begin(), networkMounts.end(), entry->d_name ) !=
				 networkMounts.end() ) {
				return Err( VFSError( VFSErrorCode::Unsupported, URI::fromFSPath( path ),
									  "Remove nested network mounts separately" ) );
			}
			path.push_back( '/' );
			path.append( entry->d_name );
			auto result = removeAt( fd, entry->d_name, path, token, depth + 1 );
			path.resize( pathLength );
			if ( !result )
				return result;
		}
		// This check preserves a detected replacement. The final rmdir can race with another
		// rename, but it cannot follow a symlink or remove any replacement directory's contents.
		struct stat current;
		if ( ::fstatat( parent, name, &current, AT_SYMLINK_NOFOLLOW ) != 0 )
			return Err( errnoError( path, "Cannot check removed directory identity" ) );
		if ( current.st_dev != opened.st_dev || current.st_ino != opened.st_ino )
			return changed( path );
		if ( token.isCancellationRequested() )
			return Err( cancelledError( URI::fromFSPath( path ) ) );
		if ( ::unlinkat( parent, name, AT_REMOVEDIR ) != 0 )
			return Err( errnoError( path, "Cannot remove directory" ) );
		return Ok();
	}
};

} // namespace

VFSTask<void> LocalProvider::remove( const URI& uri, const RemoveOptions& options,
									 CancellationToken token ) {
	if ( options.recursive ) {
		return run<void>( uri, LastLink::Keep, token,
						  [scheduler = mScheduler, token]( const std::string& path ) {
							  return LocalTreeRemover::remove(
								  path, token, scheduler->routeFor( path ).isNetworkMount() );
						  } );
	}
	return run<void>(
		uri, LastLink::Keep, std::move( token ), []( const std::string& path ) -> VFSResult<void> {
			struct stat st;
			if ( ::lstat( path.c_str(), &st ) != 0 )
				return Err( errnoError( path, "remove failed" ) );
			int result = S_ISDIR( st.st_mode ) ? ::rmdir( path.c_str() ) : ::unlink( path.c_str() );
			if ( result != 0 )
				return Err( errnoError( path, "remove failed" ) );
			return Ok();
		} );
}

namespace {

/** Publish without replacing any destination, including one created concurrently. */
VFSResult<void> renameNoReplace( const std::string& from, const std::string& to ) {
	int result = -1;
	errno = ENOTSUP;
#if defined( __linux__ ) && defined( SYS_renameat2 )
	result = static_cast<int>( ::syscall( SYS_renameat2, AT_FDCWD, from.c_str(), AT_FDCWD,
										  to.c_str(), RENAME_NOREPLACE ) );
#elif defined( __APPLE__ ) && defined( RENAME_EXCL )
	result = ::renamex_np( from.c_str(), to.c_str(), RENAME_EXCL );
#elif defined( __FreeBSD__ ) && defined( AT_RENAME_NOREPLACE )
	// Older FreeBSD headers (including 14.x) do not expose this API or flag.
	result = ::renameat2( AT_FDCWD, from.c_str(), AT_FDCWD, to.c_str(), AT_RENAME_NOREPLACE );
#endif
	if ( result == 0 )
		return Ok();
	if ( errno != ENOSYS && errno != EINVAL && errno != ENOTSUP && errno != EOPNOTSUPP )
		return Err( errnoError( from, "rename failed" ) );

	// link/unlink cannot rename a public source safely: another process could replace its
	// pathname after link() and before unlink(). The transfer engine uses revision-checked
	// copy/remove instead; FileWriter alone may publish an exclusively owned temporary.
	struct stat destination;
	if ( ::lstat( to.c_str(), &destination ) == 0 )
		return Err( errnoError( to, "Rename destination already exists", EEXIST ) );
	if ( errno != ENOENT )
		return Err( errnoError( to, "Cannot check rename destination" ) );
	return Err( VFSError( VFSErrorCode::Unsupported, URI::fromFSPath( from ),
						  "Native no-replace rename is unavailable" ) );
}

/** rename() is a successful no-op between hardlinks. Check native identity, independently of
 * revisions, so an overwrite move cannot report success while retaining its source name. */
VFSResult<bool> sameNativeFile( const std::string& from, const std::string& to ) {
	struct stat destination;
	if ( ::lstat( to.c_str(), &destination ) != 0 ) {
		if ( errno == ENOENT )
			return Ok( false );
		return Err( errnoError( to, "Cannot check rename destination identity" ) );
	}
	struct stat source;
	if ( ::lstat( from.c_str(), &source ) != 0 )
		return Err( errnoError( from, "Cannot check rename source identity" ) );
	return Ok( source.st_dev == destination.st_dev && source.st_ino == destination.st_ino );
}

} // namespace

VFSTask<void> LocalProvider::rename( const URI& from, const URI& to, const RenameOptions& options,
									 CancellationToken token ) {
	if ( token.isCancellationRequested() )
		return readyError<void>( cancelledError( from ) );
	auto fromPath = toNativePath( from );
	if ( !fromPath )
		return readyError<void>( std::move( fromPath ).error() );
	auto toPath = toNativePath( to );
	if ( !toPath )
		return readyError<void>( std::move( toPath ).error() );
	// A rename never crosses file systems, and paths on different routes (a local disk and a
	// network mount, or two mounts, their parents' links followed) are never on the same one:
	// answer it before the call, as the system would (the VFS then copies and removes). The call
	// then never waits on the other path's mount, which the system call would look up.
	const std::string& native = fromPath.value();
	const std::string& target = toPath.value();
	return mScheduler->submitOnSameRoute<void>(
		native, target,
		VFSError( VFSErrorCode::Unsupported, from,
				  "The source and destination are on different file systems" ),
		guarded<void>( native, std::move( token ),
					   [to = target, overwrite = options.overwrite](
						   const std::string& path ) -> VFSResult<void> {
						   if ( !overwrite )
							   return renameNoReplace( path, to );
						   auto same = sameNativeFile( path, to );
						   if ( !same )
							   return Err( std::move( same ).error() );
						   if ( same.value() ) {
							   return Err( VFSError( VFSErrorCode::InvalidOperation,
													 URI::fromFSPath( path ),
													 "Source and destination are the same file" ) );
						   }
						   if ( ::rename( path.c_str(), to.c_str() ) != 0 )
							   return Err( errnoError( path, "rename failed" ) );
						   return Ok();
					   } ) );
}

VFSTask<void> LocalProvider::publishTemporary( const URI& from, const URI& to,
											   CancellationToken token ) {
	if ( from.getParent() != to.getParent() ) {
		return readyError<void>( VFSErrorCode::InvalidOperation, from,
								 "Temporary publication requires sibling paths" );
	}
	auto toPath = toNativePath( to );
	if ( !toPath )
		return readyError<void>( std::move( toPath ).error() );
	return run<void>(
		from, LastLink::Keep, std::move( token ),
		[to = std::move( toPath ).value()]( const std::string& path ) -> VFSResult<void> {
			struct stat source;
			if ( ::lstat( path.c_str(), &source ) != 0 )
				return Err( errnoError( path, "Cannot check temporary file type" ) );
			if ( !S_ISREG( source.st_mode ) ) {
				return Err( VFSError( VFSErrorCode::ChangedExternally, URI::fromFSPath( path ),
									  "Temporary file is no longer a regular file" ) );
			}
			// The writer owns this exclusive temporary name. linkat() publishes the complete
			// inode atomically and fails if the destination already exists; flags 0 never follow
			// a source symlink. This is not an atomic move of an arbitrary public pathname.
			if ( ::linkat( AT_FDCWD, path.c_str(), AT_FDCWD, to.c_str(), 0 ) != 0 )
				return Err( errnoError( path, "No-replace publication failed" ) );
			if ( ::unlink( path.c_str() ) != 0 ) {
				// Publication succeeded. Never roll back the destination or request a copy
				// fallback.
				auto error =
					errnoError( path, "Destination published, but temporary cleanup failed; "
									  "both names may remain" );
				error.code = VFSErrorCode::IO;
				return Err( std::move( error ) );
			}
			return Ok();
		} );
}

VFSTask<URI> LocalProvider::canonicalize( const URI& uri, CancellationToken token ) {
	return run<URI>( uri, LastLink::Follow, std::move( token ),
					 []( const std::string& path ) -> VFSResult<URI> {
						 char* resolved = ::realpath( path.c_str(), nullptr );
						 if ( !resolved )
							 return Err( errnoError( path, "realpath failed" ) );
						 URI result = URI::fromFSPath( resolved );
						 ::free( resolved );
						 return Ok( std::move( result ) );
					 } );
}

VFSTask<void> LocalProvider::setPermissions( const URI& uri, Uint32 permissions,
											 CancellationToken token ) {
	return run<void>(
		uri, LastLink::Follow, std::move( token ),
		[permissions]( const std::string& path ) -> VFSResult<void> {
			if ( ::chmod( path.c_str(), static_cast<mode_t>( permissions & 07777 ) ) != 0 )
				return Err( errnoError( path, "chmod failed" ) );
			return Ok();
		} );
}

VFSTask<SpaceInfo> LocalProvider::spaceInfo( const URI& uri, CancellationToken token ) {
	return run<SpaceInfo>(
		uri, LastLink::Follow, std::move( token ),
		[]( const std::string& path ) -> VFSResult<SpaceInfo> {
			struct statvfs st;
			if ( ::statvfs( path.c_str(), &st ) != 0 )
				return Err( errnoError( path, "statvfs failed" ) );
			Uint64 unit = st.f_frsize ? st.f_frsize : st.f_bsize;
			return Ok( SpaceInfo{ unit * st.f_blocks, unit * st.f_bfree, unit * st.f_bavail } );
		} );
}

}} // namespace EE::VFS

#endif
