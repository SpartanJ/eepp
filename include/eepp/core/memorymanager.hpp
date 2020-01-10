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
	AllocatedPointer( void* Data, const std::string& File, int Line, size_t Memory );

	std::string mFile;
	int mLine;
	size_t mMemory;
	void* mData;
};

typedef std::map<void*, AllocatedPointer> AllocatedPointerMap;
typedef AllocatedPointerMap::iterator AllocatedPointerMapIt;

class EE_API MemoryManager {
  public:
	static void* addPointer( const AllocatedPointer& aAllocatedPointer );

	static void* addPointerInPlace( void* Place, const AllocatedPointer& aAllocatedPointer );

	static bool removePointer( void* Data );

	static void showResults();

	template <class T> static T* deletePtr( T* Data ) {
		delete Data;
		return Data;
	}

	template <class T> static T* deleteArrayPtr( T* Data ) {
		delete[] Data;
		return Data;
	}

	template <class T> static T* free( T* Data ) {
		::free( Data );
		return Data;
	}

	inline static void* allocate( size_t size ) { return malloc( size ); }

	static size_t getPeakMemoryUsage();

	static size_t getTotalMemoryUsage();

	static const AllocatedPointer& getBiggestAllocation();
};

#ifdef EE_MEMORY_MANAGER
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

#define eeDelete( data )                                                                         \
	{                                                                                            \
		if ( EE::MemoryManager::removePointer( EE::MemoryManager::deletePtr( data ) ) == false ) \
			printf( "Deleting at '%s' %d\n", __FILE__, __LINE__ );                               \
	}

#define eeDeleteArray( data )                                                                 \
	{                                                                                         \
		if ( EE::MemoryManager::removePointer( EE::MemoryManager::deleteArrayPtr( data ) ) == \
			 false )                                                                          \
			printf( "Deleting at '%s' %d\n", __FILE__, __LINE__ );                            \
	}

#define eeFree( data )                                                                      \
	{                                                                                       \
		if ( EE::MemoryManager::removePointer( EE::MemoryManager::free( data ) ) == false ) \
			printf( "Deleting at '%s' %d\n", __FILE__, __LINE__ );                          \
	}
#else
#define eeNew( classType, constructor ) new classType constructor

#define eeNewInPlace( place, classType, constructor ) new place classType constructor

#define eeNewArray( classType, amount ) new classType[amount]

#define eeMalloc( amount ) malloc( amount )

#define eeDelete( data ) delete data

#define eeDeleteArray( data ) delete[] data

#define eeFree( data ) free( data )
#endif

} // namespace EE

#endif
