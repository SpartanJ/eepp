#ifndef EE_SYSTEMCIOSTREAMMEMORY_HPP
#define EE_SYSTEMCIOSTREAMMEMORY_HPP

#include <eepp/system/iostream.hpp>

namespace EE { namespace System {

/** @brief Implementation of a memory stream file */
class EE_API IOStreamMemory : public IOStream {
	public:
		static IOStreamMemory * New( const char * data, ios_size size );

		static IOStreamMemory * New( char * data, ios_size size );

		/** @brief Use the data buffer for reading
		**	@param data The buffer to read from
		**	@param size The size of the buffer
		*/
		IOStreamMemory( const char * data, ios_size size );

		/** @brief Use the data buffer for writing
		**	@param data The buffer to write to
		**	@param size The size of the buffer
		*/
		IOStreamMemory( char * data, ios_size size );

		virtual ~IOStreamMemory();

		ios_size read( char * data, ios_size size );

		ios_size write( const char * data, ios_size size );

		ios_size seek( ios_size position );

		ios_size tell();

		ios_size getSize();

		bool isOpen();
	protected:
		const char *	mReadPtr;
		char *			mWritePtr;
		ios_size		mPos;
		ios_size		mSize;
};

}}

#endif
