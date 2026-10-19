#ifndef EE_VFS_PROVIDERS_LOCALIOSCHEDULER_HPP
#define EE_VFS_PROVIDERS_LOCALIOSCHEDULER_HPP

#include <atomic>
#include <eepp/core/task.hpp>
#include <eepp/system/threadpool.hpp>
#include <eepp/system/time.hpp>
#include <eepp/vfs/config.hpp>
#include <eepp/vfs/error.hpp>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace EE { namespace VFS {

class MountLane;
class MountWatchdog;

/** @brief Runs blocking local filesystem calls, keeping network mounts apart.
 *
 * Calls on local disks run on the shared I/O pool. Calls under a network mount (NFS, SMB, SSHFS,
 * ...) run on that mount's own lane: an unreachable server then blocks at most that lane's
 * threads, never the pool that every local folder needs.
 *
 * A call on a lane that does not return within the timeout fails with TimedOut, and so does every
 * call queued behind it. Until the stuck call returns, further calls on that mount fail at once.
 * The kernel cannot interrupt a call on an unreachable hard NFS mount, so its thread stays blocked;
 * nobody waits for it anymore, and its result is discarded if it ever arrives.
 */
class EE_VFS_API LocalIOScheduler {
  public:
	/** A file system mounted from another machine. */
	struct EE_VFS_API NetworkMount {
		/** Where it is mounted (a native path). */
		std::string root;
		/** What is mounted, for messages ("server:/export", "//server/share"). */
		std::string source;
	};

	/** Where the calls of a path run: its network mount's lane, or the shared pool. Resolve it
	 * once for a handle (an open file or directory) and reuse it for the handle's calls. */
	class EE_VFS_API Route {
	  public:
		bool isNetworkMount() const { return mLane != nullptr; }

		/** The same pool or the same mount's lane. Paths on different routes are never on the
		 * same file system. */
		bool operator==( const Route& other ) const { return mLane == other.mLane; }

	  private:
		friend class LocalIOScheduler;

		std::shared_ptr<MountLane> mLane;
	};

	/** Finds the network mount of a native path (findNetworkMount(), or a test's stand-in). */
	using MountLookup = std::function<std::optional<NetworkMount>( const std::string& path )>;

	/** @param pool Where the calls on local disks run. Not owned: its owner keeps it for as long
	 * as the scheduler is used. Once it is gone, every call submitted fails with Cancelled, on
	 * network mounts too (the file system is shutting down). Owning
	 * it would let a call that holds the last scheduler reference (through a file handle)
	 * destroy the pool on one of its own threads, which waits for that thread.
	 * @param timeout How long a call on a network mount may take before the mount counts as not
	 * responding. */
	LocalIOScheduler( std::weak_ptr<System::ThreadPool> pool, System::Time timeout,
					  MountLookup lookup = &LocalIOScheduler::findNetworkMount );

	~LocalIOScheduler();

	LocalIOScheduler( const LocalIOScheduler& ) = delete;

	LocalIOScheduler& operator=( const LocalIOScheduler& ) = delete;

	Route routeFor( const std::string& path );

	/** Runs fn() -> VFSResult<T> where route's calls run. */
	template <typename T, typename F> VFSTask<T> submit( const Route& route, F&& fn );

	template <typename T, typename F> VFSTask<T> submit( const std::string& path, F&& fn ) {
		return submit<T>( routeFor( path ), std::forward<F>( fn ) );
	}

	/** submit() for a call that writes into the caller's memory, which the caller may release as
	 * soon as the task fails with TimedOut, while the call itself goes on. work() -> R runs where
	 * route's calls run, without touching that memory; commit( R&& ) -> VFSResult<T> runs after it
	 * only if the call was not timed out meanwhile, and completes the task. A read reads into a
	 * buffer of its own, and commit copies it into the caller's. */
	template <typename T, typename W, typename C>
	VFSTask<T> submitCommitted( const Route& route, W&& work, C&& commit );

	/** Runs fn() -> VFSResult<T> where path's calls run once its symbolic links are followed: a
	 * path through a link into a network mount runs on that mount's lane. The links are read on
	 * the shared pool first (they are on local disks), only when some network mount exists.
	 * @param followLast Whether the call follows a link in the last component (stat, open), or
	 * acts on the link itself (lstat, remove). */
	template <typename T, typename F>
	VFSTask<T> submitResolved( const std::string& path, bool followLast, F&& fn );

	/** submitResolved() for a call on two paths that must be on the same file system (rename):
	 * fails with `different`, without running fn, when they resolve to different routes. The
	 * last component of each is not followed. */
	template <typename T, typename F>
	VFSTask<T> submitOnSameRoute( const std::string& from, const std::string& to,
								  VFSError different, F&& fn );

	/** The route of path once its symbolic links are followed (see resolvePath()). Blocking: call
	 * it where path's calls run. */
	Route resolvedRouteFor( const std::string& path, bool followLast );

	/** path with its symbolic links followed, until it reaches a network mount: the rest is
	 * left as written, so the mount itself is never asked. Blocking: reads the links on local
	 * disks. A path that cannot be resolved (missing, a loop) is returned as it is, for the call
	 * itself to report. Links are not followed on Windows yet.
	 * @param followLast Whether a link in the last component is followed too. */
	static std::string
	resolvePath( const std::string& path, bool followLast,
				 const MountLookup& lookup = &LocalIOScheduler::findNetworkMount );

	/** The network mount that path is on, if any. Never touches the mount itself, so it does not
	 * block on an unreachable server. Symbolic links into a mount are not followed. */
	static std::optional<NetworkMount> findNetworkMount( const std::string& path );

	/** The names of the network mounts mounted directly in directory (a native path): what a
	 * listing must not stat, since that would ask their servers. */
	static std::vector<std::string> networkMountsIn( const std::string& directory );

	/** A call queued on a lane: run once on the lane's thread, or failed by the watchdog, and
	 * only the first of the two completes its task. */
	class EE_VFS_API Call {
	  public:
		virtual ~Call() = default;

		virtual void run() = 0;

		virtual void fail( VFSError error ) = 0;

	  protected:
		/** True for the first completion only. */
		bool claim() { return !mCompleted.exchange( true ); }

	  private:
		std::atomic<bool> mCompleted{ false };
	};

  private:
	/** The commit of calls without one: their result as it is. */
	struct AsIs {
		template <typename R> R operator()( R&& result ) const { return std::move( result ); }
	};

	template <typename T, typename F, typename C = AsIs> class CallOf;

	/** What the routing needs, shared with the calls that resolve their route on the pool (which
	 * can outlive the scheduler). */
	struct EE_VFS_API Routing {
		MountLookup lookup;
		std::shared_ptr<MountWatchdog> watchdog;

		Route routeFor( const std::string& path ) const;

		Route resolvedRouteFor( const std::string& path, bool followLast ) const;

		void enqueue( const Route& route, std::shared_ptr<Call> call ) const;
	};

	/** Runs pick( routing ) -> VFSResult<Route> on the shared pool, then fn() on the route it
	 * picks (right away when that is the pool), or fails with pick's error. */
	template <typename T, typename P, typename F> VFSTask<T> submitAfter( P pick, F&& fn );

	/** Whether a symbolic link may lead into a network mount: some network mount exists (always
	 * true with a test's lookup). */
	bool linksMayReachNetworkMounts() const;

	/** Whether any network mount is mounted. Never touches the mounts. */
	static bool hasNetworkMounts();

	/** The pool, or a Cancelled error once it is gone. */
	template <typename T> static VFSTask<T> poolGone() {
		return readyError<T>(
			VFSError( VFSErrorCode::Cancelled, {}, "The file system is shutting down" ) );
	}

	std::weak_ptr<System::ThreadPool> mPool;
	std::shared_ptr<const Routing> mRouting;
	bool mCustomLookup;
};

template <typename T, typename F, typename C> class LocalIOScheduler::CallOf : public Call {
  public:
	CallOf( F fn, TaskResolver<VFSResult<T>> resolve, C commit = {} ) :
		mFn( std::move( fn ) ), mCommit( std::move( commit ) ), mResolve( std::move( resolve ) ) {}

	void run() override {
		auto result = mFn();
		// Once claimed, the watchdog can no longer fail the task: its caller is still waiting.
		if ( claim() )
			mResolve( mCommit( std::move( result ) ) );
	}

	void fail( VFSError error ) override {
		if ( claim() )
			mResolve( Err( std::move( error ) ) );
	}

  private:
	F mFn;
	C mCommit;
	TaskResolver<VFSResult<T>> mResolve;
};

template <typename T, typename F>
VFSTask<T> LocalIOScheduler::submit( const Route& route, F&& fn ) {
	if ( !route.mLane ) {
		if ( auto pool = mPool.lock() )
			return pool->submit( std::forward<F>( fn ) );
		return poolGone<T>();
	}
	if ( mPool.expired() )
		return poolGone<T>();
	TaskResolver<VFSResult<T>> resolve;
	auto task = VFSTask<T>::create( resolve );
	mRouting->enqueue( route, std::make_shared<CallOf<T, std::decay_t<F>>>(
								  std::forward<F>( fn ), std::move( resolve ) ) );
	return task;
}

template <typename T, typename W, typename C>
VFSTask<T> LocalIOScheduler::submitCommitted( const Route& route, W&& work, C&& commit ) {
	using Work = std::decay_t<W>;
	using Commit = std::decay_t<C>;
	if ( !route.mLane ) {
		auto pool = mPool.lock();
		if ( !pool )
			return poolGone<T>();
		// The pool's calls never time out: the caller waits for them.
		return pool->submit(
			[work = Work( std::forward<W>( work ) ),
			 commit = Commit( std::forward<C>( commit ) )]() mutable { return commit( work() ); } );
	}
	if ( mPool.expired() )
		return poolGone<T>();
	TaskResolver<VFSResult<T>> resolve;
	auto task = VFSTask<T>::create( resolve );
	mRouting->enqueue( route, std::make_shared<CallOf<T, Work, Commit>>(
								  Work( std::forward<W>( work ) ), std::move( resolve ),
								  Commit( std::forward<C>( commit ) ) ) );
	return task;
}

template <typename T, typename P, typename F>
VFSTask<T> LocalIOScheduler::submitAfter( P pick, F&& fn ) {
	using Fn = std::decay_t<F>;
	TaskResolver<VFSResult<T>> resolve;
	auto task = VFSTask<T>::create( resolve );
	auto pool = mPool.lock();
	if ( !pool )
		return poolGone<T>();
	pool->submit( [routing = mRouting, pick = std::move( pick ), fn = Fn( std::forward<F>( fn ) ),
				   resolve = std::move( resolve )]() mutable {
		VFSResult<Route> route = pick( *routing );
		if ( !route ) {
			resolve( Err( std::move( route ).error() ) );
		} else if ( !route.value().mLane ) {
			resolve( fn() );
		} else {
			routing->enqueue( route.value(), std::make_shared<CallOf<T, Fn>>(
												 std::move( fn ), std::move( resolve ) ) );
		}
	} );
	return task;
}

template <typename T, typename F>
VFSTask<T> LocalIOScheduler::submitResolved( const std::string& path, bool followLast, F&& fn ) {
	Route route = routeFor( path );
	// Already on a mount (whatever follows is the mount's to resolve), or no mount to reach.
	if ( route.mLane || !linksMayReachNetworkMounts() )
		return submit<T>( route, std::forward<F>( fn ) );
	return submitAfter<T>(
		[path, followLast]( const Routing& routing ) -> VFSResult<Route> {
			return Ok( routing.resolvedRouteFor( path, followLast ) );
		},
		std::forward<F>( fn ) );
}

template <typename T, typename F>
VFSTask<T> LocalIOScheduler::submitOnSameRoute( const std::string& from, const std::string& to,
												VFSError different, F&& fn ) {
	Route fromRoute = routeFor( from );
	Route toRoute = routeFor( to );
	if ( !linksMayReachNetworkMounts() || ( fromRoute.mLane && toRoute.mLane ) ) {
		if ( !( fromRoute == toRoute ) )
			return readyError<T>( std::move( different ) );
		return submit<T>( fromRoute, std::forward<F>( fn ) );
	}
	return submitAfter<T>(
		[from, to,
		 different = std::move( different )]( const Routing& routing ) -> VFSResult<Route> {
			Route route = routing.resolvedRouteFor( from, false );
			if ( !( route == routing.resolvedRouteFor( to, false ) ) )
				return Err( different );
			return Ok( std::move( route ) );
		},
		std::forward<F>( fn ) );
}

}} // namespace EE::VFS

#endif
