#include "ctimeelapsed.hpp"

namespace EE { namespace System {

cTimeElapsed::cTimeElapsed() :
	mFirstCheck( GetMicroseconds() ),
	mLastCheck( mFirstCheck ),
	mElapsed(0)
{
}

cTimeElapsed::~cTimeElapsed()
{
}

eeDouble cTimeElapsed::Elapsed() {
	mElapsed 	= (eeDouble)( GetMicroseconds() - mLastCheck ) / 1000.0;
	mLastCheck 	= GetMicroseconds();

	return mElapsed;
}

eeDouble cTimeElapsed::ElapsedSinceStart() {
	return (eeDouble)( GetMicroseconds() - mFirstCheck ) / 1000.0;
}

void cTimeElapsed::Reset() {
	mFirstCheck = GetMicroseconds();
	mLastCheck	= mFirstCheck;
	mElapsed	= 0;
}

}}
