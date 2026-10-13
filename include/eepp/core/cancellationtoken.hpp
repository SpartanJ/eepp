#ifndef EE_CORE_CANCELLATIONTOKEN_HPP
#define EE_CORE_CANCELLATIONTOKEN_HPP

#include <atomic>
#include <condition_variable>
#include <eepp/config.hpp>
#include <eepp/core/small_unique_function.hpp>
#include <eepp/core/small_vector.hpp>
#include <memory>
#include <mutex>
#include <thread>

namespace EE {

class CancellationToken;
class CancellationRegistration;

namespace detail {

/** Shared cancellation state. Owned jointly by the source, tokens and live registrations. */
class EE_API CancellationState {
  public:
	using Callback = SmallUniqueFunction<void(), 64>;

	bool isCancellationRequested() const noexcept {
		return mCancelled.load( std::memory_order_acquire );
	}

	bool cancel();

	/** @return Registration id, or 0 when cancellation already happened (callback not stored). */
	Uint64 add( Callback& callback );

	/** Removes the registration. Blocks while its callback is running on another thread. */
	void remove( Uint64 id );

  private:
	struct Entry {
		Uint64 id;
		Callback callback;
	};

	std::mutex mMutex;
	std::condition_variable mCallbackDone;
	SmallVector<Entry, 2> mEntries;
	std::thread::id mExecutingThread;
	Uint64 mNextId{ 1 };
	Uint64 mExecutingId{ 0 };
	std::atomic<bool> mCancelled{ false };
};

} // namespace detail

/** @brief RAII handle for a callback registered on a CancellationToken.
 *
 * Destroying (or reset()-ing) the registration unregisters the callback if it has not fired. If
 * the callback is running concurrently on the cancelling thread, destruction waits for it to
 * finish, so state captured by the callback may be released right after the registration dies.
 * A callback may destroy its own registration without deadlocking.
 */
class EE_API CancellationRegistration {
  public:
	CancellationRegistration() noexcept = default;

	CancellationRegistration( CancellationRegistration&& other ) noexcept;

	CancellationRegistration& operator=( CancellationRegistration&& other ) noexcept;

	CancellationRegistration( const CancellationRegistration& ) = delete;

	CancellationRegistration& operator=( const CancellationRegistration& ) = delete;

	~CancellationRegistration();

	/** Unregisters the callback now. */
	void reset();

	/** @return True while the callback is registered and has not been invoked. */
	explicit operator bool() const noexcept { return mId != 0; }

  private:
	friend class CancellationToken;

	CancellationRegistration( std::shared_ptr<detail::CancellationState> state, Uint64 id ) :
		mState( std::move( state ) ), mId( id ) {}

	std::shared_ptr<detail::CancellationState> mState;
	Uint64 mId{ 0 };
};

/** @brief Cheap, copyable observer of a cancellation request.
 *
 * A default-constructed token can never be cancelled. Tokens keep the shared state alive
 * independently of their CancellationSource.
 */
class EE_API CancellationToken {
  public:
	CancellationToken() noexcept = default;

	/** @return True once the owning source requested cancellation. */
	bool isCancellationRequested() const noexcept {
		return mState && mState->isCancellationRequested();
	}

	/** @return False for the default "never cancelled" token. */
	bool canBeCancelled() const noexcept { return nullptr != mState; }

	/** Registers a callback that runs synchronously on the thread calling cancel(). If
	 * cancellation already happened, the callback runs immediately on the calling thread and the
	 * returned registration is empty. */
	CancellationRegistration onCancel( detail::CancellationState::Callback callback ) const;

	bool operator==( const CancellationToken& other ) const noexcept {
		return mState == other.mState;
	}

  private:
	friend class CancellationSource;

	explicit CancellationToken( std::shared_ptr<detail::CancellationState> state ) noexcept :
		mState( std::move( state ) ) {}

	std::shared_ptr<detail::CancellationState> mState;
};

/** @brief Producer side of a cancellation request.
 *
 * Move-only. Destroying the source does not cancel its tokens.
 */
class EE_API CancellationSource {
  public:
	CancellationSource();

	CancellationSource( CancellationSource&& ) noexcept = default;

	CancellationSource& operator=( CancellationSource&& ) noexcept = default;

	CancellationSource( const CancellationSource& ) = delete;

	CancellationSource& operator=( const CancellationSource& ) = delete;

	CancellationToken token() const noexcept { return CancellationToken( mState ); }

	/** Requests cancellation and runs every registered callback on this thread.
	 * @return True only for the call that performed the cancellation. */
	bool cancel();

	bool isCancellationRequested() const noexcept {
		return mState && mState->isCancellationRequested();
	}

  private:
	std::shared_ptr<detail::CancellationState> mState;
};

} // namespace EE

#endif
