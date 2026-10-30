#ifndef EE_VFS_VIRTUALFILESYSTEM_HPP
#define EE_VFS_VIRTUALFILESYSTEM_HPP

#include <eepp/core/containers.hpp>
#include <eepp/system/threadpool.hpp>
#include <eepp/vfs/config.hpp>
#include <eepp/vfs/directorystats.hpp>
#include <eepp/vfs/iostreamvfs.hpp>
#include <eepp/vfs/permissionchanges.hpp>
#include <eepp/vfs/provider.hpp>
#include <eepp/vfs/providers/localioscheduler.hpp>
#include <eepp/vfs/transfer.hpp>
#include <eepp/vfs/watch.hpp>
#include <memory>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <vector>

namespace EE { namespace VFS {

using EE::System::ThreadPool;

/** @brief URI-based virtual file system facade.
 *
 * Routes every operation to the provider registered for the URI scheme, and orchestrates the
 * multi-step operations that must not be reimplemented per provider: listAll(), openStream(),
 * atomic writeAll(), makeDirs(), recursive remove(), and cross-provider copy() / move().
 *
 * Threading:
 * - All methods are non-blocking and callable from any thread, including the UI thread.
 * - Tasks resolve on VFS or provider threads; continue with then( uiExecutor, fn ) before
 *   touching UI or model state.
 * - Multi-step operations run on a dedicated orchestration pool that waits on provider tasks.
 *   Provider work never runs on that pool, so waits there cannot starve provider I/O. Copies and
 *   moves run on a pool of their own: long transfers never delay the other operations.
 */
class EE_VFS_API VirtualFileSystem {
  public:
	struct Config {
		/** Worker threads for blocking local I/O (LocalProvider and similar). */
		Uint32 ioThreads{ 4 };
		/** Worker threads for multi-step orchestration (stream opening, whole-file writes,
		 * recursive operations). */
		Uint32 orchestrationThreads{ 2 };
		/** Copies and moves running at the same time; more wait for one to finish. */
		Uint32 transferThreads{ 2 };
		/** How long a call on a network mount (NFS, SMB, SSHFS...) may take before the mount
		 * counts as not responding: the call fails with TimedOut, and so do further calls until
		 * the server answers again (see LocalIOScheduler). */
		System::Time networkMountTimeout{ System::Seconds( 15 ) };
		/** Register file://, zip://, archive:// and res:// (and sftp:// when built with SFTP
		 * support). */
		bool registerDefaultProviders{ true };
	};

	/** @return The process-wide instance, created with the default configuration on first use. */
	static VirtualFileSystem& instance();

	/** Destroys the process-wide instance, if any. Pending operations complete with errors or are
	 * abandoned; call this only at shutdown. */
	static void destroyInstance();

	VirtualFileSystem();

	explicit VirtualFileSystem( const Config& config );

	~VirtualFileSystem();

	VirtualFileSystem( const VirtualFileSystem& ) = delete;

	VirtualFileSystem& operator=( const VirtualFileSystem& ) = delete;

	/** Registers (or replaces) the provider for provider->scheme(). */
	void registerProvider( std::shared_ptr<VFSProvider> provider );

	void unregisterProvider( std::string_view scheme );

	std::shared_ptr<VFSProvider> provider( std::string_view scheme ) const;

	std::shared_ptr<VFSProvider> providerFor( const URI& uri ) const;

	std::vector<std::string> schemes() const;

	ProviderCapabilities capabilities( const URI& uri ) const;

	/** Rewrites a saved URI after a completed rename, using that URI's provider. No I/O occurs.
	 * Returns false without changing uri if unaffected or its provider is unregistered.
	 * See VFSProvider::relocateURI(). */
	bool relocateURI( URI& uri, const URI& from, const URI& to ) const;

	/** Pool used by providers for blocking local I/O. */
	const std::shared_ptr<ThreadPool>& ioPool() const { return mIOPool; }

	/** Where the VFS runs blocking calls on local paths. Applications run their own blocking
	 * work on a path here too, so that a network mount that stopped answering fails it after
	 * the timeout instead of holding one of their threads. */
	LocalIOScheduler& localIO() { return *mLocalIO; }

	WatchManager& watchManager() { return *mWatchManager; }

	// Primitive operations (forwarded to the provider).

	VFSTask<VFSFileInfo> stat( const URI& uri, const StatOptions& options = {},
							   CancellationToken token = {} );

	VFSTask<FileHandle> open( const URI& uri, const OpenOptions& options = {},
							  CancellationToken token = {} );

	VFSTask<DirectoryHandle> openDirectory( const URI& uri,
											const DirectoryOpenOptions& options = {},
											CancellationToken token = {} );

	VFSTask<void> makeDir( const URI& uri, const MkDirOptions& options = {},
						   CancellationToken token = {} );

	VFSTask<void> rename( const URI& from, const URI& to, const RenameOptions& options = {},
						  CancellationToken token = {} );

	/** Provider-level canonicalization (symlinks, aliases). URIs are never canonicalized
	 * implicitly. */
	VFSTask<URI> canonicalize( const URI& uri, CancellationToken token = {} );

	VFSTask<void> setPermissions( const URI& uri, Uint32 permissions,
								  CancellationToken token = {} );

	VFSTask<void> setOwnership( const URI& uri, std::optional<Uint32> uid,
								std::optional<Uint32> gid, CancellationToken token = {} );

	/** Resolves names or numeric ownership IDs using the URI's provider. Empty text preserves
	 * that ID. Providers without a name service accept numeric IDs only. */
	VFSTask<Ownership> resolveOwnership( const URI& uri, std::string user, std::string group,
										 CancellationToken token = {} );

	VFSTask<SpaceInfo> spaceInfo( const URI& uri, CancellationToken token = {} );

	// Orchestrated operations.

	/** Applies permission/ownership edits off the caller thread. Stops at the first error;
	 * earlier changes remain applied. Cancellation is cooperative. Recursive traversal skips
	 * observed symlinks; callers must coordinate concurrent writers of the tree. */
	VFSTask<void> changePermissions( std::vector<URI> roots, PermissionChanges changes,
									 CancellationToken token = {} );

	/** Reads every entry of a directory by repeatedly consuming a DirectoryHandle. */
	VFSTask<std::vector<VFSFileInfo>> listAll( const URI& uri,
											   const DirectoryOpenOptions& options = {},
											   CancellationToken token = {} );

	/** Recursively measures a file, folder, or selection. Root errors fail the task; unreadable
	 * descendants produce an incomplete result with subtotals. Progress runs on the worker.
	 * Cancellation fails with Cancelled after publishing a final subtotal. The scan is not an
	 * atomic snapshot of a tree being modified. */
	VFSTask<DirectoryStats> directoryStats( const URI& uri,
											DirectoryStatsProgressCallback onProgress = {},
											CancellationToken token = {} );

	VFSTask<DirectoryStats> directoryStats( std::vector<URI> roots,
											DirectoryStatsProgressCallback onProgress = {},
											CancellationToken token = {} );

	/** Opens an owned, buffered IOStream without reading the whole file. Opening and length
	 * validation are asynchronous; the resulting stream has synchronous operations for loaders.
	 * Use it on a worker independent of provider I/O, never on the UI thread or ioPool().
	 * maxSize bounds the length at open. Unknown or unrepresentable lengths fail explicitly.
	 * The cancellation token remains attached for the stream's entire lifetime. */
	VFSTask<std::unique_ptr<IOStreamVFS>> openStream( const URI& uri,
													  const OpenOptions& options = {},
													  CancellationToken token = {},
													  Uint64 maxSize = IOStreamVFS::Unlimited );

	/** Writes a whole file (atomically by default) and returns its new metadata, including the
	 * revision to use for the next conflict-checked save. */
	VFSTask<VFSFileInfo> writeAll( const URI& uri, std::vector<Uint8> data,
								   const WriteOptions& options = {}, CancellationToken token = {} );

	VFSTask<VFSFileInfo> writeAll( const URI& uri, std::string_view data,
								   const WriteOptions& options = {}, CancellationToken token = {} );

	/** Creates a directory and every missing parent. Succeeds if it already exists. */
	VFSTask<void> makeDirs( const URI& uri, const MkDirOptions& options = {},
							CancellationToken token = {} );

	/** Removes an entry. Directories require options.recursive unless empty. Inspected symlinks
	 * are removed themselves. Recursive removal without RecursiveRemove uses pathname traversal:
	 * callers must prevent concurrent directory/link replacement (Windows local and SFTP included).
	 * Check capabilities(uri) for RecursiveRemove when directory-bound traversal is required. */
	VFSTask<void> remove( const URI& uri, const RemoveOptions& options = {},
						  CancellationToken token = {} );

	/** Copies a file or directory tree, across providers when necessary. */
	Transfer copy( const URI& source, const URI& destination, const CopyOptions& options = {} );

	/** Copy with per-entry conflict decisions, including collisions inside directory trees. */
	Transfer copy( const URI& source, const URI& destination, const CopyOptions& options,
				   TransferConflictResolver onConflict );

	/** Moves a file or directory: native rename when possible, otherwise copy + remove of the
	 * entries that were completely copied. */
	Transfer move( const URI& source, const URI& destination, const CopyOptions& options = {} );

	/** Move with per-entry conflict decisions. Skipped source entries are retained. */
	Transfer move( const URI& source, const URI& destination, const CopyOptions& options,
				   TransferConflictResolver onConflict );

	/** Creates a new ZIP at a local destination from VFS files/folders. Streams at DEFLATE level 9,
	 * preserves empty folders, timestamps, permissions and symbolic links (without following them).
	 * Existing destinations are never replaced. Cancellation/failure removes only the temporary
	 * output. Source trees must remain stable; observed changes fail before publication.
	 * Progress counts uncompressed input bytes. The output is not published until completion. */
	Transfer createZip( std::vector<URI> sources, URI destination );

	/** Convenience for watchManager().watch(). */
	VFSTask<WatchHandle> watch( const URI& root, const WatchOptions& options = {} );

  private:
	template <typename T> VFSTask<T> unsupportedScheme( const URI& uri ) const;

	/** Runs a blocking job on the orchestration pool and returns its result as a task. */
	template <typename T, typename F> VFSTask<T> orchestrate( F&& job );

	void registerDefaultProviders();

	mutable std::shared_mutex mProvidersMutex;
	UnorderedMap<std::string, std::shared_ptr<VFSProvider>> mProviders;
	std::shared_ptr<ThreadPool> mIOPool;
	std::shared_ptr<LocalIOScheduler> mLocalIO;
	std::shared_ptr<ThreadPool> mOrchestrationPool;
	std::shared_ptr<ThreadPool> mTransferPool;
	std::unique_ptr<WatchManager> mWatchManager;
};

}} // namespace EE::VFS

#endif
