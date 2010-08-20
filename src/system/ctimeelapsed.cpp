#include "ctimeelapsed.hpp"

namespace EE { namespace System {

cTimeElapsed::cTimeElapsed() : mFirstCheck( getMicroseconds() ), mLastCheck( getMicroseconds() ), mElapsed(0)
{
}

cTimeElapsed::~cTimeElapsed()
{
}

eeDouble cTimeElapsed::Elapsed() {
	mElapsed 	= (eeDouble)( getMicroseconds() - mLastCheck ) / 1000.0;
	mLastCheck 	= getMicroseconds();

	return mElapsed;
}

eeDouble cTimeElapsed::ElapsedSinceStart() {
	return (eeDouble)( getMicroseconds() - mFirstCheck ) / 1000.0;
}

void cTimeElapsed::Reset() {
	mFirstCheck = getMicroseconds();
}

}}
