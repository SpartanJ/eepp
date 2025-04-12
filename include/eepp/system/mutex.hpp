#ifndef EE_SYSTEMCMUTEX_H
#define EE_SYSTEMCMUTEX_H

#include <eepp/core.hpp>
#include <eepp/core/noncopyable.hpp>
#include <mutex>

namespace EE { namespace System {

/** @brief Blocks concurrent access to shared resources from multiple threads. This class is being
 * kept for historical reasons. */
class EE_API Mutex : NonCopyable {
  public:
	/** @brief Lock the mutex
	**	If the mutex is already locked in another thread,
	**	this call will block the execution until the mutex
	**	is released. */
	void lock();

	/** @brief Unlock the mutex */
	void unlock();

	/** @brief Tries to lock de mutex if possible */
	bool tryLock();

  private:
	std::recursive_mutex mMutexImpl;
};

}} // namespace EE::System

#endif
