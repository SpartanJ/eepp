#ifndef EE_SYS_IOSTREAMZIP_HPP
#define EE_SYS_IOSTREAMZIP_HPP

#include <eepp/system/iostream.hpp>

#include <string>

struct zip;
struct zip_file;

namespace EE { namespace System {
class Zip;

/** @brief An implementation for a zip file steam */
class EE_API IOStreamZip : public IOStream {
  public:
	static IOStreamZip* New( Zip* pack, const std::string& path );

	/** @brief Open a file from a zip file. The pack must outlive the stream.
	**	@param pack Pack to open from path
	**	@param path Path of the file in the pack file
	**/
	IOStreamZip( Zip* pack, const std::string& path );

	IOStreamZip( Zip* pack, Uint64 index );

	virtual ~IOStreamZip();

	ios_size read( char* data, ios_size size );

	ios_size write( const char* data, ios_size size );

	ios_size seek( ios_size position );

	ios_size tell();

	ios_size getSize();

	bool isOpen();

  protected:
	struct zip* mZip;
	struct zip_file* mFile;
	Uint64 mIndex;
	ios_size mSize;
	ios_size mPos;
};

}} // namespace EE::System

#endif
