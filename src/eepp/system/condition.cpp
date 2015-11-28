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

void Condition::Lock() {
	mCondImpl->Lock();
}

bool Condition::WaitAndLock( int awaitedValue, int autorelease ) {
	bool flag = mCondImpl->WaitAndRetain( awaitedValue );
	
	if ( autorelease ) {
		mCondImpl->Release( awaitedValue );
	}
	
	return flag;
}

void Condition::Unlock( int value ) {
	mCondImpl->Release(value);
}

void Condition::Unlock() {
	mCondImpl->Unlock();
}

int Condition::operator=( int value ) {
	mCondImpl->SetValue( value );
	return value;
}

int Condition::Value() const {
	return mCondImpl->Value();
}

void Condition::Signal() {
	mCondImpl->Signal();
}

void Condition::Invalidate() {
	mCondImpl->Invalidate();
}

void Condition::Restore() {
	mCondImpl->Restore();
}

}}
