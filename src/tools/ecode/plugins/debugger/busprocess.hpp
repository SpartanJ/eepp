#include "bus.hpp"
#include <eepp/system/process.hpp>

using namespace EE::System;

namespace ecode {

class BusProcess : public Bus {
  public:
	BusProcess( const Command& command );

	virtual void startAsyncRead( ReadFn readFn );

	virtual size_t write( const char* buffer, const size_t& size );

  protected:
	Process mProcess;
};

} // namespace ecode
