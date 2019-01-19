#ifndef EE_SYSTEM_SAFEDATAPOINTER
#define EE_SYSTEM_SAFEDATAPOINTER

#include <eepp/config.hpp>
#include <eepp/core/memorymanager.hpp>
#include <cstddef>

namespace EE { namespace System {

/** @brief Keep a pointer and release it in the SafeDataPointer destructor */
template <typename T>
class TSafeDataPointer {
	public:
		TSafeDataPointer();

		TSafeDataPointer( Uint32 size );

		TSafeDataPointer( T * data, Uint32 size );

		/** @brief The destructor deletes the buffer */
		~TSafeDataPointer();

		void clear();

		/** Pointer to the buffer */
		T * data;

		/** Buffer size */
		Uint32	size;
};


template <typename T>
TSafeDataPointer<T>::TSafeDataPointer() :
	data( NULL ),
	size( 0 )
{
}

template <typename T>
TSafeDataPointer<T>::TSafeDataPointer( Uint32 size ) :
	data( eeNewArray( T, size ) ),
	size( size )
{
}

template <typename T>
TSafeDataPointer<T>::TSafeDataPointer( T * data, Uint32 size ) :
	data( data ),
	size( size )
{
}

template <typename T>
TSafeDataPointer<T>::~TSafeDataPointer() {
	clear();
}

template <typename T>
void TSafeDataPointer<T>::clear() {
	eeSAFE_DELETE_ARRAY( data );
}

typedef TSafeDataPointer<Uint8> SafeDataPointer;

}}

#endif
