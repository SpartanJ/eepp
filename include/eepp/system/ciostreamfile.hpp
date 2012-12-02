#ifndef EE_SYSTEMCIOSTREAMFILE_HPP
#define EE_SYSTEMCIOSTREAMFILE_HPP

#include <eepp/system/ciostream.hpp>
#include <iostream>
#include <fstream>

namespace EE { namespace System {

class EE_API cIOStreamFile : public cIOStream {
	public:
		cIOStreamFile( const std::string& path, std::ios_base::openmode mode = std::ios::in | std::ios::binary );

		virtual ~cIOStreamFile();

		ios_size Read( char * data, ios_size size );

		ios_size Write( const char * data, ios_size size );

		ios_size Seek( ios_size position );

		ios_size Tell();

		ios_size GetSize();

		bool IsOpen();
	protected:
		std::fstream	mFS;
		ios_size		mSize;
};

}}

#endif
