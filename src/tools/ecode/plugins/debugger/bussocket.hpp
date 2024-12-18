#include "bus.hpp"
#include "config.hpp"
#include <eepp/network/tcpsocket.hpp>

using namespace EE::Network;

namespace ecode {

class BusSocket : public Bus {
  public:
	BusSocket( const Connection& connection );

	bool start() override;

	bool close() override;

	void startAsyncRead( ReadFn readFn ) override;

	size_t write( const char* buffer, const size_t& size ) override;

  protected:
	Connection mConnection;
	TcpSocket mSocket;
};

} // namespace ecode
