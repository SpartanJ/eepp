#include <eepp/system/safedatapointer.hpp>
#include <eepp/core/memorymanager.hpp>
#include <cstddef>

namespace EE { namespace System {

SafeDataPointer::SafeDataPointer() :
	data( NULL ),
	size( 0 )
{
}

SafeDataPointer::SafeDataPointer( Uint32 size ) :
	data( eeNewArray( Uint8, size ) ),
	size( size )
{
}

SafeDataPointer::SafeDataPointer( Uint8 *data, Uint32 size ) :
	data( data ),
	size( size )
{
}

SafeDataPointer::~SafeDataPointer() {
	clear();
}

void SafeDataPointer::clear() {
	eeSAFE_DELETE_ARRAY( data );
}

}}
