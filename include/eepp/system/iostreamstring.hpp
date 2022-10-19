#ifndef EE_SYSTEM_IOSTREAMSTRING_HPP
#define EE_SYSTEM_IOSTREAMSTRING_HPP

#include <cstring>
#include <eepp/system/iostream.hpp>

namespace EE { namespace System {

/** @brief Implementation of a memory stream file using an std::string as a container */
class EE_API IOStreamString : public IOStream {
  public:
	IOStreamString();

	IOStreamString( const std::string& filepath );

	virtual ios_size read( char* data, ios_size size );

	virtual ios_size write( const char* data, ios_size size );

	virtual ios_size write( const std::string& string );

	virtual ios_size seek( ios_size position );

	virtual ios_size tell();

	virtual ios_size getSize();

	virtual bool isOpen();

	void clear();

	/** @return Pointer to the current position in the stream */
	const char* getPositionPointer();

	/** @return The pointer to the beggining of the stream */
	const char* getStreamPointer() const;

	const std::string& getStream() const;

  protected:
	std::string mStream;
	ios_size mPos{ 0 };
};

}} // namespace EE::System

#endif // EE_SYSTEM_IOSTREAMSTRING_HPP
