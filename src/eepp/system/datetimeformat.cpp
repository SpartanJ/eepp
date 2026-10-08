#include <eepp/system/datetimeformat.hpp>

#include <algorithm>
#include <cctype>
#include <charconv>
#include <eepp/core/small_vector.hpp>
#include <eepp/core/string.hpp>
#include <iomanip>
#include <limits>
#include <locale>
#include <sstream>

#if EE_PLATFORM == EE_PLATFORM_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace EE { namespace System {

namespace {

bool isTokenChar( char c ) {
	switch ( c ) {
		case 'd':
		case 'M':
		case 'y':
		case 'H':
		case 'h':
		case 'm':
		case 's':
		case 'S':
		case 'a':
			return true;
		default:
			return false;
	}
}

DateTimeFormatTokenType tokenType( char c ) {
	switch ( c ) {
		case 'd':
			return DateTimeFormatTokenType::Day;
		case 'M':
			return DateTimeFormatTokenType::Month;
		case 'y':
			return DateTimeFormatTokenType::Year;
		case 'H':
			return DateTimeFormatTokenType::Hour24;
		case 'h':
			return DateTimeFormatTokenType::Hour12;
		case 'm':
			return DateTimeFormatTokenType::Minute;
		case 's':
			return DateTimeFormatTokenType::Second;
		case 'S':
			return DateTimeFormatTokenType::Millisecond;
		case 'a':
			return DateTimeFormatTokenType::AmPm;
		default:
			return DateTimeFormatTokenType::Literal;
	}
}

bool validTokenWidth( DateTimeFormatTokenType type, size_t width ) {
	switch ( type ) {
		case DateTimeFormatTokenType::Day:
		case DateTimeFormatTokenType::Month:
		case DateTimeFormatTokenType::Hour24:
		case DateTimeFormatTokenType::Hour12:
		case DateTimeFormatTokenType::Minute:
		case DateTimeFormatTokenType::Second:
			return width >= 1 && width <= 2;
		case DateTimeFormatTokenType::Year:
			return width == 2 || width == 4;
		case DateTimeFormatTokenType::Millisecond:
			return width >= 1 && width <= 3;
		case DateTimeFormatTokenType::AmPm:
			return width == 1;
		case DateTimeFormatTokenType::Literal:
			return width == 0;
	}
	return false;
}

void appendLiteral( std::vector<DateTimeFormatToken>& tokens, std::string_view literal ) {
	if ( literal.empty() )
		return;
	if ( !tokens.empty() && tokens.back().type == DateTimeFormatTokenType::Literal ) {
		tokens.back().literal.append( literal );
	} else {
		tokens.push_back( { std::string( literal ), DateTimeFormatTokenType::Literal, 0 } );
	}
}

std::string lowerASCII( std::string_view value ) {
	std::string result;
	result.reserve( value.size() );
	for ( char c : value )
		result.push_back( static_cast<char>( std::tolower( static_cast<unsigned char>( c ) ) ) );
	return result;
}

bool equalsASCIIInsensitive( std::string_view a, std::string_view b ) {
	return lowerASCII( a ) == lowerASCII( b );
}

bool containsASCIIInsensitive( std::string_view haystack, std::string_view needle ) {
	if ( needle.empty() )
		return false;
	return lowerASCII( haystack ).find( lowerASCII( needle ) ) != std::string::npos;
}

std::string formatTm( const std::locale& locale, std::tm value, const char* pattern ) {
	std::ostringstream stream;
	stream.imbue( locale );
	stream << std::put_time( &value, pattern );
	return stream.str();
}

char detectDateSeparator( const std::string& formattedDate, char fallback ) {
	for ( char c : formattedDate ) {
		if ( c == '/' || c == '-' || c == '.' )
			return c;
	}
	return fallback;
}

DateOrder fallbackDateOrder( std::string_view localeName ) {
	const std::string name = lowerASCII( localeName );
	if ( name == "c" || name == "posix" || name.find( "en_us" ) != std::string::npos ||
		 name.find( "en-us" ) != std::string::npos || name.find( "en_ph" ) != std::string::npos ||
		 name.find( "en-ph" ) != std::string::npos )
		return DateOrder::MDY;
	if ( name.rfind( "ja", 0 ) == 0 || name.rfind( "zh", 0 ) == 0 || name.rfind( "ko", 0 ) == 0 )
		return DateOrder::YMD;
	return DateOrder::DMY;
}

bool fallbackSundayFirst( std::string_view localeName ) {
	const std::string name = lowerASCII( localeName );
	return name == "c" || name == "posix" || name.find( "_us" ) != std::string::npos ||
		   name.find( "-us" ) != std::string::npos || name.find( "_ca" ) != std::string::npos ||
		   name.find( "-ca" ) != std::string::npos || name.find( "_ph" ) != std::string::npos ||
		   name.find( "-ph" ) != std::string::npos;
}

bool fallback12Hour( std::string_view localeName ) {
	const std::string name = lowerASCII( localeName );
	return name.find( "_us" ) != std::string::npos || name.find( "-us" ) != std::string::npos ||
		   name.find( "_ph" ) != std::string::npos || name.find( "-ph" ) != std::string::npos;
}

std::string paddedNumber( int value, size_t width ) {
	char buffer[16];
	const auto result = std::to_chars( buffer, buffer + sizeof( buffer ), value );
	const size_t length = static_cast<size_t>( result.ptr - buffer );
	std::string text;
	text.reserve( std::max( width, length ) );
	if ( value < 0 ) {
		text.push_back( '-' );
		if ( width > length )
			text.append( width - length, '0' );
		text.append( buffer + 1, length - 1 );
	} else {
		if ( width > length )
			text.append( width - length, '0' );
		text.append( buffer, length );
	}
	return text;
}

std::string twoDigits( int value ) {
	return paddedNumber( value, 2 );
}

std::string fourDigits( int value ) {
	return paddedNumber( value, 4 );
}

std::string numberForToken( DateTimeFormatTokenType type, Uint8 width, const CalendarDate* date,
							const TimeOfDay* time, const DateTimeLocale& locale, bool& supported ) {
	supported = true;
	int value = 0;
	switch ( type ) {
		case DateTimeFormatTokenType::Day:
			if ( !date ) {
				supported = false;
				return {};
			}
			value = date->day;
			break;
		case DateTimeFormatTokenType::Month:
			if ( !date ) {
				supported = false;
				return {};
			}
			value = date->month;
			break;
		case DateTimeFormatTokenType::Year:
			if ( !date ) {
				supported = false;
				return {};
			}
			if ( width == 2 )
				return twoDigits( ( date->year % 100 + 100 ) % 100 );
			return fourDigits( date->year );
		case DateTimeFormatTokenType::Hour24:
			if ( !time ) {
				supported = false;
				return {};
			}
			value = time->hour;
			break;
		case DateTimeFormatTokenType::Hour12:
			if ( !time ) {
				supported = false;
				return {};
			}
			value = time->hour % 12;
			if ( value == 0 )
				value = 12;
			break;
		case DateTimeFormatTokenType::Minute:
			if ( !time ) {
				supported = false;
				return {};
			}
			value = time->minute;
			break;
		case DateTimeFormatTokenType::Second:
			if ( !time ) {
				supported = false;
				return {};
			}
			value = time->second;
			break;
		case DateTimeFormatTokenType::Millisecond:
			if ( !time ) {
				supported = false;
				return {};
			}
			value = time->millisecond;
			if ( width == 1 )
				value /= 100;
			else if ( width == 2 )
				value /= 10;
			break;
		case DateTimeFormatTokenType::AmPm:
			if ( !time ) {
				supported = false;
				return {};
			}
			return time->hour < 12 ? locale.am : locale.pm;
		case DateTimeFormatTokenType::Literal:
			return {};
	}

	return paddedNumber( value, width );
}

FormattedDateTime formatImpl( const CalendarDate* date, const TimeOfDay* time,
							  const DateTimeFormat& format, const DateTimeLocale& locale ) {
	FormattedDateTime result;
	if ( !format.isValid() || ( date && !date->isValid() ) || ( time && !time->isValid() ) )
		return result;

	const auto& tokens = format.getTokens();
	result.text.reserve( format.getPattern().size() );
	result.ranges.reserve( tokens.size() );
	for ( size_t i = 0; i < tokens.size(); ++i ) {
		const auto& token = tokens[i];
		const size_t start = result.text.size();
		if ( token.type == DateTimeFormatTokenType::Literal ) {
			result.text += token.literal;
		} else {
			bool supported = false;
			result.text += numberForToken( token.type, token.width, date, time, locale, supported );
			if ( !supported )
				return {};
		}
		result.ranges.push_back( { token.type, i, start, result.text.size() - start } );
	}
	return result;
}

struct ParsedFields {
	std::optional<int> day;
	std::optional<int> month;
	std::optional<int> year;
	std::optional<int> hour24;
	std::optional<int> hour12;
	std::optional<int> minute;
	std::optional<int> second;
	std::optional<int> millisecond;
	std::optional<bool> pm;
};

bool parseUnsigned( std::string_view text, size_t& pos, size_t minDigits, size_t maxDigits,
					int& out ) {
	const size_t start = pos;
	Int64 value = 0;
	while ( pos < text.size() && pos - start < maxDigits &&
			std::isdigit( static_cast<unsigned char>( text[pos] ) ) ) {
		value = value * 10 + ( text[pos] - '0' );
		++pos;
	}
	if ( pos - start < minDigits )
		return false;
	out = static_cast<int>( value );
	return true;
}

bool parseLiteral( std::string_view text, size_t& pos, std::string_view literal ) {
	if ( pos + literal.size() > text.size() || text.substr( pos, literal.size() ) != literal )
		return false;
	pos += literal.size();
	return true;
}

bool parseYear( std::string_view text, size_t& pos, int& out ) {
	const size_t start = pos;
	const bool negative = pos < text.size() && text[pos] == '-';
	if ( negative )
		++pos;
	const size_t digits = pos;
	while ( pos < text.size() && std::isdigit( static_cast<unsigned char>( text[pos] ) ) )
		++pos;
	// Formatting pads to four characters, including a negative year's sign.
	if ( pos - digits < ( negative ? 3u : 4u ) )
		return false;
	const auto result = std::from_chars( text.data() + start, text.data() + pos, out );
	return result.ec == std::errc{} && result.ptr == text.data() + pos;
}

bool parseAmPm( std::string_view text, size_t& pos, const DateTimeLocale& locale, bool& pm ) {
	const std::array<std::pair<std::string_view, bool>, 4> candidates = {
		std::pair<std::string_view, bool>{ locale.am, false },
		{ locale.pm, true },
		{ "AM", false },
		{ "PM", true } };
	for ( const auto& candidate : candidates ) {
		if ( candidate.first.empty() || pos + candidate.first.size() > text.size() )
			continue;
		if ( equalsASCIIInsensitive( text.substr( pos, candidate.first.size() ),
									 candidate.first ) ) {
			pos += candidate.first.size();
			pm = candidate.second;
			return true;
		}
	}
	return false;
}

std::optional<ParsedFields> parseFields( std::string_view text, const DateTimeFormat& format,
										 const DateTimeLocale& locale ) {
	if ( !format.isValid() )
		return std::nullopt;

	ParsedFields fields;
	size_t pos = 0;
	const auto& tokens = format.getTokens();
	for ( size_t index = 0; index < tokens.size(); ++index ) {
		const auto& token = tokens[index];
		if ( token.type == DateTimeFormatTokenType::Literal ) {
			if ( !parseLiteral( text, pos, token.literal ) )
				return std::nullopt;
			continue;
		}

		if ( token.type == DateTimeFormatTokenType::AmPm ) {
			bool pm = false;
			if ( !parseAmPm( text, pos, locale, pm ) )
				return std::nullopt;
			fields.pm = pm;
			continue;
		}

		int number = 0;
		size_t minDigits = token.width;
		size_t maxDigits = token.width;
		if ( token.width == 1 && token.type != DateTimeFormatTokenType::Year )
			maxDigits = token.type == DateTimeFormatTokenType::Millisecond ? 1 : 2;
		// Delimited full years must accept every year emitted by the formatter. Adjacent
		// numeric tokens keep their fixed widths, so yyyyMMdd remains unambiguous.
		const bool fullYear = token.type == DateTimeFormatTokenType::Year && token.width == 4 &&
							  ( index + 1 == tokens.size() ||
								( tokens[index + 1].type == DateTimeFormatTokenType::Literal &&
								  !tokens[index + 1].literal.empty() &&
								  !std::isdigit( static_cast<unsigned char>(
									  tokens[index + 1].literal.front() ) ) ) );
		if ( !( fullYear ? parseYear( text, pos, number )
						 : parseUnsigned( text, pos, minDigits, maxDigits, number ) ) )
			return std::nullopt;

		switch ( token.type ) {
			case DateTimeFormatTokenType::Day:
				fields.day = number;
				break;
			case DateTimeFormatTokenType::Month:
				fields.month = number;
				break;
			case DateTimeFormatTokenType::Year:
				fields.year = token.width == 2 ? 2000 + number : number;
				break;
			case DateTimeFormatTokenType::Hour24:
				fields.hour24 = number;
				break;
			case DateTimeFormatTokenType::Hour12:
				fields.hour12 = number;
				break;
			case DateTimeFormatTokenType::Minute:
				fields.minute = number;
				break;
			case DateTimeFormatTokenType::Second:
				fields.second = number;
				break;
			case DateTimeFormatTokenType::Millisecond:
				fields.millisecond = token.width == 1	? number * 100
									 : token.width == 2 ? number * 10
														: number;
				break;
			default:
				break;
		}
	}

	if ( pos != text.size() )
		return std::nullopt;
	return fields;
}

std::optional<CalendarDate> dateFromFields( const ParsedFields& fields ) {
	if ( !fields.day || !fields.month || !fields.year )
		return std::nullopt;
	if ( *fields.month < 1 || *fields.month > 12 || *fields.day < 1 || *fields.day > 31 )
		return std::nullopt;
	CalendarDate value{ static_cast<Int32>( *fields.year ), static_cast<Uint8>( *fields.month ),
						static_cast<Uint8>( *fields.day ) };
	return value.isValid() ? std::optional<CalendarDate>( value ) : std::nullopt;
}

std::optional<TimeOfDay> timeFromFields( const ParsedFields& fields ) {
	if ( !fields.minute || ( !fields.hour24 && !fields.hour12 ) )
		return std::nullopt;
	if ( fields.hour24 && fields.hour12 )
		return std::nullopt;

	int hour = 0;
	if ( fields.hour24 ) {
		if ( fields.pm )
			return std::nullopt;
		hour = *fields.hour24;
	} else {
		if ( !fields.pm || *fields.hour12 < 1 || *fields.hour12 > 12 )
			return std::nullopt;
		hour = *fields.hour12 % 12 + ( *fields.pm ? 12 : 0 );
	}

	TimeOfDay value{ static_cast<Uint8>( hour ), static_cast<Uint8>( *fields.minute ),
					 fields.second ? static_cast<Uint8>( *fields.second ) : Uint8{ 0 },
					 fields.millisecond ? static_cast<Uint16>( *fields.millisecond )
										: Uint16{ 0 } };
	return value.isValid() ? std::optional<TimeOfDay>( value ) : std::nullopt;
}

SmallVector<std::pair<int, size_t>, 4> numericGroups( std::string_view text ) {
	SmallVector<std::pair<int, size_t>, 4> values;
	for ( size_t pos = 0; pos < text.size(); ) {
		if ( !std::isdigit( static_cast<unsigned char>( text[pos] ) ) ) {
			const char c = text[pos];
			if ( c != '/' && c != '-' && c != '.' && c != ':' &&
				 !std::isspace( static_cast<unsigned char>( c ) ) )
				return {};
			++pos;
			continue;
		}
		const size_t start = pos;
		int value = 0;
		while ( pos < text.size() && std::isdigit( static_cast<unsigned char>( text[pos] ) ) ) {
			if ( value > ( std::numeric_limits<int>::max() - ( text[pos] - '0' ) ) / 10 )
				return {};
			value = value * 10 + ( text[pos] - '0' );
			++pos;
		}
		if ( values.size() == 4 )
			return {};
		values.emplace_back( value, pos - start );
	}
	return values;
}

DateOrder dateOrderFromFormat( const DateTimeFormat& format, DateOrder fallback ) {
	SmallVector<DateTimeFormatTokenType, 3> dateTokens;
	for ( const auto& token : format.getTokens() ) {
		if ( token.type == DateTimeFormatTokenType::Day ||
			 token.type == DateTimeFormatTokenType::Month ||
			 token.type == DateTimeFormatTokenType::Year )
			dateTokens.push_back( token.type );
	}
	if ( dateTokens.size() < 3 )
		return fallback;
	if ( dateTokens[0] == DateTimeFormatTokenType::Day )
		return DateOrder::DMY;
	if ( dateTokens[0] == DateTimeFormatTokenType::Month )
		return DateOrder::MDY;
	if ( dateTokens[0] == DateTimeFormatTokenType::Year )
		return DateOrder::YMD;
	return fallback;
}

std::string_view trimView( std::string_view text ) {
	while ( !text.empty() && std::isspace( static_cast<unsigned char>( text.front() ) ) )
		text.remove_prefix( 1 );
	while ( !text.empty() && std::isspace( static_cast<unsigned char>( text.back() ) ) )
		text.remove_suffix( 1 );
	return text;
}

} // namespace

std::string DateTimeLocale::datePattern() const {
	const std::string separator( 1, dateSeparator );
	switch ( dateOrder ) {
		case DateOrder::DMY:
			return "dd" + separator + "MM" + separator + "yyyy";
		case DateOrder::MDY:
			return "MM" + separator + "dd" + separator + "yyyy";
		case DateOrder::YMD:
			return "yyyy" + separator + "MM" + separator + "dd";
	}
	return "dd/MM/yyyy";
}

std::string DateTimeLocale::timePattern( bool showSeconds, bool showMilliseconds,
										 HourCycle requestedCycle ) const {
	const HourCycle resolved = requestedCycle == HourCycle::Locale ? hourCycle : requestedCycle;
	std::string result = resolved == HourCycle::H12 ? "hh:mm" : "HH:mm";
	if ( showSeconds || showMilliseconds )
		result += ":ss";
	if ( showMilliseconds )
		result += ".SSS";
	if ( resolved == HourCycle::H12 )
		result += " a";
	return result;
}

std::string DateTimeLocale::dateTimePattern( bool showSeconds, bool showMilliseconds,
											 HourCycle requestedCycle ) const {
	return datePattern() + " " + timePattern( showSeconds, showMilliseconds, requestedCycle );
}

DateTimeLocale DateTimeLocale::fromLocaleName( std::string_view localeName ) {
	DateTimeLocale result;
	result.dateOrder = fallbackDateOrder( localeName );
	result.dateSeparator = result.dateOrder == DateOrder::YMD ? '-' : '/';
	result.hourCycle = fallback12Hour( localeName ) ? HourCycle::H12 : HourCycle::H24;
	result.firstDayOfWeek = fallbackSundayFirst( localeName ) ? 0 : 1;
	return result;
}

DateTimeLocale DateTimeLocale::system() {
#if EE_PLATFORM == EE_PLATFORM_WIN
	// MinGW does not provide the named std::locale facets on all installations. Read Windows
	// regional settings directly, including the user's overrides, instead of falling back to C.
	const auto text = []( LCTYPE type ) {
		wchar_t buffer[128]{};
		return GetLocaleInfoEx( LOCALE_NAME_USER_DEFAULT, type, buffer, 128 ) > 0
				   ? String::fromWide( buffer ).toUtf8()
				   : std::string{};
	};
	const auto number = []( LCTYPE type, DWORD fallback ) {
		DWORD value;
		return GetLocaleInfoEx( LOCALE_NAME_USER_DEFAULT, type | LOCALE_RETURN_NUMBER,
								reinterpret_cast<wchar_t*>( &value ),
								sizeof( value ) / sizeof( wchar_t ) ) > 0
				   ? value
				   : fallback;
	};
	DateTimeLocale result = fromLocaleName( text( LOCALE_SNAME ) );
	switch ( number( LOCALE_IDATE, 1 ) ) {
		case 0:
			result.dateOrder = DateOrder::MDY;
			break;
		case 1:
			result.dateOrder = DateOrder::DMY;
			break;
		case 2:
			result.dateOrder = DateOrder::YMD;
			break;
	}
	result.dateSeparator = detectDateSeparator( text( LOCALE_SDATE ), result.dateSeparator );
	result.hourCycle = number( LOCALE_ITIME, 1 ) == 0 ? HourCycle::H12 : HourCycle::H24;
	// Windows numbers weekdays from Monday; the public date/time API numbers from Sunday.
	result.firstDayOfWeek = static_cast<Uint8>( ( number( LOCALE_IFIRSTDAYOFWEEK, 0 ) + 1 ) % 7 );
	if ( auto am = text( LOCALE_S1159 ); !am.empty() )
		result.am = std::move( am );
	if ( auto pm = text( LOCALE_S2359 ); !pm.empty() )
		result.pm = std::move( pm );
	for ( int month = 0; month < 12; ++month ) {
		if ( auto name = text( LOCALE_SABBREVMONTHNAME1 + month ); !name.empty() )
			result.shortMonthNames[month] = std::move( name );
		if ( auto name = text( LOCALE_SMONTHNAME1 + month ); !name.empty() )
			result.monthNames[month] = std::move( name );
	}
	for ( int weekday = 0; weekday < 7; ++weekday ) {
		const int index = ( weekday + 6 ) % 7;
		if ( auto name = text( LOCALE_SABBREVDAYNAME1 + index ); !name.empty() )
			result.shortWeekdayNames[weekday] = std::move( name );
		if ( auto name = text( LOCALE_SDAYNAME1 + index ); !name.empty() )
			result.weekdayNames[weekday] = std::move( name );
	}
	return result;
#else
	try {
		const std::locale locale( "" );
		DateTimeLocale result = fromLocaleName( locale.name() );

		const auto order = std::use_facet<std::time_get<char>>( locale ).date_order();
		switch ( order ) {
			case std::time_base::dmy:
				result.dateOrder = DateOrder::DMY;
				break;
			case std::time_base::mdy:
				result.dateOrder = DateOrder::MDY;
				break;
			case std::time_base::ymd:
			case std::time_base::ydm:
				result.dateOrder = DateOrder::YMD;
				break;
			case std::time_base::no_order:
				break;
		}

		std::tm sample{};
		sample.tm_year = 103; // 2003
		sample.tm_mon = 10;	  // November
		sample.tm_mday = 22;
		sample.tm_hour = 13;
		sample.tm_min = 45;
		sample.tm_sec = 56;
		result.dateSeparator =
			detectDateSeparator( formatTm( locale, sample, "%x" ), result.dateSeparator );

		const std::string pm = formatTm( locale, sample, "%p" );
		if ( !pm.empty() )
			result.pm = pm;
		sample.tm_hour = 1;
		const std::string am = formatTm( locale, sample, "%p" );
		if ( !am.empty() )
			result.am = am;
		sample.tm_hour = 13;
		const std::string displayTime = formatTm( locale, sample, "%X" );
		if ( !result.pm.empty() && containsASCIIInsensitive( displayTime, result.pm ) )
			result.hourCycle = HourCycle::H12;

		for ( int month = 0; month < 12; ++month ) {
			sample.tm_mon = month;
			result.shortMonthNames[month] = formatTm( locale, sample, "%b" );
			result.monthNames[month] = formatTm( locale, sample, "%B" );
		}
		for ( int weekday = 0; weekday < 7; ++weekday ) {
			sample.tm_wday = weekday;
			result.shortWeekdayNames[weekday] = formatTm( locale, sample, "%a" );
			result.weekdayNames[weekday] = formatTm( locale, sample, "%A" );
		}
		return result;
	} catch ( ... ) {
		return fromLocaleName( "C" );
	}
#endif
}

DateTimeFormat::DateTimeFormat( std::string pattern ) :
	DateTimeFormat( compile( std::move( pattern ) ) ) {}

DateTimeFormat DateTimeFormat::compile( std::string pattern ) {
	DateTimeFormat result;
	result.mPattern = std::move( pattern );
	if ( result.mPattern.empty() )
		return result;

	for ( size_t pos = 0; pos < result.mPattern.size(); ) {
		const char c = result.mPattern[pos];
		if ( c == '\'' ) {
			std::string literal;
			++pos;
			bool closed = false;
			while ( pos < result.mPattern.size() ) {
				if ( result.mPattern[pos] == '\'' ) {
					if ( pos + 1 < result.mPattern.size() && result.mPattern[pos + 1] == '\'' ) {
						literal.push_back( '\'' );
						pos += 2;
						continue;
					}
					++pos;
					closed = true;
					break;
				}
				literal.push_back( result.mPattern[pos++] );
			}
			if ( !closed )
				return result;
			appendLiteral( result.mTokens, literal );
			continue;
		}

		if ( isTokenChar( c ) ) {
			size_t end = pos + 1;
			while ( end < result.mPattern.size() && result.mPattern[end] == c )
				++end;
			const auto type = tokenType( c );
			const size_t width = end - pos;
			if ( !validTokenWidth( type, width ) )
				return result;
			result.mTokens.push_back( { {}, type, static_cast<Uint8>( width ) } );
			pos = end;
			continue;
		}

		if ( std::isalpha( static_cast<unsigned char>( c ) ) )
			return result;
		appendLiteral( result.mTokens, std::string_view( &result.mPattern[pos], 1 ) );
		++pos;
	}

	result.mValid = !result.mTokens.empty();
	return result;
}

FormattedDateTime DateTimeFormatter::formatDateDetailed( const CalendarDate& value,
														 const DateTimeFormat& format,
														 const DateTimeLocale& locale ) {
	return formatImpl( &value, nullptr, format, locale );
}

FormattedDateTime DateTimeFormatter::formatTimeDetailed( const TimeOfDay& value,
														 const DateTimeFormat& format,
														 const DateTimeLocale& locale ) {
	return formatImpl( nullptr, &value, format, locale );
}

FormattedDateTime DateTimeFormatter::formatDateTimeDetailed( const LocalDateTime& value,
															 const DateTimeFormat& format,
															 const DateTimeLocale& locale ) {
	if ( !value.isValid() )
		return {};
	return formatImpl( &value.date, &value.time, format, locale );
}

std::string DateTimeFormatter::formatDate( const CalendarDate& value, const DateTimeFormat& format,
										   const DateTimeLocale& locale ) {
	return formatDateDetailed( value, format, locale ).text;
}

std::string DateTimeFormatter::formatTime( const TimeOfDay& value, const DateTimeFormat& format,
										   const DateTimeLocale& locale ) {
	return formatTimeDetailed( value, format, locale ).text;
}

std::string DateTimeFormatter::formatDateTime( const LocalDateTime& value,
											   const DateTimeFormat& format,
											   const DateTimeLocale& locale ) {
	return formatDateTimeDetailed( value, format, locale ).text;
}

std::optional<CalendarDate> DateTimeFormatter::parseDate( std::string_view text,
														  const DateTimeFormat& format,
														  const DateTimeLocale& locale ) {
	const auto fields = parseFields( trimView( text ), format, locale );
	return fields ? dateFromFields( *fields ) : std::nullopt;
}

std::optional<TimeOfDay> DateTimeFormatter::parseTime( std::string_view text,
													   const DateTimeFormat& format,
													   const DateTimeLocale& locale ) {
	const auto fields = parseFields( trimView( text ), format, locale );
	return fields ? timeFromFields( *fields ) : std::nullopt;
}

std::optional<LocalDateTime> DateTimeFormatter::parseDateTime( std::string_view text,
															   const DateTimeFormat& format,
															   const DateTimeLocale& locale ) {
	const auto fields = parseFields( trimView( text ), format, locale );
	if ( !fields )
		return std::nullopt;
	const auto date = dateFromFields( *fields );
	const auto time = timeFromFields( *fields );
	if ( !date || !time )
		return std::nullopt;
	return LocalDateTime{ *date, *time };
}

std::optional<CalendarDate>
DateTimeFormatter::parseDateTolerant( std::string_view text, const DateTimeFormat& displayFormat,
									  const DateTimeLocale& locale ) {
	text = trimView( text );
	if ( text.empty() )
		return std::nullopt;
	static const DateTimeFormat ISODate( "yyyy-MM-dd" );
	if ( auto iso = parseDate( text, ISODate, locale ) )
		return iso;
	if ( auto exact = parseDate( text, displayFormat, locale ) )
		return exact;

	const auto groups = numericGroups( text );
	if ( groups.size() != 3 )
		return std::nullopt;
	DateOrder order = dateOrderFromFormat( displayFormat, locale.dateOrder );
	if ( groups[0].second == 4 )
		order = DateOrder::YMD;
	int day = 0;
	int month = 0;
	int year = 0;
	switch ( order ) {
		case DateOrder::DMY:
			day = groups[0].first;
			month = groups[1].first;
			year = groups[2].first;
			break;
		case DateOrder::MDY:
			month = groups[0].first;
			day = groups[1].first;
			year = groups[2].first;
			break;
		case DateOrder::YMD:
			year = groups[0].first;
			month = groups[1].first;
			day = groups[2].first;
			break;
	}
	if ( year < 1 || year > 9999 || month < 1 || month > 12 || day < 1 || day > 31 )
		return std::nullopt;
	CalendarDate value{ static_cast<Int32>( year ), static_cast<Uint8>( month ),
						static_cast<Uint8>( day ) };
	return value.isValid() ? std::optional<CalendarDate>( value ) : std::nullopt;
}

std::optional<TimeOfDay> DateTimeFormatter::parseTimeTolerant( std::string_view text,
															   const DateTimeFormat& displayFormat,
															   const DateTimeLocale& locale ) {
	text = trimView( text );
	if ( text.empty() )
		return std::nullopt;
	if ( auto exact = parseTime( text, displayFormat, locale ) )
		return exact;

	std::string normalized( text );
	bool hasPm = false;
	bool hasAmPm = false;
	for ( const auto& marker : { std::pair<std::string_view, bool>{ locale.am, false },
								 std::pair<std::string_view, bool>{ locale.pm, true },
								 { "AM", false },
								 { "PM", true } } ) {
		if ( marker.first.empty() )
			continue;
		const std::string lowerText = lowerASCII( normalized );
		const std::string lowerMarker = lowerASCII( marker.first );
		const size_t found = lowerText.find( lowerMarker );
		if ( found != std::string::npos ) {
			normalized.erase( found, marker.first.size() );
			hasAmPm = true;
			hasPm = marker.second;
			break;
		}
	}

	normalized = std::string( trimView( normalized ) );
	const auto groups = numericGroups( normalized );
	if ( groups.size() < 2 || groups.size() > 4 )
		return std::nullopt;
	int hour = groups[0].first;
	const int minute = groups[1].first;
	const int second = groups.size() >= 3 ? groups[2].first : 0;
	int millisecond = 0;
	if ( groups.size() == 4 ) {
		millisecond = groups[3].first;
		if ( groups[3].second == 1 )
			millisecond *= 100;
		else if ( groups[3].second == 2 )
			millisecond *= 10;
		else if ( groups[3].second > 3 )
			return std::nullopt;
	}
	if ( hasAmPm ) {
		if ( hour < 1 || hour > 12 )
			return std::nullopt;
		hour = hour % 12 + ( hasPm ? 12 : 0 );
	}
	if ( hour < 0 || hour > 23 || minute < 0 || minute > 59 || second < 0 || second > 59 ||
		 millisecond < 0 || millisecond > 999 )
		return std::nullopt;
	TimeOfDay value{ static_cast<Uint8>( hour ), static_cast<Uint8>( minute ),
					 static_cast<Uint8>( second ), static_cast<Uint16>( millisecond ) };
	return value.isValid() ? std::optional<TimeOfDay>( value ) : std::nullopt;
}

std::optional<LocalDateTime> DateTimeFormatter::parseDateTimeTolerant(
	std::string_view text, const DateTimeFormat& displayFormat, const DateTimeLocale& locale ) {
	text = trimView( text );
	if ( text.empty() )
		return std::nullopt;
	// ISO paste has stable ordering even when a custom display pattern swaps day and month.
	const size_t isoSeparator = text.find( 'T' );
	if ( isoSeparator != std::string_view::npos ) {
		static const DateTimeFormat isoDate( "yyyy-MM-dd" );
		static const DateTimeFormat isoTime( "HH:mm:ss" );
		const auto date = parseDate( text.substr( 0, isoSeparator ), isoDate, locale );
		const auto time = parseTimeTolerant( text.substr( isoSeparator + 1 ), isoTime, locale );
		if ( date && time )
			return LocalDateTime{ *date, *time };
	}
	if ( auto exact = parseDateTime( text, displayFormat, locale ) )
		return exact;

	size_t separator = text.find( 'T' );
	if ( separator == std::string_view::npos ) {
		for ( size_t i = 0; i < text.size(); ++i ) {
			if ( std::isspace( static_cast<unsigned char>( text[i] ) ) ) {
				separator = i;
				break;
			}
		}
	}
	if ( separator == std::string_view::npos )
		return std::nullopt;

	const auto dateText = trimView( text.substr( 0, separator ) );
	const auto timeText = trimView( text.substr( separator + 1 ) );
	std::string datePattern;
	for ( const auto& token : displayFormat.getTokens() ) {
		if ( token.type == DateTimeFormatTokenType::Hour24 ||
			 token.type == DateTimeFormatTokenType::Hour12 )
			break;
		if ( token.type == DateTimeFormatTokenType::Literal )
			datePattern += "'" + token.literal + "'";
		else if ( token.type == DateTimeFormatTokenType::Day ||
				  token.type == DateTimeFormatTokenType::Month ||
				  token.type == DateTimeFormatTokenType::Year )
			datePattern +=
				std::string( token.width, token.type == DateTimeFormatTokenType::Day	 ? 'd'
										  : token.type == DateTimeFormatTokenType::Month ? 'M'
																						 : 'y' );
	}
	DateTimeFormat dateFormat( datePattern.empty() ? locale.datePattern() : datePattern );
	DateTimeFormat timeFormat( locale.timePattern( true, true ) );
	const auto date = parseDateTolerant( dateText, dateFormat, locale );
	const auto time = parseTimeTolerant( timeText, timeFormat, locale );
	if ( !date || !time )
		return std::nullopt;
	return LocalDateTime{ *date, *time };
}

std::string DateTimeFormatter::toISODate( const CalendarDate& value ) {
	if ( !value.isValid() )
		return {};
	return fourDigits( value.year ) + "-" + twoDigits( value.month ) + "-" + twoDigits( value.day );
}

std::string DateTimeFormatter::toISOTime( const TimeOfDay& value, bool includeMilliseconds ) {
	if ( !value.isValid() )
		return {};
	std::string result =
		twoDigits( value.hour ) + ":" + twoDigits( value.minute ) + ":" + twoDigits( value.second );
	if ( includeMilliseconds ) {
		result += '.';
		result += paddedNumber( value.millisecond, 3 );
	}
	return result;
}

std::string DateTimeFormatter::toISOLocalDateTime( const LocalDateTime& value,
												   bool includeMilliseconds ) {
	if ( !value.isValid() )
		return {};
	return toISODate( value.date ) + "T" + toISOTime( value.time, includeMilliseconds );
}

}} // namespace EE::System
