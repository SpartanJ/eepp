#ifndef EE_SYSTEMCTHREADLOCALIMPLPOSIX_HPP
#define EE_SYSTEMCTHREADLOCALIMPLPOSIX_HPP

#include <eepp/base.hpp>
#include <eepp/base/noncopyable.hpp>

#if defined( EE_PLATFORM_POSIX )

#include <pthread.h>

namespace EE { namespace System { namespace Private {

class cThreadLocalImpl : NonCopyable {
	public:
		cThreadLocalImpl();

		~cThreadLocalImpl();

		void Value(void* value);

		void* Value() const;
	private :
		pthread_key_t mKey;
};

}}}

#endif

#endif

