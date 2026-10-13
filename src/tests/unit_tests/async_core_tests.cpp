#include "utest.h"
#include <atomic>
#include <eepp/core/cancellationtoken.hpp>
#include <eepp/core/executor.hpp>
#include <eepp/core/result.hpp>
#include <eepp/core/small_unique_function.hpp>
#include <eepp/core/task.hpp>
#include <eepp/system/threadpool.hpp>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

using namespace EE;
using namespace EE::System;

namespace {

struct LifetimeCounter {
	static inline std::atomic<int> alive{ 0 };
	int value{ 0 };
	explicit LifetimeCounter( int v ) : value( v ) { ++alive; }
	LifetimeCounter( const LifetimeCounter& o ) : value( o.value ) { ++alive; }
	LifetimeCounter( LifetimeCounter&& o ) noexcept : value( o.value ) { ++alive; }
	LifetimeCounter& operator=( const LifetimeCounter& ) = default;
	LifetimeCounter& operator=( LifetimeCounter&& ) noexcept = default;
	~LifetimeCounter() { --alive; }
};

struct TestError {
	int code{ 0 };
	std::string message;
};

/** Executor that queues work until drain() is called, to make dispatch observable. */
class QueueExecutor : public Executor {
  public:
	void execute( Function function ) override {
		std::lock_guard<std::mutex> lock( mMutex );
		mQueue.emplace_back( std::move( function ) );
	}

	size_t pending() {
		std::lock_guard<std::mutex> lock( mMutex );
		return mQueue.size();
	}

	size_t drain() {
		size_t count = 0;
		while ( true ) {
			std::vector<Function> queue;
			{
				std::lock_guard<std::mutex> lock( mMutex );
				queue.swap( mQueue );
			}
			if ( queue.empty() )
				return count;
			for ( auto& fn : queue ) {
				fn();
				++count;
			}
		}
	}

	void clear() {
		std::vector<Function> queue;
		{
			std::lock_guard<std::mutex> lock( mMutex );
			queue.swap( mQueue );
		}
	}

  private:
	std::mutex mMutex;
	std::vector<Function> mQueue;
};

/** Captures task diagnostics for the lifetime of the object. */
class DiagnosticCapture {
  public:
	static inline std::atomic<int> abandoned{ 0 };
	static inline std::atomic<int> resolvedTwice{ 0 };
	static inline std::atomic<int> retrievedTwice{ 0 };

	DiagnosticCapture() {
		abandoned = 0;
		resolvedTwice = 0;
		retrievedTwice = 0;
		mPrevious = TaskDiagnostics::setHandler( []( TaskDiagnostic d ) {
			switch ( d ) {
				case TaskDiagnostic::Abandoned:
					++abandoned;
					break;
				case TaskDiagnostic::ResolvedTwice:
					++resolvedTwice;
					break;
				case TaskDiagnostic::TaskRetrievedTwice:
					++retrievedTwice;
					break;
			}
		} );
	}

	~DiagnosticCapture() { TaskDiagnostics::setHandler( mPrevious ); }

  private:
	TaskDiagnostics::Handler mPrevious{ nullptr };
};

} // namespace

// ---------------------------------------------------------------------------------------------
// Result
// ---------------------------------------------------------------------------------------------

UTEST( Result, trivialValuesAndErrors ) {
	Result<int, int> ok = Ok( 5 );
	EXPECT_TRUE( ok.hasValue() );
	EXPECT_TRUE( static_cast<bool>( ok ) );
	EXPECT_EQ( 5, ok.value() );
	EXPECT_EQ( 5, *ok );

	Result<int, int> err = Err( 7 );
	EXPECT_FALSE( err.hasValue() );
	EXPECT_TRUE( err.hasError() );
	EXPECT_EQ( 7, err.error() );
	EXPECT_EQ( 3, err.valueOr( 3 ) );

	// Same value and error type must remain unambiguous through Ok/Err.
	Result<std::string, std::string> s = Err( std::string( "bad" ) );
	EXPECT_TRUE( s.hasError() );
	EXPECT_TRUE( s.error() == "bad" );
}

UTEST( Result, moveOnlyValueAndError ) {
	Result<std::unique_ptr<int>, std::unique_ptr<TestError>> r = Ok( std::make_unique<int>( 42 ) );
	EXPECT_TRUE( r.hasValue() );
	auto moved = std::move( r );
	EXPECT_EQ( 42, *moved.value() );
	std::unique_ptr<int> extracted = std::move( moved ).value();
	EXPECT_EQ( 42, *extracted );

	Result<std::unique_ptr<int>, std::unique_ptr<TestError>> e =
		Err( std::make_unique<TestError>( TestError{ 3, "x" } ) );
	EXPECT_EQ( 3, e.error()->code );
	auto errMoved = std::move( e );
	std::unique_ptr<TestError> err = std::move( errMoved ).error();
	EXPECT_EQ( 3, err->code );

	static_assert( !std::is_copy_constructible_v<decltype( r )> );
	static_assert( std::is_move_constructible_v<decltype( r )> );
	static_assert( std::is_copy_constructible_v<Result<int, std::string>> );
	static_assert( std::is_copy_assignable_v<Result<int, std::string>> );
	static_assert( !std::is_copy_constructible_v<Result<int, std::unique_ptr<int>>> );
	static_assert( !std::is_copy_constructible_v<Result<void, std::unique_ptr<int>>> );
}

UTEST( Result, destructorsRunExactlyOnce ) {
	LifetimeCounter::alive = 0;
	{
		Result<LifetimeCounter, LifetimeCounter> a = Ok( LifetimeCounter( 1 ) );
		EXPECT_EQ( 1, LifetimeCounter::alive.load() );
		Result<LifetimeCounter, LifetimeCounter> b = Err( LifetimeCounter( 2 ) );
		EXPECT_EQ( 2, LifetimeCounter::alive.load() );
		// Assignment across alternatives destroys the previous member.
		a = b;
		EXPECT_EQ( 2, LifetimeCounter::alive.load() );
		EXPECT_TRUE( a.hasError() );
		EXPECT_EQ( 2, a.error().value );
		a = Result<LifetimeCounter, LifetimeCounter>( Ok( LifetimeCounter( 9 ) ) );
		EXPECT_EQ( 2, LifetimeCounter::alive.load() );
		EXPECT_EQ( 9, a.value().value );
		Result<LifetimeCounter, LifetimeCounter> c( std::move( a ) );
		EXPECT_EQ( 3, LifetimeCounter::alive.load() );
	}
	EXPECT_EQ( 0, LifetimeCounter::alive.load() );
}

UTEST( Result, voidResult ) {
	Result<void, TestError> ok = Ok();
	EXPECT_TRUE( ok.hasValue() );
	Result<void, TestError> err = Err( TestError{ 4, "four" } );
	EXPECT_FALSE( err.hasValue() );
	EXPECT_EQ( 4, err.error().code );
	Result<void, TestError> copy = err;
	EXPECT_EQ( 4, copy.error().code );
	copy = ok;
	EXPECT_TRUE( copy.hasValue() );
}

UTEST( Result, accessorValueCategories ) {
	using R = Result<std::string, int>;
	static_assert( std::is_same_v<decltype( std::declval<R&>().value() ), std::string&> );
	static_assert(
		std::is_same_v<decltype( std::declval<const R&>().value() ), const std::string&> );
	static_assert( std::is_same_v<decltype( std::declval<R&&>().value() ), std::string&&> );
	static_assert( std::is_same_v<decltype( std::declval<R&>().error() ), int&> );
	static_assert( std::is_same_v<decltype( std::declval<const R&>().error() ), const int&> );
	static_assert( std::is_same_v<decltype( std::declval<R&&>().error() ), int&&> );
	EXPECT_TRUE( true );
}

UTEST( Result, combinators ) {
	Result<int, std::string> r = Ok( 2 );
	auto mapped = std::move( r ).map( []( int v ) { return std::to_string( v * 2 ); } );
	EXPECT_TRUE( mapped.value() == "4" );

	Result<int, std::string> e = Err( std::string( "e" ) );
	auto mappedErr = std::move( e ).mapError( []( std::string&& s ) { return (int)s.size(); } );
	EXPECT_EQ( 1, mappedErr.error() );

	Result<int, std::string> chain = Ok( 3 );
	auto chained = std::move( chain ).andThen( []( int v ) -> Result<int, std::string> {
		if ( v > 2 )
			return Err( std::string( "too big" ) );
		return Ok( v );
	} );
	EXPECT_TRUE( chained.error() == "too big" );

	auto recovered = std::move( chained ).orElse(
		[]( std::string&& ) -> Result<int, std::string> { return Ok( 0 ); } );
	EXPECT_EQ( 0, recovered.value() );
}

// ---------------------------------------------------------------------------------------------
// SmallUniqueFunction
// ---------------------------------------------------------------------------------------------

UTEST( SmallUniqueFunction, moveOnlyCapturesAndHeapFallback ) {
	auto p = std::make_unique<int>( 41 );
	SmallUniqueFunction<int( int ), 32> f = [p = std::move( p )]( int a ) { return *p + a; };
	EXPECT_TRUE( f.isInline() );
	SmallUniqueFunction<int( int ), 32> g = std::move( f );
	EXPECT_FALSE( static_cast<bool>( f ) );
	EXPECT_EQ( 42, g( 1 ) );

	struct Big {
		char data[128]{};
	};
	LifetimeCounter::alive = 0;
	{
		SmallUniqueFunction<void(), 32> big = [b = Big{}, c = LifetimeCounter( 1 )]() { (void)b; };
		EXPECT_FALSE( big.isInline() );
		auto moved = std::move( big );
		moved();
		EXPECT_EQ( 1, LifetimeCounter::alive.load() );
	}
	EXPECT_EQ( 0, LifetimeCounter::alive.load() );
}

// ---------------------------------------------------------------------------------------------
// Cancellation
// ---------------------------------------------------------------------------------------------

UTEST( Cancellation, defaultTokenNeverCancels ) {
	CancellationToken token;
	EXPECT_FALSE( token.canBeCancelled() );
	EXPECT_FALSE( token.isCancellationRequested() );
	bool called = false;
	auto reg = token.onCancel( [&] { called = true; } );
	EXPECT_FALSE( static_cast<bool>( reg ) );
	EXPECT_FALSE( called );
}

UTEST( Cancellation, cancelBeforeAndAfterRegistration ) {
	CancellationSource source;
	auto token = source.token();
	int calls = 0;
	auto reg = token.onCancel( [&] { ++calls; } );
	EXPECT_TRUE( static_cast<bool>( reg ) );
	EXPECT_FALSE( token.isCancellationRequested() );
	EXPECT_TRUE( source.cancel() );
	EXPECT_EQ( 1, calls );
	EXPECT_TRUE( token.isCancellationRequested() );
	// Cancel exactly once.
	EXPECT_FALSE( source.cancel() );
	EXPECT_EQ( 1, calls );

	// Registration after cancellation runs immediately.
	int late = 0;
	auto lateReg = token.onCancel( [&] { ++late; } );
	EXPECT_EQ( 1, late );
	EXPECT_FALSE( static_cast<bool>( lateReg ) );
}

UTEST( Cancellation, destroyedRegistrationDoesNotFire ) {
	CancellationSource source;
	int calls = 0;
	{
		auto reg = source.token().onCancel( [&] { ++calls; } );
	}
	auto kept = source.token().onCancel( [&] { calls += 10; } );
	source.cancel();
	EXPECT_EQ( 10, calls );
}

UTEST( Cancellation, callbackMayDestroyItsOwnRegistration ) {
	CancellationSource source;
	CancellationRegistration reg;
	int calls = 0;
	reg = source.token().onCancel( [&] {
		++calls;
		reg.reset();
	} );
	source.cancel();
	EXPECT_EQ( 1, calls );
	EXPECT_FALSE( static_cast<bool>( reg ) );
}

UTEST( Cancellation, sourceAndTokenLifetimeIndependence ) {
	CancellationToken token;
	{
		CancellationSource source;
		token = source.token();
	}
	EXPECT_TRUE( token.canBeCancelled() );
	EXPECT_FALSE( token.isCancellationRequested() );
	auto reg = token.onCancel( [] {} );
	EXPECT_TRUE( static_cast<bool>( reg ) );

	CancellationRegistration outliving;
	{
		CancellationSource source;
		outliving = source.token().onCancel( [] {} );
		source.cancel();
	}
	outliving.reset();
	EXPECT_TRUE( true );
}

UTEST( Cancellation, concurrentRegistrationsAllFireOnce ) {
	for ( int iteration = 0; iteration < 50; ++iteration ) {
		CancellationSource source;
		auto token = source.token();
		std::atomic<int> calls{ 0 };
		constexpr int Threads = 8;
		constexpr int PerThread = 64;
		std::vector<std::thread> threads;
		std::vector<std::vector<CancellationRegistration>> regs( Threads );
		std::atomic<bool> go{ false };
		for ( int t = 0; t < Threads; ++t ) {
			threads.emplace_back( [&, t] {
				while ( !go.load() )
					std::this_thread::yield();
				for ( int i = 0; i < PerThread; ++i )
					regs[t].emplace_back( token.onCancel( [&] { ++calls; } ) );
			} );
		}
		go = true;
		std::thread canceller( [&] { source.cancel(); } );
		for ( auto& t : threads )
			t.join();
		canceller.join();
		// Every registration either fired from cancel() or immediately at registration time.
		EXPECT_EQ( Threads * PerThread, calls.load() );
	}
}

UTEST( Cancellation, unregisterRaceWaitsForRunningCallback ) {
	for ( int iteration = 0; iteration < 200; ++iteration ) {
		CancellationSource source;
		auto alive = std::make_shared<std::atomic<bool>>( true );
		std::atomic<bool> usedAfterRelease{ false };
		std::atomic<bool> callbackStarted{ false };
		auto reg = source.token().onCancel( [&, alive] {
			callbackStarted = true;
			std::this_thread::sleep_for( std::chrono::microseconds( 50 ) );
			if ( !alive->load() )
				usedAfterRelease = true;
		} );
		std::thread canceller( [&] { source.cancel(); } );
		if ( iteration % 2 == 0 ) {
			while ( !callbackStarted.load() )
				std::this_thread::yield();
		}
		reg.reset();
		// After reset() returns the callback is either not run, or fully finished.
		alive->store( false );
		canceller.join();
		EXPECT_FALSE( usedAfterRelease.load() );
	}
}

// ---------------------------------------------------------------------------------------------
// Task
// ---------------------------------------------------------------------------------------------

UTEST( Task, readyBeforeThen ) {
	TaskSource<int> source;
	auto task = source.task();
	EXPECT_EQ( TaskState::Pending, task.state() );
	source.resolve( 21 );
	EXPECT_EQ( TaskState::Ready, task.state() );
	int observed = 0;
	auto next = std::move( task ).then( [&]( int v ) {
		observed = v;
		return v * 2;
	} );
	// then() on a ready task runs immediately.
	EXPECT_EQ( 21, observed );
	EXPECT_TRUE( next.isReady() );
	EXPECT_EQ( 42, std::move( next ).takeValue() );
}

UTEST( Task, thenBeforeReady ) {
	TaskSource<std::string> source;
	std::string observed;
	auto next = source.task().then( [&]( std::string&& v ) { observed = std::move( v ); } );
	EXPECT_TRUE( observed.empty() );
	EXPECT_TRUE( next.isPending() );
	source.resolve( "hello" );
	EXPECT_TRUE( observed == "hello" );
	EXPECT_TRUE( next.isReady() );
	static_assert( std::is_same_v<decltype( next ), Task<void>> );
}

UTEST( Task, moveOnlyValues ) {
	TaskSource<std::unique_ptr<int>> source;
	auto task = source.task().then( []( std::unique_ptr<int>&& p ) {
		*p += 1;
		return std::move( p );
	} );
	source.resolve( std::make_unique<int>( 1 ) );
	auto value = std::move( task ).takeValue();
	EXPECT_EQ( 2, *value );

	static_assert( !std::is_copy_constructible_v<Task<int>> );
	static_assert( !std::is_copy_constructible_v<TaskSource<int>> );
}

UTEST( Task, voidTasks ) {
	TaskSource<void> source;
	int steps = 0;
	auto t = source.task().then( [&] { ++steps; } ).then( [&] {
		++steps;
		return 5;
	} );
	source.resolve();
	EXPECT_EQ( 2, steps );
	EXPECT_EQ( 5, std::move( t ).takeValue() );
	auto ready = makeReadyTask();
	EXPECT_TRUE( ready.isReady() );
}

UTEST( Task, resultValues ) {
	using R = Result<int, TestError>;
	auto task = makeReadyTask<R>( Err( TestError{ 1, "nope" } ) );
	auto mapped = std::move( task ).then( []( R&& r ) -> R {
		if ( !r )
			return Err( std::move( r ).error() );
		return Ok( r.value() + 1 );
	} );
	auto value = std::move( mapped ).takeValue();
	EXPECT_TRUE( value.hasError() );
	EXPECT_EQ( 1, value.error().code );
}

UTEST( Task, flattensNestedTasks ) {
	TaskSource<int> outer;
	TaskSource<std::string> inner;
	auto flattened = outer.task().then( [&inner]( int v ) {
		return inner.task().then( [v]( std::string&& s ) { return s + std::to_string( v ); } );
	} );
	static_assert( std::is_same_v<decltype( flattened ), Task<std::string>> );
	outer.resolve( 7 );
	EXPECT_TRUE( flattened.isPending() );
	inner.resolve( "n" );
	EXPECT_TRUE( flattened.isReady() );
	EXPECT_TRUE( std::move( flattened ).takeValue() == "n7" );

	// Already ready inner tasks flatten synchronously.
	auto direct = makeReadyTask( 1 ).then( []( int v ) { return makeReadyTask( v + 1 ); } );
	EXPECT_EQ( 2, std::move( direct ).takeValue() );

	// Task<void> flattening.
	auto voidFlat = makeReadyTask( 1 ).then( []( int ) { return makeReadyTask(); } );
	static_assert( std::is_same_v<decltype( voidFlat ), Task<void>> );
	EXPECT_TRUE( voidFlat.isReady() );
}

UTEST( Task, thenOnAlwaysDispatchesThroughExecutor ) {
	QueueExecutor executor;
	int observed = 0;
	auto ready = makeReadyTask( 3 ).thenOn( executor, [&]( int v ) {
		observed = v;
		return v + 1;
	} );
	// Never inline, even though the value was already available.
	EXPECT_EQ( 0, observed );
	EXPECT_EQ( 1u, executor.pending() );
	EXPECT_TRUE( ready.isPending() );
	executor.drain();
	EXPECT_EQ( 3, observed );
	EXPECT_EQ( 4, std::move( ready ).takeValue() );

	TaskSource<int> source;
	auto pending = source.task().thenOn( executor, [&]( int v ) { observed = v; } );
	EXPECT_EQ( 0u, executor.pending() );
	source.resolve( 9 );
	EXPECT_EQ( 3, observed );
	EXPECT_EQ( 1u, executor.pending() );
	executor.drain();
	EXPECT_EQ( 9, observed );
	EXPECT_TRUE( pending.isReady() );
}

UTEST( Task, onCompleteObservesTerminalState ) {
	DiagnosticCapture capture;
	TaskState seen = TaskState::Pending;
	int value = 0;
	makeReadyTask( 4 ).onComplete( [&]( Task<int>&& t ) {
		seen = t.state();
		value = std::move( t ).takeValue();
	} );
	EXPECT_EQ( TaskState::Ready, seen );
	EXPECT_EQ( 4, value );

	{
		TaskSource<int> source;
		source.task().onComplete( [&]( Task<int>&& t ) { seen = t.state(); } );
	}
	EXPECT_EQ( TaskState::Abandoned, seen );
}

UTEST( Task, abandonmentPropagatesThroughChains ) {
	DiagnosticCapture capture;
	QueueExecutor executor;
	bool called = false;
	Task<int> tail;
	Task<int> flattenedTail;
	{
		TaskSource<int> source;
		tail = source.task()
				   .then( [&]( int v ) {
					   called = true;
					   return v;
				   } )
				   .thenOn( executor, [&]( int v ) {
					   called = true;
					   return v;
				   } );
		TaskSource<int> inner;
		flattenedTail = makeReadyTask( 1 ).then( [&]( int ) { return inner.task(); } );
		EXPECT_TRUE( flattenedTail.isPending() );
	}
	EXPECT_FALSE( called );
	EXPECT_EQ( TaskState::Abandoned, tail.state() );
	EXPECT_EQ( TaskState::Abandoned, flattenedTail.state() );
	EXPECT_EQ( 0u, executor.pending() );
	// Only the roots report; chained tasks abandon silently.
	EXPECT_EQ( 2, DiagnosticCapture::abandoned.load() );
	EXPECT_EQ( TaskState::Abandoned, tail.wait() );
	EXPECT_FALSE( std::move( tail ).get().has_value() );
}

UTEST( Task, droppedExecutorWorkAbandonsChain ) {
	DiagnosticCapture capture;
	QueueExecutor executor;
	auto t = makeReadyTask( 1 ).thenOn( executor, []( int v ) { return v; } );
	executor.clear();
	EXPECT_EQ( TaskState::Abandoned, t.state() );
	EXPECT_EQ( 1, DiagnosticCapture::abandoned.load() );
}

UTEST( Task, doubleResolveAndDoubleTaskAreRejected ) {
	DiagnosticCapture capture;
	TaskSource<int> source;
	auto task = source.task();
	auto second = source.task();
	EXPECT_FALSE( second.valid() );
	EXPECT_EQ( TaskState::Abandoned, second.state() );
	EXPECT_EQ( 1, DiagnosticCapture::retrievedTwice.load() );
	EXPECT_TRUE( source.resolve( 1 ) );
	EXPECT_FALSE( source.resolve( 2 ) );
	EXPECT_EQ( 1, DiagnosticCapture::resolvedTwice.load() );
	EXPECT_EQ( 1, std::move( task ).takeValue() );

	// then() on an invalid task yields an abandoned task without calling the continuation.
	bool called = false;
	auto fromInvalid = std::move( second ).then( [&]( int ) { called = true; } );
	EXPECT_FALSE( called );
	EXPECT_EQ( TaskState::Abandoned, fromInvalid.state() );
}

UTEST( Task, resolveVersusThenRaceRunsContinuationExactlyOnce ) {
	constexpr int Iterations = 20000;
	std::atomic<int> totalCalls{ 0 };
	for ( int i = 0; i < Iterations; ++i ) {
		TaskSource<int> source;
		auto task = source.task();
		std::atomic<int> calls{ 0 };
		std::atomic<int> ready{ 0 };
		Task<void> chained;
		std::thread producer( [&] {
			++ready;
			while ( ready.load() < 2 ) {
			}
			source.resolve( i );
		} );
		++ready;
		while ( ready.load() < 2 ) {
		}
		chained = std::move( task ).then( [&calls, i]( int v ) {
			if ( v == i )
				++calls;
		} );
		producer.join();
		EXPECT_EQ( 1, calls.load() );
		EXPECT_TRUE( chained.isReady() );
		totalCalls += calls.load();
	}
	EXPECT_EQ( Iterations, totalCalls.load() );
}

UTEST( Task, waitBlocksUntilResolvedFromAnotherThread ) {
	TaskSource<int> source;
	auto task = source.task();
	EXPECT_EQ( TaskState::Pending, task.waitFor( std::chrono::milliseconds( 1 ) ) );
	std::thread producer( [&] {
		std::this_thread::sleep_for( std::chrono::milliseconds( 5 ) );
		source.resolve( 11 );
	} );
	EXPECT_EQ( TaskState::Ready, task.wait() );
	producer.join();
	auto value = std::move( task ).get();
	EXPECT_TRUE( value.has_value() );
	EXPECT_EQ( 11, *value );
}

UTEST( Task, layout ) {
	// Keep the shared state compact: one allocation holding mutex, continuation and value.
	EXPECT_LE( sizeof( detail::TaskSharedState<int> ),
			   sizeof( std::mutex ) + sizeof( detail::TaskSharedState<int>::Continuation ) +
				   sizeof( void* ) + sizeof( std::optional<int> ) + 8 );
	EXPECT_EQ( sizeof( void* ), sizeof( Task<int> ) );
	EXPECT_EQ( sizeof( void* ), sizeof( TaskSource<int> ) );
}

// ---------------------------------------------------------------------------------------------
// ThreadPool integration
// ---------------------------------------------------------------------------------------------

UTEST( ThreadPoolTask, submitResolvesOnWorker ) {
	auto pool = ThreadPool::createUnique( 4 );
	auto mainThread = std::this_thread::get_id();
	auto task = pool->submit( [mainThread] { return std::this_thread::get_id() != mainThread; } );
	auto value = std::move( task ).get();
	EXPECT_TRUE( value.has_value() && *value );

	auto voidTask = pool->submit( [] {} );
	EXPECT_TRUE( std::move( voidTask ).get() );

	auto moveOnly = pool->submit( [] { return std::make_unique<int>( 3 ); } );
	auto ptr = std::move( moveOnly ).get();
	EXPECT_EQ( 3, **ptr );
}

UTEST( ThreadPoolTask, poolIsAnExecutor ) {
	auto pool = ThreadPool::createUnique( 2 );
	Executor& executor = *pool;
	auto t = makeReadyTask( 2 ).thenOn( executor, []( int v ) { return v * 10; } );
	auto v = std::move( t ).get();
	EXPECT_EQ( 20, *v );

	// Existing run() API with move-only captures.
	std::atomic<int> result{ 0 };
	TaskSource<void> done;
	auto doneTask = done.task();
	pool->run( [p = std::make_unique<int>( 5 ), &result, done = std::move( done )]() mutable {
		result = *p;
		done.resolve();
	} );
	EXPECT_EQ( TaskState::Ready, doneTask.wait() );
	EXPECT_EQ( 5, result.load() );
}

UTEST( ThreadPoolTask, manyConcurrentChains ) {
	auto pool = ThreadPool::createUnique( 4 );
	constexpr int Count = 2000;
	std::vector<Task<int>> tasks;
	tasks.reserve( Count );
	for ( int i = 0; i < Count; ++i ) {
		tasks.emplace_back(
			pool->submit( [i] { return i; } )
				.thenOn( *pool, []( int v ) { return v * 2; } )
				.then( [&pool]( int v ) { return pool->submit( [v] { return v + 1; } ); } ) );
	}
	long long sum = 0;
	for ( auto& t : tasks ) {
		auto v = std::move( t ).get();
		EXPECT_TRUE( v.has_value() );
		sum += *v;
	}
	long long expected = 0;
	for ( int i = 0; i < Count; ++i )
		expected += i * 2 + 1;
	EXPECT_EQ( expected, sum );
}
