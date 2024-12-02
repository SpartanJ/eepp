#pragma once

#include "config.hpp"

namespace ecode {

class Bus {
  public:
	typedef std::function<void( const char* bytes, size_t n )> ReadFn;

	virtual void startAsyncRead( ReadFn readFn ) = 0;

	virtual size_t write( const char* buffer, const size_t& size ) = 0;
};

} // namespace ecode
