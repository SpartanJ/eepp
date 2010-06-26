#ifndef EE_SYSTEMCMUTEX_H
#define EE_SYSTEMCMUTEX_H

#include "base.hpp"

namespace EE { namespace System {

/** Simple mutex class */
class EE_API cMutex {
	public:
		cMutex();
		~cMutex();

		/** Lock the mutex */
		bool Lock();

		/** Unlock the mutex */
		bool Unlock();
	private:
		SDL_mutex* mMutex;
};

}}

#endif
