#include <chrono>
#include <condition_variable>
#include <eepp/vfs/detail/await.hpp>
#include <eepp/vfs/virtualfilesystem.hpp>
#include <eepp/vfs/watchers/pollingwatchbackend.hpp>
#include <list>
#include <mutex>
#include <thread>
#include <unordered_map>

namespace EE { namespace VFS {

namespace detail {

class PollingWatchBackendImpl : public std::enable_shared_from_this<PollingWatchBackendImpl> {
  public:
	struct Snapshot {
		struct Entry {
			FileRevision revision;
			Int64 modified{ 0 };
			Uint64 size{ 0 };
			FileType type{ FileType::Unknown };

			bool sameAs( const Entry& other ) const {
				if ( type != other.type )
					return false;
				if ( !revision.empty() || !other.revision.empty() )
					return revision == other.revision;
				return modified == other.modified && size == other.size;
			}
		};
		std::unordered_map<std::string, Entry> entries;
	};

	struct Watch {
		URI root;
		WatchOptions options;
		std::weak_ptr<WatchSink> sink;
		Snapshot snapshot;
		std::chrono::steady_clock::time_point due;
		Uint64 id{ 0 };
		bool failed{ false };
	};

	explicit PollingWatchBackendImpl( VirtualFileSystem& vfs ) : mVfs( vfs ) {}

	~PollingWatchBackendImpl() { stop(); }

	void stop() {
		{
			std::lock_guard<std::mutex> lock( mMutex );
			mStopping = true;
		}
		mWake.notify_all();
		if ( mThread.joinable() ) {
			if ( mThread.get_id() == std::this_thread::get_id() )
				mThread.detach();
			else
				mThread.join();
		}
	}

	/** Takes the initial snapshot on the polling thread and resolves once the watch is active.
	 */
	VFSTask<Uint64> add( URI root, WatchOptions options, std::shared_ptr<WatchSink> sink ) {
		std::lock_guard<std::mutex> lock( mMutex );
		if ( mStopping )
			return readyError<Uint64>( VFSErrorCode::InvalidOperation, root, "Shutting down" );
		TaskResolver<VFSResult<Uint64>> resolve;
		auto task = VFSTask<Uint64>::create( resolve );
		auto watch = std::make_shared<Watch>();
		watch->root = std::move( root );
		watch->options = options;
		watch->sink = sink;
		watch->id = ++mLastId;
		mPendingAdds.push_back( { watch, std::move( resolve ) } );
		ensureThread();
		mWake.notify_all();
		return task;
	}

	void remove( Uint64 id ) {
		std::lock_guard<std::mutex> lock( mMutex );
		mWatches.erase( id );
	}

  private:
	struct PendingAdd {
		std::shared_ptr<Watch> watch;
		TaskResolver<VFSResult<Uint64>> resolve;
	};

	void ensureThread() {
		if ( !mThread.joinable() )
			// A synchronous watch callback may release the backend on this thread. Keep its
			// state alive until the loop observes stop(), without joining the current thread.
			mThread = std::thread( [self = shared_from_this()] { self->loop(); } );
	}

	void loop() {
		std::unique_lock<std::mutex> lock( mMutex );
		while ( !mStopping ) {
			if ( !mPendingAdds.empty() ) {
				auto pending = std::move( mPendingAdds.front() );
				mPendingAdds.pop_front();
				lock.unlock();
				auto snapshot = take( *pending.watch );
				lock.lock();
				if ( !snapshot ) {
					lock.unlock();
					pending.resolve( Err( std::move( snapshot ).error() ) );
					lock.lock();
					continue;
				}
				pending.watch->snapshot = std::move( snapshot ).value();
				pending.watch->due = std::chrono::steady_clock::now() + interval( *pending.watch );
				mWatches[pending.watch->id] = pending.watch;
				Uint64 id = pending.watch->id;
				lock.unlock();
				if ( auto sink = pending.watch->sink.lock() )
					sink->setStatus( WatchStatus::Polling );
				pending.resolve( Ok( id ) );
				lock.lock();
				continue;
			}

			auto now = std::chrono::steady_clock::now();
			std::shared_ptr<Watch> next;
			for ( auto& watch : mWatches ) {
				if ( !next || watch.second->due < next->due )
					next = watch.second;
			}
			if ( !next ) {
				mWake.wait( lock );
				continue;
			}
			if ( next->due > now ) {
				mWake.wait_until( lock, next->due );
				continue;
			}
			next->due = now + interval( *next );
			lock.unlock();
			poll( *next );
			lock.lock();
		}
		// Fail pending registrations instead of abandoning them.
		while ( !mPendingAdds.empty() ) {
			auto pending = std::move( mPendingAdds.front() );
			mPendingAdds.pop_front();
			lock.unlock();
			pending.resolve( Err( VFSError( VFSErrorCode::InvalidOperation, pending.watch->root,
											"Watch backend shutting down" ) ) );
			lock.lock();
		}
	}

	static std::chrono::milliseconds interval( const Watch& watch ) {
		return std::chrono::milliseconds(
			std::max<Int64>( 50, watch.options.pollingInterval.asMilliseconds() ) );
	}

	VFSResult<Snapshot> take( const Watch& watch ) {
		Snapshot snapshot;
		auto rootInfo = await( mVfs.stat( watch.root ) );
		if ( !rootInfo )
			return Err( std::move( rootInfo ).error() );
		if ( !rootInfo.value().isDirectory() )
			return Err( VFSError( VFSErrorCode::NotDirectory, watch.root ) );
		std::vector<std::pair<URI, std::string>> pending{ { watch.root, std::string() } };
		while ( !pending.empty() ) {
			auto [dir, prefix] = std::move( pending.back() );
			pending.pop_back();
			auto listed = await( mVfs.listAll( dir ) );
			if ( !listed ) {
				// A subdirectory vanishing mid-scan is a normal race; the root failing is not.
				if ( prefix.empty() || !listed.error().is( VFSErrorCode::NotFound ) )
					return Err( std::move( listed ).error() );
				continue;
			}
			for ( auto& entry : listed.value() ) {
				if ( snapshot.entries.size() >= PollingWatchBackend::MaxEntries )
					return Ok( std::move( snapshot ) );
				std::string relative = prefix.empty() ? entry.name : prefix + "/" + entry.name;
				Snapshot::Entry value;
				value.revision = entry.revision;
				value.modified = entry.modifiedTime().value_or( 0 );
				value.size = entry.size().value_or( 0 );
				value.type = entry.type;
				if ( watch.options.recursive && entry.type == FileType::Directory )
					pending.emplace_back( dir.withAppendedSegment( entry.name ), relative );
				snapshot.entries.emplace( std::move( relative ), std::move( value ) );
			}
		}
		return Ok( std::move( snapshot ) );
	}

	void poll( Watch& watch ) {
		auto sink = watch.sink.lock();
		if ( !sink )
			return;
		auto snapshot = take( watch );
		if ( !snapshot ) {
			if ( !watch.failed ) {
				watch.failed = true;
				bool rootGone = snapshot.error().is( VFSErrorCode::NotFound ) ||
								snapshot.error().is( VFSErrorCode::NotDirectory );
				if ( rootGone ) {
					sink->rescanRequired();
				} else {
					sink->setStatus( WatchStatus::Reconnecting );
				}
			}
			return;
		}
		if ( watch.failed ) {
			// Continuity was lost while failing: consumers must re-list.
			watch.failed = false;
			watch.snapshot = std::move( snapshot ).value();
			sink->setStatus( WatchStatus::Polling );
			sink->rescanRequired();
			return;
		}
		std::vector<WatchEvent> events;
		auto& previous = watch.snapshot.entries;
		auto& current = snapshot.value().entries;
		for ( const auto& entry : current ) {
			auto it = previous.find( entry.first );
			if ( it == previous.end() ) {
				events.push_back( WatchEvent{ entry.first, {}, WatchEventType::Added } );
			} else if ( !it->second.sameAs( entry.second ) ) {
				events.push_back( WatchEvent{ entry.first, {}, WatchEventType::Modified } );
			}
		}
		for ( const auto& entry : previous ) {
			if ( current.find( entry.first ) == current.end() )
				events.push_back( WatchEvent{ entry.first, {}, WatchEventType::Removed } );
		}
		watch.snapshot = std::move( snapshot ).value();
		if ( !events.empty() )
			sink->push( std::move( events ) );
	}

	VirtualFileSystem& mVfs;
	std::mutex mMutex;
	std::condition_variable mWake;
	std::thread mThread;
	std::unordered_map<Uint64, std::shared_ptr<Watch>> mWatches;
	std::list<PendingAdd> mPendingAdds;
	Uint64 mLastId{ 0 };
	bool mStopping{ false };
};

class PollingWatchRegistration : public WatchRegistration {
  public:
	PollingWatchRegistration( std::weak_ptr<PollingWatchBackendImpl> impl, Uint64 id ) :
		mImpl( std::move( impl ) ), mId( id ) {}

	~PollingWatchRegistration() override {
		if ( auto impl = mImpl.lock() )
			impl->remove( mId );
	}

  private:
	std::weak_ptr<PollingWatchBackendImpl> mImpl;
	Uint64 mId;
};

} // namespace detail

PollingWatchBackend::PollingWatchBackend( VirtualFileSystem& vfs ) :
	mImpl( std::make_shared<detail::PollingWatchBackendImpl>( vfs ) ) {}

PollingWatchBackend::~PollingWatchBackend() {
	mImpl->stop();
}

VFSTask<std::unique_ptr<WatchRegistration>>
PollingWatchBackend::watch( const URI& root, const WatchOptions& options,
							std::shared_ptr<WatchSink> sink ) {
	using Registration = std::unique_ptr<WatchRegistration>;
	if ( !options.allowPolling )
		return readyError<Registration>( VFSErrorCode::Unsupported, root, "Polling disabled" );
	std::weak_ptr<detail::PollingWatchBackendImpl> weak = mImpl;
	return mImpl->add( root, options, std::move( sink ) )
		.then( [weak]( VFSResult<Uint64>&& id ) -> VFSResult<Registration> {
			if ( !id )
				return Err( std::move( id ).error() );
			return Ok( Registration(
				std::make_unique<detail::PollingWatchRegistration>( weak, id.value() ) ) );
		} );
}

}} // namespace EE::VFS
