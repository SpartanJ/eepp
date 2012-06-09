#include <eepp/utils/safedatapointer.hpp>

namespace EE { namespace Utils {

SafeDataPointer::SafeDataPointer() :
	Data( NULL ),
	DataSize( 0 )
{
}

SafeDataPointer::~SafeDataPointer() {
	eeSAFE_DELETE_ARRAY( Data );
}

}}
