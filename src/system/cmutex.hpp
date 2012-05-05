#ifndef EE_SYSTEMCMUTEX_H
#define EE_SYSTEMCMUTEX_H

#include "../base.hpp"
#include "platform/platformimpl.hpp"

namespace EE { namespace System {

/** Simple mutex class */
class EE_API cMutex {
	public:
		cMutex();

		~cMutex();

		/** Lock the mutex */
		void Lock();

		/** Unlock the mutex */
		void Unlock();
	private:
		Platform::cMutexImpl *		mMutexImpl;
};

}}

#endif
