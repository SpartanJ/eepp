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

#if defined( __cpp_exceptions ) || defined( _CPPUNWIND )
/** Counts only fully constructed objects, exposing destruction of an inactive union member. */
template <bool NothrowMove> struct ThrowingPayload {
	static inline int alive{ 0 };
	static inline bool failCopy{ false };
	static inline bool failMove{ false };
	int value;

	explicit ThrowingPayload( int value ) : value( value ) { ++alive; }

	ThrowingPayload( const ThrowingPayload& other ) : value( other.value ) {
		if ( failCopy )
			throw 1;
		++alive;
	}

	ThrowingPayload( ThrowingPayload&& other ) noexcept( NothrowMove ) : value( other.value ) {
		if constexpr ( !NothrowMove ) {
			if ( failMove )
				throw 2;
		}
		++alive;
	}

	ThrowingPayload& operator=( const ThrowingPayload& ) = default;

	ThrowingPayload& operator=( ThrowingPayload&& ) = default;

	~ThrowingPayload() { --alive; }
};
#endif

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

	DiagnosticCapture() {
		abandoned = 0;
		resolvedTwice = 0;
		mPrevious = TaskDiagnostics::setHandler( []( TaskDiagnostic d ) {
			switch ( d ) {
				case TaskDiagnostic::Abandoned:
					++abandoned;
					break;
				case TaskDiagnostic::ResolvedTwice:
					++resolvedTwice;
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

#if defined( __cpp_exceptions ) || defined( _CPPUNWIND )
UTEST( Result, failedAlternativeConstructionPreservesThePreviousValue ) {
	using Safe = ThrowingPayload<true>;
	using Risky = ThrowingPayload<false>;
	using R = Result<Safe, Risky>;
	static_assert( std::is_copy_constructible_v<Result<Risky, Risky>> );
	static_assert( !std::is_copy_assignable_v<Result<Risky, Risky>> );
	static_assert( !std::is_move_assignable_v<Result<Risky, Risky>> );
	EXPECT_EQ( 0, Safe::alive );
	EXPECT_EQ( 0, Risky::alive );
	{
		R value = Ok( Safe( 7 ) );
		R error = Err( Risky( 9 ) );
		Risky::failCopy = true;
		bool threw = false;
		try {
			value = error;
		} catch ( int ) {
			threw = true;
		}
		Risky::failCopy = false;
		EXPECT_TRUE( threw );
		ASSERT_TRUE( value );
		EXPECT_EQ( 7, value->value );
		EXPECT_EQ( 1, Safe::alive );
		EXPECT_EQ( 1, Risky::alive );
		Risky::failMove = true;
		threw = false;
		try {
			value = std::move( error );
		} catch ( int ) {
			threw = true;
		}
		Risky::failMove = false;
		EXPECT_TRUE( threw );
		ASSERT_TRUE( value );
		EXPECT_EQ( 7, value->value );
		EXPECT_EQ( 1, Safe::alive );
		EXPECT_EQ( 1, Risky::alive );
		Safe::failCopy = true;
		threw = false;
		try {
			error = value;
		} catch ( int ) {
			threw = true;
		}
		Safe::failCopy = false;
		EXPECT_TRUE( threw );
		ASSERT_TRUE( error.hasError() );
		EXPECT_EQ( 9, error.error().value );
		EXPECT_EQ( 1, Safe::alive );
		EXPECT_EQ( 1, Risky::alive );
		// Both transitions remain usable after a failed assignment.
		value = error;
		EXPECT_TRUE( value.hasError() );
		EXPECT_EQ( 9, value.error().value );
		error = R( Ok( Safe( 11 ) ) );
		EXPECT_TRUE( error.hasValue() );
		EXPECT_EQ( 11, error->value );
	}
	EXPECT_EQ( 0, Safe::alive );
	EXPECT_EQ( 0, Risky::alive );
}

UTEST( Result, voidAssignmentCommitsTheErrorOnlyAfterConstruction ) {
	using Payload = ThrowingPayload<false>;
	EXPECT_EQ( 0, Payload::alive );
	{
		Result<void, Payload> value = Ok();
		Result<void, Payload> error = Err( Payload( 3 ) );
		Payload::failCopy = true;
		bool threw = false;
		try {
			value = error;
		} catch ( int ) {
			threw = true;
		}
		Payload::failCopy = false;
		EXPECT_TRUE( threw );
		EXPECT_TRUE( value.hasValue() );
		EXPECT_EQ( 1, Payload::alive );
		Payload::failMove = true;
		threw = false;
		try {
			value = std::move( error );
		} catch ( int ) {
			threw = true;
		}
		Payload::failMove = false;
		EXPECT_TRUE( threw );
		EXPECT_TRUE( value.hasValue() );
		EXPECT_EQ( 1, Payload::alive );
		value = error;
		EXPECT_EQ( 2, Payload::alive );
		EXPECT_EQ( 3, value.error().value );
		value = Result<void, Payload>( Ok() );
		EXPECT_TRUE( value.hasValue() );
		EXPECT_EQ( 1, Payload::alive );
	}
	EXPECT_EQ( 0, Payload::alive );
}
#endif

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

UTEST( Cancellation, callbackMayDestroyTheSource ) {
	// The state must outlive the callbacks even when one of them destroys the source that is
	// cancelling (use-after-free under AddressSanitizer otherwise).
	auto source = std::make_unique<CancellationSource>();
	CancellationRegistration reg;
	int calls = 0;
	reg = source->token().onCancel( [&] {
		++calls;
		reg.reset();
		source.reset();
	} );
	source->cancel();
	EXPECT_EQ( 1, calls );
	EXPECT_TRUE( source == nullptr );
}

UTEST( Cancellation, sourceAndTokenLifetimeIndependence ) {
	CancellationToken token;
	{
		CancellationSource source;
		token = source.token();
	}
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
	TaskResolver<int> resolve;
	auto task = Task<int>::create( resolve );
	EXPECT_EQ( TaskState::Pending, task.state() );
	resolve( 21 );
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
	TaskResolver<std::string> resolve;
	std::string observed;
	auto next = Task<std::string>::create( resolve ).then(
		[&]( std::string&& v ) { observed = std::move( v ); } );
	EXPECT_TRUE( observed.empty() );
	EXPECT_TRUE( next.isPending() );
	resolve( "hello" );
	EXPECT_TRUE( observed == "hello" );
	EXPECT_TRUE( next.isReady() );
	static_assert( std::is_same_v<decltype( next ), Task<void>> );
}

UTEST( Task, moveOnlyValues ) {
	TaskResolver<std::unique_ptr<int>> resolve;
	auto task = Task<std::unique_ptr<int>>::create( resolve ).then( []( std::unique_ptr<int>&& p ) {
		*p += 1;
		return std::move( p );
	} );
	resolve( std::make_unique<int>( 1 ) );
	auto value = std::move( task ).takeValue();
	EXPECT_EQ( 2, *value );

	static_assert( !std::is_copy_constructible_v<Task<int>> );
	static_assert( std::is_copy_constructible_v<TaskResolver<int>> );
}

UTEST( Task, voidTasks ) {
	TaskResolver<void> resolve;
	int steps = 0;
	auto t = Task<void>::create( resolve ).then( [&] { ++steps; } ).then( [&] {
		++steps;
		return 5;
	} );
	resolve();
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
	TaskResolver<int> resolveOuter;
	TaskResolver<std::string> resolveInner;
	Task<std::string> inner = Task<std::string>::create( resolveInner );
	auto flattened = Task<int>::create( resolveOuter ).then( [&inner]( int v ) {
		return std::move( inner ).then(
			[v]( std::string&& s ) { return s + std::to_string( v ); } );
	} );
	static_assert( std::is_same_v<decltype( flattened ), Task<std::string>> );
	resolveOuter( 7 );
	EXPECT_TRUE( flattened.isPending() );
	resolveInner( "n" );
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

UTEST( Task, thenAlwaysDispatchesThroughExecutor ) {
	QueueExecutor executor;
	int observed = 0;
	auto ready = makeReadyTask( 3 ).then( executor, [&]( int v ) {
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

	TaskResolver<int> resolve;
	auto pending = Task<int>::create( resolve ).then( executor, [&]( int v ) { observed = v; } );
	EXPECT_EQ( 0u, executor.pending() );
	resolve( 9 );
	EXPECT_EQ( 3, observed );
	EXPECT_EQ( 1u, executor.pending() );
	executor.drain();
	EXPECT_EQ( 9, observed );
	EXPECT_TRUE( pending.isReady() );
}

UTEST( Task, onCompleteObservesBothOutcomes ) {
	DiagnosticCapture capture;
	std::optional<int> seen;
	bool called = false;
	makeReadyTask( 4 ).onComplete( [&]( std::optional<int>&& value ) {
		called = true;
		seen = value;
	} );
	EXPECT_TRUE( called );
	EXPECT_TRUE( seen.has_value() && *seen == 4 );

	called = false;
	{
		TaskResolver<int> resolve;
		Task<int>::create( resolve ).onComplete( [&]( std::optional<int>&& value ) {
			called = true;
			seen = value;
		} );
	}
	EXPECT_TRUE( called );
	EXPECT_FALSE( seen.has_value() );

	bool completed = true;
	{
		TaskResolver<void> resolve;
		Task<void>::create( resolve ).onComplete( [&]( bool ready ) { completed = ready; } );
	}
	EXPECT_FALSE( completed );
}

UTEST( Task, resolverCopiesShareOneTask ) {
	DiagnosticCapture capture;
	TaskResolver<int> resolve;
	auto task = Task<int>::create( resolve );
	{
		TaskResolver<int> copy = resolve;
		TaskResolver<int> assigned;
		assigned = copy;
	}
	// Other copies are gone, but one is left: the task is still pending.
	EXPECT_TRUE( task.isPending() );
	EXPECT_EQ( 0, DiagnosticCapture::abandoned.load() );
	TaskResolver<int> late = resolve;
	EXPECT_TRUE( late( 3 ) );
	EXPECT_FALSE( resolve( 4 ) );
	EXPECT_EQ( 1, DiagnosticCapture::resolvedTwice.load() );
	EXPECT_EQ( 3, std::move( task ).takeValue() );

	// The last copy abandons the task.
	Task<int> abandoned;
	{
		TaskResolver<int> first;
		abandoned = Task<int>::create( first );
		TaskResolver<int> second = first;
		first = TaskResolver<int>();
		EXPECT_TRUE( abandoned.isPending() );
	}
	EXPECT_EQ( TaskState::Abandoned, abandoned.state() );
	EXPECT_EQ( 1, DiagnosticCapture::abandoned.load() );

	// A resolver that create() never filled belongs to no task.
	TaskResolver<int> empty;
	EXPECT_FALSE( empty( 1 ) );
	EXPECT_EQ( 2, DiagnosticCapture::resolvedTwice.load() );
}

UTEST( Task, abandonmentPropagatesThroughChains ) {
	DiagnosticCapture capture;
	QueueExecutor executor;
	bool called = false;
	Task<int> tail;
	Task<int> flattenedTail;
	{
		TaskResolver<int> resolve;
		tail = Task<int>::create( resolve )
				   .then( [&]( int v ) {
					   called = true;
					   return v;
				   } )
				   .then( executor, [&]( int v ) {
					   called = true;
					   return v;
				   } );
		TaskResolver<int> resolveInner;
		Task<int> inner = Task<int>::create( resolveInner );
		flattenedTail = makeReadyTask( 1 ).then( [&]( int ) { return std::move( inner ); } );
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
	auto t = makeReadyTask( 1 ).then( executor, []( int v ) { return v; } );
	executor.clear();
	EXPECT_EQ( TaskState::Abandoned, t.state() );
	EXPECT_EQ( 1, DiagnosticCapture::abandoned.load() );
}

UTEST( Task, doubleResolveIsRejected ) {
	DiagnosticCapture capture;
	TaskResolver<int> resolve;
	auto task = Task<int>::create( resolve );
	EXPECT_TRUE( resolve( 1 ) );
	EXPECT_FALSE( resolve( 2 ) );
	EXPECT_EQ( 1, DiagnosticCapture::resolvedTwice.load() );
	EXPECT_EQ( 1, std::move( task ).takeValue() );

	// A continuation on an invalid task yields an abandoned task without being called.
	Task<int> invalid;
	EXPECT_FALSE( invalid.valid() );
	EXPECT_EQ( TaskState::Abandoned, invalid.state() );
	bool called = false;
	auto fromInvalid = std::move( invalid ).then( [&]( int ) { called = true; } );
	EXPECT_FALSE( called );
	EXPECT_EQ( TaskState::Abandoned, fromInvalid.state() );
}

UTEST( Task, resolveVersusThenRaceRunsContinuationExactlyOnce ) {
	constexpr int Iterations = 20000;
	std::atomic<int> totalCalls{ 0 };
	for ( int i = 0; i < Iterations; ++i ) {
		TaskResolver<int> resolve;
		auto task = Task<int>::create( resolve );
		std::atomic<int> calls{ 0 };
		std::atomic<int> ready{ 0 };
		Task<void> chained;
		std::thread producer( [&] {
			++ready;
			while ( ready.load() < 2 ) {
			}
			resolve( i );
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
	TaskResolver<int> resolve;
	auto task = Task<int>::create( resolve );
	EXPECT_EQ( TaskState::Pending, task.waitFor( std::chrono::milliseconds( 1 ) ) );
	std::thread producer( [&] {
		std::this_thread::sleep_for( std::chrono::milliseconds( 5 ) );
		resolve( 11 );
	} );
	EXPECT_EQ( TaskState::Ready, task.wait() );
	producer.join();
	auto value = std::move( task ).get();
	EXPECT_TRUE( value.has_value() );
	EXPECT_EQ( 11, *value );
}

UTEST( Task, concurrentWaitersObserveResolutionAndAbandonment ) {
	for ( bool abandon : { false, true } ) {
		for ( int iteration = 0; iteration < 20; ++iteration ) {
			TaskResolver<int> resolve;
			const auto task = Task<int>::create( resolve );
			std::atomic<int> started{ 0 };
			TaskState first{}, second{};
			std::thread a( [&] {
				++started;
				first = task.wait();
			} );
			std::thread b( [&] {
				++started;
				second = task.wait();
			} );
			while ( started.load() != 2 )
				std::this_thread::yield();
			std::this_thread::sleep_for( std::chrono::milliseconds( 2 ) );
			if ( abandon )
				resolve = {};
			else
				resolve( 7 );
			a.join();
			b.join();
			EXPECT_EQ( abandon ? TaskState::Abandoned : TaskState::Ready, first );
			EXPECT_EQ( first, second );
		}
	}
}

UTEST( Task, oneWaiterTimingOutDoesNotUnregisterAnother ) {
	TaskResolver<int> resolve;
	const auto task = Task<int>::create( resolve );
	TaskState longer{};
	std::thread thread( [&] { longer = task.waitFor( std::chrono::seconds( 1 ) ); } );
	EXPECT_EQ( TaskState::Pending, task.waitFor( std::chrono::milliseconds( 20 ) ) );
	auto start = std::chrono::steady_clock::now();
	resolve( 1 );
	thread.join();
	EXPECT_EQ( TaskState::Ready, longer );
	EXPECT_LT( std::chrono::duration_cast<std::chrono::milliseconds>(
				   std::chrono::steady_clock::now() - start )
				   .count(),
			   500 );
}

UTEST( Task, layout ) {
	// Keep the shared state compact: one allocation holding mutex, continuation, shared condition
	// variable, value, the two counters and the state.
	EXPECT_LE( sizeof( detail::TaskSharedState<int> ),
			   sizeof( std::mutex ) + sizeof( detail::TaskSharedState<int>::Continuation ) +
				   sizeof( std::condition_variable ) + sizeof( std::optional<int> ) + 16 );
	EXPECT_EQ( sizeof( void* ), sizeof( Task<int> ) );
	EXPECT_EQ( sizeof( void* ), sizeof( TaskResolver<int> ) );
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
	auto t = makeReadyTask( 2 ).then( executor, []( int v ) { return v * 10; } );
	auto v = std::move( t ).get();
	EXPECT_EQ( 20, *v );

	// Existing run() API with move-only captures.
	std::atomic<int> result{ 0 };
	TaskResolver<void> done;
	auto doneTask = Task<void>::create( done );
	pool->run( [p = std::make_unique<int>( 5 ), &result, done]() mutable {
		result = *p;
		done();
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
				.then( *pool, []( int v ) { return v * 2; } )
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
