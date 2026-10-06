#ifndef EE_SYSTEM_DATETIMEFORMAT_HPP
#define EE_SYSTEM_DATETIMEFORMAT_HPP

#include <array>
#include <eepp/config.hpp>
#include <eepp/core/small_vector.hpp>
#include <eepp/system/datetime.hpp>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace EE { namespace System {

enum class DateTimeFormatTokenType : Uint8 {
	Literal,
	Day,
	Month,
	Year,
	Hour24,
	Hour12,
	Minute,
	Second,
	Millisecond,
	AmPm
};

struct EE_API DateTimeFormatToken {
	std::string literal;
	DateTimeFormatTokenType type{ DateTimeFormatTokenType::Literal };
	Uint8 width{ 0 };
};

struct EE_API DateTimeFormatTokenRange {
	DateTimeFormatTokenType type{ DateTimeFormatTokenType::Literal };
	size_t tokenIndex{ 0 };
	size_t start{ 0 };
	size_t length{ 0 };
};

struct EE_API FormattedDateTime {
	std::string text;
	SmallVector<DateTimeFormatTokenRange, 16> ranges;
};

enum class DateOrder : Uint8 { DMY, MDY, YMD };

enum class HourCycle : Uint8 { Locale, H12, H24 };

/** Resolved locale information used by the date/time controls.
 *
 * The standard library is used where it exposes the information. Fields that are not available
 * portably use one centralized locale-name fallback instead of leaking locale assumptions into UI
 * code.
 */
struct EE_API DateTimeLocale {
	DateOrder dateOrder{ DateOrder::DMY };
	char dateSeparator{ '/' };
	HourCycle hourCycle{ HourCycle::H24 };
	Uint8 firstDayOfWeek{ 1 }; // 0 = Sunday, 1 = Monday
	std::string am{ "AM" };
	std::string pm{ "PM" };
	std::array<std::string, 7> shortWeekdayNames{ "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
	std::array<std::string, 7> weekdayNames{ "Sunday",	 "Monday", "Tuesday", "Wednesday",
											 "Thursday", "Friday", "Saturday" };
	std::array<std::string, 12> shortMonthNames{ "Jan", "Feb", "Mar", "Apr", "May", "Jun",
												 "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
	std::array<std::string, 12> monthNames{ "January",	 "February", "March",	 "April",
											"May",		 "June",	 "July",	 "August",
											"September", "October",	 "November", "December" };

	std::string datePattern() const;

	std::string timePattern( bool showSeconds = false, bool showMilliseconds = false,
							 HourCycle requestedCycle = HourCycle::Locale ) const;

	std::string dateTimePattern( bool showSeconds = false, bool showMilliseconds = false,
								 HourCycle requestedCycle = HourCycle::Locale ) const;

	bool operator==( const DateTimeLocale& ) const = default;

	static DateTimeLocale system();

	static DateTimeLocale fromLocaleName( std::string_view localeName );
};

class EE_API DateTimeFormat {
  public:
	DateTimeFormat() = default;

	explicit DateTimeFormat( std::string pattern );

	static DateTimeFormat compile( std::string pattern );

	bool isValid() const { return mValid; }

	const std::string& getPattern() const { return mPattern; }

	const std::vector<DateTimeFormatToken>& getTokens() const { return mTokens; }

  protected:
	std::string mPattern;
	std::vector<DateTimeFormatToken> mTokens;
	bool mValid{ false };
};

class EE_API DateTimeFormatter {
  public:
	static FormattedDateTime
	formatDateDetailed( const CalendarDate& value, const DateTimeFormat& format,
						const DateTimeLocale& locale = DateTimeLocale::system() );

	static FormattedDateTime
	formatTimeDetailed( const TimeOfDay& value, const DateTimeFormat& format,
						const DateTimeLocale& locale = DateTimeLocale::system() );

	static FormattedDateTime
	formatDateTimeDetailed( const LocalDateTime& value, const DateTimeFormat& format,
							const DateTimeLocale& locale = DateTimeLocale::system() );

	static std::string formatDate( const CalendarDate& value, const DateTimeFormat& format,
								   const DateTimeLocale& locale = DateTimeLocale::system() );

	static std::string formatTime( const TimeOfDay& value, const DateTimeFormat& format,
								   const DateTimeLocale& locale = DateTimeLocale::system() );

	static std::string formatDateTime( const LocalDateTime& value, const DateTimeFormat& format,
									   const DateTimeLocale& locale = DateTimeLocale::system() );

	static std::optional<CalendarDate>
	parseDate( std::string_view text, const DateTimeFormat& format,
			   const DateTimeLocale& locale = DateTimeLocale::system() );

	static std::optional<TimeOfDay>
	parseTime( std::string_view text, const DateTimeFormat& format,
			   const DateTimeLocale& locale = DateTimeLocale::system() );

	static std::optional<LocalDateTime>
	parseDateTime( std::string_view text, const DateTimeFormat& format,
				   const DateTimeLocale& locale = DateTimeLocale::system() );

	/** Parsing intended for paste/replacement. ISO is always accepted, then the display format and
	 * separator-tolerant numeric forms are attempted. */
	static std::optional<CalendarDate>
	parseDateTolerant( std::string_view text, const DateTimeFormat& displayFormat,
					   const DateTimeLocale& locale = DateTimeLocale::system() );

	static std::optional<TimeOfDay>
	parseTimeTolerant( std::string_view text, const DateTimeFormat& displayFormat,
					   const DateTimeLocale& locale = DateTimeLocale::system() );

	static std::optional<LocalDateTime>
	parseDateTimeTolerant( std::string_view text, const DateTimeFormat& displayFormat,
						   const DateTimeLocale& locale = DateTimeLocale::system() );

	static std::string toISODate( const CalendarDate& value );

	static std::string toISOTime( const TimeOfDay& value, bool includeMilliseconds = false );

	static std::string toISOLocalDateTime( const LocalDateTime& value,
										   bool includeMilliseconds = false );
};

}} // namespace EE::System

#endif
