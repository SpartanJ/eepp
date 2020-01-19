#include <eepp/core/string.hpp>
#include <eepp/system/time.hpp>

namespace EE { namespace System {

const Time Time::Zero;

Time::Time() : mMicroseconds( 0 ) {}

double Time::asSeconds() const {
	return mMicroseconds / 1000000.0;
}

double Time::asMilliseconds() const {
	return mMicroseconds / 1000.0;
}

Int64 Time::asMicroseconds() const {
	return mMicroseconds;
}

Time::Time( Int64 Microseconds ) : mMicroseconds( Microseconds ) {}

std::string Time::toString() {
	Uint64 totalSeconds = asSeconds();

	if ( asSeconds() < 1 ) {
		return String::format( "%4.2fms", asMilliseconds() );
	} else if ( totalSeconds < 60 ) {
		return String::format( "%lus", static_cast<unsigned long>( totalSeconds ) );
	}

	long minutesLeft = totalSeconds / 60;
	long secondsLeft = totalSeconds - minutesLeft * 60;

	if ( minutesLeft < 60 ) {
		return String::format( "%02ldm %02lds", minutesLeft, secondsLeft );
	} else {
		long hoursLeft = minutesLeft / 60;
		minutesLeft = minutesLeft - hoursLeft * 60;
		return String::format( "%02ldh %02ldm %02lds", hoursLeft, minutesLeft, secondsLeft );
	}
}

Time Seconds( double amount ) {
	return Time( static_cast<Int64>( amount * 1000000 ) );
}

Time Milliseconds( double amount ) {
	return Time( static_cast<Int64>( amount * 1000 ) );
}

Time Microseconds( Int64 amount ) {
	return Time( amount );
}

bool operator==( Time left, Time right ) {
	return left.asMicroseconds() == right.asMicroseconds();
}

bool operator!=( Time left, Time right ) {
	return left.asMicroseconds() != right.asMicroseconds();
}

bool operator<( Time left, Time right ) {
	return left.asMicroseconds() < right.asMicroseconds();
}

bool operator>( Time left, Time right ) {
	return left.asMicroseconds() > right.asMicroseconds();
}

bool operator<=( Time left, Time right ) {
	return left.asMicroseconds() <= right.asMicroseconds();
}

bool operator>=( Time left, Time right ) {
	return left.asMicroseconds() >= right.asMicroseconds();
}

Time operator-( Time right ) {
	return Microseconds( -right.asMicroseconds() );
}

Time operator+( Time left, Time right ) {
	return Microseconds( left.asMicroseconds() + right.asMicroseconds() );
}

Time& operator+=( Time& left, Time right ) {
	return left = left + right;
}

Time operator-( Time left, Time right ) {
	return Microseconds( left.asMicroseconds() - right.asMicroseconds() );
}

Time& operator-=( Time& left, Time right ) {
	return left = left - right;
}

Time operator*( Time left, Time right ) {
	return Microseconds( left.asMicroseconds() * right.asMicroseconds() );
}

Time operator*( Time left, double right ) {
	return Seconds( left.asSeconds() * right );
}

Time operator*( double left, Time right ) {
	return right * left;
}

Time operator*( Time left, Int64 right ) {
	return Microseconds( left.asMicroseconds() * right );
}

Time operator*( Int64 left, Time right ) {
	return right * left;
}

Time& operator*=( Time& left, Time right ) {
	return left = left * right;
}

Time& operator*=( Time& left, double right ) {
	return left = left * right;
}

Time& operator*=( Time& left, Int64 right ) {
	return left = left * right;
}

Time operator/( Time left, Time right ) {
	return Microseconds( left.asMicroseconds() / right.asMicroseconds() );
}

Time operator/( Time left, double right ) {
	return Seconds( left.asSeconds() / right );
}

Time operator/( Time left, Int64 right ) {
	return Microseconds( left.asMicroseconds() / right );
}

Time& operator/=( Time& left, Time right ) {
	return left = left / right;
}

Time& operator/=( Time& left, Int64 right ) {
	return left = left / right;
}

Time operator%( Time left, Time right ) {
	return Microseconds( left.asMicroseconds() % right.asMicroseconds() );
}

Time& operator%=( Time& left, Time right ) {
	return left = left % right;
}

}} // namespace EE::System
