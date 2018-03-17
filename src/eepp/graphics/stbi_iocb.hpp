#ifndef EE_STBI_IOCB_HPP
#define EE_STBI_IOCB_HPP

#include <eepp/system/iostream.hpp>
using namespace EE::System;

namespace IOCb
{
	// stb_image callbacks that operate on a IOStream
	static inline int read(void* user, char* data, int size)
	{
		IOStream * stream = static_cast<IOStream*>(user);
		return static_cast<int>(stream->read(data, size));
	}

	static inline void skip(void* user, int size)
	{
		IOStream * stream = static_cast<IOStream*>(user);
		stream->seek(stream->tell() + size);
	}

	static inline int eof(void* user)
	{
		IOStream* stream = static_cast<IOStream*>(user);
		return stream->tell() >= stream->getSize();
	}
}

#endif
