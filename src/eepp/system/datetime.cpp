#include <eepp/system/datetime.hpp>

#include <algorithm>
#include <chrono>
#include <ctime>
#include <limits>

namespace EE { namespace System {

namespace {

// Howard Hinnant's civil calendar conversion, adapted to eepp integer types. The epoch is
// arbitrary; only differences and round trips matter here.
constexpr Int64 daysFromCivil( Int64 year, unsigned month, unsigned day ) {
	year -= month <= 2;
	const Int64 era = ( year >= 0 ? year : year - 399 ) / 400;
	const unsigned yoe = static_cast<unsigned>( year - era * 400 );
	const unsigned doy = ( 153 * ( month > 2 ? month - 3 : month + 9 ) + 2 ) / 5 + day - 1;
	const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
	return era * 146097 + static_cast<Int64>( doe );
}

CalendarDate civilFromDays( Int64 days ) {
	const Int64 era = ( days >= 0 ? days : days - 146096 ) / 146097;
	const unsigned doe = static_cast<unsigned>( days - era * 146097 );
	const unsigned yoe = ( doe - doe / 1460 + doe / 36524 - doe / 146096 ) / 365;
	Int64 year = static_cast<Int64>( yoe ) + era * 400;
	const unsigned doy = doe - ( 365 * yoe + yoe / 4 - yoe / 100 );
	const unsigned mp = ( 5 * doy + 2 ) / 153;
	const unsigned day = doy - ( 153 * mp + 2 ) / 5 + 1;
	const unsigned month = mp < 10 ? mp + 3 : mp - 9;
	year += month <= 2;

	if ( year < std::numeric_limits<Int32>::min() || year > std::numeric_limits<Int32>::max() )
		return {};
	return { static_cast<Int32>( year ), static_cast<Uint8>( month ), static_cast<Uint8>( day ) };
}

Int64 floorDiv( Int64 numerator, Int64 denominator ) {
	const Int64 quotient = numerator / denominator;
	const Int64 remainder = numerator % denominator;
	return quotient - ( remainder != 0 && ( remainder < 0 ) != ( denominator < 0 ) );
}

Int64 positiveModulo( Int64 value, Int64 modulus ) {
	const Int64 remainder = value % modulus;
	return remainder < 0 ? remainder + modulus : remainder;
}

bool getLocalTime( std::time_t time, std::tm& out ) {
#if EE_PLATFORM == EE_PLATFORM_WIN
	return localtime_s( &out, &time ) == 0;
#else
	return localtime_r( &time, &out ) != nullptr;
#endif
}

constexpr Int64 SecondsPerDay = 24 * 60 * 60;
constexpr Int64 EpochCivilDays = daysFromCivil( 1970, 1, 1 );

std::optional<CalendarDate> dateFromEpochDays( Int64 days ) {
	constexpr Int64 minimum =
		daysFromCivil( std::numeric_limits<Int32>::min(), 1, 1 ) - EpochCivilDays;
	constexpr Int64 maximum =
		daysFromCivil( std::numeric_limits<Int32>::max(), 12, 31 ) - EpochCivilDays;
	if ( days < minimum || days > maximum )
		return std::nullopt;
	return civilFromDays( days + EpochCivilDays );
}

std::optional<LocalDateTime> dateTimeFromUnixTimestamp( Int64 timestamp, Int32 utcOffsetSeconds,
														Int64 unitsPerSecond ) {
	const Int64 unitsPerDay = SecondsPerDay * unitsPerSecond;
	// Apply the offset after splitting the timestamp so even Int64 endpoints cannot overflow.
	const Int64 dayUnits = positiveModulo( timestamp, unitsPerDay ) +
						   static_cast<Int64>( utcOffsetSeconds ) * unitsPerSecond;
	const auto date =
		dateFromEpochDays( floorDiv( timestamp, unitsPerDay ) + floorDiv( dayUnits, unitsPerDay ) );
	if ( !date )
		return std::nullopt;
	const Int64 wrapped = positiveModulo( dayUnits, unitsPerDay );
	const Int64 seconds = wrapped / unitsPerSecond;
	return LocalDateTime{ *date,
						  { static_cast<Uint8>( seconds / 3600 ),
							static_cast<Uint8>( seconds / 60 % 60 ),
							static_cast<Uint8>( seconds % 60 ),
							static_cast<Uint16>( unitsPerSecond == 1000 ? wrapped % 1000 : 0 ) } };
}

} // namespace

bool CalendarDate::isLeapYear( Int32 year ) {
	return year % 4 == 0 && ( year % 100 != 0 || year % 400 == 0 );
}

int CalendarDate::daysInMonth( Int32 year, Uint8 month ) {
	static constexpr int Days[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
	if ( month < 1 || month > 12 )
		return 0;
	if ( month == 2 && isLeapYear( year ) )
		return 29;
	return Days[month - 1];
}

bool CalendarDate::isValid() const {
	return month >= 1 && month <= 12 && day >= 1 && day <= daysInMonth( year, month );
}

int CalendarDate::daysInMonth() const {
	return daysInMonth( year, month );
}

Int64 CalendarDate::daysSinceEpoch() const {
	return daysFromCivil( year, month, day ) - EpochCivilDays;
}

std::optional<Int64> CalendarDate::toUnixTimestamp() const {
	if ( !isValid() )
		return std::nullopt;
	return daysSinceEpoch() * SecondsPerDay;
}

std::optional<CalendarDate> CalendarDate::fromUnixTimestamp( Int64 seconds ) {
	return dateFromEpochDays( floorDiv( seconds, SecondsPerDay ) );
}

CalendarDate CalendarDate::addDays( Int64 days ) const {
	if ( !isValid() )
		return *this;
	const Int64 current = daysFromCivil( year, month, day );
	const Int64 minimum = daysFromCivil( std::numeric_limits<Int32>::min(), 1, 1 );
	const Int64 maximum = daysFromCivil( std::numeric_limits<Int32>::max(), 12, 31 );
	if ( days < minimum - current || days > maximum - current )
		return *this;
	return civilFromDays( current + days );
}

CalendarDate CalendarDate::addMonths( Int64 months ) const {
	if ( !isValid() )
		return *this;

	const Int64 current = static_cast<Int64>( year ) * 12 + ( month - 1 );
	const Int64 minimum = static_cast<Int64>( std::numeric_limits<Int32>::min() ) * 12;
	const Int64 maximum = static_cast<Int64>( std::numeric_limits<Int32>::max() ) * 12 + 11;
	if ( months < minimum - current || months > maximum - current )
		return *this;
	const Int64 totalMonths = current + months;
	const Int64 newYear = floorDiv( totalMonths, 12 );
	if ( newYear < std::numeric_limits<Int32>::min() ||
		 newYear > std::numeric_limits<Int32>::max() )
		return *this;
	const Uint8 newMonth = static_cast<Uint8>( positiveModulo( totalMonths, 12 ) + 1 );
	const Uint8 newDay = static_cast<Uint8>(
		std::min<int>( day, daysInMonth( static_cast<Int32>( newYear ), newMonth ) ) );
	return { static_cast<Int32>( newYear ), newMonth, newDay };
}

CalendarDate CalendarDate::addYears( Int64 years ) const {
	if ( !isValid() )
		return *this;
	if ( years < static_cast<Int64>( std::numeric_limits<Int32>::min() ) - year ||
		 years > static_cast<Int64>( std::numeric_limits<Int32>::max() ) - year )
		return *this;
	const Int64 newYear = static_cast<Int64>( year ) + years;
	if ( newYear < std::numeric_limits<Int32>::min() ||
		 newYear > std::numeric_limits<Int32>::max() )
		return *this;
	const Uint8 newDay = static_cast<Uint8>(
		std::min<int>( day, daysInMonth( static_cast<Int32>( newYear ), month ) ) );
	return { static_cast<Int32>( newYear ), month, newDay };
}

CalendarDate CalendarDate::today() {
	const std::time_t current = std::time( nullptr );
	std::tm local{};
	if ( !getLocalTime( current, local ) )
		return {};
	return { static_cast<Int32>( local.tm_year + 1900 ), static_cast<Uint8>( local.tm_mon + 1 ),
			 static_cast<Uint8>( local.tm_mday ) };
}

bool TimeOfDay::isValid() const {
	return hour < 24 && minute < 60 && second < 60 && millisecond < 1000;
}

TimeOfDay TimeOfDay::addHours( Int64 hours ) const {
	if ( !isValid() )
		return *this;
	TimeOfDay result = *this;
	result.hour =
		static_cast<Uint8>( positiveModulo( static_cast<Int64>( hour ) + hours % 24, 24 ) );
	return result;
}

TimeOfDay TimeOfDay::addMinutes( Int64 minutes ) const {
	if ( !isValid() )
		return *this;
	const Int64 totalMinutes = static_cast<Int64>( hour ) * 60 + minute + minutes % ( 24 * 60 );
	const Int64 wrapped = positiveModulo( totalMinutes, 24 * 60 );
	TimeOfDay result = *this;
	result.hour = static_cast<Uint8>( wrapped / 60 );
	result.minute = static_cast<Uint8>( wrapped % 60 );
	return result;
}

TimeOfDay TimeOfDay::addSeconds( Int64 seconds ) const {
	if ( !isValid() )
		return *this;
	const Int64 totalSeconds =
		static_cast<Int64>( hour ) * 3600 + minute * 60 + second + seconds % SecondsPerDay;
	const Int64 wrapped = positiveModulo( totalSeconds, SecondsPerDay );
	TimeOfDay result = *this;
	result.hour = static_cast<Uint8>( wrapped / 3600 );
	result.minute = static_cast<Uint8>( ( wrapped % 3600 ) / 60 );
	result.second = static_cast<Uint8>( wrapped % 60 );
	return result;
}

TimeOfDay TimeOfDay::now() {
	return LocalDateTime::now().time;
}

bool LocalDateTime::isValid() const {
	return date.isValid() && time.isValid();
}

std::optional<Int64> LocalDateTime::toUnixTimestamp( Int32 utcOffsetSeconds ) const {
	if ( !isValid() )
		return std::nullopt;
	// The complete Int32 year range fits in Int64 seconds, including any Int32 offset.
	return date.daysSinceEpoch() * SecondsPerDay + time.hour * 3600 + time.minute * 60 +
		   time.second - static_cast<Int64>( utcOffsetSeconds );
}

std::optional<Int64> LocalDateTime::toUnixTimestampMilliseconds( Int32 utcOffsetSeconds ) const {
	const auto seconds = toUnixTimestamp( utcOffsetSeconds );
	if ( !seconds )
		return std::nullopt;
	constexpr Int64 minimum = std::numeric_limits<Int64>::min();
	constexpr Int64 maximum = std::numeric_limits<Int64>::max();
	const Int64 minimumSeconds = floorDiv( minimum, 1000 );
	const Int64 minimumFraction = positiveModulo( minimum, 1000 );
	if ( *seconds < minimumSeconds || *seconds > maximum / 1000 ||
		 ( *seconds == minimumSeconds && time.millisecond < minimumFraction ) ||
		 ( *seconds == maximum / 1000 && time.millisecond > maximum % 1000 ) )
		return std::nullopt;
	// The floor-divided minimum second cannot be multiplied by 1000 without overflowing.
	if ( *seconds == minimumSeconds )
		return minimum + ( time.millisecond - minimumFraction );
	return *seconds * 1000 + time.millisecond;
}

std::optional<LocalDateTime> LocalDateTime::fromUnixTimestamp( Int64 seconds,
															   Int32 utcOffsetSeconds ) {
	return dateTimeFromUnixTimestamp( seconds, utcOffsetSeconds, 1 );
}

std::optional<LocalDateTime>
LocalDateTime::fromUnixTimestampMilliseconds( Int64 milliseconds, Int32 utcOffsetSeconds ) {
	return dateTimeFromUnixTimestamp( milliseconds, utcOffsetSeconds, 1000 );
}

namespace {

LocalDateTime addLocalUnits( const LocalDateTime& value, Int64 amount, Int64 unitsPerDay,
							 Int64 secondsPerUnit ) {
	if ( !value.isValid() )
		return value;
	const Int64 current = value.time.hour * 3600 + value.time.minute * 60 + value.time.second;
	// Divide before multiplying: arbitrary Int64 steps must not overflow intermediates.
	const Int64 total = current + ( amount % unitsPerDay ) * secondsPerUnit;
	const Int64 dayDelta = amount / unitsPerDay + floorDiv( total, SecondsPerDay );
	const CalendarDate date = value.date.addDays( dayDelta );
	if ( dayDelta != 0 && date == value.date )
		return value;
	const Int64 wrapped = positiveModulo( total, SecondsPerDay );
	return { date,
			 { static_cast<Uint8>( wrapped / 3600 ), static_cast<Uint8>( ( wrapped % 3600 ) / 60 ),
			   static_cast<Uint8>( wrapped % 60 ), value.time.millisecond } };
}

} // namespace

LocalDateTime LocalDateTime::addHours( Int64 hours ) const {
	return addLocalUnits( *this, hours, 24, 3600 );
}

LocalDateTime LocalDateTime::addMinutes( Int64 minutes ) const {
	return addLocalUnits( *this, minutes, 1440, 60 );
}

LocalDateTime LocalDateTime::addSeconds( Int64 seconds ) const {
	return addLocalUnits( *this, seconds, SecondsPerDay, 1 );
}

LocalDateTime LocalDateTime::now() {
	// Sample once so crossing midnight cannot combine yesterday's date with today's time.
	const auto now = std::chrono::system_clock::now();
	const std::time_t current = std::chrono::system_clock::to_time_t( now );
	std::tm local{};
	if ( !getLocalTime( current, local ) )
		return {};
	const auto milliseconds =
		std::chrono::duration_cast<std::chrono::milliseconds>( now.time_since_epoch() ).count();
	return { { static_cast<Int32>( local.tm_year + 1900 ), static_cast<Uint8>( local.tm_mon + 1 ),
			   static_cast<Uint8>( local.tm_mday ) },
			 { static_cast<Uint8>( local.tm_hour ), static_cast<Uint8>( local.tm_min ),
			   static_cast<Uint8>( local.tm_sec ),
			   static_cast<Uint16>( positiveModulo( milliseconds, 1000 ) ) } };
}

bool DateRange::isValid() const {
	return start.isValid() && end.isValid() && start <= end;
}

bool DateRange::contains( const CalendarDate& date ) const {
	return isValid() && date.isValid() && date >= start && date <= end;
}

}} // namespace EE::System
