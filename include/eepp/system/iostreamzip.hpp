#ifndef EE_SYS_IOSTREAMZIP_HPP
#define EE_SYS_IOSTREAMZIP_HPP

#include <eepp/system/iostream.hpp>

struct zip;
struct zip_file;

namespace EE { namespace System {
class Zip;

/** @brief An implementation for a zip file steam */
class EE_API IOStreamZip: public IOStream {
	public:
		static IOStreamZip * New( Zip * pack, const std::string& path );

		/** @brief Open a file from a zip file
		**	@param pack Pack to open from path
		**	@param path Path of the file in the pack file
		**/
		IOStreamZip( Zip * pack, const std::string& path );

		virtual ~IOStreamZip();

		ios_size read( char * data, ios_size size );

		ios_size write( const char * data, ios_size size );

		ios_size seek( ios_size position );

		ios_size tell();

		ios_size getSize();

		bool isOpen();
	protected:
		std::string mPath;
		struct zip * mZip;
		struct zip_file * mFile;
		ios_size mPos;
};

}}

#endif
