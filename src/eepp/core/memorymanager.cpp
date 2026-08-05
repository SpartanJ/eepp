#include <cstddef>
#include <eepp/core/debug.hpp>
#include <eepp/core/memorymanager.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/lock.hpp>
#include <eepp/system/log.hpp>
#include <eepp/system/mutex.hpp>
#include <eepp/window/engine.hpp>
#include <new>
#include <tabulate/tabulate.hpp>

#if EE_PLATFORM == EE_PLATFORM_WIN
#include <malloc.h>
#endif

using namespace EE::System;
using namespace EE::Window;

namespace EE {

AllocatedPointer::AllocatedPointer( void* data, const std::string& file, int line, size_t memory,
									bool track, bool globalAllocation ) {
	mData = data;
	mFile = file;
	mLine = line;
	mMemory = memory;
	mTrack = track;
	mGlobalAllocation = globalAllocation;
}

namespace {

thread_local bool insideMemoryManager = false;

class MemoryManagerScope {
  public:
	MemoryManagerScope() : mWasInside( insideMemoryManager ) { insideMemoryManager = true; }
	~MemoryManagerScope() { insideMemoryManager = mWasInside; }

  private:
	bool mWasInside;
};

struct MemoryManagerState {
	AllocatedPointerMap pointers;
	size_t totalMemoryUsage{ 0 };
	size_t peakMemoryUsage{ 0 };
	AllocatedPointer biggestAllocation{ NULL, "", 0, 0 };
	AllocatedPointer biggestNonAnonymousAllocation{ NULL, "", 0, 0 };
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
	MemoryManagerScope scope;
	auto& state = getMemoryManagerState();
	Lock lock( state.allocationMutex );
	AllocatedPointerMapIt it = state.pointers.find( place );

	if ( it != state.pointers.end() ) {
		removePointer( place, aAllocatedPointer.mFile.c_str(), aAllocatedPointer.mLine );
	}

	return addPointer( aAllocatedPointer );
}

void* MemoryManager::addPointer( const AllocatedPointer& aAllocatedPointer ) {
	MemoryManagerScope scope;
	auto& state = getMemoryManagerState();
	Lock lock( state.allocationMutex );

	auto result = state.pointers.insert(
		AllocatedPointerMap::value_type( aAllocatedPointer.mData, aAllocatedPointer ) );
	if ( !result.second ) {
		state.totalMemoryUsage -= result.first->second.mMemory;
		result.first->second = aAllocatedPointer;
	}

	state.totalMemoryUsage += aAllocatedPointer.mMemory;

	if ( state.peakMemoryUsage < state.totalMemoryUsage ) {
		state.peakMemoryUsage = state.totalMemoryUsage;
	}

	if ( aAllocatedPointer.mMemory > state.biggestAllocation.mMemory ) {
		state.biggestAllocation = aAllocatedPointer;
	}

	if ( !aAllocatedPointer.mGlobalAllocation &&
		 aAllocatedPointer.mMemory > state.biggestNonAnonymousAllocation.mMemory ) {
		state.biggestNonAnonymousAllocation = aAllocatedPointer;
	}

	if ( aAllocatedPointer.mTrack )
		eePRINTL( "Allocating pointer %p at '%s' %d", aAllocatedPointer.mData,
				  aAllocatedPointer.mFile.c_str(), aAllocatedPointer.mLine );

	return aAllocatedPointer.mData;
}

void* MemoryManager::reallocPointer( void* data, const AllocatedPointer& aAllocatedPointer ) {
	MemoryManagerScope scope;
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

		if ( !aAllocatedPointer.mGlobalAllocation &&
			 aAllocatedPointer.mMemory > state.biggestNonAnonymousAllocation.mMemory ) {
			state.biggestNonAnonymousAllocation = aAllocatedPointer;
		}
	}

	return aAllocatedPointer.mData;
}

bool MemoryManager::removePointer( void* data, const char* file, const size_t& line ) {
	MemoryManagerScope scope;
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

bool MemoryManager::removePointerIfTracked( void* data ) {
	MemoryManagerScope scope;
	auto& state = getMemoryManagerState();
	Lock lock( state.allocationMutex );
	auto it = state.pointers.find( data );
	if ( it == state.pointers.end() )
		return false;
	state.totalMemoryUsage -= it->second.mMemory;
	state.pointers.erase( it );
	return true;
}

void* MemoryManager::allocateGlobal( size_t size, size_t alignment ) {
	size = size == 0 ? 1 : size;
	void* ptr = nullptr;
#if EE_PLATFORM == EE_PLATFORM_WIN
	ptr = _aligned_malloc( size, alignment );
#else
	if ( alignment <= alignof( std::max_align_t ) ) {
		ptr = malloc( size );
	} else if ( posix_memalign( &ptr, alignment, size ) != 0 ) {
		ptr = nullptr;
	}
#endif
	if ( ptr == nullptr )
		throw std::bad_alloc();
	if ( !insideMemoryManager ) {
		MemoryManagerScope scope;
		try {
			addPointer( AllocatedPointer( ptr, "<global new>", 0, size, false, true ) );
		} catch ( ... ) {
#if EE_PLATFORM == EE_PLATFORM_WIN
			_aligned_free( ptr );
#else
			free( ptr );
#endif
			throw;
		}
	}
	return ptr;
}

void MemoryManager::freeGlobal( void* ptr ) noexcept {
	if ( ptr == nullptr )
		return;
	if ( !insideMemoryManager )
		removePointerIfTracked( ptr );
#if EE_PLATFORM == EE_PLATFORM_WIN
	_aligned_free( ptr );
#else
	free( ptr );
#endif
}

size_t MemoryManager::getPeakMemoryUsage() {
	MemoryManagerScope scope;
	auto& state = getMemoryManagerState();
	Lock lock( state.allocationMutex );
	return state.peakMemoryUsage;
}

size_t MemoryManager::getTotalMemoryUsage() {
	MemoryManagerScope scope;
	auto& state = getMemoryManagerState();
	Lock lock( state.allocationMutex );
	return state.totalMemoryUsage;
}

AllocatedPointer MemoryManager::getBiggestAllocation() {
	MemoryManagerScope scope;
	auto& state = getMemoryManagerState();
	Lock lock( state.allocationMutex );
	return state.biggestAllocation;
}

AllocatedPointer MemoryManager::getBiggestNonAnonymousAllocation() {
	MemoryManagerScope scope;
	auto& state = getMemoryManagerState();
	Lock lock( state.allocationMutex );
	return state.biggestNonAnonymousAllocation;
}

void MemoryManager::showResults() {
#ifdef EE_MEMORY_MANAGER

	if ( EE::PrintDebugInLog ) {
		Log::destroySingleton();
		EE::PrintDebugInLog = false;
	}

	Engine::destroySingleton();
	MemoryManagerScope scope;
	auto& state = getMemoryManagerState();
	Lock lock( state.allocationMutex );
	size_t globalAllocationCount = 0;
	size_t globalAllocationMemory = 0;
	size_t leakCount = 0;
	for ( const auto& pointer : state.pointers ) {
		if ( pointer.second.mGlobalAllocation ) {
			++globalAllocationCount;
			globalAllocationMemory += pointer.second.mMemory;
		} else {
			++leakCount;
		}
	}

	eePRINTL( "\nMemory Manager Report" );
	eePRINTL( "=====================" );

	if ( leakCount == 0 ) {
		eePRINTL( "\nNo actionable memory leaks detected." );
	} else {
		eePRINTL( "\nActionable memory leaks: %zu\n", leakCount );
		tabulate::Table leaks;
		leaks.add_row( { "Address", "File", "Line", "Memory usage" } );
		for ( size_t column = 0; column < 4; ++column )
			leaks[0][column].format().font_style( { tabulate::FontStyle::bold } );

		for ( const auto& pointer : state.pointers ) {
			const AllocatedPointer& ap = pointer.second;
			if ( ap.mGlobalAllocation )
				continue;

			char address[2 + sizeof( void* ) * 2 + 1];
			snprintf( address, sizeof( address ), "%p", ap.mData );
			leaks.add_row( { address, ap.mFile, std::to_string( ap.mLine ),
							 FileSystem::sizeToString( static_cast<Int64>( ap.mMemory ) ) } );
		}

		eePRINTL( "%s", leaks.str().c_str() );
	}

	auto allocationLocation = []( const AllocatedPointer& allocation ) {
		return allocation.mData == nullptr
				   ? std::string( "-" )
				   : allocation.mFile + ":" + std::to_string( allocation.mLine );
	};

	tabulate::Table summary;
	summary.add_row( { "Metric", "Value", "Details" } );
	for ( size_t column = 0; column < 3; ++column )
		summary[0][column].format().font_style( { tabulate::FontStyle::bold } );
	summary.add_row( { "Anonymous allocations still live",
					   std::to_string( globalAllocationCount ) + " / " +
						   FileSystem::sizeToString( static_cast<Int64>( globalAllocationMemory ) ),
					   "Snapshot only; includes process-static and shared-library allocations" } );
	summary.add_row( { "Total memory still live",
					   FileSystem::sizeToString( static_cast<Int64>( state.totalMemoryUsage ) ),
					   "All tracked allocations" } );
	summary.add_row( { "Biggest non-anonymous allocation",
					   FileSystem::sizeToString( state.biggestNonAnonymousAllocation.mMemory ),
					   allocationLocation( state.biggestNonAnonymousAllocation ) } );
	summary.add_row( { "Biggest allocation overall",
					   FileSystem::sizeToString( state.biggestAllocation.mMemory ),
					   allocationLocation( state.biggestAllocation ) } );
	summary.add_row(
		{ "Peak memory usage", FileSystem::sizeToString( state.peakMemoryUsage ), "-" } );

	eePRINTL( "\nSummary\n-------\n%s\n", summary.str().c_str() );

#endif
}

} // namespace EE
