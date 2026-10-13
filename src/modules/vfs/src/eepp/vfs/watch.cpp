#include <algorithm>
#include <eepp/vfs/detail/coalesceddelivery.hpp>
#include <eepp/vfs/watch.hpp>

namespace EE { namespace VFS {

const char* toString( WatchEventType type ) {
	switch ( type ) {
		case WatchEventType::Added:
			return "Added";
		case WatchEventType::Removed:
			return "Removed";
		case WatchEventType::Modified:
			return "Modified";
		case WatchEventType::Moved:
			return "Moved";
		case WatchEventType::RescanRequired:
			return "RescanRequired";
	}
	return "Unknown";
}

const char* toString( WatchStatus status ) {
	switch ( status ) {
		case WatchStatus::Active:
			return "Active";
		case WatchStatus::Polling:
			return "Polling";
		case WatchStatus::Reconnecting:
			return "Reconnecting";
		case WatchStatus::Failed:
			return "Failed";
	}
	return "Unknown";
}

namespace detail {

struct WatchDeliveryPolicy {
	struct Payload {
		std::vector<WatchEvent> events;
		WatchStatus status{ WatchStatus::Active };
		bool statusDirty{ false };
	};

	using Delivered = WatchEventBatch;

	static bool hasData( const Payload& payload ) {
		return !payload.events.empty() || payload.statusDirty;
	}

	static Delivered take( Payload& payload ) {
		WatchEventBatch batch;
		batch.events.swap( payload.events );
		batch.status = payload.status;
		payload.statusDirty = false;
		return batch;
	}
};

/** Sink + delivery state of one logical watch. Shared by the handle (consumer) and the backend
 * (producer); the backend holds it as a WatchSink. */
class WatchSubscriptionState : public WatchSink {
  public:
	/** Undelivered events beyond this are collapsed into a single RescanRequired, so a watch
	 * whose consumer never attaches (or stalls) cannot grow without bound. */
	static constexpr size_t MaxPendingEvents = 65536;

	using Delivery = CoalescedDelivery<WatchDeliveryPolicy>;

	explicit WatchSubscriptionState( URI root ) :
		mRoot( std::move( root ) ), mDelivery( std::make_shared<Delivery>() ) {}

	void push( WatchEvent&& event ) override {
		mDelivery->update( [&]( WatchDeliveryPolicy::Payload& payload ) {
			append( payload.events, std::move( event ) );
			return true;
		} );
	}

	void push( std::vector<WatchEvent>&& events ) override {
		if ( events.empty() )
			return;
		mDelivery->update( [&]( WatchDeliveryPolicy::Payload& payload ) {
			if ( payload.events.empty() && events.size() <= MaxPendingEvents &&
				 std::none_of( events.begin(), events.end(), []( const WatchEvent& e ) {
					 return e.type == WatchEventType::RescanRequired;
				 } ) ) {
				payload.events = std::move( events );
			} else {
				for ( auto& event : events )
					append( payload.events, std::move( event ) );
			}
			return true;
		} );
	}

	void rescanRequired() override { push( WatchEvent{ {}, {}, WatchEventType::RescanRequired } ); }

	void setStatus( WatchStatus status ) override {
		mDelivery->update( [status]( WatchDeliveryPolicy::Payload& payload ) {
			if ( payload.status == status )
				return false;
			payload.status = status;
			payload.statusDirty = true;
			return true;
		} );
	}

	const URI& root() const override { return mRoot; }

	WatchStatus status() const {
		return mDelivery->inspect(
			[]( const WatchDeliveryPolicy::Payload& payload ) { return payload.status; } );
	}

	void setTarget( Executor& executor, WatchCallback callback ) {
		mDelivery->setTarget( &executor, [root = mRoot, callback = std::move( callback )](
											 WatchEventBatch&& batch ) mutable {
			batch.root = root;
			callback( std::move( batch ) );
		} );
	}

	void close() { mDelivery->close(); }

  private:
	static void append( std::vector<WatchEvent>& pending, WatchEvent&& event ) {
		if ( event.type == WatchEventType::RescanRequired ) {
			// A rescan supersedes every pending incremental event. Events queued after it are
			// kept: the consumer rescans on delivery and they may land after its listing.
			pending.clear();
			pending.push_back( std::move( event ) );
			return;
		}
		if ( pending.size() >= MaxPendingEvents ) {
			pending.clear();
			pending.push_back( WatchEvent{ {}, {}, WatchEventType::RescanRequired } );
			return;
		}
		pending.push_back( std::move( event ) );
	}

	URI mRoot;
	std::shared_ptr<Delivery> mDelivery;
};

} // namespace detail

WatchHandle::WatchHandle( std::shared_ptr<detail::WatchSubscriptionState> state,
						  std::unique_ptr<WatchRegistration> registration ) :
	mState( std::move( state ) ), mRegistration( std::move( registration ) ) {}

WatchHandle& WatchHandle::operator=( WatchHandle&& other ) noexcept {
	if ( this != &other ) {
		reset();
		mState = std::move( other.mState );
		mRegistration = std::move( other.mRegistration );
	}
	return *this;
}

WatchHandle::~WatchHandle() {
	reset();
}

void WatchHandle::onEvents( Executor& executor, WatchCallback callback ) {
	if ( mState )
		mState->setTarget( executor, std::move( callback ) );
}

void WatchHandle::reset() {
	// Stop the producer first so no new events race with the close.
	mRegistration.reset();
	if ( mState ) {
		mState->close();
		mState.reset();
	}
}

WatchStatus WatchHandle::status() const {
	return mState ? mState->status() : WatchStatus::Failed;
}

const URI& WatchHandle::root() const {
	static const URI empty;
	return mState ? mState->root() : empty;
}

WatchManager::WatchManager() = default;

WatchManager::~WatchManager() = default;

void WatchManager::registerBackend( std::string scheme, std::shared_ptr<WatchBackend> backend ) {
	std::unique_lock<std::shared_mutex> lock( mMutex );
	mBackends[std::move( scheme )] = std::move( backend );
}

void WatchManager::unregisterBackend( std::string_view scheme ) {
	std::unique_lock<std::shared_mutex> lock( mMutex );
	mBackends.erase( std::string( scheme ) );
}

void WatchManager::setFallbackBackend( std::shared_ptr<WatchBackend> backend ) {
	std::unique_lock<std::shared_mutex> lock( mMutex );
	mFallback = std::move( backend );
}

std::shared_ptr<WatchBackend> WatchManager::backendFor( const URI& uri ) const {
	std::shared_lock<std::shared_mutex> lock( mMutex );
	auto it = mBackends.find( uri.getScheme() );
	return it != mBackends.end() ? it->second : nullptr;
}

VFSTask<WatchHandle> WatchManager::watch( const URI& root, const WatchOptions& options ) {
	auto backend = backendFor( root );
	if ( !backend && options.allowPolling ) {
		std::shared_lock<std::shared_mutex> lock( mMutex );
		backend = mFallback;
	}
	if ( !backend ) {
		return readyError<WatchHandle>( VFSErrorCode::Unsupported, root,
										"No watch backend for this scheme" );
	}
	auto state = std::make_shared<detail::WatchSubscriptionState>( root );
	return backend->watch( root, options, state )
		.then( [state]( VFSResult<std::unique_ptr<WatchRegistration>>&& result )
				   -> VFSResult<WatchHandle> {
			if ( !result )
				return Err( std::move( result ).error() );
			return Ok( WatchHandle( state, std::move( result ).value() ) );
		} );
}

}} // namespace EE::VFS
