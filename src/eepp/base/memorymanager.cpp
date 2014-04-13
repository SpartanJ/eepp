#include <iostream>
#include <sstream>
#include <eepp/base/memorymanager.hpp>
#include <eepp/base/debug.hpp>
#include <eepp/system/clog.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/cmutex.hpp>
#include <eepp/system/clock.hpp>

using namespace EE::System;

namespace EE {

static tAllocatedPointerMap sMapPointers;
static size_t sTotalMemoryUsage = 0;
static size_t sPeakMemoryUsage = 0;
static cAllocatedPointer sBiggestAllocation = cAllocatedPointer( NULL, "", 0, 0 );
static cMutex sAllocMutex;

cAllocatedPointer::cAllocatedPointer( void * Data, const std::string& File, int Line, size_t Memory ) {
	mData 		= Data;
	mFile 		= File;
	mLine 		= Line;
	mMemory 	= Memory;
}

void * MemoryManager::AddPointerInPlace( void * Place, const cAllocatedPointer& aAllocatedPointer ) {
	tAllocatedPointerMapIt it = sMapPointers.find( Place );

	if ( it != sMapPointers.end() ) {
		RemovePointer( Place );
	}

	return AddPointer( aAllocatedPointer );
}

void * MemoryManager::AddPointer( const cAllocatedPointer& aAllocatedPointer ) {
	cLock l( sAllocMutex );

	sMapPointers.insert( tAllocatedPointerMap::value_type( aAllocatedPointer.mData, aAllocatedPointer ) );

	sTotalMemoryUsage += aAllocatedPointer.mMemory;

	if ( sPeakMemoryUsage < sTotalMemoryUsage ) {
		sPeakMemoryUsage = sTotalMemoryUsage;
	}

	if ( aAllocatedPointer.mMemory > sBiggestAllocation.mMemory ) {
		sBiggestAllocation = aAllocatedPointer;
	}

	return aAllocatedPointer.mData;
}

bool MemoryManager::RemovePointer( void * Data ) {
	cLock l( sAllocMutex );

	tAllocatedPointerMapIt it = sMapPointers.find( Data );

	if ( it == sMapPointers.end() ) {
		eePRINTL( "Trying to delete pointer %p created that does not exist!", Data );

		return false;
	}

	sTotalMemoryUsage -= it->second.mMemory;

	sMapPointers.erase( it );

	return true;
}

size_t MemoryManager::GetPeakMemoryUsage() {
	return sPeakMemoryUsage;
}

size_t MemoryManager::GetTotalMemoryUsage() {
	return sTotalMemoryUsage;
}

const cAllocatedPointer& MemoryManager::GetBiggestAllocation() {
	return sBiggestAllocation;
}

void MemoryManager::ShowResults() {
	#ifdef EE_MEMORY_MANAGER

	if ( EE::PrintDebugInLog ) {
		cLog::DestroySingleton();
		EE::PrintDebugInLog = false;
	}

	eePRINTL("\n|--Memory Manager Report-------------------------------------|");
	eePRINTL("|");

	if( sMapPointers.empty() ) {
		eePRINTL( "| No memory leaks detected." );
	} else {
		eePRINTL( "| Memory leaks detected: " );
		eePRINTL( "|");
		eePRINTL( "| address\t file" );

		//Get max length of file name
		int lMax =0;
		tAllocatedPointerMapIt it = sMapPointers.begin();

		for( ; it != sMapPointers.end(); ++it ){
			cAllocatedPointer &ap = it->second;

			if( (int)ap.mFile.length() > lMax )
				lMax = (int)ap.mFile.length();
		}

		lMax += 5;

		for( int i = 0; i < lMax - 4; ++i )
			eePRINT(" ");

		eePRINTL( "line\t\t memory usage\t  " );

		eePRINTL( "|-----------------------------------------------------------|" );

		it = sMapPointers.begin();

		for( ; it != sMapPointers.end(); ++it ) {
			cAllocatedPointer &ap = it->second;

			eePRINT( "| %p\t %s", ap.mData, ap.mFile.c_str() );

			for ( int i=0; i < lMax - (int)ap.mFile.length(); ++i )
				eePRINT(" ");

			eePRINTL( "%d\t\t %d\t", ap.mLine, ap.mMemory );
		}
	}

	eePRINTL( "|" );
	eePRINTL( "| Memory left: %s", FileSystem::SizeToString( static_cast<Int64>( sTotalMemoryUsage ) ).c_str() );
	eePRINTL( "| Biggest allocation:" );
	eePRINTL( "| %s in file: %s at line: %d", FileSystem::SizeToString( sBiggestAllocation.mMemory ).c_str(), sBiggestAllocation.mFile.c_str(), sBiggestAllocation.mLine );
	eePRINTL( "| Peak Memory Usage: %s", FileSystem::SizeToString( sPeakMemoryUsage ).c_str() );
	eePRINTL( "|------------------------------------------------------------|\n" );

	#endif
}

}
