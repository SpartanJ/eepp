#ifndef EE_MEMORY_MANAGER_HPP
#define EE_MEMORY_MANAGER_HPP

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <eepp/config.hpp>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>

namespace EE {

class EE_API AllocatedPointer {
  public:
	AllocatedPointer( void* data, const std::string& File, int Line, size_t memory,
					  bool track = false, bool globalAllocation = false );

	std::string mFile;
	int mLine;
	size_t mMemory;
	void* mData;
	bool mTrack;
	bool mGlobalAllocation;
};

typedef std::unordered_map<void*, AllocatedPointer> AllocatedPointerMap;
typedef AllocatedPointerMap::iterator AllocatedPointerMapIt;

#if defined( __GNUC__ ) && __GNUC__ >= 12
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuse-after-free"
#endif
class EE_API MemoryManager {
  public:
	static void* addPointer( const AllocatedPointer& aAllocatedPointer );

	static void* reallocPointer( void* data, const AllocatedPointer& aAllocatedPointer );

	static void* addPointerInPlace( void* place, const AllocatedPointer& aAllocatedPointer );

	static bool removePointer( void* data, const char* file, const size_t& line );

	/** Removes a pointer when it is tracked, without diagnosing foreign allocator bookkeeping. */
	static bool removePointerIfTracked( void* data );

	static void showResults();

	template <class T> static T* deletePtr( T* data, const char* file, size_t line ) {
		removePointer( data, file, line );
		delete data;
		return data;
	}

	template <class T> static T* deleteArrayPtr( T* data, const char* file, size_t line ) {
		removePointer( data, file, line );
		delete[] data;
		return data;
	}

	template <class T> static T* free( T* data ) {
		::free( data );
		return data;
	}

	static void* allocate( size_t size );

	static void* reallocate( void* ptr, size_t size );

	/** Allocation entry points used by the debug global new/delete overrides. */
	static void* allocateGlobal( size_t size, size_t alignment );

	static void freeGlobal( void* ptr ) noexcept;

	template <typename Factory>
	static auto create( Factory&& factory, const char* file, int line ) -> decltype( factory() ) {
		auto* pointer = std::forward<Factory>( factory )();
		using Type = typename std::remove_pointer<decltype( pointer )>::type;
		return static_cast<Type*>(
			addPointer( AllocatedPointer( pointer, file, line, sizeof( Type ) ) ) );
	}

	static size_t getPeakMemoryUsage();

	static size_t getTotalMemoryUsage();

	static AllocatedPointer getBiggestAllocation();

	static AllocatedPointer getBiggestNonAnonymousAllocation();
};
#if defined( __GNUC__ ) && __GNUC__ >= 12
#pragma GCC diagnostic pop
#endif

#ifdef EE_MEMORY_MANAGER
#define eeNewExpression( constructor ) \
	EE::MemoryManager::create( [&]() { return new constructor; }, __FILE__, __LINE__ )

#define eeNewLegacy( classType, constructor ) \
	EE::MemoryManager::create( [&]() { return new classType constructor; }, __FILE__, __LINE__ )

#define eeNewSelect( _1, _2, NAME, ... ) NAME

#define eeNew( ... ) eeNewSelect( __VA_ARGS__, eeNewLegacy, eeNewExpression )( __VA_ARGS__ )

#define eeNewTracked( classType, constructor )                       \
	(classType*)EE::MemoryManager::addPointer( EE::AllocatedPointer( \
		new classType constructor, __FILE__, __LINE__, sizeof( classType ), true ) )

#define eeNewInPlace( place, classType, constructor )                                     \
	(classType*)EE::MemoryManager::addPointerInPlace(                                     \
		place, EE::AllocatedPointer( new place classType constructor, __FILE__, __LINE__, \
									 sizeof( classType ) ) )

#define eeNewArray( classType, amount )                              \
	(classType*)EE::MemoryManager::addPointer( EE::AllocatedPointer( \
		new classType[amount], __FILE__, __LINE__, amount * sizeof( classType ) ) )

#define eeMalloc( amount )                                                                      \
	EE::MemoryManager::addPointer( EE::AllocatedPointer( EE::MemoryManager::allocate( amount ), \
														 __FILE__, __LINE__, amount ) )

#if defined( __GNUC__ ) && __GNUC__ >= 12
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuse-after-free"
#endif
#define eeRealloc( ptr, amount )                                                           \
	EE::MemoryManager::reallocPointer(                                                     \
		ptr, EE::AllocatedPointer( EE::MemoryManager::reallocate( ptr, amount ), __FILE__, \
								   __LINE__, amount ) )
#if defined( __GNUC__ ) && __GNUC__ >= 12
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuse-after-free"
#endif
#define eeDelete( data ) EE::MemoryManager::deletePtr( data, __FILE__, __LINE__ )
#if defined( __GNUC__ ) && __GNUC__ >= 12
#pragma GCC diagnostic pop

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuse-after-free"
#endif

#define eeDeleteArray( data ) EE::MemoryManager::deleteArrayPtr( data, __FILE__, __LINE__ )
#if defined( __GNUC__ ) && __GNUC__ >= 12
#pragma GCC diagnostic pop

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuse-after-free"
#endif
#define eeFree( data )                                                                    \
	{                                                                                     \
		if ( EE::MemoryManager::removePointer( EE::MemoryManager::free( data ), __FILE__, \
											   __LINE__ ) == false )                      \
			printf( "Deleting at '%s' %d\n", __FILE__, __LINE__ );                        \
	}
#if defined( __GNUC__ ) && __GNUC__ >= 12
#pragma GCC diagnostic pop
#endif

#else

#define eeNewTracked( classType, constructor ) new classType constructor

#define eeNewExpression( constructor ) new constructor

#define eeNewLegacy( classType, constructor ) new classType constructor

#define eeNewSelect( _1, _2, NAME, ... ) NAME

#define eeNew( ... ) eeNewSelect( __VA_ARGS__, eeNewLegacy, eeNewExpression )( __VA_ARGS__ )

#define eeNewInPlace( place, classType, constructor ) new place classType constructor

#define eeNewArray( classType, amount ) new classType[amount]

#define eeMalloc( amount ) malloc( amount )

#define eeRealloc( ptr, amount ) realloc( ptr, amount )

#define eeDelete( data ) delete data

#define eeDeleteArray( data ) delete[] data

#define eeFree( data ) free( data )

#endif

} // namespace EE

#endif
