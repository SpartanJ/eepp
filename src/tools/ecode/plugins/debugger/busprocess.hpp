#include "bus.hpp"
#include "config.hpp"
#include <eepp/system/process.hpp>

using namespace EE::System;

namespace ecode {

class BusProcess : public Bus {
  public:
	BusProcess( const Command& command );

	virtual ~BusProcess();

	bool start() override;

	bool close() override;

	void startAsyncRead( ReadFn readFn ) override;

	size_t write( const char* buffer, const size_t& size ) override;

	bool hasProcess() override { return true; }

  protected:
	Command mCommand;
	Process mProcess;
};

} // namespace ecode
