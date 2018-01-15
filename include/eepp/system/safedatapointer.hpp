#ifndef EE_SYSTEM_SAFEDATAPOINTER
#define EE_SYSTEM_SAFEDATAPOINTER

#include <eepp/config.hpp>

namespace EE { namespace System {

/** @brief Keep a pointer and release it in the SafeDataPointer destructor */
class EE_API SafeDataPointer {
	public:
		SafeDataPointer();

		SafeDataPointer( Uint8 * data, Uint32 size );

		/** @brief The destructor deletes the buffer */
		~SafeDataPointer();

		void clear();

		/** Pointer to the buffer */
		Uint8 * Data;

		/** Buffer size */
		Uint32	DataSize;
};

}}

#endif
