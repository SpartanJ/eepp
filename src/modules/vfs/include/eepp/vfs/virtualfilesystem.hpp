#ifndef EE_VFS_VIRTUALFILESYSTEM_HPP
#define EE_VFS_VIRTUALFILESYSTEM_HPP

#include <eepp/system/threadpool.hpp>
#include <eepp/vfs/provider.hpp>
#include <eepp/vfs/transfer.hpp>
#include <eepp/vfs/watch.hpp>
#include <memory>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace EE { namespace VFS {

using EE::System::ThreadPool;

/** @brief URI-based virtual file system facade.
 *
 * Routes every operation to the provider registered for the URI scheme, and orchestrates the
 * multi-step operations that must not be reimplemented per provider: listAll(), readAll(),
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
class VirtualFileSystem {
  public:
	struct Config {
		/** Worker threads for blocking local I/O (LocalProvider and similar). */
		Uint32 ioThreads{ 4 };
		/** Worker threads for multi-step orchestration (whole-file reads and writes, recursive
		 * operations). */
		Uint32 orchestrationThreads{ 2 };
		/** Copies and moves running at the same time; more wait for one to finish. */
		Uint32 transferThreads{ 2 };
		/** Register file:// and res:// (and sftp:// when built with SFTP support). */
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

	/** Pool used by providers for blocking local I/O. */
	const std::shared_ptr<ThreadPool>& ioPool() const { return mIOPool; }

	WatchManager& watchManager() { return *mWatchManager; }

	// Primitive operations (forwarded to the provider).

	VFSTask<FileInfo> stat( const URI& uri, const StatOptions& options = {},
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

	VFSTask<SpaceInfo> spaceInfo( const URI& uri, CancellationToken token = {} );

	// Orchestrated operations.

	/** Reads every entry of a directory by repeatedly consuming a DirectoryHandle. */
	VFSTask<std::vector<FileInfo>> listAll( const URI& uri,
											const DirectoryOpenOptions& options = {},
											CancellationToken token = {} );

	/** Reads a whole file. Fails with InvalidOperation if it is larger than maxSize. */
	VFSTask<std::vector<Uint8>> readAll( const URI& uri, CancellationToken token = {},
										 Uint64 maxSize = 512ull * 1024 * 1024 );

	/** Writes a whole file (atomically by default) and returns its new metadata, including the
	 * revision to use for the next conflict-checked save. */
	VFSTask<FileInfo> writeAll( const URI& uri, std::vector<Uint8> data,
								const WriteOptions& options = {}, CancellationToken token = {} );

	VFSTask<FileInfo> writeAll( const URI& uri, std::string_view data,
								const WriteOptions& options = {}, CancellationToken token = {} );

	/** Creates a directory and every missing parent. Succeeds if it already exists. */
	VFSTask<void> makeDirs( const URI& uri, const MkDirOptions& options = {},
							CancellationToken token = {} );

	/** Removes an entry. Directories require options.recursive unless empty. Symlinks are
	 * removed, never followed. */
	VFSTask<void> remove( const URI& uri, const RemoveOptions& options = {},
						  CancellationToken token = {} );

	/** Copies a file or directory tree, across providers when necessary. */
	Transfer copy( const URI& source, const URI& destination, const CopyOptions& options = {} );

	/** Moves a file or directory: native rename when possible, otherwise copy + remove of the
	 * entries that were completely copied. */
	Transfer move( const URI& source, const URI& destination, const CopyOptions& options = {} );

	/** Convenience for watchManager().watch(). */
	VFSTask<WatchHandle> watch( const URI& root, const WatchOptions& options = {} );

  private:
	template <typename T> VFSTask<T> unsupportedScheme( const URI& uri ) const;

	/** Runs a blocking job on the orchestration pool and returns its result as a task. */
	template <typename T, typename F> VFSTask<T> orchestrate( F&& job );

	void registerDefaultProviders();

	mutable std::shared_mutex mProvidersMutex;
	std::unordered_map<std::string, std::shared_ptr<VFSProvider>> mProviders;
	std::shared_ptr<ThreadPool> mIOPool;
	std::shared_ptr<ThreadPool> mOrchestrationPool;
	std::shared_ptr<ThreadPool> mTransferPool;
	std::unique_ptr<WatchManager> mWatchManager;
};

}} // namespace EE::VFS

#endif
