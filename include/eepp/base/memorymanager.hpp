#ifndef EE_MEMORY_MANAGER_HPP
#define EE_MEMORY_MANAGER_HPP

#include <eepp/declares.hpp>
#include <cstdlib>
#include <string>
#include <cstring>
#include <map>

namespace EE {

class EE_API cAllocatedPointer {
	public:
		cAllocatedPointer( void * Data, const std::string& File, int Line, size_t Memory );

		std::string 	mFile;
		int 			mLine;
		size_t			mMemory;
		void *			mData;
};

typedef std::map<void*, cAllocatedPointer> 	tAllocatedPointerMap;
typedef tAllocatedPointerMap::iterator 		tAllocatedPointerMapIt;

class EE_API MemoryManager {
	public:
		static void * AddPointer( const cAllocatedPointer& aAllocatedPointer );

		static void * AddPointerInPlace( void * Place, const cAllocatedPointer& aAllocatedPointer );

		static bool RemovePointer( void * Data );

		static void LogResults();

		template<class T>
		static T* Delete( T * Data ) {
			delete Data;
			return Data;
		}

		template<class T>
		static T* DeleteArray( T * Data ) {
			delete [] Data;
			return Data;
		}

		template<class T>
		static T * Free( T * Data ) {
			free( Data );
			return Data;
		}

		inline static void * Allocate( size_t size ) {
			return malloc( size );
		}

		static size_t					GetPeakMemoryUsage()	{ return mPeakMemoryUsage;	}
		static size_t					GetTotalMemoryUsage()	{ return mTotalMemoryUsage;	}
		static const cAllocatedPointer&	GetBiggestAllocation()	{ return mBiggestAllocation; }

		static tAllocatedPointerMap 	mMapPointers;
		static size_t					mTotalMemoryUsage;
		static size_t					mPeakMemoryUsage;
		static cAllocatedPointer		mBiggestAllocation;
};

#ifdef EE_MEMORY_MANAGER
	#define eeNew( classType, constructor ) \
			( classType *)EE::MemoryManager::AddPointer( EE::cAllocatedPointer( new classType constructor ,__FILE__,__LINE__, sizeof(classType) ) )

	#define eeNewInPlace( place, classType, constructor ) \
			( classType *)EE::MemoryManager::AddPointerInPlace( place, EE::cAllocatedPointer( new place classType constructor ,__FILE__,__LINE__, sizeof(classType) ) )

	#define eeNewArray( classType, amount ) \
			( classType *) EE::MemoryManager::AddPointer( EE::cAllocatedPointer( new classType [ amount ], __FILE__, __LINE__, amount * sizeof( classType ) ) )

	#define eeMalloc(amount) \
			EE::MemoryManager::AddPointer( EE::cAllocatedPointer( EE::MemoryManager::Allocate( amount ), __FILE__, __LINE__, amount ) )

	#define eeDelete( data ){ \
			if( EE::MemoryManager::RemovePointer( EE::MemoryManager::Delete( data ) ) == false ) printf( "Deleting at '%s' %d\n", __FILE__, __LINE__ ); \
			}

	#define eeDeleteArray( data ){ \
			if ( EE::MemoryManager::RemovePointer( EE::MemoryManager::DeleteArray( data ) ) == false ) printf( "Deleting at '%s' %d\n", __FILE__, __LINE__ ); \
			}

	#define eeFree( data ){ \
			if( EE::MemoryManager::RemovePointer( EE::MemoryManager::Free( data ) ) == false ) printf( "Deleting at '%s' %d\n", __FILE__, __LINE__ ); \
			}
#else
	#define eeNew( classType, constructor ) \
			new classType constructor

	#define eeNewInPlace( place, classType, constructor ) \
			new place classType constructor

	#define eeNewArray( classType, amount ) \
			new classType [ amount ]

	#define eeMalloc( amount ) \
			malloc( amount )

	#define eeDelete( data ) \
		delete data

	#define eeDeleteArray( data ) \
		delete [] data

	#define eeFree( data ) \
		free(data)
#endif

}

#endif
