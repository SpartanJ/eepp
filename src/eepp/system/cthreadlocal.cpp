#include <eepp/system/cthreadlocal.hpp>
#include <eepp/system/platform/platformimpl.hpp>

namespace EE { namespace System {

cThreadLocal::cThreadLocal(void* value) :
	mImpl( eeNew( Private::cThreadLocalImpl, () ) )
{
    Value( value );
}

cThreadLocal::~cThreadLocal() {
    eeSAFE_DELETE( mImpl );
}

void cThreadLocal::Value(void* value) {
	mImpl->Value(value);
}

void* cThreadLocal::Value() const {
	return mImpl->Value();
}

}}
