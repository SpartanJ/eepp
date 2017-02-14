#include <eepp/system/threadlocal.hpp>
#include <eepp/system/platform/platformimpl.hpp>

namespace EE { namespace System {

ThreadLocal::ThreadLocal(void* val) :
	mImpl( eeNew( Private::ThreadLocalImpl, () ) )
{
	value( val );
}

ThreadLocal::~ThreadLocal() {
	eeSAFE_DELETE( mImpl );
}

void ThreadLocal::value(void* val) {
	mImpl->value(val);
}

void* ThreadLocal::value() const {
	return mImpl->value();
}

}}
