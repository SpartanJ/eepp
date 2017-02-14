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

		void value(void* val);

		void* value() const;
	private :
		pthread_key_t mKey;
};

}}}

#endif

#endif

