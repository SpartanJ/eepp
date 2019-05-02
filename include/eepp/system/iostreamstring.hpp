#ifndef EE_SYSTEM_IOSTREAMSTRING_HPP
#define EE_SYSTEM_IOSTREAMSTRING_HPP

#include <eepp/system/iostream.hpp>
#include <cstring>

namespace EE { namespace System {

/** @brief Implementation of a memory stream file using an std::string as a container */
class EE_API IOStreamString : public IOStream {
	public:
		IOStreamString();

		ios_size read( char * data, ios_size size );

		ios_size write( const char * data, ios_size size );

		ios_size write( const std::string& string );

		ios_size seek( ios_size position );

		ios_size tell();

		ios_size getSize();

		bool isOpen();

		void clear();

		/** @return Pointer to the current position in the stream */
		const char * getPositionPointer();

		/** @return The pointer to the beggining of the stream */
		const char * getStreamPointer() const;

		const std::string& getStream() const;
	protected:
		std::string mStream;
		ios_size mPos;
};

}}

#endif // EE_SYSTEM_IOSTREAMSTRING_HPP
