#ifndef EE_VFS_WATCH_HPP
#define EE_VFS_WATCH_HPP

#include <atomic>
#include <eepp/core/executor.hpp>
#include <eepp/core/small_unique_function.hpp>
#include <eepp/system/time.hpp>
#include <eepp/vfs/error.hpp>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace EE { namespace VFS {

using EE::System::Time;

enum class WatchEventType : Uint8 {
	Added,
	Removed,
	Modified,
	/** relativePath is the new location, oldRelativePath the previous one. */
	Moved,
	/** Exact continuity cannot be guaranteed (missed native events, transport loss, watcher
	 * re-establishment). Consumers must re-list the watched root. Paths are empty. */
	RescanRequired,
};

const char* toString( WatchEventType type );

/** One change, relative to the watched root ("dir/file.txt", '/' separated, never leading '/').
 */
struct WatchEvent {
	std::string relativePath;
	std::string oldRelativePath;
	WatchEventType type{ WatchEventType::Modified };
};

/** Health of a logical watch. */
enum class WatchStatus : Uint8 {
	/** Native or remote-agent notifications are flowing. */
	Active,
	/** Notifications come from a polling fallback (higher latency). */
	Polling,
	/** The backend lost its transport and is re-establishing. Events may be missing; a
	 * RescanRequired event is delivered once it recovers. */
	Reconnecting,
	/** The watch cannot deliver events anymore. */
	Failed,
};

const char* toString( WatchStatus status );

/** A coalesced group of events for one watch. Events are in arrival order. */
struct WatchEventBatch {
	URI root;
	std::vector<WatchEvent> events;
	WatchStatus status{ WatchStatus::Active };
};

struct WatchOptions {
	bool recursive{ false };
	bool followSymlinks{ false };
	bool allowOutOfScopeLinks{ false };
	/** Report moves inside the watched tree as single Moved events where the backend can. */
	bool reportCrossDirectoryMoves{ true };
	/** Interval used when the watch falls back to polling. */
	Time pollingInterval{ System::Seconds( 2 ) };
	/** Allow falling back to polling when no event-driven backend is available. */
	bool allowPolling{ true };
};

using WatchCallback = SmallUniqueFunction<void( WatchEventBatch&& ), 48>;

namespace detail {
class WatchSubscriptionState;
}

/** @brief Receiver used by watch backends to publish events for one logical watch.
 *
 * Thread-safe. Events are accumulated and delivered as one batch per executor dispatch, which
 * coalesces bursts without adding a latency timer.
 */
class WatchSink {
  public:
	virtual ~WatchSink() = default;

	virtual void push( WatchEvent&& event ) = 0;

	virtual void push( std::vector<WatchEvent>&& events ) = 0;

	/** Pushes a RescanRequired event (collapsing consecutive ones). */
	virtual void rescanRequired() = 0;

	virtual void setStatus( WatchStatus status ) = 0;

	virtual const URI& root() const = 0;
};

/** Backend-side registration of one logical watch. Destroying it stops native watching. */
class WatchRegistration {
  public:
	virtual ~WatchRegistration() = default;
};

/** @brief Notification engine for a set of URIs. Backends adapt efsw (local), a remote
 * efsw-agent or polling into URI space; they never mutate UI or model state. */
class WatchBackend {
  public:
	virtual ~WatchBackend() = default;

	/** Establishes a watch. The task resolves once the backend is actively watching (so a listing
	 * started afterwards cannot miss changes). */
	virtual VFSTask<std::unique_ptr<WatchRegistration>>
	watch( const URI& root, const WatchOptions& options, std::shared_ptr<WatchSink> sink ) = 0;
};

/** @brief RAII handle of a logical watch.
 *
 * Events are delivered to the callback registered with onEvents() through the given executor;
 * no backend thread ever calls it directly. Destroying the handle stops the watch and guarantees
 * that the callback is not running and will not run again (a callback may destroy its own
 * handle).
 */
class WatchHandle {
  public:
	WatchHandle() = default;

	WatchHandle( WatchHandle&& ) noexcept = default;

	WatchHandle& operator=( WatchHandle&& other ) noexcept;

	WatchHandle( const WatchHandle& ) = delete;

	WatchHandle& operator=( const WatchHandle& ) = delete;

	~WatchHandle();

	/** Sets the delivery executor and callback. Events that arrived before this call are
	 * delivered right after it. The executor must outlive the handle. */
	void onEvents( Executor& executor, WatchCallback callback );

	/** Stops watching now. */
	void reset();

	WatchStatus status() const;

	const URI& root() const;

	bool isValid() const noexcept { return nullptr != mState; }

  private:
	friend class WatchManager;

	WatchHandle( std::shared_ptr<detail::WatchSubscriptionState> state,
				 std::unique_ptr<WatchRegistration> registration );

	std::shared_ptr<detail::WatchSubscriptionState> mState;
	std::unique_ptr<WatchRegistration> mRegistration;
};

/** @brief Selects a backend for each URI and creates logical watches.
 *
 * Backends are registered per scheme. A scheme without a dedicated backend uses the fallback
 * backend (normally polling through the VFS) when WatchOptions::allowPolling is set.
 */
class WatchManager {
  public:
	WatchManager();

	~WatchManager();

	void registerBackend( std::string scheme, std::shared_ptr<WatchBackend> backend );

	void unregisterBackend( std::string_view scheme );

	void setFallbackBackend( std::shared_ptr<WatchBackend> backend );

	std::shared_ptr<WatchBackend> backendFor( const URI& uri ) const;

	VFSTask<WatchHandle> watch( const URI& root, const WatchOptions& options = {} );

  private:
	mutable std::shared_mutex mMutex;
	std::unordered_map<std::string, std::shared_ptr<WatchBackend>> mBackends;
	std::shared_ptr<WatchBackend> mFallback;
};

}} // namespace EE::VFS

#endif
