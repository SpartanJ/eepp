#ifndef EE_UTILS_SAFEDATAPOINTER
#define EE_UTILS_SAFEDATAPOINTER

#include <eepp/utils/base.hpp>

namespace EE { namespace Utils {

class EE_API SafeDataPointer {
	public:
		SafeDataPointer();

		~SafeDataPointer();

		Uint8 * Data;
		Uint32	DataSize;
};

}}

#endif
