#ifndef EE_SYSTEMCIOSTREAMMEMORY_HPP
#define EE_SYSTEMCIOSTREAMMEMORY_HPP

#include <eepp/system/ciostream.hpp>

namespace EE { namespace System {

class EE_API cIOStreamMemory : public cIOStream {
	public:
		cIOStreamMemory( const char * data, ios_size size );

		cIOStreamMemory( char * data, ios_size size );

		virtual ~cIOStreamMemory();

		ios_size Read( char * data, ios_size size );

		ios_size Write( const char * data, ios_size size );

		ios_size Seek( ios_size position );

		ios_size Tell();

		ios_size GetSize();

		bool IsOpen();
	protected:
		const char *	mReadPtr;
		char *			mWritePtr;
		ios_size		mPos;
		ios_size		mSize;
};

}}

#endif
