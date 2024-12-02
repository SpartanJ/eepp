#include "bussocket.hpp"
#include <eepp/network/ipaddress.hpp>

namespace ecode {

BusSocket::BusSocket( const Connection& connection ) {
	mSocket.connect( IpAddress( connection.host ), connection.port );
}

void BusSocket::startAsyncRead( ReadFn readFn ) {
	mSocket.startAsyncRead( readFn );
}

size_t BusSocket::write( const char* buffer, const size_t& size ) {
	size_t sent = 0;
	mSocket.send( buffer, size, sent );
	return sent;
}

} // namespace ecode
