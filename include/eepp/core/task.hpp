#ifndef EE_CORE_TASK_HPP
#define EE_CORE_TASK_HPP

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <eepp/config.hpp>
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
	/** The last TaskSource was destroyed while its task was still pending. */
	Abandoned,
	/** resolve() was called on an already completed TaskSource; the value was discarded. */
	ResolvedTwice,
	/** TaskSource::task() was called more than once; an empty task was returned. */
	TaskRetrievedTwice,
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
template <typename T> class TaskSource;

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

/** State shared by one TaskSource<T> and one Task<T>.
 *
 * Reference counting is intrusive so that continuations can re-acquire ownership from the plain
 * reference they receive (needed to keep the value alive while a continuation is queued on an
 * executor) and so that every task costs exactly one allocation.
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
			// Notified while holding the lock: the waiter cannot return (and destroy its
			// stack-allocated condition variable) before we release the mutex.
			if ( mWaiter )
				mWaiter->notify_all();
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
			if ( mWaiter )
				mWaiter->notify_all();
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
			std::condition_variable waiter;
			mWaiter = &waiter;
			waiter.wait( lock, [this] {
				return mState.load( std::memory_order_relaxed ) != TaskState::Pending;
			} );
			mWaiter = nullptr;
		}
		return mState.load( std::memory_order_relaxed );
	}

	template <typename Rep, typename Period>
	TaskState waitFor( const std::chrono::duration<Rep, Period>& timeout ) {
		std::unique_lock<std::mutex> lock( mMutex );
		if ( mState.load( std::memory_order_relaxed ) == TaskState::Pending ) {
			std::condition_variable waiter;
			mWaiter = &waiter;
			waiter.wait_for( lock, timeout, [this] {
				return mState.load( std::memory_order_relaxed ) != TaskState::Pending;
			} );
			mWaiter = nullptr;
		}
		return mState.load( std::memory_order_relaxed );
	}

	/** Only valid once Ready, and only from the single consumer. */
	Value& value() noexcept { return *mValue; }

	bool markTaskRetrieved() noexcept {
		return !mTaskRetrieved.exchange( true, std::memory_order_relaxed );
	}

  private:
	std::mutex mMutex;
	Continuation mContinuation;
	std::condition_variable* mWaiter{ nullptr };
	std::optional<Value> mValue;
	std::atomic<Uint32> mRefs{ 1 };
	std::atomic<TaskState> mState{ TaskState::Pending };
	std::atomic<bool> mTaskRetrieved{ false };
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
	/** Abandons a chained source without a diagnostic: the root abandonment was already
	 * reported where it happened. */
	template <typename U> static void abandonSilently( TaskSource<U>& source ) {
		source.abandonIfPending( false );
	}

	/** Resolves `next` with the outcome of `inner` (task flattening). */
	template <typename U> static void forward( Task<U>&& inner, TaskSource<U>&& next ) {
		std::move( inner ).attach( [next = std::move( next )]( TaskSharedState<U>& state ) mutable {
			if ( state.state() != TaskState::Ready ) {
				abandonSilently( next );
			} else if constexpr ( std::is_void_v<U> ) {
				next.resolve();
			} else {
				next.resolve( std::move( state.value() ) );
			}
		} );
	}
};

} // namespace detail

/** @brief Producer side of a Task.
 *
 * Move-only. task() may be called exactly once. Destroying an unresolved source abandons its task,
 * reports TaskDiagnostic::Abandoned and propagates the abandonment through every chained
 * continuation, so consumers never stay silently pending forever.
 */
template <typename T> class TaskSource {
  public:
	TaskSource() : mState( detail::TaskStateRef<T>::adopt( new detail::TaskSharedState<T>() ) ) {}

	TaskSource( TaskSource&& ) noexcept = default;

	TaskSource& operator=( TaskSource&& other ) noexcept {
		if ( this != &other ) {
			abandonIfPending( true );
			mState = std::move( other.mState );
		}
		return *this;
	}

	TaskSource( const TaskSource& ) = delete;

	TaskSource& operator=( const TaskSource& ) = delete;

	~TaskSource() { abandonIfPending( true ); }

	/** @return The consumer task. Valid only on the first call. */
	Task<T> task() {
		if ( !mState || !mState->markTaskRetrieved() ) {
			TaskDiagnostics::report( TaskDiagnostic::TaskRetrievedTwice );
			return Task<T>();
		}
		return Task<T>( detail::TaskStateRef<T>::retain( mState.get() ) );
	}

	/** Completes the task. Continuations attached with then() run synchronously on this thread.
	 * @return False (and reports TaskDiagnostic::ResolvedTwice) if already completed. */
	template <typename... Args> bool resolve( Args&&... args ) {
		static_assert( !std::is_void_v<T> || sizeof...( Args ) == 0,
					   "TaskSource<void>::resolve() takes no arguments" );
		if ( !mState || !mState->resolve( std::forward<Args>( args )... ) ) {
			TaskDiagnostics::report( TaskDiagnostic::ResolvedTwice );
			return false;
		}
		return true;
	}

	/** @return True while the source owns a state that has not been completed. */
	bool isPending() const noexcept { return mState && mState->state() == TaskState::Pending; }

	bool valid() const noexcept { return static_cast<bool>( mState ); }

  private:
	friend struct detail::TaskAccess;

	void abandonIfPending( bool diagnose ) {
		if ( mState && mState->abandon() && diagnose )
			TaskDiagnostics::report( TaskDiagnostic::Abandoned );
		mState.reset();
	}

	detail::TaskStateRef<T> mState;
};

/** @brief Single-consumer handle to a value that a producer will eventually make available.
 *
 * Task is move-only and carries no domain error semantics: fallible operations return
 * Task<Result<T, E>>. Continuations consume the task (`std::move( task ).then( ... )`), so only
 * one continuation chain can ever exist.
 *
 * - then( fn ): runs fn on the thread that resolves the task, or immediately if already ready.
 * - thenOn( executor, fn ): always runs fn through the executor, even if already ready.
 * - fn may return a value, void, or a Task<U>; nested tasks are flattened into Task<U>.
 * - If the producer is abandoned, fn is not called and the returned task is abandoned too.
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

	/** @return A task that is already Ready with the given value. */
	template <typename... Args> static Task ready( Args&&... args ) {
		TaskSource<T> source;
		Task task = source.task();
		source.resolve( std::forward<Args>( args )... );
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

	/** Attaches a continuation, see the class documentation. */
	template <typename F> auto then( F&& fn ) && {
		using Fn = std::decay_t<F>;
		using Out = detail::ContinuationValue<T, Fn>;
		TaskSource<Out> next;
		Task<Out> result = next.task();
		attach( [fn = Fn( std::forward<F>( fn ) ),
				 next = std::move( next )]( detail::TaskSharedState<T>& state ) mutable {
			Task::runContinuation( state, fn, next );
		} );
		return result;
	}

	/** Attaches a continuation that is always scheduled on executor, even when the task is
	 * already ready. The executor must outlive the task chain. */
	template <typename F> auto thenOn( Executor& executor, F&& fn ) && {
		using Fn = std::decay_t<F>;
		using Out = detail::ContinuationValue<T, Fn>;
		TaskSource<Out> next;
		Task<Out> result = next.task();
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

	/** Observes completion without producing a new task. fn( Task<T>&& completed ) receives the
	 * terminal task (Ready or Abandoned) on the completing thread. */
	template <typename F> void onComplete( F&& fn ) && {
		using Fn = std::decay_t<F>;
		attach( [fn = Fn( std::forward<F>( fn ) )]( detail::TaskSharedState<T>& state ) mutable {
			std::invoke( fn, Task( detail::TaskStateRef<T>::retain( &state ) ) );
		} );
	}

	/** Same as onComplete(), but fn is always dispatched through executor. */
	template <typename F> void onCompleteOn( Executor& executor, F&& fn ) && {
		using Fn = std::decay_t<F>;
		attach( [executor = &executor,
				 fn = Fn( std::forward<F>( fn ) )]( detail::TaskSharedState<T>& state ) mutable {
			executor->execute(
				[ref = detail::TaskStateRef<T>::retain( &state ), fn = std::move( fn )]() mutable {
					std::invoke( fn, Task( std::move( ref ) ) );
				} );
		} );
	}

  private:
	template <typename> friend class Task;
	template <typename> friend class TaskSource;
	friend struct detail::TaskAccess;

	explicit Task( detail::TaskStateRef<T> state ) noexcept : mState( std::move( state ) ) {}

	void attach( typename detail::TaskSharedState<T>::Continuation continuation ) {
		if ( !mState ) {
			// An invalid task behaves as abandoned. Run the continuation against a throwaway
			// abandoned state so every chained source is abandoned consistently.
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
								 TaskSource<Out>& next ) {
		if ( state.state() != TaskState::Ready ) {
			detail::TaskAccess::abandonSilently( next );
			return;
		}
		using R = detail::ContinuationResult<T, Fn>;
		if constexpr ( std::is_void_v<R> ) {
			invoke( state, fn );
			next.resolve();
		} else if constexpr ( detail::IsTask<R>::value ) {
			detail::TaskAccess::forward( invoke( state, fn ), std::move( next ) );
		} else {
			next.resolve( invoke( state, fn ) );
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
