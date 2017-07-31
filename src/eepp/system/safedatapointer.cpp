#include <eepp/system/safedatapointer.hpp>
#include <eepp/core/memorymanager.hpp>
#include <cstddef>

namespace EE { namespace System {

SafeDataPointer::SafeDataPointer() :
	Data( NULL ),
	DataSize( 0 )
{
}

SafeDataPointer::SafeDataPointer( Uint8 *data, Uint32 size ) :
	Data( data ),
	DataSize( size )
{
}

SafeDataPointer::~SafeDataPointer() {
	clear();
}

void SafeDataPointer::clear() {
	eeSAFE_DELETE_ARRAY( Data );
}

}}
