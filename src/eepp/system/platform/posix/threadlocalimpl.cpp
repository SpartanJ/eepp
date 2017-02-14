#include <eepp/system/platform/posix/threadlocalimpl.hpp>

#if defined( EE_PLATFORM_POSIX )

namespace EE { namespace System { namespace Private {

ThreadLocalImpl::ThreadLocalImpl() {
	pthread_key_create(&mKey, NULL);
}

ThreadLocalImpl::~ThreadLocalImpl() {
	pthread_key_delete(mKey);
}

void ThreadLocalImpl::value(void* val) {
	pthread_setspecific(mKey, val);
}

void* ThreadLocalImpl::value() const {
	return pthread_getspecific(mKey);
}

}}}

#endif
