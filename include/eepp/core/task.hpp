#ifndef EE_CORE_TASK_HPP
#define EE_CORE_TASK_HPP

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <eepp/config.hpp>
#include <eepp/core/debug.hpp>
#include <eepp/core/executor.hpp>
#include <eepp/core/small_unique_function.hpp>
#include <functional>
#include <mutex>
#include <optional>
#include <type_traits>
#include <utility>

namespace EE {

/** Terminal and non-terminal states of a Task.
 *
 * Abandoned is an infrastructure-contract failure: the producer was destroyed without resolving.
 * It is not a domain error and must not be used as control flow. Domain outcomes such as
 * cancellation or I/O errors belong in the value type (for example Result<T, E>).
 */
enum class TaskState : Uint8 {
	Pending,
	Ready,
	Abandoned,
};

/** Infrastructure misuse reported by tasks. */
enum class TaskDiagnostic : Uint8 {
	/** The last TaskResolver of a pending task was destroyed without being called. */
	Abandoned,
	/** A TaskResolver was called for an already completed task; the value was discarded. */
	ResolvedTwice,
};

/** Diagnostics hooks for task contract violations.
 *
 * Debug builds print a loud diagnostic by default; release builds stay silent. Tests and tools
 * may install a custom handler (for example to count or to hard-assert). The handler can be
 * called from any thread.
 */
class EE_API TaskDiagnostics {
  public:
	using Handler = void ( * )( TaskDiagnostic );

	/** Installs a handler and returns the previous one. nullptr restores the default. */
	static Handler setHandler( Handler handler );

	static void report( TaskDiagnostic diagnostic );
};

template <typename T> class Task;
template <typename T> class TaskResolver;

namespace detail {

struct TaskVoidValue {};

template <typename T> struct IsTask : std::false_type {};

template <typename T> struct IsTask<Task<T>> : std::true_type {};

template <typename T> struct UnwrapTask {
	using Type = T;
};

template <typename T> struct UnwrapTask<Task<T>> {
	using Type = T;
};

template <typename T, typename F> struct ContinuationInvokeResult {
	using Type = std::invoke_result_t<F&, T&&>;
};

template <typename F> struct ContinuationInvokeResult<void, F> {
	using Type = std::invoke_result_t<F&>;
};

template <typename T, typename F>
using ContinuationResult = typename ContinuationInvokeResult<T, F>::Type;

/** Value type produced by a continuation F attached to a Task<T>; nested tasks are flattened. */
template <typename T, typename F>
using ContinuationValue = typename UnwrapTask<ContinuationResult<T, F>>::Type;

/** State shared by one Task<T> and the copies of its TaskResolver<T>.
 *
 * Reference counting is intrusive so that continuations can re-acquire ownership from the plain
 * reference they receive (needed to keep the value alive while a continuation is queued on an
 * executor) and so that every task costs exactly one allocation. Resolver copies are counted
 * apart, so the task is abandoned when the last resolver goes away, whoever else still holds the
 * state.
 */
template <typename T> class TaskSharedState {
  public:
	using Value = std::conditional_t<std::is_void_v<T>, TaskVoidValue, T>;
	using Continuation = SmallUniqueFunction<void( TaskSharedState& ), 64>;

	void retain() noexcept { mRefs.fetch_add( 1, std::memory_order_relaxed ); }

	void release() noexcept {
		if ( mRefs.fetch_sub( 1, std::memory_order_acq_rel ) == 1 )
			delete this;
	}

	void retainResolver() noexcept { mResolvers.fetch_add( 1, std::memory_order_relaxed ); }

	/** @return True if the last resolver was released. */
	bool releaseResolver() noexcept {
		return mResolvers.fetch_sub( 1, std::memory_order_acq_rel ) == 1;
	}

	TaskState state() const noexcept { return mState.load( std::memory_order_acquire ); }

	template <typename... Args> bool resolve( Args&&... args ) {
		Continuation continuation;
		{
			std::lock_guard<std::mutex> lock( mMutex );
			if ( mState.load( std::memory_order_relaxed ) != TaskState::Pending )
				return false;
			mValue.emplace( std::forward<Args>( args )... );
			mState.store( TaskState::Ready, std::memory_order_release );
			continuation = std::move( mContinuation );
			mWaiters.notify_all();
		}
		if ( continuation )
			continuation( *this );
		return true;
	}

	/** @return True if this call moved the state from Pending to Abandoned. */
	bool abandon() {
		Continuation continuation;
		{
			std::lock_guard<std::mutex> lock( mMutex );
			if ( mState.load( std::memory_order_relaxed ) != TaskState::Pending )
				return false;
			mState.store( TaskState::Abandoned, std::memory_order_release );
			continuation = std::move( mContinuation );
			mWaiters.notify_all();
		}
		if ( continuation )
			continuation( *this );
		return true;
	}

	/** Stores the continuation, or runs it immediately if the state is already terminal. Never
	 * invokes user code while holding the mutex. */
	void attach( Continuation continuation ) {
		{
			std::lock_guard<std::mutex> lock( mMutex );
			if ( mState.load( std::memory_order_relaxed ) == TaskState::Pending ) {
				mContinuation = std::move( continuation );
				return;
			}
		}
		continuation( *this );
	}

	TaskState wait() {
		std::unique_lock<std::mutex> lock( mMutex );
		if ( mState.load( std::memory_order_relaxed ) == TaskState::Pending ) {
			mWaiters.wait( lock, [this] {
				return mState.load( std::memory_order_relaxed ) != TaskState::Pending;
			} );
		}
		return mState.load( std::memory_order_relaxed );
	}

	template <typename Rep, typename Period>
	TaskState waitFor( const std::chrono::duration<Rep, Period>& timeout ) {
		std::unique_lock<std::mutex> lock( mMutex );
		if ( mState.load( std::memory_order_relaxed ) == TaskState::Pending ) {
			mWaiters.wait_for( lock, timeout, [this] {
				return mState.load( std::memory_order_relaxed ) != TaskState::Pending;
			} );
		}
		return mState.load( std::memory_order_relaxed );
	}

	/** Only valid once Ready, and only from the single consumer. */
	Value& value() noexcept { return *mValue; }

  private:
	std::mutex mMutex;
	Continuation mContinuation;
	std::condition_variable mWaiters;
	std::optional<Value> mValue;
	std::atomic<Uint32> mRefs{ 1 };
	std::atomic<Uint32> mResolvers{ 1 };
	std::atomic<TaskState> mState{ TaskState::Pending };
};

/** Intrusive owning pointer to a TaskSharedState. */
template <typename T> class TaskStateRef {
  public:
	using State = TaskSharedState<T>;

	TaskStateRef() noexcept = default;

	/** Adopts an existing reference (no retain). */
	static TaskStateRef adopt( State* state ) noexcept {
		TaskStateRef ref;
		ref.mState = state;
		return ref;
	}

	/** Acquires a new reference. */
	static TaskStateRef retain( State* state ) noexcept {
		if ( state )
			state->retain();
		return adopt( state );
	}

	TaskStateRef( TaskStateRef&& other ) noexcept : mState( other.mState ) {
		other.mState = nullptr;
	}

	TaskStateRef& operator=( TaskStateRef&& other ) noexcept {
		if ( this != &other ) {
			reset();
			mState = other.mState;
			other.mState = nullptr;
		}
		return *this;
	}

	TaskStateRef( const TaskStateRef& ) = delete;

	TaskStateRef& operator=( const TaskStateRef& ) = delete;

	~TaskStateRef() { reset(); }

	void reset() noexcept {
		if ( mState ) {
			mState->release();
			mState = nullptr;
		}
	}

	State* get() const noexcept { return mState; }

	State* operator->() const noexcept { return mState; }

	State& operator*() const noexcept { return *mState; }

	explicit operator bool() const noexcept { return nullptr != mState; }

  private:
	State* mState{ nullptr };
};

struct TaskAccess {
	/** Abandons a chained task without a diagnostic: the root abandonment was already reported
	 * where it happened. */
	template <typename U> static void abandonSilently( TaskResolver<U>& resolver ) {
		resolver.abandonNow();
	}

	/** Resolves `next` with the outcome of `inner` (task flattening). */
	template <typename U> static void forward( Task<U>&& inner, TaskResolver<U>&& next ) {
		std::move( inner ).attach( [next = std::move( next )]( TaskSharedState<U>& state ) mutable {
			if ( state.state() != TaskState::Ready ) {
				abandonSilently( next );
			} else if constexpr ( std::is_void_v<U> ) {
				next();
			} else {
				next( std::move( state.value() ) );
			}
		} );
	}
};

} // namespace detail

/** @brief Completes a Task: call it with the value.
 *
 * Filled in by Task<T>::create(), which returns its task. Copyable: every copy completes the same
 * task, so it can be captured by several callbacks, and the first call wins (a later call reports
 * TaskDiagnostic::ResolvedTwice). Calling it runs the continuations attached without an executor
 * (then( fn ), onComplete( fn )) on the calling thread before it returns.
 *
 * When the last copy is destroyed without having been called, the task is abandoned and
 * TaskDiagnostic::Abandoned is reported: its consumer is still told (then() skips its function,
 * onComplete() receives std::nullopt), so it never waits forever.
 *
 * A default-constructed resolver belongs to no task until create() fills it; calling it before
 * that only reports ResolvedTwice.
 */
template <typename T> class TaskResolver {
  public:
	TaskResolver() noexcept = default;

	TaskResolver( const TaskResolver& other ) noexcept :
		mState( detail::TaskStateRef<T>::retain( other.mState.get() ) ) {
		if ( mState )
			mState->retainResolver();
	}

	TaskResolver( TaskResolver&& ) noexcept = default;

	TaskResolver& operator=( const TaskResolver& other ) {
		if ( this != &other ) {
			TaskResolver copy( other );
			*this = std::move( copy );
		}
		return *this;
	}

	TaskResolver& operator=( TaskResolver&& other ) {
		if ( this != &other ) {
			release();
			mState = std::move( other.mState );
		}
		return *this;
	}

	~TaskResolver() { release(); }

	/** Completes the task with the value built from args.
	 * @return False (and reports TaskDiagnostic::ResolvedTwice) if it was already completed. */
	template <typename... Args> bool operator()( Args&&... args ) const {
		static_assert( !std::is_void_v<T> || sizeof...( Args ) == 0,
					   "TaskResolver<void> is called without arguments" );
		if ( !mState || !mState->resolve( std::forward<Args>( args )... ) ) {
			TaskDiagnostics::report( TaskDiagnostic::ResolvedTwice );
			return false;
		}
		return true;
	}

  private:
	friend class Task<T>;
	friend struct detail::TaskAccess;

	explicit TaskResolver( detail::TaskStateRef<T> state ) noexcept :
		mState( std::move( state ) ) {}

	/** Drops this copy; the last copy abandons a still pending task. */
	void release() {
		if ( !mState )
			return;
		if ( mState->releaseResolver() && mState->abandon() )
			TaskDiagnostics::report( TaskDiagnostic::Abandoned );
		mState.reset();
	}

	/** Abandons the task now, without a diagnostic, and drops this copy. */
	void abandonNow() {
		if ( !mState )
			return;
		mState->abandon();
		mState->releaseResolver();
		mState.reset();
	}

	detail::TaskStateRef<T> mState;
};

/** @brief The result of an operation that completes later.
 *
 * The consumer's end. The producer completes it through the TaskResolver that create() fills
 * in. Task is move-only and carries no domain error semantics: fallible operations return
 * Task<Result<T, E>>. Attaching a continuation consumes the task (`std::move( task ).then( ... )`),
 * so only one continuation chain can ever exist.
 *
 * - then( executor, fn ): runs fn through the executor (for example the UI thread's), even if the
 *   task has already completed. This is the normal way to use a result.
 * - then( fn ): runs fn on whichever thread completes the task, or right away on the calling
 *   thread if it already has. For cheap transformations that touch no thread-bound state.
 * - fn may return a value, void, or a Task<U>; nested tasks are flattened into Task<U>.
 * - If the task is abandoned, fn is not called and the returned task is abandoned too.
 * - onComplete( fn ) also observes abandonment: fn receives std::nullopt.
 */
template <typename T> class Task {
  public:
	using ValueType = T;

	/** Creates an invalid task. Its state() is Abandoned. */
	Task() noexcept = default;

	Task( Task&& ) noexcept = default;

	Task& operator=( Task&& ) noexcept = default;

	Task( const Task& ) = delete;

	Task& operator=( const Task& ) = delete;

	/** @return A pending task. resolve is set to the resolver that completes it (a resolver it
	 * held before is released).
	 *
	 * @code
	 * TaskResolver<int> resolve;
	 * auto task = Task<int>::create( resolve );
	 * pool.execute( [resolve] { resolve( 42 ); } );
	 * return task;
	 * @endcode
	 */
	static Task create( TaskResolver<T>& resolve ) {
		auto* state = new detail::TaskSharedState<T>(); // One reference, one resolver.
		resolve = TaskResolver<T>( detail::TaskStateRef<T>::adopt( state ) );
		return Task( detail::TaskStateRef<T>::retain( state ) );
	}

	/** @return A task that is already Ready with the given value. */
	template <typename... Args> static Task ready( Args&&... args ) {
		TaskResolver<T> resolve;
		Task task = create( resolve );
		resolve( std::forward<Args>( args )... );
		return task;
	}

	bool valid() const noexcept { return static_cast<bool>( mState ); }

	TaskState state() const noexcept { return mState ? mState->state() : TaskState::Abandoned; }

	bool isReady() const noexcept { return state() == TaskState::Ready; }

	bool isPending() const noexcept { return state() == TaskState::Pending; }

	/** Blocks until the task is Ready or Abandoned.
	 *
	 * This is a low-level bridge for worker-thread adapters and tests. It must never be called on
	 * the UI thread, nor on the thread that is expected to resolve the task. */
	TaskState wait() const { return mState ? mState->wait() : TaskState::Abandoned; }

	/** Blocks until the task completes or the timeout expires. @return The state on return. */
	template <typename Rep, typename Period>
	TaskState waitFor( const std::chrono::duration<Rep, Period>& timeout ) const {
		return mState ? mState->waitFor( timeout ) : TaskState::Abandoned;
	}

	/** Extracts the value of a Ready task and consumes it. Precondition: isReady(). */
	T takeValue() && {
		eeASSERTM( isReady(), "Task::takeValue() requires a ready task" );
		auto state = std::move( mState );
		if constexpr ( !std::is_void_v<T> )
			return std::move( state->value() );
	}

	/** Blocks until completion and returns the value, or std::nullopt if abandoned.
	 * Same threading restrictions as wait(). */
	auto get() && {
		if constexpr ( std::is_void_v<T> ) {
			return wait() == TaskState::Ready;
		} else {
			if ( wait() != TaskState::Ready )
				return std::optional<T>();
			return std::optional<T>( std::move( *this ).takeValue() );
		}
	}

	/** Runs fn( value ) through executor once the task completes, even if it already has. The
	 * executor must outlive the task chain. See the class documentation. */
	template <typename F> auto then( Executor& executor, F&& fn ) && {
		using Fn = std::decay_t<F>;
		using Out = detail::ContinuationValue<T, Fn>;
		TaskResolver<Out> next;
		Task<Out> result = Task<Out>::create( next );
		attach( [executor = &executor, fn = Fn( std::forward<F>( fn ) ),
				 next = std::move( next )]( detail::TaskSharedState<T>& state ) mutable {
			if ( state.state() != TaskState::Ready ) {
				// No user code runs for abandoned tasks, so there is nothing to dispatch.
				detail::TaskAccess::abandonSilently( next );
				return;
			}
			executor->execute(
				[ref = detail::TaskStateRef<T>::retain( &state ), fn = std::move( fn ),
				 next = std::move( next )]() mutable { Task::runContinuation( *ref, fn, next ); } );
		} );
		return result;
	}

	/** Runs fn( value ) on the thread that completes the task, or right away on the calling
	 * thread if it already has. See the class documentation. */
	template <typename F> auto then( F&& fn ) && {
		using Fn = std::decay_t<F>;
		using Out = detail::ContinuationValue<T, Fn>;
		TaskResolver<Out> next;
		Task<Out> result = Task<Out>::create( next );
		attach( [fn = Fn( std::forward<F>( fn ) ),
				 next = std::move( next )]( detail::TaskSharedState<T>& state ) mutable {
			Task::runContinuation( state, fn, next );
		} );
		return result;
	}

	/** Observes the outcome without producing a new task, on the thread that completes the task
	 * (or right away on the calling thread if it already has). fn( std::optional<T> value )
	 * receives the value, or std::nullopt if the task was abandoned; for Task<void>,
	 * fn( bool completed ) receives false if it was abandoned. */
	template <typename F> void onComplete( F&& fn ) && {
		using Fn = std::decay_t<F>;
		attach( [fn = Fn( std::forward<F>( fn ) )]( detail::TaskSharedState<T>& state ) mutable {
			const bool ready = state.state() == TaskState::Ready;
			if constexpr ( std::is_void_v<T> ) {
				std::invoke( fn, ready );
			} else if ( ready ) {
				std::invoke( fn, std::optional<T>( std::move( state.value() ) ) );
			} else {
				std::invoke( fn, std::optional<T>() );
			}
		} );
	}

  private:
	template <typename> friend class Task;
	friend struct detail::TaskAccess;

	explicit Task( detail::TaskStateRef<T> state ) noexcept : mState( std::move( state ) ) {}

	void attach( typename detail::TaskSharedState<T>::Continuation continuation ) {
		if ( !mState ) {
			// An invalid task behaves as abandoned. Run the continuation against a throwaway
			// abandoned state so every chained task is abandoned consistently.
			detail::TaskStateRef<T> dead =
				detail::TaskStateRef<T>::adopt( new detail::TaskSharedState<T>() );
			dead->abandon();
			dead->attach( std::move( continuation ) );
			return;
		}
		auto state = std::move( mState );
		state->attach( std::move( continuation ) );
	}

	template <typename Fn, typename Out>
	static void runContinuation( detail::TaskSharedState<T>& state, Fn& fn,
								 TaskResolver<Out>& next ) {
		if ( state.state() != TaskState::Ready ) {
			detail::TaskAccess::abandonSilently( next );
			return;
		}
		using R = detail::ContinuationResult<T, Fn>;
		if constexpr ( std::is_void_v<R> ) {
			invoke( state, fn );
			next();
		} else if constexpr ( detail::IsTask<R>::value ) {
			detail::TaskAccess::forward( invoke( state, fn ), std::move( next ) );
		} else {
			next( invoke( state, fn ) );
		}
	}

	template <typename Fn>
	static decltype( auto ) invoke( detail::TaskSharedState<T>& state, Fn& fn ) {
		if constexpr ( std::is_void_v<T> )
			return std::invoke( fn );
		else
			return std::invoke( fn, std::move( state.value() ) );
	}

	detail::TaskStateRef<T> mState;
};

/** @return A Task<T> that is already Ready, constructing T from args. */
template <typename T, typename... Args> Task<T> makeReadyTask( Args&&... args ) {
	return Task<T>::ready( std::forward<Args>( args )... );
}

/** @return A Ready Task<std::decay_t<T>> holding value. */
template <typename T> Task<std::decay_t<T>> makeReadyTask( T&& value ) {
	return Task<std::decay_t<T>>::ready( std::forward<T>( value ) );
}

/** @return A Ready Task<void>. */
inline Task<void> makeReadyTask() {
	return Task<void>::ready();
}

} // namespace EE

#endif
