#ifndef EE_SYSTEMCIOSTREAM_HPP
#define EE_SYSTEMCIOSTREAM_HPP

#include <eepp/system/base.hpp>

namespace EE {
	typedef std::streamsize ios_size;
}

namespace EE { namespace System {

/** @brief An abstraction for custom input/output stream files. */
class EE_API cIOStream {
	public:
		virtual ~cIOStream() {}

		/** @brief Read data from the stream
		**	@param data Buffer where to copy the read data
		**	@param size Desired number of bytes to read
		**	@return The number of bytes actually read */
		virtual ios_size Read( char * data, ios_size size ) = 0;

		/** @brief Write data to the virtual file
		**	@param data Data to write in the file
		**	@param size Size of the data that needs to be writed */
		virtual ios_size Write( const char * data, ios_size size ) = 0;

		/**	@brief Change the current reading position
		**	@param position The position to seek to, from the beginning
		**	@return The position actually sought to. */
		virtual ios_size Seek( ios_size position ) = 0;

		/** @brief Get the current reading position in the stream
		**	@return The current position, or -1 on error. */
		virtual ios_size Tell() = 0;

		/**	@brief Return the size of the stream
		**	@return The total number of bytes available in the stream */
		virtual ios_size GetSize() = 0;

		/** @return If the virtual stream file is open */
		virtual bool IsOpen() = 0;
};

}}

#endif
