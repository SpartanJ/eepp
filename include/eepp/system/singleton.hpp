#ifndef EE_SYSTEMSINGLETON_H
#define EE_SYSTEMSINGLETON_H

#include <atomic>
#include <eepp/core.hpp>
#include <eepp/system/lock.hpp>
#include <eepp/system/mutex.hpp>

/** Internally we gonna use the macro singleton because it works with the engine compiled as dynamic
 * libraries.
 *
 * Lifecycle contract:
 * - Creation is thread-safe. The first caller creates the singleton under ms_mutex, publishes it
 *   with release semantics, and later callers use an acquire-load fast path without locking.
 * - Destruction and detach are teardown operations only. They must not run concurrently with
 *   instance(), existsSingleton(), or normal use of a previously returned raw pointer. The atomic
 *   only protects pointer publication; it does not protect pointee lifetime after return.
 * - destroySingleton() intentionally keeps the pointer visible while T::~T() runs because several
 *   eepp teardown paths query singleton state while ms_is_shutting_down is true.
 * - Expand SINGLETON_DECLARE_IMPLEMENTATION exactly once in a .cpp file. Expanding it in a header
 *   creates one internal-linkage singleton per translation unit.
 */

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

#define SINGLETON_DECLARE_IMPLEMENTATION( T )                           \
                                                                        \
	static std::atomic<T*> ms_singleton{ NULL };                        \
	static std::atomic<bool> ms_is_shutting_down{ false };              \
	static Mutex ms_mutex;                                              \
                                                                        \
	T* T::createSingleton() {                                           \
		T* singleton = ms_singleton.load( std::memory_order_acquire );  \
		if ( NULL != singleton )                                        \
			return singleton;                                           \
		Lock l( ms_mutex );                                             \
		singleton = ms_singleton.load( std::memory_order_acquire );     \
		if ( NULL == singleton ) {                                      \
			singleton = eeNew( T, () );                                 \
			ms_singleton.store( singleton, std::memory_order_release ); \
		}                                                               \
		return singleton;                                               \
	}                                                                   \
                                                                        \
	T* T::existsSingleton() {                                           \
		return ms_singleton.load( std::memory_order_acquire );          \
	}                                                                   \
                                                                        \
	bool T::isShuttingDown() {                                          \
		return ms_is_shutting_down.load( std::memory_order_acquire );   \
	}                                                                   \
                                                                        \
	T* T::instance() {                                                  \
		return createSingleton();                                       \
	}                                                                   \
                                                                        \
	void T::destroySingleton() {                                        \
		ms_is_shutting_down.store( true, std::memory_order_release );   \
		Lock l( ms_mutex );                                             \
		T* singleton = ms_singleton.load( std::memory_order_acquire );  \
		eeSAFE_DELETE( singleton );                                     \
		ms_singleton.store( NULL, std::memory_order_release );          \
		ms_is_shutting_down.store( false, std::memory_order_release );  \
	}                                                                   \
                                                                        \
	void T::detachSingleton() {                                         \
		Lock l( ms_mutex );                                             \
		ms_singleton.store( nullptr, std::memory_order_release );       \
	}

namespace EE { namespace System {

/** @brief Template class for only one instance classes.
 * Creation is thread-safe, but destroySingleton() and detachSingleton() have the same teardown-only
 * raw-pointer lifetime contract documented above for SINGLETON_DECLARE_IMPLEMENTATION.
 */
template <typename T> class Singleton {
  protected:
	static std::atomic<T*> ms_singleton;
	static Mutex ms_mutex;

  public:
	/** Get the singleton pointer (without instance verification) */
	static T* existsSingleton() { return ms_singleton.load( std::memory_order_acquire ); }

	/** Get the singleton pointer */
	static T* instance() {
		T* singleton = ms_singleton.load( std::memory_order_acquire );
		if ( NULL != singleton )
			return singleton;
		Lock l( ms_mutex );
		singleton = ms_singleton.load( std::memory_order_acquire );
		if ( NULL == singleton ) {
			singleton = eeNew( T, () );
			ms_singleton.store( singleton, std::memory_order_release );
		}
		return singleton;
	}

	/** Destroy the singleton instance */
	static void destroySingleton() {
		Lock l( ms_mutex );
		T* singleton = ms_singleton.load( std::memory_order_acquire );
		eeSAFE_DELETE( singleton );
		ms_singleton.store( NULL, std::memory_order_release );
	}

	/** Detaches the existing singleton. Instance will keep existing but not associated */
	static void detachSingleton() {
		Lock l( ms_mutex );
		ms_singleton.store( nullptr, std::memory_order_release );
	}
};

}} // namespace EE::System
#endif
