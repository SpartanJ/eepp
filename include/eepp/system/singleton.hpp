#ifndef EE_SYSTEMSINGLETON_H
#define EE_SYSTEMSINGLETON_H

#include <eepp/core.hpp>

/** Internally we gonna use the macro singleton because it works with the engine compiled as dynamic
 * libraries */

#define SINGLETON_DECLARE_HEADERS( T ) \
  public:                              \
	static T* createSingleton();       \
                                       \
	static T* existsSingleton();       \
                                       \
	static T* instance();              \
                                       \
	static void destroySingleton();

#define SINGLETON_DECLARE_IMPLEMENTATION( T )        \
                                                     \
	static T* ms_singleton = NULL;                   \
                                                     \
	T* T::createSingleton() {                        \
		if ( NULL == ms_singleton ) {                \
			ms_singleton = eeNew( T, () );           \
		}                                            \
                                                     \
		return ms_singleton;                         \
	}                                                \
                                                     \
	T* T::existsSingleton() { return ms_singleton; } \
                                                     \
	T* T::instance() { return createSingleton(); }   \
                                                     \
	void T::destroySingleton() { eeSAFE_DELETE( ms_singleton ); }

namespace EE { namespace System {

/** @brief Template class for only one instance classes. */
template <typename T> class Singleton {
  protected:
	static T* ms_singleton;

  public:
	/** Get the singleton pointer */
	static T* createSingleton() {
		if ( NULL == ms_singleton ) {
			ms_singleton = eeNew( T, () );
		}

		return ms_singleton;
	}

	/** Get the singleton pointer (without instance verification) */
	static T* existsSingleton() { return ms_singleton; }

	/** Get the singleton pointer */
	static T* instance() { return createSingleton(); }

	/** Destroy the singleton instance */
	static void destroySingleton() { eeSAFE_DELETE( ms_singleton ); }
};

}} // namespace EE::System
#endif
