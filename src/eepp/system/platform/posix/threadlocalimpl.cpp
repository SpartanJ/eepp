#include <eepp/system/platform/posix/threadlocalimpl.hpp>

#if defined( EE_PLATFORM_POSIX )

namespace EE { namespace System { namespace Private {

ThreadLocalImpl::ThreadLocalImpl() {
	pthread_key_create(&mKey, NULL);
}

ThreadLocalImpl::~ThreadLocalImpl() {
	pthread_key_delete(mKey);
}

void ThreadLocalImpl::Value(void* value) {
	pthread_setspecific(mKey, value);
}

void* ThreadLocalImpl::Value() const {
	return pthread_getspecific(mKey);
}

}}}

#endif
