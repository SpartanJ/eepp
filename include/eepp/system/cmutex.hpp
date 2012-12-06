#ifndef EE_SYSTEMCMUTEX_H
#define EE_SYSTEMCMUTEX_H

#include <eepp/base.hpp>

namespace EE { namespace System {
	
namespace Platform { class cMutexImpl; }

/** Simple mutex class */
class EE_API cMutex {
	public:
		cMutex();

		~cMutex();

		/** Lock the mutex */
		void Lock();

		/** Unlock the mutex */
		void Unlock();

		/** Tries to lock de mutex */
		int TryLock();
	private:
		Platform::cMutexImpl *		mMutexImpl;
};

}}

#endif
