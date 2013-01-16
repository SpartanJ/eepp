#ifndef EE_SYSTEMCIOSTREAMMEMORY_HPP
#define EE_SYSTEMCIOSTREAMMEMORY_HPP

#include <eepp/system/ciostream.hpp>

namespace EE { namespace System {

/** @brief Implementation of a memory stream file */
class EE_API cIOStreamMemory : public cIOStream {
	public:
		/** @brief Use the data buffer for reading
		**	@param data The buffer to read from
		**	@param size The size of the buffer
		*/
		cIOStreamMemory( const char * data, ios_size size );

		/** @brief Use the data buffer for writing
		**	@param data The buffer to write to
		**	@param size The size of the buffer
		*/
		cIOStreamMemory( char * data, ios_size size );

		virtual ~cIOStreamMemory();

		ios_size Read( char * data, ios_size size );

		ios_size Write( const char * data, ios_size size );

		ios_size Seek( ios_size position );

		ios_size Tell();

		ios_size GetSize();

		bool IsOpen();
	protected:
		const char *	mReadPtr;
		char *			mWritePtr;
		ios_size		mPos;
		ios_size		mSize;
};

}}

#endif
