#include <eepp/system/platform/posix/threadlocalimpl.hpp>

#if defined( EE_PLATFORM_POSIX )

namespace EE { namespace System { namespace Private {

ThreadLocalImpl::ThreadLocalImpl() {
	pthread_key_create( &mKey, NULL );
}

ThreadLocalImpl::~ThreadLocalImpl() {
	pthread_key_delete( mKey );
}

void ThreadLocalImpl::setValue( void* val ) {
	pthread_setspecific( mKey, val );
}

void* ThreadLocalImpl::getValue() const {
	return pthread_getspecific( mKey );
}

}}} // namespace EE::System::Private

#endif
