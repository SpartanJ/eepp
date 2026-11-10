#ifndef EE_SYSTEM_MONOTONICCLOCK_HPP
#define EE_SYSTEM_MONOTONICCLOCK_HPP

#include <eepp/system/time.hpp>

namespace EE { namespace System {

/** Monotonic time source for deadlines and timestamps shared between threads. Its origin is
 * unspecified: timestamps have no calendar meaning and must not be persisted or sent between
 * processes. Resolution is microseconds; suspend behavior follows the platform's steady clock. */
class EE_API MonotonicClock {
  public:
	/** An instant, distinct from a Time duration. Arithmetic must remain within the signed
	 * 64-bit microsecond range. A default value denotes the clock's origin, not now(). */
	class TimePoint {
	  public:
		constexpr TimePoint() noexcept = default;

		constexpr auto operator<=>( const TimePoint& ) const noexcept = default;

		constexpr TimePoint operator+( Time duration ) const noexcept {
			return TimePoint( mMicroseconds + duration.asMicroseconds() );
		}

		constexpr TimePoint operator-( Time duration ) const noexcept {
			return TimePoint( mMicroseconds - duration.asMicroseconds() );
		}

		constexpr Time operator-( TimePoint other ) const noexcept {
			return Microseconds( mMicroseconds - other.mMicroseconds );
		}

		constexpr TimePoint& operator+=( Time duration ) noexcept {
			return *this = *this + duration;
		}

		constexpr TimePoint& operator-=( Time duration ) noexcept {
			return *this = *this - duration;
		}

		friend constexpr TimePoint operator+( Time duration, TimePoint point ) noexcept {
			return point + duration;
		}

	  private:
		friend class MonotonicClock;

		constexpr explicit TimePoint( Int64 microseconds ) noexcept :
			mMicroseconds( microseconds ) {}

		Int64 mMicroseconds{ 0 };
	};

	/** Thread-safe. Calls use one shared steady time source without allocating or locking. */
	static TimePoint now() noexcept;
};

}} // namespace EE::System

#endif
