#include <eepp/system/clock.hpp>
#include <eepp/system/platform/platformimpl.hpp>

namespace EE { namespace System {

Clock::Clock() :
	mClockImpl( new Platform::ClockImpl() )
{
	Restart();
}

Clock::~Clock() {
	delete mClockImpl;
}

void Clock::Restart() {
	mClockImpl->Restart();
}

Time Clock::GetElapsedTime() const {
	return Microseconds( mClockImpl->GetElapsedTime() );
}

Time Clock::Elapsed() {
	Time r = GetElapsedTime();
	Restart();
	return r;
}

}}
