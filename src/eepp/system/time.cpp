#include <eepp/core/string.hpp>
#include <eepp/system/time.hpp>

namespace EE { namespace System {

const Time Time::Zero;

bool Time::isValid( const std::string& str ) {
	if ( String::endsWith( str, "s" ) || String::endsWith( str, "ms" ) ) {
		size_t to = str.find_last_of( "sm" );
		for ( size_t pos = 0; pos < to; pos++ ) {
			if ( !String::isNumber( str[pos], true ) ) {
				return false;
			}
		}
		return true;
	}

	return false;
}

Time Time::fromString( const std::string& str ) {
	auto timePart = String::split( str, " " );
	Time time = Time::Zero;
	for ( auto& part : timePart ) {
		String::trimInPlace( part );
		if ( part.empty() )
			continue;
		bool isMilliseconds = String::endsWith( part, "ms" );
		bool isSeconds = false;
		bool isMinutes = false;
		if ( isMilliseconds || ( isSeconds = String::endsWith( part, "s" ) ) ||
			 ( isMinutes = String::endsWith( part, "m" ) ) ||
			 ( isSeconds = String::isNumber( part, true ) ) ) {
			size_t to = part.find_first_of( "sm" );
			if ( to == std::string::npos )
				to = part.size();
			std::string number;
			for ( size_t pos = 0; pos < to; pos++ ) {
				if ( String::isNumber( part[pos], true ) ) {
					number += part[pos];
				} else {
					return Time::Zero;
				}
			}
			double val;
			if ( String::fromString( val, number ) ) {
				if ( isSeconds ) {
					time += Seconds( val );
				} else if ( isMinutes ) {
					time += Minutes( val );
				} else {
					time += Milliseconds( val );
				}
			}
		}
	}
	return time;
}

std::string Time::toString() const {
	Uint64 totalSeconds = asSeconds();

	if ( asSeconds() < 1 ) {
		if ( asMilliseconds() == static_cast<double>( (Int64)asMilliseconds() ) )
			return String::format( "%.fms", asMilliseconds() );
		return String::format( "%.2fms", asMilliseconds() );
	} else if ( totalSeconds < 60 ) {
		return String::format( "%lus", static_cast<unsigned long>( totalSeconds ) );
	}

	long minutesLeft = totalSeconds / 60;
	long secondsLeft = totalSeconds - minutesLeft * 60;

	if ( minutesLeft < 60 ) {
		if ( secondsLeft == 0 )
			return String::format( "%ldm", minutesLeft );
		return String::format( "%ldm %lds", minutesLeft, secondsLeft );
	} else {
		long hoursLeft = minutesLeft / 60;
		minutesLeft = minutesLeft - hoursLeft * 60;
		if ( secondsLeft == 0 )
			return String::format( "%ldh %ldm", hoursLeft, minutesLeft );
		return String::format( "%ldh %ldm %lds", hoursLeft, minutesLeft, secondsLeft );
	}
}

}} // namespace EE::System
