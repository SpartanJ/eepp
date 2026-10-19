#include <eepp/vfs/providers/localioscheduler.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <system_error>
#include <thread>
#include <unordered_map>
#include <vector>

using namespace EE::System;

namespace EE { namespace VFS {

using Clock = std::chrono::steady_clock;

/** The calls of one network mount, on threads of their own. */
class MountLane : public std::enable_shared_from_this<MountLane> {
  public:
	/** Each lane runs this many calls at once: enough for a listing next to a read, while an
	 * unreachable server blocks no more threads than this. */
	static constexpr size_t Threads = 2;

	MountLane( LocalIOScheduler::NetworkMount mount, Clock::duration timeout ) :
		mMount( std::move( mount ) ), mTimeout( timeout ) {}

	void start() {
		for ( size_t i = 0; i < Threads; ++i )
			mWorkers[i].thread = std::thread( [self = shared_from_this(), i] { self->work( i ); } );
	}

	/** Queues call, or fails it at once while the mount is not responding. */
	void enqueue( std::shared_ptr<LocalIOScheduler::Call> call ) {
		std::unique_lock<std::mutex> lock( mMutex );
		if ( mStopping ) {
			lock.unlock();
			call->fail(
				VFSError( VFSErrorCode::Cancelled, {}, "The file system is shutting down" ) );
			return;
		}
		if ( stuck( Clock::now() ) ) {
			lock.unlock();
			call->fail( notResponding() );
			return;
		}
		mQueue.push_back( std::move( call ) );
		mWork.notify_one();
	}

	/** Collects in expired the calls running for longer than the timeout, and every call queued
	 * behind them, for the caller to fail.
	 * @return Whether the lane still has calls, queued or running. */
	bool expire( Clock::time_point now,
				 std::vector<std::shared_ptr<LocalIOScheduler::Call>>& expired ) {
		bool busy = false;
		{
			std::lock_guard<std::mutex> lock( mMutex );
			if ( stuck( now ) ) {
				for ( auto& worker : mWorkers ) {
					if ( worker.running && now - worker.started > mTimeout )
						expired.push_back( worker.running );
				}
				expired.insert( expired.end(), mQueue.begin(), mQueue.end() );
				mQueue.clear();
			}
			busy = !mQueue.empty();
			for ( const auto& worker : mWorkers )
				busy |= worker.running != nullptr;
		}
		return busy;
	}

	VFSError notResponding() const {
		return VFSError( VFSErrorCode::TimedOut, URI::fromFSPath( mMount.root ),
						 mMount.source + " is not responding" );
	}

	/** Fails the queued calls and ends the threads. A thread still inside a call is detached
	 * instead of joined: it may never return. It keeps the lane alive until it does. So is the
	 * calling thread when it is one of the lane's (a finished call released the last owner). */
	void stop() {
		std::deque<std::shared_ptr<LocalIOScheduler::Call>> queued;
		std::array<bool, Threads> inCall{};
		{
			std::lock_guard<std::mutex> lock( mMutex );
			mStopping = true;
			queued.swap( mQueue );
			for ( size_t i = 0; i < Threads; ++i )
				inCall[i] = mWorkers[i].running != nullptr;
			mWork.notify_all();
		}
		for ( auto& call : queued )
			call->fail(
				VFSError( VFSErrorCode::Cancelled, {}, "The file system is shutting down" ) );
		for ( size_t i = 0; i < Threads; ++i ) {
			if ( !mWorkers[i].thread.joinable() )
				continue;
			if ( inCall[i] || mWorkers[i].thread.get_id() == std::this_thread::get_id() )
				mWorkers[i].thread.detach();
			else
				mWorkers[i].thread.join();
		}
	}

  private:
	struct Worker {
		std::thread thread;
		std::shared_ptr<LocalIOScheduler::Call> running;
		Clock::time_point started;
	};

	void work( size_t index ) {
		Worker& worker = mWorkers[index];
		std::unique_lock<std::mutex> lock( mMutex );
		for ( ;; ) {
			mWork.wait( lock, [this] { return mStopping || !mQueue.empty(); } );
			if ( mStopping )
				return;
			worker.running = std::move( mQueue.front() );
			mQueue.pop_front();
			worker.started = Clock::now();
			auto call = worker.running;
			lock.unlock();
			call->run();
			lock.lock();
			worker.running.reset();
			// The last reference: destroying the call can release anything it captured, up to
			// the scheduler itself, whose teardown locks this lane.
			lock.unlock();
			call.reset();
			lock.lock();
		}
	}

	/** Whether a call has been running for longer than the timeout. Under mMutex. */
	bool stuck( Clock::time_point now ) const {
		for ( const auto& worker : mWorkers ) {
			if ( worker.running && now - worker.started > mTimeout )
				return true;
		}
		return false;
	}

	LocalIOScheduler::NetworkMount mMount;
	Clock::duration mTimeout;
	std::mutex mMutex;
	std::condition_variable mWork;
	std::deque<std::shared_ptr<LocalIOScheduler::Call>> mQueue;
	std::array<Worker, Threads> mWorkers;
	bool mStopping{ false };
};

/** Owns the lanes (one per network mount, created on first use) and expires their calls. Its
 * thread sleeps while no lane has calls. */
class MountWatchdog {
  public:
	explicit MountWatchdog( Clock::duration timeout ) :
		mState( std::make_shared<State>( timeout ) ) {}

	~MountWatchdog() {
		std::vector<std::shared_ptr<MountLane>> lanes;
		{
			std::lock_guard<std::mutex> lock( mState->mutex );
			mState->stopping = true;
			mState->wake.notify_all();
			for ( auto& lane : mState->lanes )
				lanes.push_back( lane.second );
		}
		// Destroyed by its own thread when a call it releases held the last owner: that thread
		// cannot wait for itself. It owns the state it uses, and ends at its next check.
		if ( mThread.joinable() ) {
			if ( mThread.get_id() == std::this_thread::get_id() )
				mThread.detach();
			else
				mThread.join();
		}
		for ( auto& lane : lanes )
			lane->stop();
	}

	std::shared_ptr<MountLane> laneFor( LocalIOScheduler::NetworkMount mount ) {
		std::lock_guard<std::mutex> lock( mState->mutex );
		auto& lane = mState->lanes[mount.root];
		if ( !lane ) {
			lane = std::make_shared<MountLane>( std::move( mount ), mState->timeout );
			lane->start();
			if ( !mThread.joinable() )
				mThread = std::thread( [state = mState] { watch( *state ); } );
		}
		return lane;
	}

	/** A lane received a call: check it until it has none. */
	void wake() {
		std::lock_guard<std::mutex> lock( mState->mutex );
		mState->busy = true;
		mState->wake.notify_all();
	}

  private:
	/** What the thread uses, shared with it so that it outlives the watchdog if needed. */
	struct State {
		explicit State( Clock::duration timeout ) : timeout( timeout ) {}

		const Clock::duration timeout;
		std::mutex mutex;
		std::condition_variable wake;
		std::unordered_map<std::string, std::shared_ptr<MountLane>> lanes;
		bool busy{ false };
		bool stopping{ false };
	};

	static void watch( State& state ) {
		// Often enough that a call fails close to its timeout, rarely enough to cost nothing.
		const auto interval =
			std::max<Clock::duration>( std::chrono::milliseconds( 50 ), state.timeout / 8 );
		std::unique_lock<std::mutex> lock( state.mutex );
		for ( ;; ) {
			if ( state.busy )
				state.wake.wait_for( lock, interval );
			else
				state.wake.wait( lock, [&state] { return state.stopping || state.busy; } );
			if ( state.stopping )
				return;
			std::vector<std::shared_ptr<MountLane>> lanes;
			for ( auto& lane : state.lanes )
				lanes.push_back( lane.second );
			state.busy = false;
			lock.unlock();
			bool busy = false;
			const auto now = Clock::now();
			for ( auto& lane : lanes ) {
				std::vector<std::shared_ptr<LocalIOScheduler::Call>> expired;
				busy |= lane->expire( now, expired );
				if ( expired.empty() )
					continue;
				// May destroy the watchdog (see ~MountWatchdog()): only state is used after.
				deliver( std::move( expired ), lane->notResponding() );
			}
			lanes.clear();
			lock.lock();
			state.busy |= busy;
		}
	}

	/** Fails the expired calls on a thread of their own, detached: their completions run there.
	 * Not on the watchdog's thread, nor on the I/O pool's, where a completion that blocks would
	 * delay the timeouts of the other mounts. Destroying the calls there may also release the last
	 * owner of anything they hold, and nothing ever waits for a detached thread to end. Timeouts
	 * are rare (a mount stopped answering): a thread each costs nothing. */
	static void deliver( std::vector<std::shared_ptr<LocalIOScheduler::Call>> expired,
						 VFSError error ) {
		// A call that already completed ignores this (Call::claim()).
		auto failAll = [expired = std::move( expired ), error = std::move( error )]() mutable {
			for ( auto& call : expired )
				call->fail( error );
			expired.clear();
		};
		try {
			// A copy (shared calls): if no thread starts, failAll still holds them.
			std::thread( failAll ).detach();
		} catch ( const std::system_error& ) {
			failAll(); // No thread available: here, late rather than never.
		}
	}

	std::shared_ptr<State> mState;
	std::thread mThread;
};

LocalIOScheduler::LocalIOScheduler( std::weak_ptr<ThreadPool> pool, Time timeout,
									MountLookup lookup ) :
	mPool( std::move( pool ) ), mCustomLookup( [&lookup] {
		using Function = std::optional<NetworkMount> ( * )( const std::string& );
		const Function* function = lookup.target<Function>();
		return !function || *function != &LocalIOScheduler::findNetworkMount;
	}() ) {
	mRouting = std::make_shared<const Routing>(
		Routing{ std::move( lookup ), std::make_shared<MountWatchdog>( std::chrono::microseconds(
										  std::max<Int64>( 1, timeout.asMicroseconds() ) ) ) } );
}

LocalIOScheduler::~LocalIOScheduler() = default;

LocalIOScheduler::Route LocalIOScheduler::routeFor( const std::string& path ) {
	return mRouting->routeFor( path );
}

LocalIOScheduler::Route LocalIOScheduler::resolvedRouteFor( const std::string& path,
															bool followLast ) {
	return mRouting->resolvedRouteFor( path, followLast );
}

bool LocalIOScheduler::linksMayReachNetworkMounts() const {
	return mCustomLookup || hasNetworkMounts();
}

LocalIOScheduler::Route LocalIOScheduler::Routing::routeFor( const std::string& path ) const {
	Route route;
	if ( auto mount = lookup( path ) )
		route.mLane = watchdog->laneFor( std::move( *mount ) );
	return route;
}

LocalIOScheduler::Route LocalIOScheduler::Routing::resolvedRouteFor( const std::string& path,
																	 bool followLast ) const {
	return routeFor( resolvePath( path, followLast, lookup ) );
}

void LocalIOScheduler::Routing::enqueue( const Route& route, std::shared_ptr<Call> call ) const {
	route.mLane->enqueue( std::move( call ) );
	watchdog->wake();
}

}} // namespace EE::VFS
