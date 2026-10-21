#ifndef EE_VFS_TRANSFERENGINE_HPP
#define EE_VFS_TRANSFERENGINE_HPP

#include <eepp/system/threadpool.hpp>
#include <eepp/vfs/fileinfo.hpp>
#include <eepp/vfs/options.hpp>
#include <eepp/vfs/transfer.hpp>
#include <memory>
#include <vector>

namespace EE { namespace VFS {

class VirtualFileSystem;
class VFSProvider;

namespace detail {
class TransferState;
}

/** Recursive copy or move of one entry, across providers when necessary. A move within one
 * provider filesystem namespace is a rename when the provider can do it. */
class TransferEngine {
  public:
	/** Runs a copy (or a move) on pool and returns its handle. Without a pool (the VFS is shutting
	 * down) the transfer completes with an error. */
	static Transfer start( VirtualFileSystem& vfs, std::shared_ptr<System::ThreadPool> pool,
						   URI source, URI destination, const CopyOptions& options, bool move,
						   TransferConflictResolver onConflict = {} );

	TransferEngine( VirtualFileSystem& vfs, std::shared_ptr<detail::TransferState> state,
					URI source, URI destination, CopyOptions options, bool move,
					TransferConflictResolver onConflict );

	/** Blocks until the transfer finished. */
	VFSResult<TransferResult> run();

  private:
	struct Node {
		VFSFileInfo info;
		std::vector<Node> children;
	};

	static constexpr int MaxDepth = 256;

	VFSResult<void> checkCancelled( const URI& uri ) const;

	/** A move must never remove a source whose scanned revision is no longer current. */
	VFSResult<void> checkSource( const VFSFileInfo& info, const URI& uri );

	/** Includes aliases through existing destination ancestors, when canonicalization is supported.
	 */
	VFSResult<void> checkDirectoryDestination( const URI& sourceURI, const URI& destinationURI );

	VFSResult<void> scan( Node& node, const URI& uri, const URI& destination, int depth,
						  Uint64& files, Uint64& bytes );

	/** @return True if the entry (and everything below it) was fully copied. */
	VFSResult<bool> copyNode( const Node& node, const URI& source, URI destination, int depth );

	VFSResult<bool> copyDirectory( const Node& node, const URI& source, URI destination,
								   int depth );

	void advanceFiles( const VFSFileInfo& info, bool addBytes = true );

	/** @return True when both URIs name the same file through symlinks or "..". Hard links are
	 * not detected; FileWriter::replace() keeps their data safe. */
	VFSResult<bool> isSameFile( const URI& source, const URI& destination );

	VFSResult<ConflictResolution> resolveConflict( const VFSFileInfo& sourceInfo,
												   const VFSFileInfo& destinationInfo,
												   const URI& source, const URI& destination );

	VFSResult<URI> renamedDestination( const URI& destination, const std::string& name,
									   bool directory );

	VFSResult<void> copyFile( const VFSFileInfo& info, const URI& source, const URI& destination,
							  bool overwrite );

	VirtualFileSystem& mVfs;
	std::shared_ptr<detail::TransferState> mState;
	URI mSource;
	URI mDestination;
	CopyOptions mOptions;
	TransferConflictResolver mOnConflict;
	CancellationToken mToken;
	VFSProvider* mSrcProvider{ nullptr };
	VFSProvider* mDstProvider{ nullptr };
	TransferResult mResult;
	bool mMove;
	/** Source and destination canonical paths belong to the same provider namespace. */
	bool mSameFileSystem{ false };
	/** A root move collision was accepted before trying the native rename. */
	bool mRootConflictResolved{ false };
};

}} // namespace EE::VFS

#endif
