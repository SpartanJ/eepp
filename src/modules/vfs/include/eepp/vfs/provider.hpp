#ifndef EE_VFS_PROVIDER_HPP
#define EE_VFS_PROVIDER_HPP

#include <eepp/core/cancellationtoken.hpp>
#include <eepp/vfs/capabilities.hpp>
#include <eepp/vfs/config.hpp>
#include <eepp/vfs/directoryhandle.hpp>
#include <eepp/vfs/error.hpp>
#include <eepp/vfs/filehandle.hpp>
#include <eepp/vfs/fileinfo.hpp>
#include <eepp/vfs/options.hpp>
#include <optional>
#include <string_view>

namespace EE { namespace VFS {

/** Resolved ownership IDs. Missing fields preserve the current owner/group. */
struct EE_VFS_API Ownership {
	std::optional<Uint32> uid;
	std::optional<Uint32> gid;
};

struct EE_VFS_API SpaceInfo {
	Uint64 capacity{ 0 };
	Uint64 free{ 0 };
	/** Free space available to the current (non-privileged) user. */
	Uint64 available{ 0 };
};

/** @brief Backend for one URI scheme.
 *
 * The interface is deliberately primitive: recursive copy, move, recursive removal, atomic
 * writes and directory creation with parents are orchestrated by VirtualFileSystem on top of
 * these operations. RecursiveRemove providers handle recursive deletion themselves to keep
 * traversal and removal bound to opened directories instead of mutable child paths.
 *
 * Threading contract:
 * - Every method may be called from any thread and must never block the caller on I/O; work is
 *   performed on provider-owned threads and reported through the returned task.
 * - Returned tasks may resolve on any thread. Callers that touch UI or model state must continue
 *   with then( uiExecutor, fn ).
 * - Cancellation is cooperative: a cancelled operation completes with VFSErrorCode::Cancelled.
 * - Providers never touch UI or model state.
 */
class EE_VFS_API VFSProvider {
  public:
	virtual ~VFSProvider() = default;

	/** @return The lowercase URI scheme served by this provider (for example "file"). */
	virtual std::string_view scheme() const = 0;

	virtual ProviderCapabilities capabilities( const URI& uri ) const = 0;

	virtual VFSTask<VFSFileInfo> stat( const URI& uri, const StatOptions& options,
									   CancellationToken token ) = 0;

	virtual VFSTask<DirectoryHandle> openDirectory( const URI& uri,
													const DirectoryOpenOptions& options,
													CancellationToken token ) = 0;

	virtual VFSTask<FileHandle> open( const URI& uri, const OpenOptions& options,
									  CancellationToken token ) = 0;

	/** Creates one directory. The parent must exist. */
	virtual VFSTask<void> mkdir( const URI& uri, const MkDirOptions& options,
								 CancellationToken token ) = 0;

	/** Removes a file, a symlink (never its target) or an empty directory. Providers advertising
	 * RecursiveRemove also accept recursive=true and must not follow replaced child symlinks. */
	virtual VFSTask<void> remove( const URI& uri, const RemoveOptions& options,
								  CancellationToken token ) = 0;

	/** Renames within this provider. URIs on different authorities may fail with Unsupported. */
	virtual VFSTask<void> rename( const URI& from, const URI& to, const RenameOptions& options,
								  CancellationToken token ) = 0;

	/** Resolves symlinks, "." and ".." and filesystem aliases into the provider's canonical URI.
	 */
	virtual VFSTask<URI> canonicalize( const URI& uri, CancellationToken token ) = 0;

	/** Whether canonical paths from these URIs belong to one resource namespace.
	 * Called only for URIs served by this provider instance. True makes canonical paths
	 * comparable even when authorities differ; false identifies disjoint namespaces within
	 * the provider's identity model (independent servers can still share physical storage).
	 * Resolve authority aliases here. Return Unsupported if identity is uncertain, so a
	 * transfer fails before modifying its destination. This does not promise native rename
	 * support or imply that both paths are on one device. */
	virtual VFSTask<bool> sameFileSystem( const URI& from, const URI& to,
										  CancellationToken token ) = 0;

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

	/** Sets numeric ownership IDs (VFSCapability::Ownership). An unset ID is preserved.
	 * Like setPermissions(), follows the final symlink. */
	virtual VFSTask<void> setOwnership( const URI& uri, std::optional<Uint32> uid,
										std::optional<Uint32> gid, CancellationToken /*token*/ ) {
		(void)uid;
		(void)gid;
		return readyError<void>( VFSErrorCode::Unsupported, uri,
								 "Provider does not support ownership changes" );
	}

	/** Resolves ownership text within this provider's namespace. Empty text preserves that ID.
	 * The default accepts decimal numeric IDs; local providers also accept account names. */
	virtual VFSTask<Ownership> resolveOwnership( const URI& uri, std::string user,
												 std::string group, CancellationToken token );

	/** Rewrites a saved location after a successful same-authority rename. This only transforms
	 * URIs and performs no I/O; it does not rename entries. Returns false and leaves uri unchanged
	 * when unaffected. The default replaces an ordinary path prefix; providers with
	 * compound URIs override this to update backing locations while preserving member paths.
	 * Paths must already be normalized, as for navigation. */
	virtual bool relocateURI( URI& uri, const URI& from, const URI& to ) const;
};

}} // namespace EE::VFS

#endif
