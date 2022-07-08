#include <eepp/core/debug.hpp>
#include <eepp/core/memorymanager.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/lock.hpp>
#include <eepp/system/log.hpp>
#include <eepp/system/mutex.hpp>
#include <eepp/window/engine.hpp>
#include <iostream>
#include <sstream>

using namespace EE::System;
using namespace EE::Window;

namespace EE {

static AllocatedPointerMap sMapPointers;
static size_t sTotalMemoryUsage = 0;
static size_t sPeakMemoryUsage = 0;
static AllocatedPointer sBiggestAllocation = AllocatedPointer( NULL, "", 0, 0 );
static Mutex sAlloMutex;

AllocatedPointer::AllocatedPointer( void* data, const std::string& file, int line, size_t memory,
									bool track ) {
	mData = data;
	mFile = file;
	mLine = line;
	mMemory = memory;
	mTrack = track;
}

void* MemoryManager::addPointerInPlace( void* place, const AllocatedPointer& aAllocatedPointer ) {
	AllocatedPointerMapIt it = sMapPointers.find( place );

	if ( it != sMapPointers.end() ) {
		removePointer( place, aAllocatedPointer.mFile.c_str(), aAllocatedPointer.mLine );
	}

	return addPointer( aAllocatedPointer );
}

void* MemoryManager::addPointer( const AllocatedPointer& aAllocatedPointer ) {
	Lock l( sAlloMutex );

	sMapPointers.insert(
		AllocatedPointerMap::value_type( aAllocatedPointer.mData, aAllocatedPointer ) );

	sTotalMemoryUsage += aAllocatedPointer.mMemory;

	if ( sPeakMemoryUsage < sTotalMemoryUsage ) {
		sPeakMemoryUsage = sTotalMemoryUsage;
	}

	if ( aAllocatedPointer.mMemory > sBiggestAllocation.mMemory ) {
		sBiggestAllocation = aAllocatedPointer;
	}

	if ( aAllocatedPointer.mTrack )
		eePRINTL( "Allocating pointer %p at '%s' %d", aAllocatedPointer.mData,
				  aAllocatedPointer.mFile.c_str(), aAllocatedPointer.mLine );

	return aAllocatedPointer.mData;
}

bool MemoryManager::removePointer( void* data, const char* file, const size_t& line ) {
	Lock l( sAlloMutex );

	AllocatedPointerMapIt it = sMapPointers.find( data );

	if ( it->second.mTrack )
		eePRINTL( "Deleting pointer %p at '%s' %d", data, file, line );

	if ( it == sMapPointers.end() ) {
		eePRINTL( "Trying to delete pointer %p created that does not exist!", data );

		return false;
	}

	sTotalMemoryUsage -= it->second.mMemory;

	sMapPointers.erase( it );

	return true;
}

size_t MemoryManager::getPeakMemoryUsage() {
	return sPeakMemoryUsage;
}

size_t MemoryManager::getTotalMemoryUsage() {
	return sTotalMemoryUsage;
}

const AllocatedPointer& MemoryManager::getBiggestAllocation() {
	return sBiggestAllocation;
}

void MemoryManager::showResults() {
#ifdef EE_MEMORY_MANAGER

	if ( EE::PrintDebugInLog ) {
		Log::destroySingleton();
		EE::PrintDebugInLog = false;
	}

	Engine::destroySingleton();

	eePRINTL( "\n|--Memory Manager Report-------------------------------------|" );
	eePRINTL( "|" );

	if ( sMapPointers.empty() ) {
		eePRINTL( "| No memory leaks detected." );
	} else {
		eePRINTL( "| Memory leaks detected: " );
		eePRINTL( "|" );
		eePRINTL( "| address\t file" );

		// Get max length of file name
		int lMax = 0;
		AllocatedPointerMapIt it = sMapPointers.begin();

		for ( ; it != sMapPointers.end(); ++it ) {
			AllocatedPointer& ap = it->second;

			if ( (int)ap.mFile.length() > lMax )
				lMax = (int)ap.mFile.length();
		}

		lMax += 5;

		for ( int i = 0; i < lMax - 4; ++i )
			eePRINT( " " );

		eePRINTL( "line\t\t memory usage\t  " );

		eePRINTL( "|-----------------------------------------------------------|" );

		it = sMapPointers.begin();

		for ( ; it != sMapPointers.end(); ++it ) {
			AllocatedPointer& ap = it->second;

			eePRINT( "| %p\t %s", ap.mData, ap.mFile.c_str() );

			for ( int i = 0; i < lMax - (int)ap.mFile.length(); ++i )
				eePRINT( " " );

			eePRINTL( "%d\t\t %d\t", ap.mLine, ap.mMemory );
		}
	}

	eePRINTL( "|" );
	eePRINTL( "| Memory left: %s",
			  FileSystem::sizeToString( static_cast<Int64>( sTotalMemoryUsage ) ).c_str() );
	eePRINTL( "| Biggest allocation:" );
	eePRINTL( "| %s in file: %s at line: %d",
			  FileSystem::sizeToString( sBiggestAllocation.mMemory ).c_str(),
			  sBiggestAllocation.mFile.c_str(), sBiggestAllocation.mLine );
	eePRINTL( "| Peak Memory Usage: %s", FileSystem::sizeToString( sPeakMemoryUsage ).c_str() );
	eePRINTL( "|------------------------------------------------------------|\n" );

#endif
}

} // namespace EE
