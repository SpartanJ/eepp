# Async core

## Introduction

eepp's async core is a small set of primitives for work that finishes later: file and network
I/O, background computation, anything a UI must not block on. They live in eepp core (no module
needed) and are designed to be used together:

| Header | Types | Role |
| --- | --- | --- |
| `<eepp/core/result.hpp>` | `Result<T, E>`, `Ok()`, `Err()` | A value or a typed error, no exceptions. |
| `<eepp/core/task.hpp>` | `Task<T>`, `TaskResolver<T>`, `makeReadyTask()`, `TaskDiagnostics` | A value that becomes available later, with continuations. |
| `<eepp/core/executor.hpp>` | `Executor`, `InlineExecutor` | Where continuations and jobs run. |
| `<eepp/system/threadpool.hpp>` | `ThreadPool` (an `Executor`, plus `submit()`) | Background threads that return tasks. |
| `<eepp/scene/mainthreadexecutor.hpp>` | `MainThreadExecutor` | Runs continuations on the UI thread. |
| `<eepp/core/cancellationtoken.hpp>` | `CancellationSource`, `CancellationToken`, `CancellationRegistration` | Cooperative cancellation. |
| `<eepp/core/small_unique_function.hpp>` | `SmallUniqueFunction<Sig, N>` | Move-only callable with inline storage (the executor work item). |

The pieces fit as one pipeline:

```text
producer (thread pool, I/O thread)          consumer (usually the UI)
TaskResolver<Result<T, E>>  --resolve( value )-->  Task<Result<T, E>>  --then( executor, fn )-->  fn( Result )
        ^                                                                      |
        +------------------- CancellationToken <---- CancellationSource -------+
```

The [virtual file system](vfs.md) is the main user: every VFS operation returns
`VFSTask<T>`, an alias of `Task<Result<T, VFSError>>`.

## Result

`Result<T, E>` holds either a value or an error. Construct it from `Ok( value )` / `Err( error )`
(`Ok()` for `Result<void, E>`):

```cpp
Result<int, std::string> parsePort( std::string_view text ) {
	int port = 0;
	if ( !String::fromString( port, std::string( text ) ) || port <= 0 || port > 65535 )
		return Err( std::string( "invalid port" ) );
	return Ok( port );
}

auto port = parsePort( "8080" );
if ( port )
	use( port.value() );
else
	log( port.error() );
```

- `hasValue()`, `hasError()` and `explicit operator bool` test the state; `value()` and `error()`
  assert in debug builds when used on the wrong state.
- Errors are values. Do not use exceptions or sentinel values for expected failures.

## Task and TaskResolver

A `Task<T>` is a value that arrives later. `Task<T>::create( resolve )` returns a pending task and
fills `resolve` with the function that completes it:

```cpp
Task<int> computeLater( ThreadPool& pool ) {
	TaskResolver<int> resolve;
	auto task = Task<int>::create( resolve );
	pool.execute( [resolve] { resolve( 42 ); } );
	return task;
}
```

`TaskResolver` is copyable, so it can be captured by several callbacks (a dialog's accept and
cancel buttons, for example). The first call completes the task; a later call is ignored and
reported as `ResolvedTwice`. Calling it runs the continuations attached without an executor before
it returns.

Consumers attach one continuation chain. Continuations consume the task, so it is always written
as `std::move( task ).then( ... )` (a temporary needs no `std::move`):

| Method | Runs `fn` |
| --- | --- |
| `then( executor, fn )` | Always through `executor`, even when already ready. |
| `then( fn )` | On the thread that resolves the task, or immediately if it is already ready. |
| `onComplete( fn )` | Like `then( fn )`, but also when abandoned: `fn( std::optional<T> )` gets `std::nullopt`. It produces nothing. |

`fn` may return a value, `void` or another `Task<U>`; nested tasks are flattened, so chains stay
`Task<U>` instead of `Task<Task<U>>`:

```cpp
vfs.stat( uri )
	.then( [&vfs, uri]( VFSResult<VFSFileInfo>&& info ) -> VFSTask<std::vector<Uint8>> {
		if ( !info || !info.value().isRegularFile() )
			return makeReadyTask(
				VFSResult<std::vector<Uint8>>( Err( VFSError( VFSErrorCode::NotFound, uri ) ) ) );
		return vfs.readAll( uri );
	} )
	.then( uiExecutor, []( VFSResult<std::vector<Uint8>>&& data ) { /* UI thread */ } );
```

### Abandonment

If the last copy of a `TaskResolver` is destroyed without being called, the task becomes
`Abandoned`: its continuations do not run (`onComplete` receives `std::nullopt`) and the
abandonment propagates down the chain. This is an
infrastructure bug (a producer was lost), not a domain outcome, so it is reported through
`TaskDiagnostics` (debug builds print a diagnostic; tools and tests can install a handler with
`TaskDiagnostics::setHandler()`). Domain outcomes, including cancellation, belong in the value:
use `Task<Result<T, E>>`.

`TaskDiagnostics` also reports `ResolvedTwice`.

### Blocking

`wait()`, `waitFor()` and `get()` block the calling thread (`get()` returns `std::optional<T>`,
empty when abandoned). They exist for worker-thread adapters and tests. Never block the UI thread, and never block a thread the producer needs
to make progress (for example a pool waiting on itself).

### Ready tasks

`makeReadyTask( value )` (or `Task<T>::ready( ... )`) creates an already-resolved task, useful for
early returns and synchronous fast paths. `then( fn )` on a ready task runs immediately;
`then( executor, fn )` still goes through its executor.

## Executors

An `Executor` accepts `Executor::Function` work items (`SmallUniqueFunction<void(), 64>`) and runs
each exactly once, unless the executor is being destroyed, in which case pending items may be
destroyed without running.

- `InlineExecutor::instance()`: runs immediately on the calling thread.
- `ThreadPool`: background workers. `run()` queues a function; `submit( fn )` returns a
  `Task` with `fn`'s result:

  ```cpp
  auto pool = ThreadPool::createShared( 4 );
  pool->submit( [] { return countLines( path ); } )   // Task<size_t>
	  .then( uiExecutor, []( size_t lines ) { showLineCount( lines ); } );
  ```

- `Scene::MainThreadExecutor( sceneNode )`: queues functions for the UI thread and drains them in
  submission order during the scene update. Execution is always deferred, even when `execute()` is
  called on the main thread. Destroying it drops queued functions.

Use `then( uiExecutor, ... )` for anything that touches widgets or models. `then( fn )` runs where
the task resolves, which for I/O is a worker or I/O thread.

### Executor lifetime

`then( executor, fn )` keeps a plain reference to its executor until the continuation is dispatched. The
executor must outlive every task chain dispatched through it. Objects that start operations and
can be destroyed before those operations finish (for example a model that is closed while a
listing is in flight) should not hand out the executor directly: route continuations through an
executor they own and close on destruction. `VFSDirectoryModel` does this; once closed, its
gate runs late continuations inline, where they find the model gone and return.

### SmallUniqueFunction

`SmallUniqueFunction<R( Args... ), N>` is a move-only `std::function` alternative that stores
callables up to `N` bytes inline and falls back to the heap for larger ones. Executor work items
use 64 bytes, enough for a continuation capturing a callable, a task source and a task state
without allocating.

## Cancellation

Cancellation is cooperative:

```cpp
CancellationSource cancel;
auto listing = vfs.listAll( uri, {}, cancel.token() );
// later, e.g. the user navigated away:
cancel.cancel();   // listing completes with VFSErrorCode::Cancelled
```

- `CancellationSource` (move-only) requests cancellation; `token()` hands out cheap copyable
  `CancellationToken`s. Destroying the source does not cancel.
- Long operations check `token.isCancellationRequested()` at safe points.
- `token.onCancel( callback )` registers a callback that runs synchronously on the thread calling
  `cancel()` (immediately if already cancelled). Keep the returned `CancellationRegistration`
  alive while the callback may run; destroying it unregisters the callback and waits for a
  callback already running on another thread. A callback may destroy its own registration.
- A default-constructed token can never be cancelled.

Cancelled operations complete normally with an error value (for VFS, `VFSErrorCode::Cancelled`),
never by abandoning their task.

## Rules of thumb

- Return `Task<Result<T, E>>` from fallible async APIs; never block inside them.
- Consume each task once; keep the returned task if you need to chain further.
- Touch UI state only from continuations dispatched to the UI executor.
- Capture `std::weak_ptr`s (or a generation counter) in continuations of objects that may die or
  move on before the operation completes, and ignore stale results.
- Make sure executors outlive the chains dispatched through them (see above).
- Treat `Abandoned` and `TaskDiagnostics` reports as bugs to fix, not states to handle.

## See also

- [Virtual file system](vfs.md): the VFS API built on these primitives.
- `include/eepp/core/task.hpp` and the other headers above for the full API.
- `src/tests/unit_tests/async_core_tests.cpp` for executable examples.
