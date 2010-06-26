#include "ctimeelapsed.hpp"

namespace EE { namespace System {

cTimeElapsed::cTimeElapsed() : mFirstCheck( getMicroseconds() ), mLastCheck( getMicroseconds() ), mElapsed(0) 
{
}

cTimeElapsed::~cTimeElapsed()
{
}

eeDouble cTimeElapsed::Elapsed() {
	mElapsed = ( getMicroseconds() - mLastCheck ) / (eeDouble)EE_CLOCKS_PER_SEC * 1000;
	mLastCheck = getMicroseconds();
	
	return mElapsed;
}

eeDouble cTimeElapsed::ElapsedSinceStart() {
	return ( getMicroseconds() - mFirstCheck ) / (eeDouble)EE_CLOCKS_PER_SEC * 1000;
}

void cTimeElapsed::Reset() {
	mFirstCheck = getMicroseconds();
}

}}
