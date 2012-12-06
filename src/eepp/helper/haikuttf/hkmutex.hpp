#ifndef HAIKUTTF_HKMUTEX_HPP
#define HAIKUTTF_HKMUTEX_HPP

#include "hkbase.hpp"

namespace HaikuTTF {

class hkMutexImpl;

class hkMutex {
	public:
		hkMutex();
		
		~hkMutex();
		
		void Lock();
		
		void Unlock();
	protected:
		hkMutexImpl * mImpl;
};

}
#endif
