#ifndef EE_SYSTEMSINGLETONDECLARATIONS_HPP
#define EE_SYSTEMSINGLETONDECLARATIONS_HPP

/** Declarations shared by the macro-based singleton implementations.
 *
 * Keep this header lightweight. The implementation macro and the Singleton template
 * require the allocation and synchronization machinery from singleton.hpp, while
 * singleton-owning public headers only need these declarations.
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
	static void destroySingleton();     \
	                                    \
	static void detachSingleton();

#endif
