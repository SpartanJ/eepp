#include <eepp/system/threadlocal.hpp>
#include <eepp/system/platform/platformimpl.hpp>

namespace EE { namespace System {

ThreadLocal::ThreadLocal(void* value) :
	mImpl( eeNew( Private::ThreadLocalImpl, () ) )
{
	Value( value );
}

ThreadLocal::~ThreadLocal() {
	eeSAFE_DELETE( mImpl );
}

void ThreadLocal::Value(void* value) {
	mImpl->Value(value);
}

void* ThreadLocal::Value() const {
	return mImpl->Value();
}

}}
