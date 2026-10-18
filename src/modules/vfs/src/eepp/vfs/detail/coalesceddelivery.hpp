#ifndef EE_VFS_DETAIL_COALESCEDDELIVERY_HPP
#define EE_VFS_DETAIL_COALESCEDDELIVERY_HPP

#include <condition_variable>
#include <eepp/core/executor.hpp>
#include <eepp/core/small_unique_function.hpp>
#include <memory>
#include <mutex>
#include <thread>

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
 *   once it returns. External close() also drains executor submissions before returning.
 * - A borrowed executor must survive close()/setTarget(). When called from its callback, it must
 *   survive the active execute() call as well; that call cannot wait for itself to return.
 * - Callback exceptions retire delivery before propagating to the executor or inline caller.
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
	 * is delivered right away. Waits for an earlier executor submission and callback, except from
	 * the active callback: that submission already entered execute() and must be allowed to finish.
	 * Queued work for an earlier target cannot invoke the replacement callback. */
	void setTarget( Executor* executor, Callback callback ) {
		Callback previous;
		bool dispatchNow;
		{
			std::unique_lock<std::mutex> lock( mMutex );
			if ( mClosed )
				return;
			// Prevent another producer from borrowing the old executor while this change waits.
			++mRetargeting;
			if ( mCallbackThread != std::this_thread::get_id() ) {
				mChanged.wait(
					lock, [this] { return !mSubmitting && mCallbackThread == std::thread::id{}; } );
			}
			if ( !mClosed ) {
				previous = std::move( mCallback );
				mCallback = std::move( callback );
				mExecutor = executor;
				mHasTarget = true;
				++mGeneration;
				mScheduled = false;
				mDispatchPending = false;
				if ( Policy::hasData( mPayload ) )
					scheduleLocked();
			}
		}
		previous = nullptr;
		{
			std::lock_guard<std::mutex> lock( mMutex );
			--mRetargeting;
			dispatchNow = mDispatchPending && !mRetargeting;
		}
		if ( dispatchNow )
			dispatch();
	}

	void close() {
		Callback discarded;
		{
			std::unique_lock<std::mutex> lock( mMutex );
			retireLocked();
			// A synchronous executor can run the callback on another thread and wait for it.
			// Its own callback must not wait for execute() to return. No other submission can
			// start during delivery; the current callable is owned by deliver() until it returns.
			if ( mCallbackThread != std::this_thread::get_id() ) {
				mChanged.wait(
					lock, [this] { return !mSubmitting && mCallbackThread == std::thread::id{}; } );
			}
			discarded = std::move( mCallback );
		}
	}

	bool isClosed() const {
		std::lock_guard<std::mutex> lock( mMutex );
		return mClosed;
	}

  private:
	void retireLocked() {
		mClosed = true;
		mExecutor = nullptr;
		mHasTarget = false;
		mScheduled = false;
		mDispatchPending = false;
	}

	/** Stack lease covers the pointer lookup through the return from execute(), including
	 * exceptions. No per-dispatch heap allocation or owned executor is needed. */
	struct Submission {
		CoalescedDelivery& owner;
		bool accepted{ false };

		~Submission() { owner.finishSubmission( accepted ); }
	};

	struct Invocation {
		CoalescedDelivery& owner;
		Callback& callback;
		Uint64 generation;
		bool completed{ false };

		~Invocation() { owner.finishDelivery( generation, callback, completed ); }
	};

	/** @return True if the caller must dispatch (after releasing the lock). */
	bool scheduleLocked() {
		if ( !mHasTarget || mScheduled || mClosed )
			return false;
		mScheduled = true;
		mDispatchPending = true;
		return true;
	}

	void dispatch() {
		// An inline callback may release its last subscription owner.
		auto keepAlive = this->shared_from_this();
		while ( true ) {
			Executor* executor;
			Uint64 generation;
			{
				std::lock_guard<std::mutex> lock( mMutex );
				if ( mClosed || !mDispatchPending || mSubmitting || mRetargeting ||
					 mCallbackThread != std::thread::id{} )
					return;
				mDispatchPending = false;
				mSubmitting = true;
				executor = mExecutor;
				generation = mGeneration;
			}
			{
				Submission submission{ *this };
				if ( executor ) {
					executor->execute( [weak = this->weak_from_this(), generation] {
						if ( auto self = weak.lock() )
							self->deliver( generation );
					} );
				} else {
					deliver( generation );
				}
				submission.accepted = true;
			}
			// Inline or synchronously awaited callbacks can request another delivery. Submit it
			// only after execute() returns, never recursively while that executor is waiting.
		}
	}

	void finishSubmission( bool accepted ) {
		Callback discarded;
		{
			std::lock_guard<std::mutex> lock( mMutex );
			mSubmitting = false;
			if ( !accepted ) {
				// If execute() (or an inline callback) throws, retire this channel while unwinding.
				// The exception still propagates to the caller; close() must remain drainable.
				retireLocked();
				discarded = std::move( mCallback );
			}
		}
		mChanged.notify_all();
	}

	void deliver( Uint64 generation ) {
		Delivered delivered;
		Callback callback;
		{
			std::lock_guard<std::mutex> lock( mMutex );
			if ( mClosed || generation != mGeneration || mCallbackThread != std::thread::id{} )
				return;
			if ( !Policy::hasData( mPayload ) || !mCallback ) {
				mScheduled = false;
				return;
			}
			delivered = Policy::take( mPayload );
			// Keep the executing callable alive even if it re-registers or closes this channel.
			callback = std::move( mCallback );
			mCallbackThread = std::this_thread::get_id();
		}
		{
			Invocation invocation{ *this, callback, generation };
			callback( std::move( delivered ) );
			invocation.completed = true;
		}
		dispatch();
	}

	void finishDelivery( Uint64 generation, Callback& callback, bool completed ) {
		{
			std::lock_guard<std::mutex> lock( mMutex );
			if ( !completed )
				retireLocked();
			if ( !mClosed && generation == mGeneration )
				mCallback = std::move( callback );
		}
		// Destruction can release captures with their own teardown. Do it outside the payload
		// lock, but before notifying an external close() that this invocation has finished.
		callback = nullptr;
		{
			std::lock_guard<std::mutex> lock( mMutex );
			mCallbackThread = {};
			if ( generation == mGeneration ) {
				mScheduled = false;
				if ( Policy::hasData( mPayload ) )
					scheduleLocked();
			}
		}
		mChanged.notify_all();
	}

	mutable std::mutex mMutex;
	std::condition_variable mChanged;
	Payload mPayload{};
	Callback mCallback;
	Executor* mExecutor{ nullptr };
	Uint64 mGeneration{ 0 };
	std::thread::id mCallbackThread;
	Uint32 mRetargeting{ 0 };
	bool mHasTarget{ false };
	bool mScheduled{ false };
	bool mDispatchPending{ false };
	bool mSubmitting{ false };
	bool mClosed{ false };
};

}}} // namespace EE::VFS::detail

#endif
