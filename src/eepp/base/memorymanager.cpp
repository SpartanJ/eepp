#include <iostream>
#include <sstream>
#include <eepp/base/memorymanager.hpp>
#include <eepp/base/debug.hpp>
#include <eepp/system/clog.hpp>
using namespace EE::System;

namespace EE {

#ifdef EE_MEMORY_MANAGER
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
#endif

tAllocatedPointerMap MemoryManager::mMapPointers;
size_t MemoryManager::mTotalMemoryUsage = 0;
size_t MemoryManager::mPeakMemoryUsage = 0;
cAllocatedPointer MemoryManager::mBiggestAllocation = cAllocatedPointer( NULL, "", 0, 0 );

cAllocatedPointer::cAllocatedPointer( void * Data, const std::string& File, int Line, size_t Memory ) {
	mData 		= Data;
	mFile 		= File;
	mLine 		= Line;
	mMemory 	= Memory;
}

void * MemoryManager::AddPointerInPlace( void * Place, const cAllocatedPointer& aAllocatedPointer ) {
	tAllocatedPointerMapIt it = mMapPointers.find( Place );

	if ( it != mMapPointers.end() ) {
		RemovePointer( Place );
	}

	return AddPointer( aAllocatedPointer );
}

void * MemoryManager::AddPointer( const cAllocatedPointer& aAllocatedPointer ) {
	mMapPointers.insert( tAllocatedPointerMap::value_type( aAllocatedPointer.mData, aAllocatedPointer ) );

	mTotalMemoryUsage += aAllocatedPointer.mMemory;

	if ( mPeakMemoryUsage < mTotalMemoryUsage ) {
		mPeakMemoryUsage = mTotalMemoryUsage;
	}

	if ( aAllocatedPointer.mMemory > mBiggestAllocation.mMemory ) {
		mBiggestAllocation = aAllocatedPointer;
	}

	return aAllocatedPointer.mData;
}

bool MemoryManager::RemovePointer( void * Data ) {
	tAllocatedPointerMapIt it = mMapPointers.find( Data );

	if ( it == mMapPointers.end() ) {
		eePRINT( "Trying to delete pointer %p created that does not exist!\n", Data );

		return false;
	}

	mTotalMemoryUsage -= it->second.mMemory;

	mMapPointers.erase( it );

	return true;
}

void MemoryManager::LogResults() {
	#ifdef EE_MEMORY_MANAGER

	if ( EE::PrintDebugInLog ) {
		cLog::DestroySingleton();
		EE::PrintDebugInLog = false;
	}

	eePRINT("\n|--Memory Manager Report-------------------------------------|\n");
	eePRINT("|\n");

	if( mMapPointers.empty() ) {
		eePRINT( "| No memory leaks detected.\n" );
	} else {
		eePRINT( "| Memory leaks detected: \n" );
		eePRINT( "|\n");

		eePRINT( "| address\t file" );

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
			eePRINT(" ");

		eePRINT( "line\t\t memory usage\t  \n" );

		eePRINT( "|-----------------------------------------------------------|\n" );

		it = mMapPointers.begin();

		for( ; it != mMapPointers.end(); ++it ) {
			cAllocatedPointer &ap = it->second;

			eePRINT( "| %p\t %s", ap.mData, ap.mFile.c_str() );

			for ( int i=0; i < lMax - (int)ap.mFile.length(); ++i )
				eePRINT(" ");

			eePRINT( "%d\t\t %d\t\n", ap.mLine, ap.mMemory );
		}
	}

	eePRINT( "|\n" );
	eePRINT( "| Memory left: %s\n", SizeToString( mTotalMemoryUsage ).c_str() );
	eePRINT( "| Biggest allocation:\n" );
	eePRINT( "| %s in file: %s at line: %d\n", SizeToString( mBiggestAllocation.mMemory ).c_str(), mBiggestAllocation.mFile.c_str(), mBiggestAllocation.mLine );
	eePRINT( "| Peak Memory Usage: %s\n", SizeToString( mPeakMemoryUsage ).c_str() );
	eePRINT( "|------------------------------------------------------------|\n\n" );

	#endif
}

}
