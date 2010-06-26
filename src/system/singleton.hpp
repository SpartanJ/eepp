#ifndef EE_SYSTEMSINGLETON_H
#define EE_SYSTEMSINGLETON_H

namespace EE { namespace System {

/** @brief Template class for only one instance classes. */
template<typename T>
class cSingleton {
	static T* ms_singleton;
	
	public:
		/** Get the singleton pointer */
		static T* Singleton() {
			if (ms_singleton == 0)
				ms_singleton = new T;
			
			return ms_singleton;
		}
		
		/** Get the singleton reference */
		static T& GetSingleton() {
			return *Singleton();
		}
		
		/** Get the singleton pointer */
		static T* GetSingletonPtr() {
			return Singleton();
		}
		
		/** Get the singleton pointer (without instance verification) */
		static T* Instance() {
			return ms_singleton;
		}
		
		/** Get the singleton pointer */
		static T* instance() {
			return Singleton();
		}
		
		/** Destroy the singleton instance */
		static void DestroySingleton() {
			if( ms_singleton != 0 ) {
				delete ms_singleton;
				ms_singleton = 0;
			}
		}
};
template <typename T> T* cSingleton <T>::ms_singleton = 0;

}}
#endif
