#ifndef EE_VFS_PROVIDERS_MEMORYPROVIDER_HPP
#define EE_VFS_PROVIDERS_MEMORYPROVIDER_HPP

#include <eepp/core/executor.hpp>
#include <eepp/vfs/config.hpp>
#include <eepp/vfs/provider.hpp>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace EE { namespace VFS {

/** @brief In-memory filesystem provider.
 *
 * Useful for tests (deterministic orchestration and transfer behavior) and scratch storage. The
 * URI authority selects an independent namespace ("mem://a/x" and "mem://b/x" differ).
 *
 * Without an executor every operation completes synchronously (ready tasks). With an executor,
 * results are delivered through it, which exercises asynchronous code paths deterministically.
 *
 * Open file handles keep their data alive when the file is removed or replaced, like POSIX inodes.
 */
class EE_VFS_API MemoryProvider : public VFSProvider {
  public:
	explicit MemoryProvider( std::string scheme = "mem", Executor* completionExecutor = nullptr );

	~MemoryProvider() override;

	std::string_view scheme() const override { return mScheme; }

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

	/** Synchronous helpers for test setup. Parents are created as needed. */
	void putFile( const URI& uri, std::string_view content, Uint32 permissions = 0644 );

	void putDirectory( const URI& uri );

	/** @return The content of a file, or nullopt. */
	std::optional<std::string> fileContent( const URI& uri ) const;

	/** Number of operations served, for tests that assert on round trips. */
	Uint64 operationCount() const;

	struct Node;

  private:
	template <typename T> VFSTask<T> complete( VFSResult<T>&& result );

	std::string key( const URI& uri ) const;

	std::shared_ptr<Node> find( const std::string& key ) const;

	VFSResult<std::shared_ptr<Node>> createNode( const URI& uri, const std::string& key,
												 bool directory, Uint32 permissions );

	VFSFileInfo describe( const URI& uri, const std::string& key, const Node& node ) const;

	std::string mScheme;
	Executor* mExecutor;
	mutable std::mutex mMutex;
	std::map<std::string, std::shared_ptr<Node>, std::less<>> mNodes;
	Uint64 mGeneration{ 0 };
	mutable Uint64 mOperations{ 0 };
};

}} // namespace EE::VFS

#endif
