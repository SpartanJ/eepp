#ifndef EE_VFS_PROVIDERS_LOCALPROVIDER_HPP
#define EE_VFS_PROVIDERS_LOCALPROVIDER_HPP

#include <eepp/vfs/config.hpp>
#include <eepp/vfs/provider.hpp>
#include <eepp/vfs/providers/localioscheduler.hpp>
#include <memory>
#include <string>

namespace EE { namespace VFS {

/** @brief `file://` provider backed by the native filesystem.
 *
 * Blocking system calls run through the LocalIOScheduler: on its I/O pool, or on a network
 * mount's own lane, where an unreachable server makes calls fail with TimedOut instead of
 * blocking every caller. Native paths are converted at the provider
 * boundary (URI::fromFSPath / URI::getFSPath); file URIs with a host other than "localhost" are
 * only accepted on Windows (UNC paths).
 *
 * Revisions are derived from device, inode (file index), modification time and size, so both
 * in-place edits and atomic replacements are detected. Overwrite renames reject matching native
 * file identities, including hardlinks, with InvalidOperation and retain both names.
 */
class EE_VFS_API LocalProvider : public VFSProvider {
  public:
	explicit LocalProvider( std::shared_ptr<LocalIOScheduler> scheduler );

	~LocalProvider() override;

	std::string_view scheme() const override { return "file"; }

	ProviderCapabilities capabilities( const URI& uri ) const override;

	VFSTask<VFSFileInfo> stat( const URI& uri, const StatOptions& options,
							   CancellationToken token ) override;

	VFSTask<DirectoryHandle> openDirectory( const URI& uri, const DirectoryOpenOptions& options,
											CancellationToken token ) override;

	VFSTask<FileHandle> open( const URI& uri, const OpenOptions& options,
							  CancellationToken token ) override;

	VFSTask<void> mkdir( const URI& uri, const MkDirOptions& options,
						 CancellationToken token ) override;

	VFSTask<void> remove( const URI& uri, const RemoveOptions& options,
						  CancellationToken token ) override;

	VFSTask<void> rename( const URI& from, const URI& to, const RenameOptions& options,
						  CancellationToken token ) override;

	VFSTask<URI> canonicalize( const URI& uri, CancellationToken token ) override;

	/** Whether canonical paths from these authorities share one resource namespace.
	 * Unknown identity returns Unsupported; this does not promise native rename support. */
	VFSTask<bool> sameFileSystem( const URI& from, const URI& to,
								  CancellationToken token ) override;

	VFSTask<void> setPermissions( const URI& uri, Uint32 permissions,
								  CancellationToken token ) override;

	VFSTask<SpaceInfo> spaceInfo( const URI& uri, CancellationToken token ) override;

	/** Blocking helpers shared by the asynchronous operations and the local watch backend. */
	static VFSResult<std::string> toNativePath( const URI& uri );

	static VFSResult<VFSFileInfo> statSync( const URI& uri, const std::string& path,
											bool followSymlinks );

	/** @return Cached user/group name for an id ("" when unknown or unsupported). Thread-safe.
	 */
	static std::string userName( Uint32 uid );

	static std::string groupName( Uint32 gid );

  private:
	static std::string userNameUncached( Uint32 uid );

	static std::string groupNameUncached( Uint32 gid );

	/** Whether a call follows a symbolic link in its path's last component (stat, open), or acts
	 * on the link itself (lstat, remove, mkdir). */
	enum class LastLink { Follow, Keep };

	template <typename T, typename F>
	VFSTask<T> run( const URI& uri, LastLink last, CancellationToken token, F&& fn );

	/** fn( path ), unless token is cancelled by the time it runs. */
	template <typename T, typename F>
	static auto guarded( std::string path, CancellationToken token, F&& fn );

	std::shared_ptr<LocalIOScheduler> mScheduler;
};

/** Runs fn( nativePath ) -> VFSResult<T> where the path's calls run, its symbolic links followed
 * (the platform implementations' common path for blocking calls). */
template <typename T, typename F>
VFSTask<T> LocalProvider::run( const URI& uri, LastLink last, CancellationToken token, F&& fn ) {
	if ( token.isCancellationRequested() )
		return readyError<T>( cancelledError( uri ) );
	auto path = toNativePath( uri );
	if ( !path )
		return readyError<T>( std::move( path ).error() );
	const std::string& native = path.value();
	return mScheduler->submitResolved<T>(
		native, last == LastLink::Follow,
		guarded<T>( native, std::move( token ), std::forward<F>( fn ) ) );
}

template <typename T, typename F>
auto LocalProvider::guarded( std::string path, CancellationToken token, F&& fn ) {
	return [fn = std::forward<F>( fn ), path = std::move( path ),
			token = std::move( token )]() mutable -> VFSResult<T> {
		if ( token.isCancellationRequested() )
			return Err( VFSError( VFSErrorCode::Cancelled, URI(), "Operation cancelled" ) );
		return fn( path );
	};
}

}} // namespace EE::VFS

#endif
