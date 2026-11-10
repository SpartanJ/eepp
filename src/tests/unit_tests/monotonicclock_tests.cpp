#include "utest.hpp"
#include <eepp/system/clock.hpp>
#include <eepp/system/monotonicclock.hpp>
#include <eepp/system/sys.hpp>
#include <limits>
#include <thread>
#include <type_traits>
#include <utility>

using namespace EE;
using namespace EE::System;

namespace {

using TimePoint = MonotonicClock::TimePoint;

static_assert( sizeof( TimePoint ) == sizeof( Int64 ) );
static_assert( alignof( TimePoint ) == alignof( Int64 ) );
static_assert( sizeof( Clock ) == sizeof( Int64 ) );
static_assert( std::is_trivially_copyable_v<TimePoint> );
static_assert( !std::is_convertible_v<Time, TimePoint> );
static_assert( !std::is_convertible_v<TimePoint, Time> );
static_assert( !std::is_constructible_v<TimePoint, Int64> );
static_assert( std::is_same_v<decltype( TimePoint{} - TimePoint{} ), Time> );
static_assert( TimePoint{} + Microseconds( 7 ) - TimePoint{} == Microseconds( 7 ) );

// The bridge remains constexpr and exact over Time's full signed microsecond range.
static_assert( sizeof( Time ) == sizeof( Int64 ) );
static_assert( std::is_same_v<decltype( Time{}.toChrono() ), std::chrono::microseconds> );
static_assert( noexcept( std::declval<const Time&>().toChrono() ) );
static_assert( Time{}.toChrono().count() == 0 );
static_assert( Milliseconds( 1 ).toChrono().count() == 1000 );
static_assert( Microseconds( -1501 ).toChrono().count() == -1501 );
static_assert( Microseconds( std::numeric_limits<Int64>::min() ).toChrono().count() ==
			   std::numeric_limits<Int64>::min() );
static_assert( Microseconds( std::numeric_limits<Int64>::max() ).toChrono().count() ==
			   std::numeric_limits<Int64>::max() );

} // namespace

UTEST( MonotonicClock, TimestampArithmeticUsesSignedTimeDurations ) {
	const auto now = MonotonicClock::now();
	const auto deadline = now + Seconds( 5 );
	EXPECT_TRUE( deadline > now );
	EXPECT_TRUE( now < deadline );
	EXPECT_TRUE( now <= deadline );
	EXPECT_TRUE( deadline >= now );
	EXPECT_TRUE( now != deadline );
	EXPECT_TRUE( deadline - now == Seconds( 5 ) );
	EXPECT_TRUE( now - deadline == Seconds( -5 ) );
	EXPECT_TRUE( deadline - Seconds( 5 ) == now );
	EXPECT_TRUE( Seconds( 5 ) + now == deadline );
	auto point = now;
	point += Microseconds( 1 );
	EXPECT_TRUE( point - now == Microseconds( 1 ) );
	point -= Microseconds( 2 );
	EXPECT_TRUE( point - now == Microseconds( -1 ) );
	EXPECT_TRUE( point + Microseconds( 1 ) == now );
	EXPECT_TRUE( deadline - Microseconds( 1 ) < deadline );
}

UTEST( MonotonicClock, ReadsShareOneTimelineAcrossThreads ) {
	const auto before = MonotonicClock::now();
	TimePoint onWorker;
	Time workerElapsed;
	Clock stopwatch;
	std::thread worker( [&] {
		onWorker = MonotonicClock::now();
		workerElapsed = stopwatch.getElapsedTime();
	} );
	worker.join();
	const auto after = MonotonicClock::now();
	EXPECT_TRUE( before <= onWorker );
	EXPECT_TRUE( onWorker <= after );
	EXPECT_TRUE( workerElapsed >= Time::Zero );
	EXPECT_TRUE( workerElapsed <= after - before );
	// Clock and TimePoint use the same source, including sub-millisecond arithmetic.
	for ( int i = 0; i < 1000; ++i ) {
		const auto first = MonotonicClock::now();
		EXPECT_TRUE( MonotonicClock::now() >= first );
	}
}

UTEST( MonotonicClock, StopwatchRestartAndResetStayWithinTimestampBounds ) {
	Clock clock;
	const auto beforeRestart = MonotonicClock::now();
	clock.restart();
	const auto beforeRead = MonotonicClock::now();
	const auto elapsed = clock.getElapsedTimeAndReset();
	const auto afterRead = MonotonicClock::now();
	EXPECT_TRUE( elapsed >= Time::Zero );
	EXPECT_TRUE( elapsed <= afterRead - beforeRestart );
	const auto sinceReset = clock.getElapsedTime();
	EXPECT_TRUE( sinceReset >= Time::Zero );
	EXPECT_TRUE( sinceReset <= MonotonicClock::now() - beforeRead );
}

UTEST( MonotonicClock, NonpositiveSleepReturnsWithoutWaiting ) {
	const auto before = MonotonicClock::now();
	Sys::sleep( Time::Zero );
	Sys::sleep( Microseconds( -1 ) );
	EXPECT_TRUE( MonotonicClock::now() - before < Seconds( 1 ) );
}
