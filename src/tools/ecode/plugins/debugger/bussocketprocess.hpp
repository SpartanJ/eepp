#include "bus.hpp"
#include "config.hpp"

namespace ecode {

class BusProcess;
class BusSocket;

class BusSocketProcess : public Bus {
  public:
	BusSocketProcess( const Command& command, const Connection& connection );

	virtual ~BusSocketProcess();

	bool start() override;

	bool close() override;

	void startAsyncRead( ReadFn readFn ) override;

	size_t write( const char* buffer, const size_t& size ) override;

	bool hasProcess() override { return true; }

  protected:
	Command mCommand;
	Connection mConnection;
	std::unique_ptr<BusProcess> mProcess;
	std::unique_ptr<BusSocket> mSocket;
};

} // namespace ecode
