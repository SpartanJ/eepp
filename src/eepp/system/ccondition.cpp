#include <eepp/system/ccondition.hpp>
#include <eepp/system/platform/platformimpl.hpp>

using namespace EE::System::Platform;

namespace EE { namespace System {

const bool cCondition::AutoUnlock = true;
const bool cCondition::ManualUnlock = false;

cCondition::cCondition( int value ) :
	mCondImpl( eeNew( cConditionImpl, ( value ) ) )
{
}

cCondition::~cCondition() {
	eeSAFE_DELETE( mCondImpl );
}

void cCondition::Lock() {
	mCondImpl->Lock();
}
	
bool cCondition::WaitAndLock( int awaitedValue, bool autorelease ) {
	bool flag = mCondImpl->WaitAndRetain( awaitedValue );
	
	if ( autorelease ) {
		mCondImpl->Release( awaitedValue );
	}
	
	return flag;
}

void cCondition::Unlock( int value ) {
	mCondImpl->Release(value);
}

void cCondition::Unlock() {
	mCondImpl->Unlock();
}

int cCondition::operator=( int value ) {
	mCondImpl->SetValue( value );
	return value;
}

int cCondition::Value() const {
	return mCondImpl->Value();
}

void cCondition::Signal() {
	mCondImpl->Signal();
}

void cCondition::Invalidate() {
	mCondImpl->Invalidate();
}

void cCondition::Restore() {
	mCondImpl->Restore();
}

}}
