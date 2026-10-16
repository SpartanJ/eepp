#ifndef EE_VFS_PROVIDER_HPP
#define EE_VFS_PROVIDER_HPP

#include <eepp/core/cancellationtoken.hpp>
#include <eepp/vfs/capabilities.hpp>
#include <eepp/vfs/directoryhandle.hpp>
#include <eepp/vfs/error.hpp>
#include <eepp/vfs/filehandle.hpp>
#include <eepp/vfs/fileinfo.hpp>
#include <eepp/vfs/options.hpp>
#include <string_view>

namespace EE { namespace VFS {

struct SpaceInfo {
	Uint64 capacity{ 0 };
	Uint64 free{ 0 };
	/** Free space available to the current (non-privileged) user. */
	Uint64 available{ 0 };
};

/** @brief Backend for one URI scheme.
 *
 * The interface is deliberately primitive: recursive copy, move, recursive removal, atomic
 * writes and directory creation with parents are orchestrated by VirtualFileSystem on top of
 * these operations, so providers do not reimplement them.
 *
 * Threading contract:
 * - Every method may be called from any thread and must never block the caller on I/O; work is
 *   performed on provider-owned threads and reported through the returned task.
 * - Returned tasks may resolve on any thread. Callers that touch UI or model state must continue
 *   with then( uiExecutor, fn ).
 * - Cancellation is cooperative: a cancelled operation completes with VFSErrorCode::Cancelled.
 * - Providers never touch UI or model state.
 */
class VFSProvider {
  public:
	virtual ~VFSProvider() = default;

	/** @return The lowercase URI scheme served by this provider (for example "file"). */
	virtual std::string_view scheme() const = 0;

	virtual ProviderCapabilities capabilities( const URI& uri ) const = 0;

	virtual VFSTask<FileInfo> stat( const URI& uri, const StatOptions& options,
									CancellationToken token ) = 0;

	virtual VFSTask<DirectoryHandle> openDirectory( const URI& uri,
													const DirectoryOpenOptions& options,
													CancellationToken token ) = 0;

	virtual VFSTask<FileHandle> open( const URI& uri, const OpenOptions& options,
									  CancellationToken token ) = 0;

	/** Creates one directory. The parent must exist. */
	virtual VFSTask<void> mkdir( const URI& uri, const MkDirOptions& options,
								 CancellationToken token ) = 0;

	/** Removes a file, a symlink (never its target) or an empty directory. */
	virtual VFSTask<void> remove( const URI& uri, const RemoveOptions& options,
								  CancellationToken token ) = 0;

	/** Renames within this provider. URIs on different authorities may fail with Unsupported. */
	virtual VFSTask<void> rename( const URI& from, const URI& to, const RenameOptions& options,
								  CancellationToken token ) = 0;

	/** Resolves symlinks, "." and ".." and filesystem aliases into the provider's canonical URI.
	 */
	virtual VFSTask<URI> canonicalize( const URI& uri, CancellationToken token ) = 0;

	/** Optional: sets POSIX permission bits (VFSCapability::Permissions). */
	virtual VFSTask<void> setPermissions( const URI& uri, Uint32 permissions,
										  CancellationToken /*token*/ ) {
		(void)permissions;
		return readyError<void>( VFSErrorCode::Unsupported, uri,
								 "Provider does not support permissions" );
	}

	/** Optional: filesystem space information (VFSCapability::FreeSpace). */
	virtual VFSTask<SpaceInfo> spaceInfo( const URI& uri, CancellationToken /*token*/ ) {
		return readyError<SpaceInfo>( VFSErrorCode::Unsupported, uri,
									  "Provider does not report free space" );
	}
};

}} // namespace EE::VFS

#endif
