#include "bus.hpp"
#include <eepp/network/tcpsocket.hpp>

using namespace EE::Network;

namespace ecode {

class BusSocket : public Bus {
  public:
	BusSocket( const Connection& connection );

	void startAsyncRead( ReadFn readFn );

	size_t write( const char* buffer, const size_t& size );

  protected:
	TcpSocket mSocket;
};

} // namespace ecode
