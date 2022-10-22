#ifndef EE_SYSTEMCTHREADLOCALIMPLPOSIX_HPP
#define EE_SYSTEMCTHREADLOCALIMPLPOSIX_HPP

#include <eepp/core.hpp>
#include <eepp/core/noncopyable.hpp>

#if defined( EE_PLATFORM_POSIX )

#include <pthread.h>

namespace EE { namespace System { namespace Private {

class ThreadLocalImpl : NonCopyable {
  public:
	ThreadLocalImpl();

	~ThreadLocalImpl();

	void setValue( void* val );

	void* getValue() const;

  private:
	pthread_key_t mKey;
};

}}} // namespace EE::System::Private

#endif

#endif
