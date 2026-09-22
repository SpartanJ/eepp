#include "utest.h"
#include <atomic>
#include <eepp/core/memorymanager.hpp>
#include <thread>
#include <vector>

using namespace EE;

namespace {

struct AllocationProbe {
	AllocationProbe( int first, int second ) : value( first + second ) {}
	int value;
};

} // namespace

#ifdef EE_MEMORY_MANAGER
UTEST( MemoryManager, tracesPlainNew ) {
	const size_t before = MemoryManager::getTotalMemoryUsage();
	auto* value = new Uint64( 42 );
	const size_t allocated = MemoryManager::getTotalMemoryUsage();
	delete value;
	const size_t after = MemoryManager::getTotalMemoryUsage();

	EXPECT_GE( allocated, before + sizeof( Uint64 ) );
	EXPECT_EQ( after, before );
}

UTEST( MemoryManager, tracked_reallocation_updates_atomically ) {
	const size_t before = MemoryManager::getTotalMemoryUsage();
	auto* allocation = static_cast<Uint8*>( eeMalloc( 64 ) );
	allocation = static_cast<Uint8*>( eeRealloc( allocation, 256 ) );
	allocation[0] = 42;
	const size_t reallocated = MemoryManager::getTotalMemoryUsage();
	eeFree( allocation );
	const size_t after = MemoryManager::getTotalMemoryUsage();

	EXPECT_GE( reallocated, before + 256 );
	EXPECT_EQ( after, before );
}

UTEST( MemoryManager, concurrent_tracked_allocation_lifecycle ) {
	constexpr int ThreadCount = 4;
	constexpr int Iterations = 5000;
	std::atomic<bool> start{ false };
	std::vector<std::thread> workers;
	workers.reserve( ThreadCount );
	for ( int thread = 0; thread < ThreadCount; ++thread ) {
		workers.emplace_back( [&start, thread] {
			while ( !start.load( std::memory_order_acquire ) )
				std::this_thread::yield();
			for ( int iteration = 0; iteration < Iterations; ++iteration ) {
				size_t initialSize = 64 + static_cast<size_t>( ( iteration + thread ) % 4 ) * 16;
				auto* allocation = static_cast<Uint8*>( eeMalloc( initialSize ) );
				allocation = static_cast<Uint8*>( eeRealloc( allocation, initialSize + 64 ) );
				allocation[0] = static_cast<Uint8>( iteration );
				eeFree( allocation );
			}
		} );
	}
	start.store( true, std::memory_order_release );
	for ( auto& worker : workers )
		worker.join();

	EXPECT_TRUE( true );
}
#endif

UTEST( MemoryManager, supportsExpressionAndLegacyNewSyntax ) {
	const size_t before = MemoryManager::getTotalMemoryUsage();
	auto* expression = eeNew( AllocationProbe( 20, 22 ) );
	auto* legacy = eeNew( AllocationProbe, ( 19, 23 ) );
	const bool valuesAreValid = expression->value == 42 && legacy->value == 42;
	eeDelete( expression );
	eeDelete( legacy );
	const size_t after = MemoryManager::getTotalMemoryUsage();

	EXPECT_TRUE( valuesAreValid );
	EXPECT_EQ( after, before );
}
