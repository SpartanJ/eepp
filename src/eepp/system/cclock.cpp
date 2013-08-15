#include <eepp/system/cclock.hpp>
#include <eepp/system/platform/platformimpl.hpp>

namespace EE { namespace System {

cClock::cClock() :
	mClockImpl( eeNew( Platform::cClockImpl, () ) )
{
	Reset();
}

cClock::~cClock() {
	eeSAFE_DELETE( mClockImpl );
}

void cClock::Reset() {
	mClockImpl->Reset();
}

cTime cClock::GetElapsedTime() {
	return Microseconds( mClockImpl->GetElapsedTime() );
}

eeDouble cClock::Elapsed() {
	eeDouble r = GetElapsedTime().AsMicroseconds() / 1000.0;
	Reset();
	return r;
}

}}
