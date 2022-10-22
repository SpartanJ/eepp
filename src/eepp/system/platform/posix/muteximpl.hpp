#ifndef EE_SYSTEMCMUTEXIMPLPOSIX_HPP
#define EE_SYSTEMCMUTEXIMPLPOSIX_HPP

#include <eepp/config.hpp>

#if defined( EE_PLATFORM_POSIX )

#include <pthread.h>

namespace EE { namespace System { namespace Platform {

class MutexImpl {
  public:
	MutexImpl();

	~MutexImpl();

	void lock();

	void unlock();

	int tryLock();

  private:
	pthread_mutex_t mMutex;
};

}}} // namespace EE::System::Platform

#endif

#endif
