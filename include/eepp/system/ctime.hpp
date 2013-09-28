
#ifndef EE_SYSTEMCTIME_HPP
#define EE_SYSTEMCTIME_HPP

#include <eepp/declares.hpp>

namespace EE { namespace System {

/** @brief Represents a time value
**  Based on the SFML2 implementation ( not the same, this version uses doubles for seconds and milliseconds ) */
class EE_API cTime
{
	public :
		/** @brief Default constructor
		**	Sets the time value to zero. */
		cTime();

		/** @brief Return the time value as a number of seconds
		**	@return cTime in seconds
		**	@see AsMilliseconds, AsMicroseconds */
		eeDouble AsSeconds() const;

		/** @brief Return the time value as a number of milliseconds
		**	@return cTime in milliseconds
		**	@see AsSeconds, AsMicroseconds */
		eeDouble AsMilliseconds() const;

		/** @brief Return the time value as a number of microseconds
		**	@return cTime in microseconds
		**	@see asSeconds, asMilliseconds */
		Int64 AsMicroseconds() const;

		static const cTime Zero; ///< Predefined "zero" time value
	private :

		friend EE_API cTime Seconds(eeDouble);
		friend EE_API cTime Milliseconds(eeDouble);
		friend EE_API cTime Microseconds(Int64);

		/** @brief Construct from a number of microseconds
		**  This function is internal. To construct time values,
		**  use Seconds, Milliseconds or Microseconds instead.
		** @param microseconds Number of microseconds */
		explicit cTime(Int64 microseconds);

		Int64 mMicroseconds; ///< cTime value stored as microseconds
};

/// @relates cTime
/// @brief Construct a time value from a number of seconds
/// @param amount Number of seconds
/// @return cTime value constructed from the amount of seconds
/// @see Milliseconds, Microseconds
EE_API cTime Seconds(eeDouble amount);

/// @relates cTime
/// @brief Construct a time value from a number of milliseconds
/// @param amount Number of milliseconds
/// @return cTime value constructed from the amount of milliseconds
/// @see Seconds, Microseconds
EE_API cTime Milliseconds(eeDouble amount);

/// @relates cTime
/// @brief Construct a time value from a number of microseconds
/// @param amount Number of microseconds
/// @return cTime value constructed from the amount of microseconds
/// @see Seconds, Milliseconds
EE_API cTime Microseconds(Int64 amount);

/// @relates cTime
/// @brief Overload of == operator to compare two time values
/// @param left  Left operand (a time)
/// @param right Right operand (a time)
/// @return True if both time values are equal
EE_API bool operator ==(cTime left, cTime right);

/// @relates cTime
/// @brief Overload of != operator to compare two time values
/// @param left  Left operand (a time)
/// @param right Right operand (a time)
/// @return True if both time values are different
EE_API bool operator !=(cTime left, cTime right);

/// @relates cTime
/// @brief Overload of < operator to compare two time values
/// @param left  Left operand (a time)
/// @param right Right operand (a time)
/// @return True if @a left is lesser than @a right
EE_API bool operator <(cTime left, cTime right);

/// @relates cTime
/// @brief Overload of > operator to compare two time values
/// @param left  Left operand (a time)
/// @param right Right operand (a time)
/// @return True if @a left is greater than @a right
EE_API bool operator >(cTime left, cTime right);

/// @relates cTime
/// @brief Overload of <= operator to compare two time values
/// @param left  Left operand (a time)
/// @param right Right operand (a time)
/// @return True if @a left is lesser or equal than @a right
EE_API bool operator <=(cTime left, cTime right);

/// @relates cTime
/// @brief Overload of >= operator to compare two time values
/// @param left  Left operand (a time)
/// @param right Right operand (a time)
/// @return True if @a left is greater or equal than @a right
EE_API bool operator >=(cTime left, cTime right);

/// @relates cTime
/// @brief Overload of unary - operator to negate a time value
/// @param right Right operand (a time)
/// @return Opposite of the time value
EE_API cTime operator -(cTime right);

/// @relates cTime
/// @brief Overload of binary + operator to add two time values
/// @param left  Left operand (a time)
/// @param right Right operand (a time)
/// @return Sum of the two times values
EE_API cTime operator +(cTime left, cTime right);

/// @relates cTime
/// @brief Overload of binary += operator to add/assign two time values
/// @param left  Left operand (a time)
/// @param right Right operand (a time)
/// @return Sum of the two times values
EE_API cTime& operator +=(cTime& left, cTime right);

/// @relates cTime
/// @brief Overload of binary - operator to subtract two time values
/// @param left  Left operand (a time)
/// @param right Right operand (a time)
/// @return Difference of the two times values
EE_API cTime operator -(cTime left, cTime right);

/// @relates cTime
/// @brief Overload of binary -= operator to subtract/assign two time values
/// @param left  Left operand (a time)
/// @param right Right operand (a time)
/// @return Difference of the two times values
EE_API cTime& operator -=(cTime& left, cTime right);

/// @relates cTime
/// @brief Overload of binary * operator to scale a time value
/// @param left  Left operand (a time)
/// @param right Right operand (a number)
/// @return @a left multiplied by @a right
EE_API cTime operator *(cTime left, cTime right);

/// @relates cTime
/// @brief Overload of binary * operator to scale a time value
/// @param left  Left operand (a time)
/// @param right Right operand (a number)
/// @return @a left multiplied by @a right
EE_API cTime operator *(cTime left, eeDouble right);

/// @relates cTime
/// @brief Overload of binary * operator to scale a time value
/// @param left  Left operand (a number)
/// @param right Right operand (a time)
/// @return @a left multiplied by @a right
EE_API cTime operator *(eeDouble left, cTime right);

/// @relates cTime
/// @brief Overload of binary * operator to scale a time value
/// @param left  Left operand (a number)
/// @param right Right operand (a time)
/// @return @a left multiplied by @a right
EE_API cTime operator *(Int64 left, cTime right);

/// @relates cTime
/// @brief Overload of binary * operator to scale a time value
/// @param left  Left operand (a time)
/// @param right Right operand (a number)
/// @return @a left multiplied by @a right
EE_API cTime operator *(cTime left, Int64 right);

/// @relates cTime
/// @brief Overload of binary *= operator to scale/assign a time value
/// @param left  Left operand (a time)
/// @param right Right operand (a number)
/// @return @a left multiplied by @a right
EE_API cTime& operator *=(cTime& left, eeDouble right);

/// @relates cTime
/// @brief Overload of binary *= operator to scale/assign a time value
/// @param left  Left operand (a time)
/// @param right Right operand (a number)
/// @return @a left multiplied by @a right
EE_API cTime& operator *=(cTime& left, Int64 right);

/// @relates cTime
/// @brief Overload of binary *= operator to scale/assign a time value
/// @param left  Left operand (a time)
/// @param right Right operand (a time)
/// @return @a left multiplied by @a right
EE_API cTime& operator *=(cTime& left, cTime right);

/// @relates cTime
/// @brief Overload of binary / operator to scale a time value
/// @param left  Left operand (a time)
/// @param right Right operand (a time)
/// @return @a left divided by @a right
EE_API cTime operator /(cTime left, cTime right);

/// @relates cTime
/// @brief Overload of binary / operator to scale a time value
/// @param left  Left operand (a time)
/// @param right Right operand (a number)
/// @return @a left divided by @a right
EE_API cTime operator /(cTime left, eeDouble right);

/// @relates cTime
/// @brief Overload of binary / operator to scale a time value
/// @param left  Left operand (a time)
/// @param right Right operand (a number)
/// @return @a left divided by @a right
EE_API cTime operator /(cTime left, Int64 right);

/// @relates cTime
/// @brief Overload of binary /= operator to scale/assign a time value
/// @param left  Left operand (a time)
/// @param right Right operand (a number)
/// @return @a left divided by @a right
EE_API cTime& operator /=(cTime& left, Int64 right);

/// @relates cTime
/// @brief Overload of binary /= operator to scale/assign a time value
/// @param left  Left operand (a time)
/// @param right Right operand (a time)
/// @return @a left divided by @a right
EE_API cTime& operator /=(cTime& left, cTime right);

/// @relates cTime
/// @brief Overload of binary % operator to compute remainder of a time value
/// @param left  Left operand (a time)
/// @param right Right operand (a time)
/// @return @a left modulo @a right
EE_API cTime operator %(cTime left, cTime right);

/// @relates cTime
/// @brief Overload of binary %= operator to compute/assign remainder of a time value
/// @param left  Left operand (a time)
/// @param right Right operand (a time)
/// @return @a left modulo @a right
EE_API cTime& operator %=(cTime& left, cTime right);

}}

#endif

/**
@class cTime
@ingroup System

EE::Systen::cTime encapsulates a time value in a flexible way.
It allows to define a time value either as a number of
seconds, milliseconds or microseconds. It also works the
other way round: you can read a time value as either
a number of seconds, milliseconds or microseconds.

By using such a flexible interface, the API doesn't
impose any fixed type or resolution for time values,
and let the user choose its own favorite representation.

cTime values support the usual mathematical operations:
you can add or subtract two times, multiply or divide
a time by a number, compare two times, etc.

Since they represent a time span and not an absolute time
value, times can also be negative.

Usage example:
@code
cTime t1 = Seconds(0.1f);
eeDouble milli = t1.AsMilliseconds(); // 100

cTime t2 = Milliseconds(30);
Int64 micro = t2.AsMicroseconds(); // 30000

cTime t3 = Microseconds(-800000);
eeDouble sec = t3.AsSeconds(); // -0.8
@endcode

@code
void update(cTime elapsed)
{
   position += speed * elapsed;
}

update(Milliseconds(100));
@endcode

@see cClock
*/
