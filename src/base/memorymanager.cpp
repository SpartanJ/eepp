#include "memorymanager.hpp"
#include <iostream>
#include <sstream>

namespace EE {

static std::string SizeToString( const unsigned int& MemSize ) {
	std::string size = " bytes";
	double mem = static_cast<double>( MemSize );
	unsigned int c = 0;

	while ( mem > 1024 ) {
		c++;
		mem = mem / 1024;
	}

	switch (c) {
		case 0: size = " bytes"; break;
		case 1: size = " KB"; break;
		case 2: size = " MB"; break;
		case 3: size = " GB"; break;
		case 4: size = " TB"; break;
		default: size = " WTF";
	}

	std::ostringstream ss;
	ss << mem;

	return std::string( ss.str() + size );
}

tAllocatedPointerMap MemoryManager::mMapPointers;
size_t MemoryManager::mTotalMemoryUsage = 0;
size_t MemoryManager::mPeakMemoryUsage = 0;

cAllocatedPointer::cAllocatedPointer( void * Data,const std::string& File, int Line, size_t Memory ) {
	mData 		= Data;
	mFile 		= File;
	mLine 		= Line;
	mMemory 	= Memory;
}

void * MemoryManager::AddPointer( const cAllocatedPointer& aAllocatedPointer ) {
	mMapPointers.insert( tAllocatedPointerMap::value_type( aAllocatedPointer.mData, aAllocatedPointer ) );

	mTotalMemoryUsage += aAllocatedPointer.mMemory;

	if ( mPeakMemoryUsage < mTotalMemoryUsage )
		mPeakMemoryUsage = mTotalMemoryUsage;

	return aAllocatedPointer.mData;
}

bool MemoryManager::RemovePointer( void * Data ) {
	bool Found = false;

	tAllocatedPointerMapIt it = mMapPointers.upper_bound( Data );

	it--;

	if( it != mMapPointers.end() ) {
		char * Test = (char*)it->second.mData;

		size_t testSize = it->second.mMemory;

		if( Data >= Test && Data < Test + testSize )
			Found = true;
	}


	if ( !Found ) {
		printf( "Trying to delete pointer %p created that does not exist!\n", Data );

		return false;
	}

	mTotalMemoryUsage -= it->second.mMemory;

	mMapPointers.erase( it );

	return true;
}

void MemoryManager::LogResults() {
	#ifdef EE_MEMORY_MANAGER
	printf("\n|--Memory Manager Report-------------------------------|\n");
	printf("|\n");

	if( mMapPointers.empty() ) {
		printf( "| No memory leaks detected.\n" );
	} else {
		printf( "| Memory leaks detected: \n" );
		printf( "|\n");

		printf( "| address\t file" );

		//Get max length of file name
		int lMax =0;
		tAllocatedPointerMapIt it = mMapPointers.begin();

		for( ; it != mMapPointers.end(); ++it ){
			cAllocatedPointer &ap = it->second;

			if( (int)ap.mFile.length() > lMax )
				lMax = (int)ap.mFile.length();
		}

		lMax += 5;

		for( int i = 0; i < lMax - 4; ++i )
			printf(" ");

		printf( "line\t\t memory usage\t  \n" );

		printf( "|------------------------------------------------------------\n" );

		it = mMapPointers.begin();

		for( ; it != mMapPointers.end(); ++it ) {
			cAllocatedPointer &ap = it->second;

			printf( "| %p\t %s", ap.mData, ap.mFile.c_str() );

			for ( int i=0; i < lMax - (int)ap.mFile.length(); ++i )
				printf(" ");

			printf( "%d\t\t %d\t\n", ap.mLine, ap.mMemory );
		}
	}

	printf( "|\n" );
	printf( "| Memory left: %s\n", SizeToString( mTotalMemoryUsage ).c_str() );
	printf( "| Peak Memory Usage: %s\n", SizeToString( mPeakMemoryUsage ).c_str() );
	printf( "|------------------------------------------------------|\n\n" );
	#endif
}

}
