#ifndef EE_VFS_TRANSFER_HPP
#define EE_VFS_TRANSFER_HPP

#include <eepp/core/cancellationtoken.hpp>
#include <eepp/core/executor.hpp>
#include <eepp/core/small_unique_function.hpp>
#include <eepp/vfs/config.hpp>
#include <eepp/vfs/error.hpp>
#include <eepp/vfs/fileinfo.hpp>
#include <functional>
#include <memory>
#include <optional>

namespace EE { namespace VFS {

enum class ConflictAction : Uint8 { Cancel, Skip, Overwrite, OverwriteOlder, Rename };

struct EE_VFS_API TransferConflict {
	URI source;
	URI destination;
	VFSFileInfo sourceInfo;
	VFSFileInfo destinationInfo;
};

struct EE_VFS_API ConflictResolution {
	/** Rename: a new basename, or empty to generate a unique name automatically. */
	std::string name;
	ConflictAction action{ ConflictAction::Cancel };
};

/** Per-entry conflict decision, called on a transfer worker. Dispatch UI work through an executor.
 * The request is owned and can be moved into asynchronous work. Cancellation stops waiting for
 * the task. OverwriteOlder skips if either modification time is unavailable. Type mismatches
 * can only be skipped or renamed. Completed entries are never replayed. */
using TransferConflictResolver =
	std::function<VFSTask<ConflictResolution>( TransferConflict, CancellationToken )>;

struct EE_VFS_API TransferProgress {
	URI currentSource;
	URI currentDestination;
	Uint64 bytesDone{ 0 };
	std::optional<Uint64> bytesTotal;
	Uint64 filesDone{ 0 };
	std::optional<Uint64> filesTotal;
};

struct EE_VFS_API TransferResult {
	Uint64 bytesTransferred{ 0 };
	Uint64 filesTransferred{ 0 };
	/** Entries left uncopied: existing destinations under Skip, symlinks inside a tree,
	 * and unsupported special files. Moves retain every skipped source entry. */
	Uint64 filesSkipped{ 0 };
	Uint64 directoriesCreated{ 0 };
	/** Move only: entries removed from the source after being copied. */
	Uint64 sourcesRemoved{ 0 };
	/** True when a move completed through a native rename instead of copy + remove. */
	bool renamed{ false };

	/** A successful task can still describe a partial transfer; surface this to the user. */
	bool partial() const { return filesSkipped != 0; }
};

using TransferProgressCallback = SmallUniqueFunction<void( const TransferProgress& ), 48>;

namespace detail {
class TransferState;
}

/** @brief RAII handle of a progress subscription. */
class EE_VFS_API TransferProgressSubscription {
  public:
	TransferProgressSubscription() = default;

	TransferProgressSubscription( TransferProgressSubscription&& ) noexcept = default;

	TransferProgressSubscription& operator=( TransferProgressSubscription&& other ) noexcept;

	TransferProgressSubscription( const TransferProgressSubscription& ) = delete;

	TransferProgressSubscription& operator=( const TransferProgressSubscription& ) = delete;

	~TransferProgressSubscription();

	void reset();

  private:
	friend class Transfer;

	TransferProgressSubscription( std::weak_ptr<detail::TransferState> state, Uint64 id ) :
		mState( std::move( state ) ), mId( id ) {}

	std::weak_ptr<detail::TransferState> mState;
	Uint64 mId{ 0 };
};

/** @brief A running copy, move, or ZIP creation orchestrated by the VFS.
 *
 * Progress is domain specific and therefore lives here instead of on Task. Progress updates are
 * coalesced: a slow subscriber receives the latest snapshot rather than every intermediate one.
 * Operations keep running when the Transfer object is destroyed; call cancel() to stop them.
 */
class EE_VFS_API Transfer {
  public:
	Transfer() = default;

	explicit Transfer( std::shared_ptr<detail::TransferState> state );

	/** @return The completion task. Can be retrieved exactly once. */
	VFSTask<TransferResult> completion();

	/** Subscribes to progress. Without an executor, the callback runs on the transfer thread.
	 * An executor is borrowed and must survive subscription reset(). When reset inside its own
	 * callback, it must survive the active execute() call as well. Callbacks should not throw:
	 * an exception stops this subscription's delivery and propagates to its executor or caller. */
	TransferProgressSubscription onProgress( TransferProgressCallback callback,
											 Executor* deliveryExecutor = nullptr );

	/** Requests cancellation. ZIP creation discards its temporary output. Copy/move complete
	 * with VFSErrorCode::Cancelled; files fully
	 * copied so far remain at the destination, the source of a move is never removed for
	 * entries that were not completely copied. */
	void cancel();

	CancellationToken token() const;

	bool isValid() const noexcept { return nullptr != mState; }

  private:
	std::shared_ptr<detail::TransferState> mState;
};

}} // namespace EE::VFS

#endif
