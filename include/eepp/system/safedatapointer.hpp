#ifndef EE_SYSTEM_SAFEDATAPOINTER
#define EE_SYSTEM_SAFEDATAPOINTER

#include <eepp/declares.hpp>

namespace EE { namespace System {

class EE_API SafeDataPointer {
	public:
		SafeDataPointer();

		~SafeDataPointer();

		Uint8 * Data;
		Uint32	DataSize;
};

}}

#endif
