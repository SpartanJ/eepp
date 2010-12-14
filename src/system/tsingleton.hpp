#ifndef EE_SYSTEMSINGLETON_H
#define EE_SYSTEMSINGLETON_H

#include "../base.hpp"

namespace EE { namespace System {

/** @brief Template class for only one instance classes. */
template<typename T>
class tSingleton {
	static T* ms_singleton;

	public:
		/** Get the singleton pointer */
		static T* CreateSingleton() {
			if (ms_singleton == 0)
				ms_singleton = eeNew( T, () );

			return ms_singleton;
		}

		/** Get the singleton pointer (without instance verification) */
		static T* ExistsSingleton() {
			return ms_singleton;
		}

		/** Get the singleton pointer */
		static T* instance() {
			return CreateSingleton();
		}

		/** Destroy the singleton instance */
		static void DestroySingleton() {
			if( ms_singleton != 0 ) {
				eeDelete( ms_singleton );
				ms_singleton = 0;
			}
		}
};
template <typename T> T* tSingleton <T>::ms_singleton = 0;

}}
#endif
