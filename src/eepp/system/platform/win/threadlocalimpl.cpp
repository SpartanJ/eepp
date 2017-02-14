#include <eepp/system/platform/win/threadlocalimpl.hpp>

#if EE_PLATFORM == EE_PLATFORM_WIN

namespace EE { namespace System { namespace Private {

ThreadLocalImpl::ThreadLocalImpl() {
	mIndex = TlsAlloc();
}

ThreadLocalImpl::~ThreadLocalImpl() {
	TlsFree(mIndex);
}

void ThreadLocalImpl::value(void* val) {
	TlsSetValue(mIndex, val);
}

void* ThreadLocalImpl::value() const {
	return TlsGetValue(mIndex);
}

}}}

#endif
