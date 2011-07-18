#ifndef EE_SYSTEMCIOSTREAMFILE_HPP
#define EE_SYSTEMCIOSTREAMFILE_HPP

#include "ciostream.hpp"

namespace EE { namespace System {

class cIOStreamFile : public cIOStream {
	public:
		cIOStreamFile( const std::string& path, std::ios_base::openmode mode );

		virtual ~cIOStreamFile();

		ios_size Read( char * data, ios_size size );

		ios_size Write( const char * data, ios_size size );

		ios_size Seek( ios_size position );

		ios_size GetPosition();

		ios_size GetSize();

		bool IsOpen();
	protected:
		std::fstream	mFS;
};

}}

#endif
