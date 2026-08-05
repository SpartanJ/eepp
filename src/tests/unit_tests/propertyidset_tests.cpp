#include "utest.h"
#include <eepp/core/memorymanager.hpp>
#include <eepp/ui/css/propertyidset.hpp>

using namespace EE;
using namespace EE::UI::CSS;

UTEST( PropertyIdSet, setOperations ) {
	PropertyIdSet first;
	first.insert( 30 );
	first.insert( 10 );
	first.insert( 20 );
	first.insert( 20 );

	PropertyIdSet second;
	second.insert( 20 );
	second.insert( 40 );

	const PropertyIdSet setUnion = first | second;
	const PropertyIdSet intersection = first & second;

	EXPECT_EQ( first.size(), 3u );
	EXPECT_TRUE( setUnion.contains( 10 ) );
	EXPECT_TRUE( setUnion.contains( 20 ) );
	EXPECT_TRUE( setUnion.contains( 30 ) );
	EXPECT_TRUE( setUnion.contains( 40 ) );
	EXPECT_EQ( intersection.size(), 1u );
	EXPECT_TRUE( intersection.contains( 20 ) );

	PropertyIdSet sameAsFirst;
	sameAsFirst.insert( 20 );
	sameAsFirst.insert( 30 );
	sameAsFirst.insert( 10 );
	EXPECT_TRUE( first == sameAsFirst );

	first &= second;
	EXPECT_TRUE( first == intersection );
	first.erase( 20 );
	EXPECT_TRUE( first.empty() );
}

UTEST( PropertyIdSet, iteratorErase ) {
	PropertyIdSet ids;
	ids.insert( 10 );
	ids.insert( 20 );
	ids.insert( 30 );

	for ( auto it = ids.begin(); it != ids.end(); ) {
		if ( *it == 20 )
			it = ids.erase( it );
		else
			++it;
	}

	EXPECT_EQ( ids.size(), 2u );
	EXPECT_FALSE( ids.contains( 20 ) );
}

UTEST( PropertyIdSet, clearedSetReusesStorage ) {
	PropertyIdSet ids;
	for ( Uint32 id = 0; id < 16; ++id )
		ids.insert( id );
	ids.clear();

	const size_t memoryBefore = MemoryManager::getTotalMemoryUsage();
	for ( Uint32 id = 0; id < 16; ++id )
		ids.insert( id );
	const size_t memoryAfter = MemoryManager::getTotalMemoryUsage();

	EXPECT_EQ( ids.size(), 16u );
	EXPECT_EQ( memoryAfter, memoryBefore );
}
