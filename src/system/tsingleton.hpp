#ifndef EE_SYSTEMSINGLETON_H
#define EE_SYSTEMSINGLETON_H

#include "../base.hpp"

#define SINGLETON_DECLARE_HEADERS( T ) \
public: \
\
		static T * CreateSingleton(); \
\
		static T * ExistsSingleton(); \
\
		static T * instance(); \
\
		static void DestroySingleton();

#define SINGLETON_DECLARE_IMPLEMENTATION( T ) \
\
static T* ms_singleton = NULL; \
\
T* T::CreateSingleton() { \
	if ( NULL == ms_singleton ) { \
		ms_singleton = eeNew( T, () ); \
	} \
\
	return ms_singleton; \
} \
\
T * T::ExistsSingleton() { \
	return ms_singleton; \
} \
\
T * T::instance() { \
	return CreateSingleton(); \
} \
\
void T::DestroySingleton() { \
	eeSAFE_DELETE(ms_singleton) ; \
}

namespace EE { namespace System {

/** @brief Template class for only one instance classes. */
template<typename T>
class tSingleton {
	protected:
		static T* ms_singleton;

	public:
		/** Get the singleton pointer */
		static T* CreateSingleton() {
			if ( NULL == ms_singleton ) {
				ms_singleton = eeNew( T, () );
			}
			
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
			eeSAFE_DELETE(ms_singleton);
		}
};

}}
#endif
