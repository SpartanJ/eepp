#ifndef EE_VFS_DETAIL_COALESCEDDELIVERY_HPP
#define EE_VFS_DETAIL_COALESCEDDELIVERY_HPP

#include <eepp/core/executor.hpp>
#include <eepp/core/small_unique_function.hpp>
#include <memory>
#include <mutex>

namespace EE { namespace VFS { namespace detail {

/** @brief Producer/consumer channel that coalesces updates into one delivery per dispatch.
 *
 * Producers mutate a payload under a lock with update(); if no delivery is scheduled, one is
 * dispatched to the target executor (or run inline when there is none). The delivery extracts
 * everything accumulated so far with Policy::take(), so a burst of updates becomes one callback,
 * without an artificial latency timer.
 *
 * Guarantees:
 * - Executors and callbacks are never invoked while the payload lock is held.
 * - Callbacks for one channel never run concurrently.
 * - After close() returns, the callback is not running (unless close() was called from inside it)
 *   and never runs again. Closing from inside the callback is allowed; the callable is released
 *   once it returns.
 *
 * Policy requirements:
 *   using Payload = ...;            accumulated state
 *   using Delivered = ...;          value passed to the callback
 *   static bool hasData( const Payload& );
 *   static Delivered take( Payload& );   extracts the delivered value, resetting as needed
 *
 * Must be owned by a std::shared_ptr (dispatches hold weak references).
 */
template <typename Policy>
class CoalescedDelivery : public std::enable_shared_from_this<CoalescedDelivery<Policy>> {
  public:
	using Payload = typename Policy::Payload;
	using Delivered = typename Policy::Delivered;
	using Callback = SmallUniqueFunction<void( Delivered&& ), 48>;

	CoalescedDelivery() = default;

	explicit CoalescedDelivery( Payload initial ) : mPayload( std::move( initial ) ) {}

	/** Applies mutate( Payload& ) -> bool under the lock; returning true requests a delivery. */
	template <typename F> void update( F&& mutate ) {
		bool dispatchNow;
		{
			std::lock_guard<std::mutex> lock( mMutex );
			if ( mClosed || !mutate( mPayload ) )
				return;
			dispatchNow = scheduleLocked();
		}
		if ( dispatchNow )
			dispatch();
	}

	/** Runs fn( const Payload& ) under the lock and returns its result. */
	template <typename F> auto inspect( F&& fn ) const {
		std::lock_guard<std::mutex> lock( mMutex );
		return fn( mPayload );
	}

	/** Sets the callback and its executor (nullptr = inline on the producer thread). Pending data
	 * is delivered right away. */
	void setTarget( Executor* executor, Callback callback ) {
		{
			std::lock_guard<std::recursive_mutex> delivery( mDeliveryMutex );
			mCallback = std::move( callback );
		}
		bool dispatchNow;
		{
			std::lock_guard<std::mutex> lock( mMutex );
			if ( mClosed )
				return;
			mExecutor = executor;
			mHasTarget = true;
			dispatchNow = Policy::hasData( mPayload ) && scheduleLocked();
		}
		if ( dispatchNow )
			dispatch();
	}

	void close() {
		{
			std::lock_guard<std::mutex> lock( mMutex );
			mClosed = true;
			mExecutor = nullptr;
			mHasTarget = false;
		}
		// Waits for a delivery running on another thread. A callback closing its own channel
		// already holds this recursive mutex on this thread; deliver() then releases the callable
		// after it returns instead of destroying it mid-call.
		std::lock_guard<std::recursive_mutex> delivery( mDeliveryMutex );
		if ( !mInCallback )
			mCallback = nullptr;
	}

	bool isClosed() const {
		std::lock_guard<std::mutex> lock( mMutex );
		return mClosed;
	}

  private:
	/** @return True if the caller must dispatch (after releasing the lock). */
	bool scheduleLocked() {
		if ( !mHasTarget || mScheduled || mClosed )
			return false;
		mScheduled = true;
		return true;
	}

	void dispatch() {
		Executor* executor;
		{
			std::lock_guard<std::mutex> lock( mMutex );
			if ( !mHasTarget ) {
				mScheduled = false;
				return;
			}
			executor = mExecutor;
		}
		if ( !executor ) {
			deliver();
			return;
		}
		executor->execute( [weak = this->weak_from_this()] {
			if ( auto self = weak.lock() )
				self->deliver();
		} );
	}

	void deliver() {
		std::lock_guard<std::recursive_mutex> delivery( mDeliveryMutex );
		if ( mInCallback ) {
			// Re-entrant update from inside the callback with an inline target: the outer
			// delivery loop below picks the new data up.
			return;
		}
		while ( true ) {
			Delivered delivered;
			{
				std::lock_guard<std::mutex> lock( mMutex );
				mScheduled = false;
				if ( mClosed || !Policy::hasData( mPayload ) )
					break;
				delivered = Policy::take( mPayload );
			}
			if ( !mCallback )
				break;
			mInCallback = true;
			mCallback( std::move( delivered ) );
			mInCallback = false;
			std::lock_guard<std::mutex> lock( mMutex );
			// Inline targets drain synchronously; executor targets were re-dispatched by update().
			if ( mClosed || mExecutor || !Policy::hasData( mPayload ) )
				break;
			mScheduled = true;
		}
		std::lock_guard<std::mutex> lock( mMutex );
		if ( mClosed )
			mCallback = nullptr;
	}

	mutable std::mutex mMutex;
	std::recursive_mutex mDeliveryMutex;
	Payload mPayload{};
	Callback mCallback;
	Executor* mExecutor{ nullptr };
	bool mHasTarget{ false };
	bool mScheduled{ false };
	bool mClosed{ false };
	bool mInCallback{ false };
};

}}} // namespace EE::VFS::detail

#endif
