#include "utest.h"
#include <eepp/core/memorymanager.hpp>
#include <eepp/ui/css/idnamemap.hpp>
#include <eepp/ui/css/propertyidset.hpp>
#include <vector>

using namespace EE;
using namespace EE::UI::CSS;

namespace {

enum class TestDenseId : Uint8 {
	Invalid = 0,
	One,
	Two,
	FirstCustomId,
	MaxNumIds = 5,
};

} // namespace

static PropertyId id( std::size_t value ) {
	return static_cast<PropertyId>( value );
}

UTEST( PropertyIdSet, setOperations ) {
	PropertyIdSet first;
	first.insert( id( 30 ) );
	first.insert( id( 10 ) );
	first.insert( id( 20 ) );
	first.insert( id( 20 ) );

	PropertyIdSet second;
	second.insert( id( 20 ) );
	second.insert( id( 40 ) );

	const PropertyIdSet setUnion = first | second;
	const PropertyIdSet intersection = first & second;

	EXPECT_EQ( first.size(), 3u );
	EXPECT_TRUE( setUnion.contains( id( 10 ) ) );
	EXPECT_TRUE( setUnion.contains( id( 20 ) ) );
	EXPECT_TRUE( setUnion.contains( id( 30 ) ) );
	EXPECT_TRUE( setUnion.contains( id( 40 ) ) );
	EXPECT_EQ( intersection.size(), 1u );
	EXPECT_TRUE( intersection.contains( id( 20 ) ) );

	PropertyIdSet sameAsFirst;
	sameAsFirst.insert( id( 20 ) );
	sameAsFirst.insert( id( 30 ) );
	sameAsFirst.insert( id( 10 ) );
	EXPECT_TRUE( first == sameAsFirst );

	first &= second;
	EXPECT_TRUE( first == intersection );
	first.erase( id( 20 ) );
	EXPECT_TRUE( first.empty() );
}

UTEST( PropertyIdSet, selfUnionAndIntersection ) {
	PropertyIdSet a;
	a.insert( id( 7 ) );
	a.insert( id( 9 ) );
	const PropertyIdSet original = a;
	a |= a;
	EXPECT_TRUE( a == original );
	a &= a;
	EXPECT_TRUE( a == original );
	EXPECT_EQ( a.size(), 2u );
}

UTEST( PropertyIdSet, iteratorErase ) {
	PropertyIdSet ids;
	ids.insert( id( 10 ) );
	ids.insert( id( 20 ) );
	ids.insert( id( 30 ) );

	for ( auto it = ids.begin(); it != ids.end(); ) {
		if ( *it == id( 20 ) )
			it = ids.erase( it );
		else
			++it;
	}

	EXPECT_EQ( ids.size(), 2u );
	EXPECT_FALSE( ids.contains( id( 20 ) ) );
	EXPECT_TRUE( ids.contains( id( 10 ) ) );
	EXPECT_TRUE( ids.contains( id( 30 ) ) );
}

UTEST( PropertyIdSet, iteratorEraseEveryElement ) {
	PropertyIdSet ids;
	for ( std::size_t i = 1; i <= 10; ++i )
		ids.insert( id( i ) );

	for ( auto it = ids.begin(); it != ids.end(); ) {
		it = ids.erase( it );
	}

	EXPECT_TRUE( ids.empty() );
}

UTEST( PropertyIdSet, ascendingIteration ) {
	PropertyIdSet ids;
	ids.insert( id( 40 ) );
	ids.insert( id( 10 ) );
	ids.insert( id( 30 ) );
	ids.insert( id( 20 ) );

	std::vector<PropertyId> seen;
	for ( auto prop : ids )
		seen.push_back( prop );

	ASSERT_EQ( seen.size(), 4u );
	EXPECT_TRUE( seen[0] == id( 10 ) );
	EXPECT_TRUE( seen[1] == id( 20 ) );
	EXPECT_TRUE( seen[2] == id( 30 ) );
	EXPECT_TRUE( seen[3] == id( 40 ) );
}

UTEST( PropertyIdSet, invalidAndBoundaryBehavior ) {
	PropertyIdSet ids;
	ids.insert( PropertyId::Invalid );
	ids.erase( PropertyId::Invalid );
	EXPECT_FALSE( ids.contains( PropertyId::Invalid ) );
	EXPECT_TRUE( ids.empty() );

	ids.insert( id( 511 ) );
	EXPECT_TRUE( ids.contains( id( 511 ) ) );
	ids.erase( id( 511 ) );
	EXPECT_TRUE( ids.empty() );
}

UTEST( PropertyIdSet, clearedSetReusesStorage ) {
	PropertyIdSet ids;
	for ( std::size_t i = 1; i <= 16; ++i )
		ids.insert( id( i ) );
	ids.clear();

	const size_t memoryBefore = MemoryManager::getTotalMemoryUsage();
	for ( std::size_t i = 1; i <= 16; ++i )
		ids.insert( id( i ) );
	const size_t memoryAfter = MemoryManager::getTotalMemoryUsage();

	EXPECT_EQ( ids.size(), 16u );
	EXPECT_EQ( memoryAfter, memoryBefore );
}

UTEST( PropertyIdSet, noAllocationOnConstruction ) {
	const size_t memoryBefore = MemoryManager::getTotalMemoryUsage();
	PropertyIdSet ids;
	const size_t memoryAfter = MemoryManager::getTotalMemoryUsage();
	EXPECT_EQ( memoryAfter, memoryBefore );
	EXPECT_TRUE( ids.empty() );
}

UTEST( PropertyIdSet, copyAndMovePreserveBits ) {
	PropertyIdSet source;
	source.insert( id( 3 ) );
	source.insert( id( 8 ) );

	const size_t memoryBefore = MemoryManager::getTotalMemoryUsage();
	PropertyIdSet copy( source );
	PropertyIdSet moved( std::move( source ) );
	const size_t memoryAfter = MemoryManager::getTotalMemoryUsage();

	EXPECT_EQ( memoryAfter, memoryBefore );
	EXPECT_EQ( copy.size(), 2u );
	EXPECT_EQ( moved.size(), 2u );
	EXPECT_TRUE( copy == moved );
	EXPECT_TRUE( copy.contains( id( 3 ) ) );
	EXPECT_TRUE( copy.contains( id( 8 ) ) );
}

UTEST( PropertyIdSet, customIdsBehaveLikeBuiltins ) {
	PropertyIdSet ids;
	ids.insert( id( 400 ) );
	ids.insert( id( 401 ) );
	EXPECT_TRUE( ids.contains( id( 400 ) ) );
	EXPECT_TRUE( ids.contains( id( 401 ) ) );
	EXPECT_EQ( ids.size(), 2u );

	PropertyIdSet other;
	other.insert( id( 401 ) );
	other.insert( id( 500 ) );
	const PropertyIdSet result = ids | other;
	EXPECT_TRUE( result.contains( id( 400 ) ) );
	EXPECT_TRUE( result.contains( id( 401 ) ) );
	EXPECT_TRUE( result.contains( id( 500 ) ) );
}

UTEST( PropertyIdSet, objectSizeIsFixed ) {
	EXPECT_EQ( sizeof( PropertyIdSet ), 64u );
}

UTEST( PropertyIdSet, checkedCompatibilityAliasUsesSelectedIdentity ) {
	EXPECT_TRUE( PropertyId::Checked == PropertyId::Selected );
}

UTEST( IdNameMap, builtinsAliasesAndCustomIds ) {
	IdNameMap<TestDenseId, 5> ids;
	EXPECT_TRUE( ids.addBuiltin( TestDenseId::One, "one" ) );
	EXPECT_TRUE( ids.addBuiltin( TestDenseId::Two, "two" ) );
	EXPECT_TRUE( ids.addAlias( "first", TestDenseId::One ) );
	EXPECT_TRUE( ids.finalizeBuiltins() );

	EXPECT_TRUE( ids.getId( "one" ) == TestDenseId::One );
	EXPECT_TRUE( ids.getId( "first" ) == TestDenseId::One );
	EXPECT_TRUE( ids.getName( TestDenseId::One ) == "one" );
	EXPECT_TRUE( ids.getId( "unknown" ) == TestDenseId::Invalid );

	const TestDenseId customOne = ids.getOrCreateId( "custom-one" );
	const TestDenseId customTwo = ids.getOrCreateId( "custom-two" );
	EXPECT_EQ( static_cast<Uint8>( customOne ), 3u );
	EXPECT_EQ( static_cast<Uint8>( customTwo ), 4u );
	EXPECT_TRUE( ids.getOrCreateId( "custom-one" ) == customOne );
	EXPECT_TRUE( ids.getOrCreateId( "over-capacity" ) == TestDenseId::Invalid );
	EXPECT_FALSE( ids.addBuiltin( static_cast<TestDenseId>( 4 ), "late-builtin" ) );
}

UTEST( IdNameMap, failedFinalizationDoesNotEnableCustomIds ) {
	IdNameMap<TestDenseId, 5> ids;
	EXPECT_TRUE( ids.addBuiltin( TestDenseId::One, "one" ) );
	EXPECT_FALSE( ids.finalizeBuiltins() );
	EXPECT_TRUE( ids.getOrCreateId( "too-early" ) == TestDenseId::Invalid );
	EXPECT_TRUE( ids.addBuiltin( TestDenseId::Two, "two" ) );
	EXPECT_TRUE( ids.finalizeBuiltins() );
	EXPECT_EQ( static_cast<Uint8>( ids.getOrCreateId( "custom" ) ), 3u );
}
