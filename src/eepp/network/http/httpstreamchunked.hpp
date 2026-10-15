#ifndef EE_NETWORK_HTTPSTREAMCHUNKED_HPP
#define EE_NETWORK_HTTPSTREAMCHUNKED_HPP

#include <eepp/core/string.hpp>
#include <eepp/system/iostreamstring.hpp>

using namespace EE::System;

namespace EE { namespace Network { namespace Private {

class EE_API HttpStreamChunked : public IOStreamString {
  public:
	HttpStreamChunked( IOStream& mWriteTo );

	ios_size write( const char* data, ios_size size );

	const std::string& getHeaderBuffer() const;

	bool isComplete() const;

	bool hasError() const;

  protected:
	enum class State : Uint8 { Size, Data, DataCR, DataLF, Trailer, Complete, Error };
	IOStream& mWriteTo;
	std::string mLine;
	std::string mHeaderBuffer;
	Uint64 mRemaining{ 0 };
	State mState{ State::Size };
};

}}} // namespace EE::Network::Private

#endif // EE_NETWORK_HTTPSTREAMCHUNKED_HPP
