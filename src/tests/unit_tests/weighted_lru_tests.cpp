#include "utest.hpp"
#include <eepp/core/lrucache.hpp>
#include <memory>

using namespace EE;

UTEST( WeightedLRU, BudgetRecencyAndReplacementUseWeightsRatherThanEntryCount ) {
	WeightedLRUCache<int, int> cache( 10 );
	EXPECT_TRUE( cache.put( 1, 100, 4 ) );
	EXPECT_TRUE( cache.put( 2, 200, 4 ) );
	ASSERT_TRUE( cache.get( 1 ) ); // Make 2 least recently used.
	EXPECT_TRUE( cache.put( 3, 300, 5 ) );
	EXPECT_FALSE( cache.get( 2 ) );
	EXPECT_EQ( cache.weight(), size_t{ 9 } );
	EXPECT_EQ( cache.size(), size_t{ 2 } );
	EXPECT_TRUE( cache.put( 1, 101, 9 ) );
	EXPECT_FALSE( cache.get( 3 ) );
	ASSERT_TRUE( cache.get( 1 ) );
	EXPECT_EQ( *cache.get( 1 ), 101 );
	EXPECT_EQ( cache.weight(), size_t{ 9 } );
	EXPECT_FALSE( cache.put( 1, 102, 11 ) );
	EXPECT_EQ( cache.weight(), size_t{ 0 } );
	EXPECT_EQ( cache.size(), size_t{ 0 } );
	EXPECT_FALSE( cache.put( 4, 400, 0 ) );
}

UTEST( WeightedLRU, EvictionReleasesSharedOwnershipAndClearResetsTheBudget ) {
	WeightedLRUCache<int, std::shared_ptr<int>> cache( 8 );
	auto image = std::make_shared<int>( 42 );
	std::weak_ptr<int> weak = image;
	cache.put( 1, std::move( image ), 8 );
	EXPECT_FALSE( weak.expired() );
	cache.put( 2, std::make_shared<int>( 24 ), 1 );
	EXPECT_TRUE( weak.expired() );
	cache.clear();
	EXPECT_EQ( cache.weight(), size_t{ 0 } );
	EXPECT_FALSE( cache.get( 2 ) );
	WeightedLRUCache<int, int> disabled( 0 );
	EXPECT_FALSE( disabled.put( 1, 1, 1 ) );
}

UTEST( WeightedLRU, RelocationAndVectorGrowthMatchARecencyReference ) {
	WeightedLRUCache<int, int> cache( 67 );
	struct Value {
		int key, value;
		size_t weight;
	};
	std::vector<Value> reference;
	Uint32 random = 7;
	for ( int i = 0; i < 20000; ++i ) {
		random = random * 1664525u + 1013904223u;
		const int key = ( random >> 8 ) % 31;
		auto found = std::find_if( reference.begin(), reference.end(),
								   [key]( const auto& value ) { return value.key == key; } );
		const int operation = random % 4;
		if ( operation < 2 ) {
			if ( found != reference.end() )
				reference.erase( found );
			const size_t weight = ( random >> 16 ) % 17 + 1;
			size_t total = 0;
			for ( const auto& entry : reference )
				total += entry.weight;
			while ( total + weight > 67 ) {
				total -= reference.back().weight;
				reference.pop_back();
			}
			reference.insert( reference.begin(), Value{ key, i, weight } );
			EXPECT_TRUE( cache.put( key, i, weight ) );
		} else if ( operation == 2 ) {
			const auto actual = cache.get( key );
			EXPECT_TRUE( actual.has_value() == ( found != reference.end() ) );
			if ( found != reference.end() ) {
				EXPECT_EQ( *actual, found->value );
				const auto value = *found;
				reference.erase( found );
				reference.insert( reference.begin(), value );
			}
		} else {
			EXPECT_TRUE( cache.erase( key ) == ( found != reference.end() ) );
			if ( found != reference.end() )
				reference.erase( found );
		}
		size_t total = 0;
		for ( const auto& entry : reference )
			total += entry.weight;
		EXPECT_EQ( cache.size(), reference.size() );
		EXPECT_EQ( cache.weight(), total );
	}
}
