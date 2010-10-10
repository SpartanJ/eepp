#ifndef EE_MEMORY_MANAGER_HPP
#define EE_MEMORY_MANAGER_HPP

#include <string>
#include <map>
#include <cstdlib>
#include <cstdio>

namespace EE {

class cAllocatedPointer {
	public:
		cAllocatedPointer( void * Data, const std::string& File, int Line, size_t Memory );

		std::string 	mFile;
		int 			mLine;
		size_t 		mMemory;
		void *			mData;
};

typedef std::map<void*, cAllocatedPointer> 	tAllocatedPointerMap;
typedef tAllocatedPointerMap::iterator 		tAllocatedPointerMapIt;

class MemoryManager {
	public:

		static void * AddPointer( const cAllocatedPointer& aAllocatedPointer );

		static bool RemovePointer( void * Data );

		static void LogResults();

		static void SetLogCreation( bool abX );

		template<class T>
		static T* DeleteAndReturn( T * Data ) {
			delete Data;
			return Data;
		}

		template<class T>
		static T* DeleteArrayAndReturn( T * Data ) {
			delete [] Data;
			return Data;
		}

		template<class T>
		static T* FreeAndReturn( T * Data ) {
			free( Data );
			return Data;
		}

		static size_t					GetPeakMemoryUsage() { return mPeakMemoryUsage; }
		static size_t					GetTotalMemoryUsage() { return mTotalMemoryUsage; }

		static tAllocatedPointerMap 	mMapPointers;
		static size_t 				mTotalMemoryUsage;
		static size_t					mPeakMemoryUsage;
};

#ifdef EE_MEMORY_MANAGER
	#define eeNew(classType, constructor) \
			( classType *)EE::MemoryManager::AddPointer( EE::cAllocatedPointer(new classType constructor ,__FILE__,__LINE__,sizeof(classType) ) )

	#define eeNewArray(classType, amount) \
			( classType *) EE::MemoryManager::AddPointer( EE::cAllocatedPointer( new classType [ amount ], __FILE__, __LINE__, amount * sizeof( classType ) ) )

	#define eeMalloc(amount) \
			EE::MemoryManager::AddPointer( EE::cAllocatedPointer( malloc( amount ) , __FILE__, __LINE__, amount ) )

	#define eeDelete(data){ \
			if( EE::MemoryManager::RemovePointer( EE::MemoryManager::DeleteAndReturn( data ) ) == false ) printf( "Deleting at '%s' %d\n", __FILE__, __LINE__ ); \
			}

	#define eeDeleteArray(data){ \
			if(EE::MemoryManager::RemovePointer( EE::MemoryManager::DeleteArrayAndReturn( data ) ) == false ) printf( "Deleting at '%s' %d\n", __FILE__, __LINE__ ); \
			}

	#define eeFree( data ){ \
			if( EE::MemoryManager::RemovePointer( EE::MemoryManager::FreeAndReturn( data ) ) == false ) printf( "Deleting at '%s' %d\n", __FILE__, __LINE__ ); \
			}
#else
	#define eeNew(classType, constructor) \
			new classType constructor

	#define eeNewArray(classType, amount) \
			new classType [ amount ]

	#define eeMalloc(amount) \
			malloc( amount )

	#define eeDelete(data) \
		delete data;

	#define eeDeleteArray(data) \
		delete [] data;

	#define eeFree(data) \
		free(data);
#endif

}

#endif
