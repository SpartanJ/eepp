#include <eepp/config.hpp>
#if EE_PLATFORM == EE_PLATFORM_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#include "utest.hpp"
#include <eepp/core/string.hpp>
#include <eepp/system/datetime.hpp>

using namespace EE;
using namespace EE::System;

UTEST( DateTime, CalendarDateValidityAndLeapYears ) {
	EXPECT_TRUE( CalendarDate::isLeapYear( 2000 ) );
	EXPECT_FALSE( CalendarDate::isLeapYear( 1900 ) );
	EXPECT_TRUE( CalendarDate::isLeapYear( 2024 ) );
	EXPECT_FALSE( CalendarDate::isLeapYear( 2025 ) );

	EXPECT_TRUE( ( CalendarDate{ 2000, 2, 29 } ).isValid() );
	EXPECT_FALSE( ( CalendarDate{ 1900, 2, 29 } ).isValid() );
	EXPECT_TRUE( ( CalendarDate{ 2024, 2, 29 } ).isValid() );
	EXPECT_FALSE( ( CalendarDate{ 2025, 2, 29 } ).isValid() );
	EXPECT_FALSE( ( CalendarDate{ 2026, 0, 1 } ).isValid() );
	EXPECT_FALSE( ( CalendarDate{ 2026, 13, 1 } ).isValid() );
	EXPECT_FALSE( ( CalendarDate{ 2026, 4, 31 } ).isValid() );
	EXPECT_FALSE( ( CalendarDate{ 2026, 1, 0 } ).isValid() );
}

UTEST( DateTime, CalendarDateMonthLengths ) {
	const int expected[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
	for ( Uint8 month = 1; month <= 12; ++month )
		EXPECT_EQ( CalendarDate::daysInMonth( 2025, month ), expected[month - 1] );
	EXPECT_EQ( CalendarDate::daysInMonth( 2024, 2 ), 29 );
	EXPECT_EQ( CalendarDate::daysInMonth( 2024, 0 ), 0 );
	EXPECT_EQ( CalendarDate::daysInMonth( 2024, 13 ), 0 );
}

UTEST( DateTime, CalendarDateDayArithmetic ) {
	EXPECT_TRUE( ( CalendarDate{ 2026, 1, 31 }.addDays( 1 ) == CalendarDate{ 2026, 2, 1 } ) );
	EXPECT_TRUE( ( CalendarDate{ 2026, 3, 1 }.addDays( -1 ) == CalendarDate{ 2026, 2, 28 } ) );
	EXPECT_TRUE( ( CalendarDate{ 2024, 3, 1 }.addDays( -1 ) == CalendarDate{ 2024, 2, 29 } ) );
	EXPECT_TRUE( ( CalendarDate{ 2026, 12, 31 }.addDays( 1 ) == CalendarDate{ 2027, 1, 1 } ) );
	EXPECT_TRUE( ( CalendarDate{ 2027, 1, 1 }.addDays( -1 ) == CalendarDate{ 2026, 12, 31 } ) );
	EXPECT_TRUE( ( CalendarDate{ 2000, 1, 1 }.addDays( 10000 ).addDays( -10000 ) ==
				   CalendarDate{ 2000, 1, 1 } ) );
}

UTEST( DateTime, CalendarDateMonthAndYearArithmeticClamps ) {
	EXPECT_TRUE( ( CalendarDate{ 2026, 1, 31 }.addMonths( 1 ) == CalendarDate{ 2026, 2, 28 } ) );
	EXPECT_TRUE( ( CalendarDate{ 2024, 1, 31 }.addMonths( 1 ) == CalendarDate{ 2024, 2, 29 } ) );
	EXPECT_TRUE( ( CalendarDate{ 2026, 3, 31 }.addMonths( -1 ) == CalendarDate{ 2026, 2, 28 } ) );
	EXPECT_TRUE( ( CalendarDate{ 2026, 1, 31 }.addMonths( -1 ) == CalendarDate{ 2025, 12, 31 } ) );
	EXPECT_TRUE( ( CalendarDate{ 2024, 2, 29 }.addYears( 1 ) == CalendarDate{ 2025, 2, 28 } ) );
	EXPECT_TRUE( ( CalendarDate{ 2024, 2, 29 }.addYears( 4 ) == CalendarDate{ 2028, 2, 29 } ) );
}

UTEST( DateTime, CalendarDateOrderingAndRange ) {
	const CalendarDate start{ 2026, 9, 1 };
	const CalendarDate middle{ 2026, 9, 15 };
	const CalendarDate end{ 2026, 9, 30 };
	EXPECT_TRUE( start < middle );
	EXPECT_TRUE( middle < end );
	EXPECT_TRUE( ( DateRange{ start, end } ).isValid() );
	EXPECT_TRUE( ( DateRange{ start, end } ).contains( start ) );
	EXPECT_TRUE( ( DateRange{ start, end } ).contains( middle ) );
	EXPECT_TRUE( ( DateRange{ start, end } ).contains( end ) );
	EXPECT_FALSE( ( DateRange{ start, end } ).contains( CalendarDate{ 2026, 10, 1 } ) );
	EXPECT_FALSE( ( DateRange{ end, start } ).isValid() );
}

UTEST( DateTime, TimeOfDayValidityAndWrapping ) {
	EXPECT_TRUE( ( TimeOfDay{ 0, 0, 0, 0 } ).isValid() );
	EXPECT_TRUE( ( TimeOfDay{ 23, 59, 59, 999 } ).isValid() );
	EXPECT_FALSE( ( TimeOfDay{ 24, 0, 0, 0 } ).isValid() );
	EXPECT_FALSE( ( TimeOfDay{ 12, 60, 0, 0 } ).isValid() );
	EXPECT_FALSE( ( TimeOfDay{ 12, 0, 60, 0 } ).isValid() );
	EXPECT_FALSE( ( TimeOfDay{ 12, 0, 0, 1000 } ).isValid() );

	EXPECT_TRUE( ( TimeOfDay{ 23, 30, 0, 7 }.addHours( 1 ) == TimeOfDay{ 0, 30, 0, 7 } ) );
	EXPECT_TRUE( ( TimeOfDay{ 0, 30, 0, 7 }.addHours( -1 ) == TimeOfDay{ 23, 30, 0, 7 } ) );
	EXPECT_TRUE( ( TimeOfDay{ 23, 59, 0, 7 }.addMinutes( 2 ) == TimeOfDay{ 0, 1, 0, 7 } ) );
	EXPECT_TRUE( ( TimeOfDay{ 0, 0, 1, 7 }.addSeconds( -2 ) == TimeOfDay{ 23, 59, 59, 7 } ) );
}

UTEST( DateTime, LocalDateTimeCarriesAcrossDates ) {
	LocalDateTime value{ CalendarDate{ 2026, 9, 28 }, TimeOfDay{ 23, 30, 45, 123 } };
	EXPECT_TRUE( value.isValid() );
	EXPECT_TRUE( ( value.addHours( 1 ) ==
				   LocalDateTime{ CalendarDate{ 2026, 9, 29 }, TimeOfDay{ 0, 30, 45, 123 } } ) );
	EXPECT_TRUE( ( value.addMinutes( 60 ) == value.addHours( 1 ) ) );
	EXPECT_TRUE(
		( LocalDateTime{ CalendarDate{ 2026, 1, 1 }, TimeOfDay{ 0, 0, 0, 0 } }.addSeconds( -1 ) ==
		  LocalDateTime{ CalendarDate{ 2025, 12, 31 }, TimeOfDay{ 23, 59, 59, 0 } } ) );
}

#include <eepp/system/datetimeformat.hpp>

UTEST( DateTime, FormatCompilerProducesSectionsAndLiterals ) {
	DateTimeFormat format( "dd/MM/yyyy HH:mm:ss.SSS" );
	ASSERT_TRUE( format.isValid() );
	const auto& tokens = format.getTokens();
	ASSERT_EQ( tokens.size(), static_cast<size_t>( 13 ) );
	EXPECT_TRUE( tokens[0].type == DateTimeFormatTokenType::Day );
	EXPECT_TRUE( tokens[1].type == DateTimeFormatTokenType::Literal );
	EXPECT_TRUE( tokens[1].literal == "/" );
	EXPECT_TRUE( tokens[2].type == DateTimeFormatTokenType::Month );
	EXPECT_TRUE( tokens[4].type == DateTimeFormatTokenType::Year );
	EXPECT_TRUE( tokens[6].type == DateTimeFormatTokenType::Hour24 );
	EXPECT_TRUE( tokens[8].type == DateTimeFormatTokenType::Minute );
	EXPECT_TRUE( tokens[10].type == DateTimeFormatTokenType::Second );
	EXPECT_TRUE( tokens[12].type == DateTimeFormatTokenType::Millisecond );
	EXPECT_FALSE( DateTimeFormat( "ddd/MM/yyyy" ).isValid() );
	EXPECT_FALSE( DateTimeFormat( "dd/QQ/yyyy" ).isValid() );
	EXPECT_TRUE( DateTimeFormat( "yyyy-MM-dd'T'HH:mm" ).isValid() );
}

UTEST( DateTime, FormattingAndTokenRanges ) {
	const DateTimeLocale locale = DateTimeLocale::fromLocaleName( "es_AR" );
	const LocalDateTime value{ CalendarDate{ 2026, 9, 28 }, TimeOfDay{ 20, 14, 32, 7 } };
	const DateTimeFormat format( "dd/MM/yyyy HH:mm:ss.SSS" );
	const auto formatted = DateTimeFormatter::formatDateTimeDetailed( value, format, locale );
	EXPECT_TRUE( formatted.text == "28/09/2026 20:14:32.007" );
	ASSERT_EQ( formatted.ranges.size(), format.getTokens().size() );
	EXPECT_EQ( formatted.ranges[0].start, static_cast<size_t>( 0 ) );
	EXPECT_EQ( formatted.ranges[0].length, static_cast<size_t>( 2 ) );
	EXPECT_EQ( formatted.ranges[4].start, static_cast<size_t>( 6 ) );
	EXPECT_EQ( formatted.ranges[4].length, static_cast<size_t>( 4 ) );
	EXPECT_EQ( formatted.ranges[8].start, static_cast<size_t>( 14 ) );
}

UTEST( DateTime, TwelveHourFormattingAndParsing ) {
	DateTimeLocale locale = DateTimeLocale::fromLocaleName( "en_US" );
	DateTimeFormat format( "hh:mm a" );
	EXPECT_TRUE( DateTimeFormatter::formatTime( TimeOfDay{ 0, 5, 0, 0 }, format, locale ) ==
				 "12:05 AM" );
	EXPECT_TRUE( DateTimeFormatter::formatTime( TimeOfDay{ 20, 14, 0, 0 }, format, locale ) ==
				 "08:14 PM" );
	const auto parsed = DateTimeFormatter::parseTime( "08:14 PM", format, locale );
	ASSERT_TRUE( parsed.has_value() );
	EXPECT_TRUE( *parsed == ( TimeOfDay{ 20, 14, 0, 0 } ) );
}

UTEST( DateTime, LocalePatternFallbacks ) {
	const auto argentina = DateTimeLocale::fromLocaleName( "es_AR.UTF-8" );
	EXPECT_TRUE( argentina.dateOrder == DateOrder::DMY );
	EXPECT_TRUE( argentina.datePattern() == "dd/MM/yyyy" );
	EXPECT_TRUE( argentina.hourCycle == HourCycle::H24 );
	EXPECT_EQ( argentina.firstDayOfWeek, static_cast<Uint8>( 1 ) );

	const auto us = DateTimeLocale::fromLocaleName( "en_US.UTF-8" );
	EXPECT_TRUE( us.dateOrder == DateOrder::MDY );
	EXPECT_TRUE( us.datePattern() == "MM/dd/yyyy" );
	EXPECT_TRUE( us.hourCycle == HourCycle::H12 );
	EXPECT_EQ( us.firstDayOfWeek, static_cast<Uint8>( 0 ) );

	const auto japan = DateTimeLocale::fromLocaleName( "ja_JP.UTF-8" );
	EXPECT_TRUE( japan.dateOrder == DateOrder::YMD );
	EXPECT_TRUE( japan.datePattern() == "yyyy-MM-dd" );
}

UTEST( DateTime, ExactDateAndTimeParsing ) {
	const DateTimeLocale locale = DateTimeLocale::fromLocaleName( "es_AR" );
	const auto date =
		DateTimeFormatter::parseDate( "28/09/2026", DateTimeFormat( "dd/MM/yyyy" ), locale );
	ASSERT_TRUE( date.has_value() );
	EXPECT_TRUE( *date == ( CalendarDate{ 2026, 9, 28 } ) );
	EXPECT_FALSE(
		DateTimeFormatter::parseDate( "31/02/2026", DateTimeFormat( "dd/MM/yyyy" ), locale ) );
	EXPECT_FALSE(
		DateTimeFormatter::parseDate( "29/02/2025", DateTimeFormat( "dd/MM/yyyy" ), locale ) );

	const auto time =
		DateTimeFormatter::parseTime( "20:14:32", DateTimeFormat( "HH:mm:ss" ), locale );
	ASSERT_TRUE( time.has_value() );
	EXPECT_TRUE( *time == ( TimeOfDay{ 20, 14, 32, 0 } ) );
	EXPECT_FALSE(
		DateTimeFormatter::parseTime( "24:00:00", DateTimeFormat( "HH:mm:ss" ), locale ) );
	EXPECT_FALSE(
		DateTimeFormatter::parseTime( "12:60:00", DateTimeFormat( "HH:mm:ss" ), locale ) );
}

UTEST( DateTime, TolerantDateParsingAcceptsISOAndAlternateSeparators ) {
	const DateTimeLocale locale = DateTimeLocale::fromLocaleName( "es_AR" );
	const DateTimeFormat display( "dd/MM/yyyy" );
	for ( const char* input : { "28/09/2026", "28-09-2026", "28.09.2026", "2026-09-28" } ) {
		const auto value = DateTimeFormatter::parseDateTolerant( input, display, locale );
		ASSERT_TRUE( value.has_value() );
		EXPECT_TRUE( *value == ( CalendarDate{ 2026, 9, 28 } ) );
	}
	EXPECT_FALSE( DateTimeFormatter::parseDateTolerant( "31-02-2026", display, locale ) );
}

UTEST( DateTime, TolerantDateParsingRespectsDisplayOrder ) {
	const DateTimeLocale locale = DateTimeLocale::fromLocaleName( "en_US" );
	const DateTimeFormat display( "MM/dd/yyyy" );
	const auto value = DateTimeFormatter::parseDateTolerant( "09-28-2026", display, locale );
	ASSERT_TRUE( value.has_value() );
	EXPECT_TRUE( *value == ( CalendarDate{ 2026, 9, 28 } ) );
	const auto iso = DateTimeFormatter::parseDateTolerant( "2026/09/28", display, locale );
	ASSERT_TRUE( iso.has_value() );
	EXPECT_TRUE( *iso == ( CalendarDate{ 2026, 9, 28 } ) );
}

UTEST( DateTime, TolerantTimeAndDateTimeParsing ) {
	const DateTimeLocale locale = DateTimeLocale::fromLocaleName( "es_AR" );
	const auto time =
		DateTimeFormatter::parseTimeTolerant( "20:14:32.125", DateTimeFormat( "HH:mm" ), locale );
	ASSERT_TRUE( time.has_value() );
	EXPECT_TRUE( *time == ( TimeOfDay{ 20, 14, 32, 125 } ) );

	const auto dateTime = DateTimeFormatter::parseDateTimeTolerant(
		"2026-09-28T20:14:32", DateTimeFormat( "dd/MM/yyyy HH:mm" ), locale );
	ASSERT_TRUE( dateTime.has_value() );
	EXPECT_TRUE( *dateTime ==
				 ( LocalDateTime{ CalendarDate{ 2026, 9, 28 }, TimeOfDay{ 20, 14, 32, 0 } } ) );
}

UTEST( DateTime, ISOFormattingIsLocaleIndependent ) {
	EXPECT_TRUE( DateTimeFormatter::toISODate( CalendarDate{ 2026, 9, 28 } ) == "2026-09-28" );
	EXPECT_TRUE( DateTimeFormatter::toISOTime( TimeOfDay{ 20, 14, 0, 125 } ) == "20:14:00" );
	EXPECT_TRUE( DateTimeFormatter::toISOTime( TimeOfDay{ 20, 14, 0, 125 }, true ) ==
				 "20:14:00.125" );
	EXPECT_TRUE( DateTimeFormatter::toISOLocalDateTime(
					 LocalDateTime{ CalendarDate{ 2026, 9, 28 }, TimeOfDay{ 20, 14, 0, 0 } } ) ==
				 "2026-09-28T20:14:00" );
}

UTEST( DateTime, SerializedYearsRoundTripAcrossTheRepresentableRange ) {
	const auto locale = DateTimeLocale::fromLocaleName( "es_AR" );
	const DateTimeFormat display( "dd/MM/yyyy" );
	for ( Int32 year : { std::numeric_limits<Int32>::min(), -1, 0, 9999, 12026,
						 std::numeric_limits<Int32>::max() } ) {
		const CalendarDate date{ year, 9, 28 };
		EXPECT_TRUE( DateTimeFormatter::parseDateTolerant( DateTimeFormatter::toISODate( date ),
														   display, locale ) == date );
		EXPECT_TRUE(
			DateTimeFormatter::parseDate( DateTimeFormatter::formatDate( date, display, locale ),
										  display, locale ) == date );
	}
	const DateTimeFormat iso( "yyyy-MM-dd" );
	EXPECT_FALSE( DateTimeFormatter::parseDate( "2147483648-09-28", iso, locale ) );
	EXPECT_FALSE( DateTimeFormatter::parseDate( "-2147483649-09-28", iso, locale ) );
	EXPECT_TRUE( DateTimeFormatter::parseDate( "20260928", DateTimeFormat( "yyyyMMdd" ), locale ) ==
				 ( CalendarDate{ 2026, 9, 28 } ) );
}

UTEST( DateTime, TolerantParsingRejectsOverflowAndTrailingGarbage ) {
	const DateTimeLocale locale = DateTimeLocale::fromLocaleName( "es_AR" );
	const DateTimeFormat dateFormat( "dd/MM/yyyy" );
	for ( const char* input : { "257/09/2026", "28/265/2026", "999999999999999999/1/2026",
								"2026-09-28junk", "2026-09-28T00:00:00Z", "garbage 28/9/2026" } ) {
		EXPECT_FALSE( DateTimeFormatter::parseDateTolerant( input, dateFormat, locale ) );
	}
	const DateTimeFormat timeFormat( "HH:mm" );
	for ( const char* input : { "256:00", "00:256", "12:00:256", "12:00 garbage", "12:00Z",
								"999999999999999999:00", "12:00:00.1234" } ) {
		EXPECT_FALSE( DateTimeFormatter::parseTimeTolerant( input, timeFormat, locale ) );
	}
}

UTEST( DateTime, GregorianArithmeticRoundTripsAcrossFourHundredYears ) {
	CalendarDate date{ 1800, 1, 1 };
	for ( int day = 0; day < 146097; ++day ) {
		const auto next = date.addDays( 1 );
		ASSERT_TRUE( next.isValid() );
		ASSERT_TRUE( next.addDays( -1 ) == date );
		date = next;
	}
	EXPECT_TRUE( date == ( CalendarDate{ 2200, 1, 1 } ) );
}

UTEST( DateTime, HugeArithmeticStepsDoNotOverflow ) {
	const auto maximum = std::numeric_limits<Int64>::max();
	const auto minimum = std::numeric_limits<Int64>::min();
	const CalendarDate date{ 2026, 9, 28 };
	for ( auto step : { minimum, maximum } ) {
		EXPECT_TRUE( date.addDays( step ) == date );
		EXPECT_TRUE( date.addMonths( step ) == date );
		EXPECT_TRUE( date.addYears( step ) == date );
		const TimeOfDay time{ 23, 59, 59, 999 };
		EXPECT_TRUE( time.addHours( step ).isValid() );
		EXPECT_TRUE( time.addMinutes( step ).isValid() );
		EXPECT_TRUE( time.addSeconds( step ).isValid() );
		const LocalDateTime dateTime{ date, time };
		EXPECT_TRUE( dateTime.addHours( step ) == dateTime );
		EXPECT_TRUE( dateTime.addMinutes( step ) == dateTime );
		EXPECT_TRUE( dateTime.addSeconds( step ) == dateTime );
	}
}

UTEST( DateTime, FractionalSecondPatternRoundTrip ) {
	const DateTimeLocale locale;
	for ( const char* pattern : { "HH:mm:ss.S", "HH:mm:ss.SS", "HH:mm:ss.SSS" } ) {
		const DateTimeFormat format( pattern );
		const TimeOfDay value{ 1, 2, 3, 400 };
		const auto parsed = DateTimeFormatter::parseTime(
			DateTimeFormatter::formatTime( value, format, locale ), format, locale );
		ASSERT_TRUE( parsed );
		EXPECT_TRUE( *parsed == value );
	}
}

UTEST( DateTime, IsoPasteTakesPrecedenceOverAmbiguousCustomFormat ) {
	DateTimeFormat custom( "yyyy-dd-MM" );
	auto date = DateTimeFormatter::parseDateTolerant( "2026-09-10", custom );
	ASSERT_TRUE( date );
	EXPECT_TRUE( *date == ( CalendarDate{ 2026, 9, 10 } ) );
}

UTEST( DateTime, IsoDateTimePasteTakesPrecedenceOverAmbiguousCustomFormat ) {
	DateTimeFormat custom( "yyyy-dd-MM'T'HH:mm:ss" );
	auto dateTime = DateTimeFormatter::parseDateTimeTolerant( "2026-09-10T20:14:32.12", custom );
	ASSERT_TRUE( dateTime );
	EXPECT_TRUE( dateTime->date == ( CalendarDate{ 2026, 9, 10 } ) );
	EXPECT_TRUE( dateTime->time == ( TimeOfDay{ 20, 14, 32, 120 } ) );
}

UTEST( DateTime, EpochDaysAreGregorianAndTimezoneIndependent ) {
	EXPECT_EQ( ( CalendarDate{ 1970, 1, 1 } ).daysSinceEpoch(), 0 );
	EXPECT_EQ( ( CalendarDate{ 1969, 12, 31 } ).daysSinceEpoch(), -1 );
	EXPECT_EQ( ( CalendarDate{ 2000, 3, 1 } ).daysSinceEpoch() -
				   ( CalendarDate{ 2000, 2, 28 } ).daysSinceEpoch(),
			   2 );
}

UTEST( DateTime, CalendarDateUnixTimestampsUseMidnightUTC ) {
	EXPECT_TRUE( ( CalendarDate{ 1970, 1, 1 } ).toUnixTimestamp() == 0 );
	EXPECT_TRUE( ( CalendarDate{ 1969, 12, 31 } ).toUnixTimestamp() == -86400 );
	EXPECT_TRUE( ( CalendarDate{ 2000, 2, 29 } ).toUnixTimestamp() == 951782400 );
	EXPECT_TRUE( CalendarDate::fromUnixTimestamp( 0 ) == ( CalendarDate{ 1970, 1, 1 } ) );
	EXPECT_TRUE( CalendarDate::fromUnixTimestamp( 86399 ) == ( CalendarDate{ 1970, 1, 1 } ) );
	EXPECT_TRUE( CalendarDate::fromUnixTimestamp( -1 ) == ( CalendarDate{ 1969, 12, 31 } ) );
	EXPECT_TRUE( CalendarDate::fromUnixTimestamp( -86401 ) == ( CalendarDate{ 1969, 12, 30 } ) );
	EXPECT_TRUE( CalendarDate::fromUnixTimestamp( 951782400 ) == ( CalendarDate{ 2000, 2, 29 } ) );
	EXPECT_FALSE( ( CalendarDate{ 2025, 2, 29 } ).toUnixTimestamp() );
}

UTEST( DateTime, CalendarDateUnixTimestampYearBoundaries ) {
	const CalendarDate minimum{ std::numeric_limits<Int32>::min(), 1, 1 };
	const CalendarDate maximum{ std::numeric_limits<Int32>::max(), 12, 31 };
	for ( auto date : { minimum, CalendarDate{ -1, 12, 31 }, CalendarDate{ 0, 1, 1 }, maximum } ) {
		const auto timestamp = date.toUnixTimestamp();
		ASSERT_TRUE( timestamp );
		EXPECT_TRUE( CalendarDate::fromUnixTimestamp( *timestamp ) == date );
	}
	EXPECT_FALSE( CalendarDate::fromUnixTimestamp( *minimum.toUnixTimestamp() - 1 ) );
	EXPECT_TRUE( CalendarDate::fromUnixTimestamp( *maximum.toUnixTimestamp() + 86399 ) == maximum );
	EXPECT_FALSE( CalendarDate::fromUnixTimestamp( *maximum.toUnixTimestamp() + 86400 ) );
	EXPECT_FALSE( CalendarDate::fromUnixTimestamp( std::numeric_limits<Int64>::min() ) );
	EXPECT_FALSE( CalendarDate::fromUnixTimestamp( std::numeric_limits<Int64>::max() ) );
}

UTEST( DateTime, LocalDateTimeUnixTimestampsAndNegativeFractions ) {
	const LocalDateTime epoch{ { 1970, 1, 1 }, {} };
	EXPECT_TRUE( epoch.toUnixTimestamp() == 0 );
	EXPECT_TRUE( LocalDateTime::fromUnixTimestamp( 0 ) == epoch );
	const LocalDateTime before{ { 1969, 12, 31 }, { 23, 59, 59, 125 } };
	EXPECT_TRUE( before.toUnixTimestamp( 0 ) == -1 );
	EXPECT_TRUE( before.toUnixTimestampMilliseconds() == -875 );
	EXPECT_TRUE( LocalDateTime::fromUnixTimestampMilliseconds( -875 ) == before );
	EXPECT_TRUE( LocalDateTime::fromUnixTimestamp( -1, 0 ) ==
				 ( LocalDateTime{ { 1969, 12, 31 }, { 23, 59, 59, 0 } } ) );
	EXPECT_TRUE( LocalDateTime::fromUnixTimestamp( 951782400, 0 ) ==
				 ( LocalDateTime{ { 2000, 2, 29 }, {} } ) );
	EXPECT_TRUE( LocalDateTime::fromUnixTimestamp( 2147483648LL, 0 ) ==
				 ( LocalDateTime{ { 2038, 1, 19 }, { 3, 14, 8, 0 } } ) );
}

UTEST( DateTime, UnixTimestampOffsetsCarryAcrossDateBoundaries ) {
	const LocalDateTime west{ { 1969, 12, 31 }, { 21, 0, 0, 0 } };
	const LocalDateTime east{ { 1970, 1, 1 }, { 5, 45, 0, 0 } };
	EXPECT_TRUE( LocalDateTime::fromUnixTimestamp( 0, -10800 ) == west );
	EXPECT_TRUE( west.toUnixTimestamp( -10800 ) == 0 );
	EXPECT_TRUE( west.toUnixTimestamp( 0 ) == -10800 );
	EXPECT_TRUE( LocalDateTime::fromUnixTimestamp( 0, 20700 ) == east );
	EXPECT_TRUE( east.toUnixTimestamp( 20700 ) == 0 );
	EXPECT_TRUE( LocalDateTime::fromUnixTimestamp( -1, 30 ) ==
				 ( LocalDateTime{ { 1970, 1, 1 }, { 0, 0, 29, 0 } } ) );
	EXPECT_TRUE( LocalDateTime::fromUnixTimestamp( 0, -30 ) ==
				 ( LocalDateTime{ { 1969, 12, 31 }, { 23, 59, 30, 0 } } ) );
}

UTEST( DateTime, UnixMillisecondsRoundTripIncludingInt64EndpointsAndOffsets ) {
	const Int64 minimum = std::numeric_limits<Int64>::min();
	const Int64 maximum = std::numeric_limits<Int64>::max();
	const Int64 timestamps[] = { minimum,		 minimum + 1, -86400001, -86400000, -1001, -1000,
								 -999,			 -1,		  0,		 1,			999,   1000,
								 951782400125LL, maximum - 1, maximum };
	const Int32 offsets[] = { 0, -10800, 20700, std::numeric_limits<Int32>::min(),
							  std::numeric_limits<Int32>::max() };
	for ( Int64 timestamp : timestamps ) {
		for ( Int32 offset : offsets ) {
			const auto value = LocalDateTime::fromUnixTimestampMilliseconds( timestamp, offset );
			ASSERT_TRUE( value );
			EXPECT_TRUE( value->isValid() );
			EXPECT_TRUE( value->toUnixTimestampMilliseconds( offset ) == timestamp );
		}
	}
	const auto first = LocalDateTime::fromUnixTimestampMilliseconds( minimum, 0 );
	const auto last = LocalDateTime::fromUnixTimestampMilliseconds( maximum, 0 );
	ASSERT_TRUE( first );
	ASSERT_TRUE( last );
	EXPECT_FALSE( first->addSeconds( -1 ).toUnixTimestampMilliseconds( 0 ) );
	EXPECT_FALSE( last->addSeconds( 1 ).toUnixTimestampMilliseconds( 0 ) );
	const LocalDateTime below{ first->date,
							   { first->time.hour, first->time.minute, first->time.second, 191 } };
	const LocalDateTime above{ last->date,
							   { last->time.hour, last->time.minute, last->time.second, 808 } };
	EXPECT_FALSE( below.toUnixTimestampMilliseconds( 0 ) );
	EXPECT_FALSE( above.toUnixTimestampMilliseconds( 0 ) );
}

UTEST( DateTime, UnixTimestampConversionsRejectInvalidValuesAndHandleYearLimits ) {
	for ( const LocalDateTime value :
		  { LocalDateTime{ { 2025, 2, 29 }, {} }, LocalDateTime{ { 1970, 1, 1 }, { 24, 0, 0, 0 } },
			LocalDateTime{ { 1970, 1, 1 }, { 0, 0, 0, 1000 } } } ) {
		EXPECT_FALSE( value.toUnixTimestamp( 0 ) );
		EXPECT_FALSE( value.toUnixTimestampMilliseconds( 0 ) );
	}
	for ( Int64 timestamp :
		  { std::numeric_limits<Int64>::min(), std::numeric_limits<Int64>::max() } ) {
		for ( Int32 offset :
			  { 0, std::numeric_limits<Int32>::min(), std::numeric_limits<Int32>::max() } )
			EXPECT_FALSE( LocalDateTime::fromUnixTimestamp( timestamp, offset ) );
	}
	for ( const LocalDateTime value :
		  { LocalDateTime{ { std::numeric_limits<Int32>::min(), 1, 1 }, {} },
			LocalDateTime{ { std::numeric_limits<Int32>::max(), 12, 31 }, { 23, 59, 59, 0 } } } ) {
		for ( Int32 offset :
			  { 0, std::numeric_limits<Int32>::min(), std::numeric_limits<Int32>::max() } ) {
			const auto timestamp = value.toUnixTimestamp( offset );
			ASSERT_TRUE( timestamp );
			EXPECT_TRUE( LocalDateTime::fromUnixTimestamp( *timestamp, offset ) == value );
			EXPECT_FALSE( value.toUnixTimestampMilliseconds( offset ) );
		}
	}
}

#if EE_PLATFORM == EE_PLATFORM_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <eepp/core/string.hpp>
#include <windows.h>

UTEST( DateTime, SystemLocaleMatchesWindowsRegionalSettings ) {
	const auto locale = DateTimeLocale::system();
	DWORD order = 0, hourCycle = 0, firstDay = 0;
	ASSERT_TRUE( GetLocaleInfoEx( LOCALE_NAME_USER_DEFAULT, LOCALE_IDATE | LOCALE_RETURN_NUMBER,
								  reinterpret_cast<wchar_t*>( &order ),
								  sizeof( order ) / sizeof( wchar_t ) ) > 0 );
	ASSERT_TRUE( GetLocaleInfoEx( LOCALE_NAME_USER_DEFAULT, LOCALE_ITIME | LOCALE_RETURN_NUMBER,
								  reinterpret_cast<wchar_t*>( &hourCycle ),
								  sizeof( hourCycle ) / sizeof( wchar_t ) ) > 0 );
	ASSERT_TRUE( GetLocaleInfoEx( LOCALE_NAME_USER_DEFAULT,
								  LOCALE_IFIRSTDAYOFWEEK | LOCALE_RETURN_NUMBER,
								  reinterpret_cast<wchar_t*>( &firstDay ),
								  sizeof( firstDay ) / sizeof( wchar_t ) ) > 0 );
	EXPECT_TRUE( locale.dateOrder == ( order == 0	? DateOrder::MDY
									   : order == 1 ? DateOrder::DMY
													: DateOrder::YMD ) );
	EXPECT_TRUE( locale.hourCycle == ( hourCycle == 0 ? HourCycle::H12 : HourCycle::H24 ) );
	EXPECT_EQ( locale.firstDayOfWeek, static_cast<Uint8>( ( firstDay + 1 ) % 7 ) );
	wchar_t month[128]{}, day[128]{};
	ASSERT_TRUE( GetLocaleInfoEx( LOCALE_NAME_USER_DEFAULT, LOCALE_SMONTHNAME1, month, 128 ) > 0 );
	ASSERT_TRUE( GetLocaleInfoEx( LOCALE_NAME_USER_DEFAULT, LOCALE_SDAYNAME7, day, 128 ) > 0 );
	EXPECT_TRUE( locale.monthNames[0] == String::fromWide( month ).toUtf8() );
	EXPECT_TRUE( locale.weekdayNames[0] == String::fromWide( day ).toUtf8() );
}
#endif
