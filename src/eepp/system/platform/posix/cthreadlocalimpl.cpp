#include <eepp/system/platform/posix/cthreadlocalimpl.hpp>

#if defined( EE_PLATFORM_POSIX )

namespace EE { namespace System { namespace Private {

cThreadLocalImpl::cThreadLocalImpl() {
	pthread_key_create(&mKey, NULL);
}

cThreadLocalImpl::~cThreadLocalImpl() {
	pthread_key_delete(mKey);
}

void cThreadLocalImpl::Value(void* value) {
	pthread_setspecific(mKey, value);
}

void* cThreadLocalImpl::Value() const {
	return pthread_getspecific(mKey);
}

}}}

#endif
