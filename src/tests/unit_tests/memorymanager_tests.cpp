#include "utest.h"
#include <eepp/core/memorymanager.hpp>

using namespace EE;

namespace {

struct AllocationProbe {
	AllocationProbe( int first, int second ) : value( first + second ) {}
	int value;
};

} // namespace

UTEST( MemoryManager, tracesPlainNew ) {
	const size_t before = MemoryManager::getTotalMemoryUsage();
	auto* value = new Uint64( 42 );
	const size_t allocated = MemoryManager::getTotalMemoryUsage();
	delete value;
	const size_t after = MemoryManager::getTotalMemoryUsage();

	EXPECT_GE( allocated, before + sizeof( Uint64 ) );
	EXPECT_EQ( after, before );
}

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
