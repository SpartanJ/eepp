# Durations, elapsed time and monotonic deadlines

Use `EE::System::Time` for durations, `Clock` to measure elapsed time, and
`MonotonicClock::TimePoint` for comparable instants and deadlines. All are available through
`<eepp/system.hpp>`, or their individual headers.

```cpp
#include <eepp/system/clock.hpp>
#include <eepp/system/monotonicclock.hpp>

using namespace EE::System;

Clock stopwatch;
auto deadline = MonotonicClock::now() + Seconds( 5 );

Time remaining = deadline - MonotonicClock::now();
if ( remaining <= Time::Zero ) {
    // The deadline has expired.
}
Time elapsed = stopwatch.getElapsedTime();
```

`TimePoint` is a microsecond-resolution value. Adding or subtracting a `Time` produces another
instant; subtracting two instants produces a signed duration. Timestamps support comparisons,
copying and sharing across threads, but mutating a shared timestamp still requires synchronization.
There is no implicit conversion between timestamps and durations. A default-constructed timestamp
represents the clock's origin, not the current time; use `std::optional<MonotonicClock::TimePoint>` when a
deadline may be absent.

The underlying steady clock has an unspecified origin and does not track calendar time or wall-clock
adjustments. Do not serialize timestamps, exchange them between processes, or convert them to dates.
Whether elapsed time includes system suspend depends on the platform. Arithmetic must stay within
the signed 64-bit microsecond range.

`Clock` uses the same time source and reports nonnegative elapsed durations. Concurrent reads are
safe when no thread calls `restart()` or `getElapsedTimeAndReset()` at the same time.

Keep durations as `Time` in application and module interfaces. Use the explicit, `constexpr`
conversion
`Time::toChrono()` where a standard-library wait requires a chrono duration:

```cpp
condition.wait_for( lock, remaining.toChrono() );
Sys::sleep( Milliseconds( 1 ) );
```

The conversion returns `std::chrono::microseconds` without truncation or allocation, preserving
zero and negative durations. For sleeping through eepp, use `Sys::sleep( Milliseconds( 1 ) )`.
When waiting on a deadline, recompute the remaining duration after each wake rather than
restarting the original delay.

`Sys::sleep()` returns immediately for zero and negative durations.
