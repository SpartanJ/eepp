#include <eepp/system/platform/win/threadlocalimpl.hpp>

#if EE_PLATFORM == EE_PLATFORM_WIN

namespace EE { namespace System { namespace Private {

ThreadLocalImpl::ThreadLocalImpl() {
	mIndex = TlsAlloc();
}

ThreadLocalImpl::~ThreadLocalImpl() {
	TlsFree( mIndex );
}

void ThreadLocalImpl::setValue( void* val ) {
	TlsSetValue( mIndex, val );
}

void* ThreadLocalImpl::getValue() const {
	return TlsGetValue( mIndex );
}

}}} // namespace EE::System::Private

#endif
