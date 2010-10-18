#ifndef HAIKUTTF_HKMUTEX_HPP
#define HAIKUTTF_HKMUTEX_HPP

#include "hkbase.hpp"

namespace HaikuTTF {

class hkMutex {
	public:
		hkMutex();
		
		~hkMutex();
		
		void Lock();
		
		void Unlock();
	protected:
		#if HK_PLATFORM == HK_PLATFORM_WIN
		CRITICAL_SECTION mMutex;
		#elif defined( HK_PLATFORM_UNIX )
		pthread_mutex_t mMutex;
		#else
		SDL_mutex * mMutex;
		#endif
};

}
#endif
