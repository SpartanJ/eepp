#include <eepp/core/debug.hpp>
#include <eepp/core/memorymanager.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/lock.hpp>
#include <eepp/system/log.hpp>
#include <eepp/system/mutex.hpp>
#include <eepp/window/engine.hpp>

using namespace EE::System;
using namespace EE::Window;

// #ifdef EE_DEBUG
// #define EE_OVERRIDE_NEW_DELETE
// #endif

#ifdef EE_OVERRIDE_NEW_DELETE
void* operator new( std::size_t n ) {
	return malloc( n );
}

void operator delete( void* p ) throw() {
	free( p );
}
#endif

namespace EE {

AllocatedPointer::AllocatedPointer( void* data, const std::string& file, int line, size_t memory,
									bool track ) {
	mData = data;
	mFile = file;
	mLine = line;
	mMemory = memory;
	mTrack = track;
}

namespace {

struct MemoryManagerState {
	AllocatedPointerMap pointers;
	size_t totalMemoryUsage{ 0 };
	size_t peakMemoryUsage{ 0 };
	AllocatedPointer biggestAllocation{ NULL, "", 0, 0 };
	Mutex allocationMutex;
};

MemoryManagerState& getMemoryManagerState() {
	// The tracker must remain valid through process-static destruction. Function-local
	// initialization makes its first concurrent use safe; intentionally retaining the state avoids
	// reintroducing a static-destruction-order dependency for late eeDelete() calls.
	static MemoryManagerState* state = new MemoryManagerState;
	return *state;
}

} // namespace

void* MemoryManager::allocate( size_t size ) {
	return malloc( size );
}

void* MemoryManager::reallocate( void* ptr, size_t size ) {
	return realloc( ptr, size );
}

void* MemoryManager::addPointerInPlace( void* place, const AllocatedPointer& aAllocatedPointer ) {
	auto& state = getMemoryManagerState();
	Lock lock( state.allocationMutex );
	AllocatedPointerMapIt it = state.pointers.find( place );

	if ( it != state.pointers.end() ) {
		removePointer( place, aAllocatedPointer.mFile.c_str(), aAllocatedPointer.mLine );
	}

	return addPointer( aAllocatedPointer );
}

void* MemoryManager::addPointer( const AllocatedPointer& aAllocatedPointer ) {
	auto& state = getMemoryManagerState();
	Lock lock( state.allocationMutex );

	state.pointers.insert(
		AllocatedPointerMap::value_type( aAllocatedPointer.mData, aAllocatedPointer ) );

	state.totalMemoryUsage += aAllocatedPointer.mMemory;

	if ( state.peakMemoryUsage < state.totalMemoryUsage ) {
		state.peakMemoryUsage = state.totalMemoryUsage;
	}

	if ( aAllocatedPointer.mMemory > state.biggestAllocation.mMemory ) {
		state.biggestAllocation = aAllocatedPointer;
	}

	if ( aAllocatedPointer.mTrack )
		eePRINTL( "Allocating pointer %p at '%s' %d", aAllocatedPointer.mData,
				  aAllocatedPointer.mFile.c_str(), aAllocatedPointer.mLine );

	return aAllocatedPointer.mData;
}

void* MemoryManager::reallocPointer( void* data, const AllocatedPointer& aAllocatedPointer ) {
	auto& state = getMemoryManagerState();
	Lock lock( state.allocationMutex );

	AllocatedPointerMapIt it = state.pointers.find( data );

	if ( it != state.pointers.end() && it->second.mTrack )
		eePRINTL( "Realloc pointer %p at '%s' %d", data, aAllocatedPointer.mFile.c_str(),
				  aAllocatedPointer.mLine );

	if ( it == state.pointers.end() )
		return addPointer( aAllocatedPointer );

	if ( aAllocatedPointer.mTrack )
		eePRINTL( "Reallocating pointer %p at '%s' %d", aAllocatedPointer.mData,
				  aAllocatedPointer.mFile.c_str(), aAllocatedPointer.mLine );

	if ( it->first != aAllocatedPointer.mData ) {
		removePointer( data, aAllocatedPointer.mFile.c_str(), aAllocatedPointer.mLine );
		addPointer( aAllocatedPointer );
	} else {
		state.totalMemoryUsage -= it->second.mMemory;
		it->second.mMemory = aAllocatedPointer.mMemory;
		it->second.mFile = aAllocatedPointer.mFile;
		it->second.mLine = aAllocatedPointer.mLine;
		it->second.mTrack = aAllocatedPointer.mTrack;

		state.totalMemoryUsage += aAllocatedPointer.mMemory;

		if ( state.peakMemoryUsage < state.totalMemoryUsage ) {
			state.peakMemoryUsage = state.totalMemoryUsage;
		}

		if ( aAllocatedPointer.mMemory > state.biggestAllocation.mMemory ) {
			state.biggestAllocation = aAllocatedPointer;
		}
	}

	return aAllocatedPointer.mData;
}

bool MemoryManager::removePointer( void* data, const char* file, const size_t& line ) {
	auto& state = getMemoryManagerState();
	Lock lock( state.allocationMutex );

	AllocatedPointerMapIt it = state.pointers.find( data );

	if ( it == state.pointers.end() ) {
		eePRINTL( "Trying to delete pointer %p created that does not exist!", data );
		eeASSERT( false );
		return false;
	}

	if ( it->second.mTrack )
		eePRINTL( "Deleting pointer %p at '%s' %d", data, file, line );

	state.totalMemoryUsage -= it->second.mMemory;

	state.pointers.erase( it );

	return true;
}

size_t MemoryManager::getPeakMemoryUsage() {
	auto& state = getMemoryManagerState();
	Lock lock( state.allocationMutex );
	return state.peakMemoryUsage;
}

size_t MemoryManager::getTotalMemoryUsage() {
	auto& state = getMemoryManagerState();
	Lock lock( state.allocationMutex );
	return state.totalMemoryUsage;
}

AllocatedPointer MemoryManager::getBiggestAllocation() {
	auto& state = getMemoryManagerState();
	Lock lock( state.allocationMutex );
	return state.biggestAllocation;
}

void MemoryManager::showResults() {
#ifdef EE_MEMORY_MANAGER

	if ( EE::PrintDebugInLog ) {
		Log::destroySingleton();
		EE::PrintDebugInLog = false;
	}

	Engine::destroySingleton();
	auto& state = getMemoryManagerState();
	Lock lock( state.allocationMutex );

	eePRINTL( "\n|--Memory Manager Report-------------------------------------|" );
	eePRINTL( "|" );

	if ( state.pointers.empty() ) {
		eePRINTL( "| No memory leaks detected." );
	} else {
		eePRINTL( "| Memory leaks detected: " );
		eePRINTL( "|" );
		eePRINTL( "| address\t file" );

		// Get max length of file name
		int lMax = 0;
		AllocatedPointerMapIt it = state.pointers.begin();

		for ( ; it != state.pointers.end(); ++it ) {
			AllocatedPointer& ap = it->second;

			if ( (int)ap.mFile.length() > lMax )
				lMax = (int)ap.mFile.length();
		}

		lMax += 5;

		for ( int i = 0; i < lMax - 4; ++i )
			eePRINT( " " );

		eePRINTL( "line\t\t memory usage\t  " );

		eePRINTL( "|-----------------------------------------------------------|" );

		it = state.pointers.begin();

		for ( ; it != state.pointers.end(); ++it ) {
			AllocatedPointer& ap = it->second;

			eePRINT( "| %p\t %s", ap.mData, ap.mFile.c_str() );

			for ( int i = 0; i < lMax - (int)ap.mFile.length(); ++i )
				eePRINT( " " );

			eePRINTL( "%d\t\t %d\t", ap.mLine, ap.mMemory );
		}
	}

	eePRINTL( "|" );
	eePRINTL( "| Memory left: %s",
			  FileSystem::sizeToString( static_cast<Int64>( state.totalMemoryUsage ) ).c_str() );
	eePRINTL( "| Biggest allocation:" );
	eePRINTL( "| %s in file: %s at line: %d",
			  FileSystem::sizeToString( state.biggestAllocation.mMemory ).c_str(),
			  state.biggestAllocation.mFile.c_str(), state.biggestAllocation.mLine );
	eePRINTL( "| Peak Memory Usage: %s",
			  FileSystem::sizeToString( state.peakMemoryUsage ).c_str() );
	eePRINTL( "|------------------------------------------------------------|\n" );

#endif
}

} // namespace EE
