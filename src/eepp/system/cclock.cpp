#include <eepp/system/cclock.hpp>
#include <eepp/system/platform/platformimpl.hpp>

namespace EE { namespace System {

cClock::cClock() :
	mClockImpl( eeNew( Platform::cClockImpl, () ) )
{
	Restart();
}

cClock::~cClock() {
	eeSAFE_DELETE( mClockImpl );
}

void cClock::Restart() {
	mClockImpl->Restart();
}

cTime cClock::GetElapsedTime() const {
	return Microseconds( mClockImpl->GetElapsedTime() );
}

cTime cClock::Elapsed() {
	cTime r = GetElapsedTime();
	Restart();
	return r;
}

}}
