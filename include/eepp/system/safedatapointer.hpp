#ifndef EE_SYSTEM_SAFEDATAPOINTER
#define EE_SYSTEM_SAFEDATAPOINTER

#include <eepp/declares.hpp>

namespace EE { namespace System {

/** @brief Keep a pointer and release it in the SafeDataPointer destructor */
class EE_API SafeDataPointer {
	public:
		SafeDataPointer();

		SafeDataPointer( Uint8 * data, Uint32 size );

		~SafeDataPointer();

		Uint8 * Data;
		Uint32	DataSize;
};

}}

#endif
