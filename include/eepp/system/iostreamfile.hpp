#ifndef EE_SYSTEMCIOSTREAMFILE_HPP
#define EE_SYSTEMCIOSTREAMFILE_HPP

#include <eepp/system/iostream.hpp>
#include <string>

namespace EE { namespace System {

/** @brief An implementation for a file system file steam */
class EE_API IOStreamFile : public IOStream {
  public:
	static IOStreamFile* New( const std::string& path, const std::string& modes = "rb" );

	/** @brief Open a file from the file system
	**	@param path File to open from path
	**	@param modes The open mode that it will be used for the file ( default read-binary )
	**/
	IOStreamFile( const std::string& path, const std::string& modes = "rb" );

	virtual ~IOStreamFile();

	ios_size read( char* data, ios_size size );

	ios_size write( const char* data, ios_size size );

	ios_size seek( ios_size position );

	ios_size tell();

	ios_size getSize();

	bool isOpen();

	/** @brief Synchronizes the buffer associated with the stream to its controlled output sequence.
	** This effectively means that all unwritten characters in the buffer are written to its
	*controlled output sequence as soon as possible. */
	void flush();

	void close();

  protected:
	std::FILE* mFS;
	ios_size mSize;
};

}} // namespace EE::System

#endif
