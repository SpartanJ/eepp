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
	static bool isShuttingDown();      \
                                       \
	static T* instance();              \
                                       \
	static void destroySingleton();    \
                                       \
	static void detachSingleton();

#define SINGLETON_DECLARE_IMPLEMENTATION( T ) \
                                              \
	static T* ms_singleton = NULL;            \
	static bool ms_is_shutting_down = false;  \
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
		return ms_singleton;                  \
	}                                         \
	                                          \
	bool T::isShuttingDown() {                \
		return ms_is_shutting_down;           \
	}                                         \
                                              \
	T* T::instance() {                        \
		return createSingleton();             \
	}                                         \
                                              \
	void T::destroySingleton() {              \
		ms_is_shutting_down = true;           \
		Lock l( ms_mutex );                   \
		eeSAFE_DELETE( ms_singleton );        \
		ms_is_shutting_down = false;          \
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
