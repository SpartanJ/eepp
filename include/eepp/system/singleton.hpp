#ifndef EE_SYSTEMSINGLETON_H
#define EE_SYSTEMSINGLETON_H

#include <eepp/core.hpp>
#include <eepp/system/lock.hpp>
#include <eepp/system/mutex.hpp>

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
	static void destroySingleton();    \
                                       \
	static void detachSingleton();

#define SINGLETON_DECLARE_IMPLEMENTATION( T ) \
                                              \
	static T* ms_singleton = NULL;            \
	static Mutex ms_mutex;                    \
                                              \
	T* T::createSingleton() {                 \
		Lock l( ms_mutex );                   \
		if ( NULL == ms_singleton )           \
			ms_singleton = eeNew( T, () );    \
		return ms_singleton;                  \
	}                                         \
                                              \
	T* T::existsSingleton() {                 \
		Lock l( ms_mutex );                   \
		return ms_singleton;                  \
	}                                         \
                                              \
	T* T::instance() {                        \
		return createSingleton();             \
	}                                         \
                                              \
	void T::destroySingleton() {              \
		Lock l( ms_mutex );                   \
		eeSAFE_DELETE( ms_singleton );        \
	}                                         \
                                              \
	void T::detachSingleton() {               \
		Lock l( ms_mutex );                   \
		ms_singleton = nullptr;               \
	}

namespace EE { namespace System {

/** @brief Template class for only one instance classes. */
template <typename T> class Singleton {
  protected:
	static T* ms_singleton;
	static Mutex ms_mutex;

  public:
	/** Get the singleton pointer (without instance verification) */
	static T* existsSingleton() {
		Lock l( ms_mutex );
		return ms_singleton;
	}

	/** Get the singleton pointer */
	static T* instance() {
		Lock l( ms_mutex );
		if ( NULL == ms_singleton )
			ms_singleton = eeNew( T, () );
		return ms_singleton;
	}

	/** Destroy the singleton instance */
	static void destroySingleton() {
		Lock l( ms_mutex );
		eeSAFE_DELETE( ms_singleton );
	}

	/** Detaches the existing singleton. Instance will keep existing but not associated */
	static void detachSingleton() {
		Lock l( ms_mutex );
		ms_singleton = nullptr;
	}
};

}} // namespace EE::System
#endif
