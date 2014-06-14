#include <eepp/system/platform/win/threadlocalimpl.hpp>

#if EE_PLATFORM == EE_PLATFORM_WIN

namespace EE { namespace System { namespace Private {

ThreadLocalImpl::ThreadLocalImpl() {
	mIndex = TlsAlloc();
}

ThreadLocalImpl::~ThreadLocalImpl() {
	TlsFree(mIndex);
}

void ThreadLocalImpl::Value(void* value) {
	TlsSetValue(mIndex, value);
}

void* ThreadLocalImpl::Value() const {
	return TlsGetValue(mIndex);
}

}}}

#endif
