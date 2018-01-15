#include <eepp/system/threadlocal.hpp>
#include <eepp/system/platform/platformimpl.hpp>

namespace EE { namespace System {

ThreadLocal::ThreadLocal(void* value) :
	mImpl( eeNew( Private::ThreadLocalImpl, () ) )
{
	setValue( value );
}

ThreadLocal::~ThreadLocal() {
	eeSAFE_DELETE( mImpl );
}

void ThreadLocal::setValue(void* val) {
	mImpl->setValue(val);
}

void* ThreadLocal::getValue() const {
	return mImpl->getValue();
}

}}
