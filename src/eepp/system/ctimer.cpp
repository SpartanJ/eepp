#include <eepp/system/ctimer.hpp>
#include <eepp/system/platform/platformimpl.hpp>

namespace EE { namespace System {

cTimer::cTimer() :
	mTimerImpl( eeNew( Platform::cTimerImpl, () ) )
{
	Reset();
}

cTimer::~cTimer() {
	eeSAFE_DELETE( mTimerImpl );
}

void cTimer::Reset() {
	mTimerImpl->Reset();
}

unsigned long cTimer::GetMilliseconds() {
	return mTimerImpl->GetMilliseconds();
}

unsigned long cTimer::GetMicroseconds() {
	return mTimerImpl->GetMicroseconds();
}

}}
