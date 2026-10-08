#ifndef EE_SYSTEM_DATETIME_HPP
#define EE_SYSTEM_DATETIME_HPP

// Required for the comparison categories deduced by defaulted operator<=>.
#include <compare> // IWYU pragma: keep
#include <eepp/config.hpp>
#include <optional>

namespace EE { namespace System {

/** Civil Gregorian calendar date without timezone or timestamp semantics.
 * Arithmetic outside the Int32 year range leaves the value unchanged. */
struct EE_API CalendarDate {
	Int32 year{ 1970 };
	Uint8 month{ 1 };
	Uint8 day{ 1 };

	bool isValid() const;

	int daysInMonth() const;

	/** Signed number of Gregorian days since 1970-01-01; requires a valid date. */
	Int64 daysSinceEpoch() const;

	/** Unix seconds at midnight UTC; returns nullopt for an invalid date. */
	std::optional<Int64> toUnixTimestamp() const;

	/** UTC date containing the Unix timestamp, including negative timestamps.
	 * Returns nullopt if the date is outside the Int32 year range. */
	static std::optional<CalendarDate> fromUnixTimestamp( Int64 seconds );

	CalendarDate addDays( Int64 days ) const;

	CalendarDate addMonths( Int64 months ) const;

	CalendarDate addYears( Int64 years ) const;

	static bool isLeapYear( Int32 year );

	static int daysInMonth( Int32 year, Uint8 month );

	static CalendarDate today();

	auto operator<=>( const CalendarDate& ) const = default;
};

/** Civil wall-clock time without timezone semantics.
 * Arithmetic wraps within the day and preserves smaller units. */
struct EE_API TimeOfDay {
	Uint8 hour{ 0 };
	Uint8 minute{ 0 };
	Uint8 second{ 0 };
	Uint16 millisecond{ 0 };

	bool isValid() const;

	TimeOfDay addHours( Int64 hours ) const;

	TimeOfDay addMinutes( Int64 minutes ) const;

	TimeOfDay addSeconds( Int64 seconds ) const;

	static TimeOfDay now();

	auto operator<=>( const TimeOfDay& ) const = default;
};

/** A local civil date and wall-clock time. This value does not imply a timezone. */
struct EE_API LocalDateTime {
	CalendarDate date;
	TimeOfDay time;

	bool isValid() const;

	/** Unix seconds using a UTC offset in seconds (local = UTC + offset), defaulting to UTC.
	 * Discards milliseconds; returns nullopt for an invalid value.
	 * Does not infer the system timezone or resolve daylight-saving transitions. */
	std::optional<Int64> toUnixTimestamp( Int32 utcOffsetSeconds = 0 ) const;

	/** Unix milliseconds preserving the millisecond field, with the same offset convention.
	 * Returns nullopt for an invalid value or an Int64 timestamp overflow. */
	std::optional<Int64> toUnixTimestampMilliseconds( Int32 utcOffsetSeconds = 0 ) const;

	/** Civil date/time at the given UTC offset (default: UTC), with zero milliseconds.
	 * Returns nullopt if the result is outside the Int32 year range. */
	static std::optional<LocalDateTime> fromUnixTimestamp( Int64 seconds,
														   Int32 utcOffsetSeconds = 0 );

	/** Civil date/time at the given UTC offset (default: UTC), preserving milliseconds.
	 * Returns nullopt if the result is outside the Int32 year range. */
	static std::optional<LocalDateTime> fromUnixTimestampMilliseconds( Int64 milliseconds,
																	   Int32 utcOffsetSeconds = 0 );

	LocalDateTime addHours( Int64 hours ) const;

	LocalDateTime addMinutes( Int64 minutes ) const;

	LocalDateTime addSeconds( Int64 seconds ) const;

	static LocalDateTime now();

	auto operator<=>( const LocalDateTime& ) const = default;
};

/** Inclusive range of civil Gregorian dates. */
struct EE_API DateRange {
	CalendarDate start;
	CalendarDate end;

	bool isValid() const;

	bool contains( const CalendarDate& date ) const;

	auto operator<=>( const DateRange& ) const = default;
};

}} // namespace EE::System

#endif
