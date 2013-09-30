#include <eepp/system/platform/win/cthreadlocalimpl.hpp>

#if EE_PLATFORM == EE_PLATFORM_WIN

namespace EE { namespace System { namespace Private {

cThreadLocalImpl::cThreadLocalImpl() {
	mIndex = TlsAlloc();
}

cThreadLocalImpl::~cThreadLocalImpl() {
	TlsFree(mIndex);
}

void cThreadLocalImpl::Value(void* value) {
	TlsSetValue(mIndex, value);
}

void* cThreadLocalImpl::Value() const {
	return TlsGetValue(mIndex);
}

}}}

#endif
