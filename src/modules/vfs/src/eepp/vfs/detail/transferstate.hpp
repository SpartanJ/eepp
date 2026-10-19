#ifndef EE_VFS_DETAIL_TRANSFERSTATE_HPP
#define EE_VFS_DETAIL_TRANSFERSTATE_HPP

#include <atomic>
#include <eepp/system/clock.hpp>
#include <eepp/vfs/detail/coalesceddelivery.hpp>
#include <eepp/vfs/transfer.hpp>
#include <mutex>
#include <vector>

namespace EE { namespace VFS { namespace detail {

struct TransferProgressPolicy {
	struct Payload {
		TransferProgress progress;
		bool dirty{ false };
	};

	using Delivered = TransferProgress;

	static bool hasData( const Payload& payload ) { return payload.dirty; }

	static Delivered take( Payload& payload ) {
		payload.dirty = false;
		return payload.progress;
	}
};

/** Shared state of a Transfer: cancellation, completion and progress fan-out. */
class TransferState {
  public:
	using Delivery = CoalescedDelivery<TransferProgressPolicy>;

	/** Minimum interval between progress notifications inside one file. */
	static constexpr Int64 ProgressIntervalMicroseconds = 16000;

	TransferState() : mCompletion( VFSTask<TransferResult>::create( mResolveCompletion ) ) {}

	CancellationToken token() const { return mCancel.token(); }

	void cancel() { mCancel.cancel(); }

	bool isCancelled() const { return mCancel.isCancellationRequested(); }

	VFSTask<TransferResult> takeCompletion() {
		if ( mCompletionTaken.exchange( true ) ) {
			return readyError<TransferResult>( VFSErrorCode::InvalidOperation, URI(),
											   "Transfer completion was already retrieved" );
		}
		return std::move( mCompletion );
	}

	void complete( VFSResult<TransferResult>&& result ) {
		flushProgress();
		mResolveCompletion( std::move( result ) );
	}

	/** Applies mutate( TransferProgress& ) and notifies subscribers, throttled unless force. */
	template <typename F> void updateProgress( F&& mutate, bool force = false ) {
		std::vector<std::shared_ptr<Delivery>> targets;
		TransferProgress snapshot;
		{
			std::lock_guard<std::mutex> lock( mMutex );
			mutate( mProgress );
			if ( !force &&
				 mThrottle.getElapsedTime().asMicroseconds() < ProgressIntervalMicroseconds ) {
				mDirty = true;
				return;
			}
			mThrottle.restart();
			mDirty = false;
			if ( mSubscriptions.empty() )
				return;
			snapshot = mProgress;
			targets.reserve( mSubscriptions.size() );
			for ( auto& subscription : mSubscriptions )
				targets.push_back( subscription.second );
		}
		publish( targets, snapshot );
	}

	/** Publishes a throttled-away update, if any. */
	void flushProgress() {
		std::vector<std::shared_ptr<Delivery>> targets;
		TransferProgress snapshot;
		{
			std::lock_guard<std::mutex> lock( mMutex );
			if ( !mDirty || mSubscriptions.empty() )
				return;
			mDirty = false;
			snapshot = mProgress;
			for ( auto& subscription : mSubscriptions )
				targets.push_back( subscription.second );
		}
		publish( targets, snapshot );
	}

	Uint64 subscribe( TransferProgressCallback callback, Executor* executor ) {
		auto delivery = std::make_shared<Delivery>();
		delivery->setTarget(
			executor, [callback = std::move( callback )]( TransferProgress&& progress ) mutable {
				callback( progress );
			} );
		std::lock_guard<std::mutex> lock( mMutex );
		Uint64 id = ++mLastSubscriptionId;
		mSubscriptions.emplace_back( id, std::move( delivery ) );
		return id;
	}

	void unsubscribe( Uint64 id ) {
		std::shared_ptr<Delivery> delivery;
		{
			std::lock_guard<std::mutex> lock( mMutex );
			for ( auto it = mSubscriptions.begin(); it != mSubscriptions.end(); ++it ) {
				if ( it->first == id ) {
					delivery = std::move( it->second );
					mSubscriptions.erase( it );
					break;
				}
			}
		}
		if ( delivery )
			delivery->close();
	}

  private:
	static void publish( const std::vector<std::shared_ptr<Delivery>>& targets,
						 const TransferProgress& snapshot ) {
		for ( const auto& target : targets ) {
			target->update( [&snapshot]( TransferProgressPolicy::Payload& payload ) {
				payload.progress = snapshot;
				payload.dirty = true;
				return true;
			} );
		}
	}

	CancellationSource mCancel;
	TaskResolver<VFSResult<TransferResult>> mResolveCompletion;
	VFSTask<TransferResult> mCompletion;
	mutable std::mutex mMutex;
	TransferProgress mProgress;
	std::vector<std::pair<Uint64, std::shared_ptr<Delivery>>> mSubscriptions;
	System::Clock mThrottle;
	Uint64 mLastSubscriptionId{ 0 };
	std::atomic<bool> mCompletionTaken{ false };
	bool mDirty{ false };
};

}}} // namespace EE::VFS::detail

#endif
