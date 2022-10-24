#ifndef EE_MEMORY_MANAGER_HPP
#define EE_MEMORY_MANAGER_HPP

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <eepp/config.hpp>
#include <map>
#include <string>

namespace EE {

class EE_API AllocatedPointer {
  public:
	AllocatedPointer( void* data, const std::string& File, int Line, size_t memory,
					  bool track = false );

	std::string mFile;
	int mLine;
	size_t mMemory;
	void* mData;
	bool mTrack;
};

typedef std::map<void*, AllocatedPointer> AllocatedPointerMap;
typedef AllocatedPointerMap::iterator AllocatedPointerMapIt;

class EE_API MemoryManager {
  public:
	static void* addPointer( const AllocatedPointer& aAllocatedPointer );

	static void* reallocPointer( void* data, const AllocatedPointer& aAllocatedPointer );

	static void* addPointerInPlace( void* place, const AllocatedPointer& aAllocatedPointer );

	static bool removePointer( void* data, const char* file, const size_t& line );

	static void showResults();

	template <class T> static T* deletePtr( T* data ) {
		delete data;
#if defined( __GNUC__ ) && __GNUC__ >= 12
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuse-after-free"
		return data;
#pragma GCC diagnostic pop
#else
		return data;
#endif
	}

	template <class T> static T* deleteArrayPtr( T* data ) {
		delete[] data;
#if defined( __GNUC__ ) && __GNUC__ >= 12
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuse-after-free"
		return data;
#pragma GCC diagnostic pop
#else
		return data;
#endif
	}

	template <class T> static T* free( T* data ) {
		::free( data );
#if defined( __GNUC__ ) && __GNUC__ >= 12
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuse-after-free"
		return data;
#pragma GCC diagnostic pop
#else
		return data;
#endif
	}

	inline static void* allocate( size_t size ) { return malloc( size ); }

	inline static void* reallocate( void* ptr, size_t size ) {
#if defined( __GNUC__ ) && __GNUC__ >= 12
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuse-after-free"
		return realloc( ptr, size );
#pragma GCC diagnostic pop
#else
		return realloc( ptr, size );
#endif
	}

	static size_t getPeakMemoryUsage();

	static size_t getTotalMemoryUsage();

	static const AllocatedPointer& getBiggestAllocation();
};

#ifdef EE_MEMORY_MANAGER
#define eeNewTracked( classType, constructor )                       \
	(classType*)EE::MemoryManager::addPointer( EE::AllocatedPointer( \
		new classType constructor, __FILE__, __LINE__, sizeof( classType ), true ) )

#define eeNew( classType, constructor )                              \
	(classType*)EE::MemoryManager::addPointer( EE::AllocatedPointer( \
		new classType constructor, __FILE__, __LINE__, sizeof( classType ) ) )

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

#define eeRealloc( ptr, amount )                                                           \
	EE::MemoryManager::reallocPointer(                                                     \
		ptr, EE::AllocatedPointer( EE::MemoryManager::reallocate( ptr, amount ), __FILE__, \
								   __LINE__, amount ) )

#define eeDelete( data )                                                                       \
	{                                                                                          \
		if ( EE::MemoryManager::removePointer( EE::MemoryManager::deletePtr( data ), __FILE__, \
											   __LINE__ ) == false )                           \
			printf( "Deleting at '%s' %d\n", __FILE__, __LINE__ );                             \
	}

#define eeDeleteArray( data )                                                             \
	{                                                                                     \
		if ( EE::MemoryManager::removePointer( EE::MemoryManager::deleteArrayPtr( data ), \
											   __FILE__, __LINE__ ) == false )            \
			printf( "Deleting at '%s' %d\n", __FILE__, __LINE__ );                        \
	}

#define eeFree( data )                                                                    \
	{                                                                                     \
		if ( EE::MemoryManager::removePointer( EE::MemoryManager::free( data ), __FILE__, \
											   __LINE__ ) == false )                      \
			printf( "Deleting at '%s' %d\n", __FILE__, __LINE__ );                        \
	}
#else

#define eeNewTracked( classType, constructor ) new classType constructor

#define eeNew( classType, constructor ) new classType constructor

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
