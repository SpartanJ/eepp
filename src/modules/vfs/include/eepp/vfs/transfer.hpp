#ifndef EE_VFS_TRANSFER_HPP
#define EE_VFS_TRANSFER_HPP

#include <eepp/core/cancellationtoken.hpp>
#include <eepp/core/executor.hpp>
#include <eepp/core/small_unique_function.hpp>
#include <eepp/vfs/error.hpp>
#include <memory>
#include <optional>

namespace EE { namespace VFS {

struct TransferProgress {
	URI currentSource;
	URI currentDestination;
	Uint64 bytesDone{ 0 };
	std::optional<Uint64> bytesTotal;
	Uint64 filesDone{ 0 };
	std::optional<Uint64> filesTotal;
};

struct TransferResult {
	Uint64 bytesTransferred{ 0 };
	Uint64 filesTransferred{ 0 };
	Uint64 filesSkipped{ 0 };
	Uint64 directoriesCreated{ 0 };
	/** Move only: entries removed from the source after being copied. */
	Uint64 sourcesRemoved{ 0 };
	/** True when a move completed through a native rename instead of copy + remove. */
	bool renamed{ false };
};

using TransferProgressCallback = SmallUniqueFunction<void( const TransferProgress& ), 48>;

namespace detail {
class TransferState;
}

/** @brief RAII handle of a progress subscription. */
class TransferProgressSubscription {
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

/** @brief A running copy or move orchestrated by the VFS.
 *
 * Progress is domain specific and therefore lives here instead of on Task. Progress updates are
 * coalesced: a slow subscriber receives the latest snapshot rather than every intermediate one.
 * Transfers keep running when the Transfer object is destroyed; call cancel() to stop them.
 */
class Transfer {
  public:
	Transfer() = default;

	explicit Transfer( std::shared_ptr<detail::TransferState> state );

	/** @return The completion task. Can be retrieved exactly once. */
	VFSTask<TransferResult> completion();

	/** Subscribes to progress. Without an executor, the callback runs on the transfer thread. */
	TransferProgressSubscription onProgress( TransferProgressCallback callback,
											 Executor* deliveryExecutor = nullptr );

	/** @return The most recent progress snapshot. */
	TransferProgress progress() const;

	/** Requests cancellation. The transfer completes with VFSErrorCode::Cancelled; files fully
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
