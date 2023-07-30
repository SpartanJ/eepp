
#ifndef EE_SYSTEMCTIME_HPP
#define EE_SYSTEMCTIME_HPP

#include <eepp/config.hpp>
#include <string>

namespace EE { namespace System {

/** Represents a time value */
class EE_API Time {
  public:
	static bool isValid( const std::string& str );

	static Time fromString( const std::string& str );

	/** @brief Default constructor
	**	Sets the time value to zero. */
	constexpr Time();

	/** @brief Return the time value as a number of seconds
	**	@return Time in seconds
	**	@see AsMilliseconds, AsMicroseconds */
	constexpr double asSeconds() const;

	/** @brief Return the time value as a number of milliseconds
	**	@return Time in milliseconds
	**	@see AsSeconds, AsMicroseconds */
	constexpr double asMilliseconds() const;

	/** @brief Return the time value as a number of microseconds
	**	@return Time in microseconds
	**	@see asSeconds, asMilliseconds */
	constexpr Int64 asMicroseconds() const;

	static const Time Zero; ///< Predefined "zero" time value

	/** Converts the time into a human readable string. */
	std::string toString() const;

  private:
	friend constexpr Time Minutes( double );
	friend constexpr Time Seconds( double );
	friend constexpr Time Milliseconds( double );
	friend constexpr Time Microseconds( Int64 );

	/** @brief Construct from a number of microseconds
	**  This function is internal. To construct time values,
	**  use Seconds, Milliseconds or Microseconds instead.
	** @param microseconds Number of microseconds */
	constexpr explicit Time( Int64 microseconds );

	Int64 mMicroseconds; ///< Time value stored as microseconds
};

constexpr Time::Time() : mMicroseconds( 0 ) {}

constexpr Time::Time( Int64 Microseconds ) : mMicroseconds( Microseconds ) {}

constexpr double Time::asSeconds() const {
	return mMicroseconds / 1000000.0;
}

constexpr double Time::asMilliseconds() const {
	return mMicroseconds / 1000.0;
}

constexpr Int64 Time::asMicroseconds() const {
	return mMicroseconds;
}

constexpr Time Minutes( double amount ) {
	return Time( static_cast<Int64>( amount * 1000000 * 60 ) );
}

constexpr Time Seconds( double amount ) {
	return Time( static_cast<Int64>( amount * 1000000 ) );
}

constexpr Time Milliseconds( double amount ) {
	return Time( static_cast<Int64>( amount * 1000 ) );
}

constexpr Time Microseconds( Int64 amount ) {
	return Time( amount );
}

constexpr bool operator==( Time left, Time right ) {
	return left.asMicroseconds() == right.asMicroseconds();
}

constexpr bool operator!=( Time left, Time right ) {
	return left.asMicroseconds() != right.asMicroseconds();
}

constexpr bool operator<( Time left, Time right ) {
	return left.asMicroseconds() < right.asMicroseconds();
}

constexpr bool operator>( Time left, Time right ) {
	return left.asMicroseconds() > right.asMicroseconds();
}

constexpr bool operator<=( Time left, Time right ) {
	return left.asMicroseconds() <= right.asMicroseconds();
}

constexpr bool operator>=( Time left, Time right ) {
	return left.asMicroseconds() >= right.asMicroseconds();
}

constexpr Time operator-( Time right ) {
	return Microseconds( -right.asMicroseconds() );
}

constexpr Time operator+( Time left, Time right ) {
	return Microseconds( left.asMicroseconds() + right.asMicroseconds() );
}

constexpr Time& operator+=( Time& left, Time right ) {
	return left = left + right;
}

constexpr Time operator-( Time left, Time right ) {
	return Microseconds( left.asMicroseconds() - right.asMicroseconds() );
}

constexpr Time& operator-=( Time& left, Time right ) {
	return left = left - right;
}

constexpr Time operator*( Time left, Time right ) {
	return Microseconds( left.asMicroseconds() * right.asMicroseconds() );
}

constexpr Time operator*( Time left, double right ) {
	return Seconds( left.asSeconds() * right );
}

constexpr Time operator*( double left, Time right ) {
	return right * left;
}

constexpr Time operator*( Time left, Int64 right ) {
	return Microseconds( left.asMicroseconds() * right );
}

constexpr Time operator*( Int64 left, Time right ) {
	return right * left;
}

constexpr Time& operator*=( Time& left, Time right ) {
	return left = left * right;
}

constexpr Time& operator*=( Time& left, double right ) {
	return left = left * right;
}

constexpr Time& operator*=( Time& left, Int64 right ) {
	return left = left * right;
}

constexpr Time operator/( Time left, Time right ) {
	return Microseconds( left.asMicroseconds() / right.asMicroseconds() );
}

constexpr Time operator/( Time left, double right ) {
	return Seconds( left.asSeconds() / right );
}

constexpr Time operator/( Time left, Int64 right ) {
	return Microseconds( left.asMicroseconds() / right );
}

constexpr Time& operator/=( Time& left, Time right ) {
	return left = left / right;
}

constexpr Time& operator/=( Time& left, Int64 right ) {
	return left = left / right;
}

constexpr Time operator%( Time left, Time right ) {
	return Microseconds( left.asMicroseconds() % right.asMicroseconds() );
}

constexpr Time& operator%=( Time& left, Time right ) {
	return left = left % right;
}

}} // namespace EE::System

#endif

/**
@class EE::System::Time

EE::System::Time encapsulates a time value in a flexible way.
It allows to define a time value either as a number of
seconds, milliseconds or microseconds. It also works the
other way round: you can read a time value as either
a number of seconds, milliseconds or microseconds.

By using such a flexible interface, the API doesn't
impose any fixed type or resolution for time values,
and let the user choose its own favorite representation.

Time values support the usual mathematical operations:
you can add or subtract two times, multiply or divide
a time by a number, compare two times, etc.

Since they represent a time span and not an absolute time
value, times can also be negative.

Usage example:
@code
Time t1 = Seconds(0.1f);
double milli = t1.asMilliseconds(); // 100

Time t2 = Milliseconds(30);
Int64 micro = t2.asMicroseconds(); // 30000

Time t3 = Microseconds(-800000);
double sec = t3.asSeconds(); // -0.8
@endcode

@code
void update(Time elapsed) {
   position += speed * elapsed;
}

update(Milliseconds(100));
@endcode

@see EE::System::Clock
*/
