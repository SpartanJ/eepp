#ifndef EE_SYSTEMCMUTEX_H
#define EE_SYSTEMCMUTEX_H

#include <eepp/base.hpp>

namespace EE { namespace System {
	
namespace Platform { class cMutexImpl; }

/** @brief Blocks concurrent access to shared resources from multiple threads */
class EE_API cMutex {
	public:
		cMutex();

		~cMutex();

		/** @brief Lock the mutex
		**	If the mutex is already locked in another thread,
		**	this call will block the execution until the mutex
		**	is released. */
		void Lock();

		/** @brief Unlock the mutex */
		void Unlock();

		/** @brief Tries to lock de mutex if possible */
		int TryLock();
	private:
		Platform::cMutexImpl *		mMutexImpl;
};

}}

#endif
