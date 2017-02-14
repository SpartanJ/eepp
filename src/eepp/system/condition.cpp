#include <eepp/system/condition.hpp>
#include <eepp/system/platform/platformimpl.hpp>

using namespace EE::System::Platform;

namespace EE { namespace System {

Condition::Condition( int value ) :
	mCondImpl( new ConditionImpl( value ) )
{
}

Condition::~Condition() {
	delete mCondImpl;
}

void Condition::lock() {
	mCondImpl->lock();
}

bool Condition::waitAndLock( int awaitedValue, int autorelease ) {
	bool flag = mCondImpl->waitAndRetain( awaitedValue );
	
	if ( autorelease ) {
		mCondImpl->release( awaitedValue );
	}
	
	return flag;
}

void Condition::unlock( int value ) {
	mCondImpl->release(value);
}

void Condition::unlock() {
	mCondImpl->unlock();
}

int Condition::operator=( int value ) {
	mCondImpl->setValue( value );
	return value;
}

int Condition::value() const {
	return mCondImpl->value();
}

void Condition::signal() {
	mCondImpl->signal();
}

void Condition::invalidate() {
	mCondImpl->invalidate();
}

void Condition::restore() {
	mCondImpl->restore();
}

}}
