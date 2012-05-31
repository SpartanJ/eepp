#ifndef EE_SYSTEMCIOSTREAM_HPP
#define EE_SYSTEMCIOSTREAM_HPP

#include "base.hpp"

namespace EE {
	typedef std::streamsize ios_size;
}

namespace EE { namespace System {

class EE_API cIOStream {
	public:
		virtual ~cIOStream() {}

		virtual ios_size Read( char * data, ios_size size ) = 0;

		virtual ios_size Write( const char * data, ios_size size ) = 0;

		virtual ios_size Seek( ios_size position ) = 0;

		virtual ios_size GetPosition() = 0;

		virtual ios_size GetSize() = 0;

		virtual bool IsOpen() = 0;
};

}}

#endif
