#include <eepp/system/safedatapointer.hpp>
#include <eepp/base/memorymanager.hpp>
#include <cstddef>

namespace EE { namespace System {

SafeDataPointer::SafeDataPointer() :
	Data( NULL ),
	DataSize( 0 )
{
}

SafeDataPointer::~SafeDataPointer() {
	eeSAFE_DELETE_ARRAY( Data );
}

}}
