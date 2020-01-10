#ifndef EE_SYS_IOSTREAMPAK_HPP
#define EE_SYS_IOSTREAMPAK_HPP

#include <eepp/system/iostream.hpp>
#include <eepp/system/pak.hpp>

namespace EE { namespace System {

class IOStreamFile;

/** @brief An implementation for a zip file steam */
class EE_API IOStreamPak : public IOStream {
  public:
	static IOStreamPak* New( Pak* pack, const std::string& path, bool writeMode = false );

	/** @brief Open a file from a zip file
	**	@param pack Pack to open from path
	**	@param path Path of the file in the pack file
	**	@param writeMode Set true if the PAK is in write mode
	**/
	IOStreamPak( Pak* pack, const std::string& path, bool writeMode = false );

	virtual ~IOStreamPak();

	ios_size read( char* data, ios_size size );

	ios_size write( const char* data, ios_size size );

	ios_size seek( ios_size position );

	ios_size tell();

	ios_size getSize();

	bool isOpen();

  protected:
	IOStreamFile* mFile;
	Pak::pakEntry mEntry;
	Int32 mPos;
	bool mOpen;
};

}} // namespace EE::System

#endif
