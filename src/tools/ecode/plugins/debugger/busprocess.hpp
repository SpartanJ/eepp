#include "bus.hpp"
#include <eepp/system/process.hpp>

using namespace EE::System;

namespace ecode {

class BusProcess : public Bus {
  public:
	BusProcess( const Command& command );

	bool start() override;

	void startAsyncRead( ReadFn readFn ) override;

	size_t write( const char* buffer, const size_t& size ) override;

  protected:
	Command mCommand;
	Process mProcess;
};

} // namespace ecode
