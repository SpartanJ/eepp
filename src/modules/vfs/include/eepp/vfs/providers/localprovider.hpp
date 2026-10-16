#ifndef EE_VFS_PROVIDERS_LOCALPROVIDER_HPP
#define EE_VFS_PROVIDERS_LOCALPROVIDER_HPP

#include <eepp/system/threadpool.hpp>
#include <eepp/vfs/provider.hpp>
#include <memory>
#include <string>

namespace EE { namespace VFS {

/** @brief `file://` provider backed by the native filesystem.
 *
 * Blocking system calls run on the supplied I/O pool. Native paths are converted at the provider
 * boundary (URI::fromFSPath / URI::getFSPath); file URIs with a host other than "localhost" are
 * only accepted on Windows (UNC paths).
 *
 * Revisions are derived from device, inode (file index), modification time and size, so both
 * in-place edits and atomic replacements are detected.
 */
class LocalProvider : public VFSProvider {
  public:
	explicit LocalProvider( std::shared_ptr<System::ThreadPool> pool );

	~LocalProvider() override;

	std::string_view scheme() const override { return "file"; }

	ProviderCapabilities capabilities( const URI& uri ) const override;

	VFSTask<FileInfo> stat( const URI& uri, const StatOptions& options,
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

	VFSTask<void> setPermissions( const URI& uri, Uint32 permissions,
								  CancellationToken token ) override;

	VFSTask<SpaceInfo> spaceInfo( const URI& uri, CancellationToken token ) override;

	/** Blocking helpers shared by the asynchronous operations and the local watch backend. */
	static VFSResult<std::string> toNativePath( const URI& uri );

	static VFSResult<FileInfo> statSync( const URI& uri, const std::string& path,
										 bool followSymlinks );

	/** @return Cached user/group name for an id ("" when unknown or unsupported). Thread-safe.
	 */
	static std::string userName( Uint32 uid );

	static std::string groupName( Uint32 gid );

	const std::shared_ptr<System::ThreadPool>& pool() const { return mPool; }

  private:
	static std::string userNameUncached( Uint32 uid );

	static std::string groupNameUncached( Uint32 gid );

	template <typename T, typename F>
	VFSTask<T> run( const URI& uri, CancellationToken token, F&& fn );

	std::shared_ptr<System::ThreadPool> mPool;
};

/** Runs fn( nativePath ) -> VFSResult<T> on the I/O pool (the platform implementations' common
 * path for blocking calls). */
template <typename T, typename F>
VFSTask<T> LocalProvider::run( const URI& uri, CancellationToken token, F&& fn ) {
	if ( token.isCancellationRequested() )
		return readyError<T>( cancelledError( uri ) );
	auto path = toNativePath( uri );
	if ( !path )
		return readyError<T>( std::move( path ).error() );
	return mPool->submit( [fn = std::forward<F>( fn ), path = std::move( path ).value(),
						   token = std::move( token )]() mutable -> VFSResult<T> {
		if ( token.isCancellationRequested() )
			return Err( VFSError( VFSErrorCode::Cancelled, URI(), "Operation cancelled" ) );
		return fn( path );
	} );
}

}} // namespace EE::VFS

#endif
