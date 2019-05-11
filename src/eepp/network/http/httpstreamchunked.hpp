#ifndef EE_NETWORK_HTTPSTREAMCHUNKED_HPP
#define EE_NETWORK_HTTPSTREAMCHUNKED_HPP

#include <eepp/system/iostreamstring.hpp>
#include <eepp/core/string.hpp>

using namespace EE::System;

namespace EE { namespace Network { namespace Private {

class HttpStreamChunked : public IOStreamString {
	public:
		HttpStreamChunked( IOStream& mWriteTo );

		ios_size write( const char * data, ios_size size );

		const std::string& getHeaderBuffer() const;
	protected:
		IOStream& mWriteTo;
		std::string mChunkBuffer;
		std::string mHeaderBuffer;
		bool mChunkNewBuffer = false;
		bool mChunkEnded = false;
};

}}}

#endif // EE_NETWORK_HTTPSTREAMCHUNKED_HPP
